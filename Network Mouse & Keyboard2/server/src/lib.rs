use itertools::Itertools;
use log::error;
use protocol::{
    CommandType, ConnectionListener, ConnectionStream, DriverConnection, InputType, Position,
    ProtocolErrorKind,
};
use std::collections::HashMap;
use std::net::SocketAddr;
use std::path::Path;
use tokio::fs::File;
use tokio::io::{stdin, AsyncReadExt};
use tokio::sync::mpsc::Receiver;
use utils::{MultiScreen, Screen};

type Id = usize;

type ClientConnection = ConnectionStream;
type DesktopConnection = Receiver<CopyFile>;

pub struct CopyFile {
    path: String,
    screen_id: utils::Id,
}

pub async fn run<P>(
    addr: &SocketAddr,
    mouse_path: P,
    keyboard_path: P,
    mut desktop_receiver: DesktopConnection,
) -> protocol::Result<()>
where
    P: AsRef<Path>,
{
    let listener = ConnectionListener::bind(addr).await?;

    let mut driver_connection =
        DriverConnection::with_mouse_and_keyboard(mouse_path, keyboard_path)?;
    let mut client_connections = accept_clients(&listener).await?;
    let mut multi_screen =
        create_multi_screen(client_connections.iter_mut().sorted_by_key(|(id, _)| *id)).await?;

    send_start_command(client_connections.values_mut()).await?;

    center_mouse_cursor(&mut driver_connection, &mut multi_screen).await?;
    loop {
        tokio::select! {
            input = driver_connection.recv_input() => {
                let (client_id, command) =
                parse_input(&mut multi_screen, input?);
                client_connections
                    .get_mut(&client_id)
                    .unwrap_or_else(|| panic!("unexpected client id {}", client_id))
                    .send_command(command)
                    .await?;
            }
            Some(CopyFile { path, screen_id }) = desktop_receiver.recv() => {
                if let Err(err) = send_file(
                        client_connections.get_mut(&screen_id).unwrap_or_else(|| panic!("unexpected client id {}", screen_id)),
                        path).await {
                            error!("{}", err);
                }
            }
        };
    }
}

async fn send_file(
    client_connection: &mut ClientConnection,
    file_path: String,
) -> protocol::Result<()> {
    let mut file = File::open(&file_path)
        .await
        .map_err(ProtocolErrorKind::IoError)?;
    client_connection
        .send_command(CommandType::File(
            Path::new(&file_path)
                .file_name()
                .unwrap()
                .to_os_string()
                .into_string()
                .unwrap(),
        ))
        .await?;
    let mut data = Vec::new();
    file.read_to_end(&mut data)
        .await
        .map_err(ProtocolErrorKind::IoError)?;

    client_connection
        .send_command(CommandType::Data(data))
        .await
}

fn parse_input(multi_screen: &mut MultiScreen, input: InputType) -> (utils::Id, CommandType) {
    use InputType::*;
    match input {
        RelativeCoordinates(x, y) => {
            let (id, location) = multi_screen.move_cursor_rel(utils::Coord(x, y));
            (id, CommandType::Coordinates(location.0, location.1))
        }
        Key(key) => (multi_screen.main_screen_id(), CommandType::Key(key)),
        Scroll(scroll_type, value) => (
            multi_screen.main_screen_id(),
            CommandType::Scroll(scroll_type, value),
        ),
    }
}

async fn center_mouse_cursor(
    driver_connection: &mut DriverConnection,
    multi_screen: &mut MultiScreen,
) -> protocol::Result<()> {
    let center = multi_screen.center_cursor();

    // move cursor to top left corner (0,0)
    driver_connection
        .send_input(InputType::RelativeCoordinates(-100_000, -100_000))
        .await?;
    driver_connection
        .send_input(InputType::RelativeCoordinates(center.0, center.1))
        .await
}

async fn send_start_command(
    client_connections: impl Iterator<Item = &mut ClientConnection>,
) -> protocol::Result<()> {
    for conn in client_connections {
        conn.send_command(CommandType::Start).await?;
    }
    Ok(())
}

async fn create_multi_screen<'a>(
    client_connections: impl Iterator<Item = (&'a utils::Id, &mut ClientConnection)>,
) -> protocol::Result<MultiScreen> {
    let main_id = 0;
    let main_screen = Screen {
        id: main_id,
        resolution: utils::get_current_resolution(),
    };

    let mut screens = vec![main_screen];

    for (id, conn) in client_connections {
        match conn.recv_command().await? {
            CommandType::ClientInfo(resolution, pos) => {
                let screen = Screen {
                    id: *id,
                    resolution,
                };
                match pos {
                    Position::Left => screens.insert(0, screen),
                    Position::Right => screens.push(screen),
                }
            }
            _ => return Err(ProtocolErrorKind::CommandError),
        }
    }

    Ok(MultiScreen::new(utils::Coord(0, 0), main_id, screens))
}

async fn accept_clients(
    listener: &ConnectionListener,
) -> protocol::Result<HashMap<Id, ClientConnection>> {
    println!("Waiting for clients");
    let mut clients = HashMap::new();
    let mut id = 1;
    loop {
        tokio::select! {
            _ = wait_for_enter() => break,
            conn = listener.accept() => {
                assert!(clients.insert(id, conn?).is_some());
                println!("Client {} connected", id);
                id += 1;
            }
        }
    }

    Ok(clients)
}

async fn wait_for_enter() -> protocol::Result<()> {
    stdin()
        .read_u8()
        .await
        .map_err(ProtocolErrorKind::IoError)
        .map(|_| ())
}
