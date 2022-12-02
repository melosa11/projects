use protocol::{CommandType, ConnectionStream, DriverConnection, InputType, ProtocolErrorKind};
use utils::{MultiScreen, Screen};

use log::error;
use std::net::SocketAddr;
use std::path::Path;
use tokio::fs::File;
use tokio::io::AsyncWriteExt;

pub async fn run(
    addr: &SocketAddr,
    pos: protocol::Position,
    mouse_path: impl AsRef<Path>,
) -> protocol::Result<()> {
    let mut driver_connection = DriverConnection::with_mouse(mouse_path)?;
    let resolution = utils::get_current_resolution();

    let mut multi_screen =
        MultiScreen::new(utils::Coord(0, 0), 0, vec![Screen { id: 0, resolution }]);

    let mut connection = ConnectionStream::connect(addr).await?;
    connection
        .send_command(CommandType::ClientInfo(resolution, pos))
        .await?;

    wait_for_start(&mut connection).await?;

    center_mouse_cursor(&mut driver_connection, &mut multi_screen).await?;

    loop {
        match poll(&mut connection, &mut driver_connection).await? {
            Event::Command(cmd) => match cmd {
                CommandType::File(name) => {
                    if let Err(err) = create_file(&mut connection, name).await {
                        error!("{}", err);
                    }
                }
                _ => {
                    driver_connection
                        .send_input(parse_command(&mut multi_screen, cmd)?)
                        .await?
                }
            },
            Event::Input(input) => {
                if let InputType::RelativeCoordinates(x, y) = input {
                    multi_screen.move_cursor_rel(utils::Coord(x, y));
                }
            }
        }
    }
}

enum Event {
    Command(CommandType),
    Input(InputType),
}

async fn poll(
    connection: &mut ConnectionStream,
    driver_connection: &mut DriverConnection,
) -> protocol::Result<Event> {
    tokio::select! {
        command = connection.recv_command() => command.map(Event::Command),
        input = driver_connection.recv_input() => input.map(Event::Input),
    }
}

async fn create_file(connection: &mut ConnectionStream, file_name: String) -> protocol::Result<()> {
    let mut file = File::create(Path::new(&format!("~/Desktop/{}", file_name)))
        .await
        .map_err(ProtocolErrorKind::IoError)?;
    match connection.recv_command().await? {
        CommandType::Data(data) => file
            .write_all(&data)
            .await
            .map_err(ProtocolErrorKind::IoError)?,
        _ => return Err(ProtocolErrorKind::CommandError),
    };
    Ok(())
}

fn parse_command(multi_screen: &mut MultiScreen, cmd: CommandType) -> protocol::Result<InputType> {
    match cmd {
        CommandType::Coordinates(x, y) => {
            let relative = multi_screen.move_cursor_abs(utils::Coord(x, y));
            Ok(InputType::RelativeCoordinates(relative.0, relative.1))
        }
        CommandType::Scroll(scroll_type, value) => Ok(InputType::Scroll(scroll_type, value)),
        CommandType::Key(value) => Ok(InputType::Key(value)),
        _ => Err(ProtocolErrorKind::CommandError),
    }
}

async fn wait_for_start(connection: &mut ConnectionStream) -> protocol::Result<()> {
    match connection.recv_command().await? {
        CommandType::Start => Ok(()),
        _ => Err(ProtocolErrorKind::CommandError),
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
