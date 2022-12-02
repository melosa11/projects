mod error;

use driver::Driver;
pub use error::ProtocolErrorKind;

use log::info;
use serde::{Deserialize, Serialize};
use std::net::SocketAddr;
use std::path::Path;

use futures::prelude::sink::SinkExt;
use futures::TryStreamExt;
use tokio::net::{
    tcp::{OwnedReadHalf, OwnedWriteHalf},
    TcpListener, TcpStream,
};
use tokio_serde::{formats::SymmetricalBincode, SymmetricallyFramed};
use tokio_util::codec::{FramedRead, FramedWrite, LengthDelimitedCodec};

pub type Result<T> = std::result::Result<T, ProtocolErrorKind>;

type TransportReader = FramedRead<OwnedReadHalf, LengthDelimitedCodec>;
type TransportWriter = FramedWrite<OwnedWriteHalf, LengthDelimitedCodec>;
pub struct ConnectionStream {
    read_stream: SymmetricallyFramed<TransportReader, CommandType, SymmetricalBincode<CommandType>>,
    write_stream:
        SymmetricallyFramed<TransportWriter, CommandType, SymmetricalBincode<CommandType>>,
}

impl ConnectionStream {
    fn new(stream: TcpStream) -> Self {
        let (read_stream, write_stream) = stream.into_split();
        let read_stream = SymmetricallyFramed::new(
            FramedRead::new(read_stream, LengthDelimitedCodec::new()),
            SymmetricalBincode::default(),
        );
        let write_stream = SymmetricallyFramed::new(
            FramedWrite::new(write_stream, LengthDelimitedCodec::new()),
            SymmetricalBincode::default(),
        );
        ConnectionStream {
            read_stream,
            write_stream,
        }
    }

    /// Establishes connection to the server.
    pub async fn connect(addr: &SocketAddr) -> Result<ConnectionStream> {
        let stream = TcpStream::connect(&addr).await;
        match stream {
            Ok(stream) => {
                info!("Connected to server on {:?}", addr);
                Ok(ConnectionStream::new(stream))
            }
            Err(_) => Err(ProtocolErrorKind::ServerConnection),
        }
    }

    /// Sends a command
    pub async fn send_command(&mut self, cmd: CommandType) -> Result<()> {
        self.write_stream
            .send(cmd)
            .await
            .map_err(ProtocolErrorKind::IoError)
    }

    /// Receives a command
    pub async fn recv_command(&mut self) -> Result<CommandType> {
        let msg = self
            .read_stream
            .try_next()
            .await
            .map_err(ProtocolErrorKind::IoError)?;

        match msg {
            Some(msg) => Ok(msg),
            None => Err(ProtocolErrorKind::NoCommand),
        }
    }
}

pub struct ConnectionListener {
    listener: TcpListener,
}

impl ConnectionListener {
    /// Starts Listening on the port
    pub async fn bind(addr: &SocketAddr) -> Result<ConnectionListener> {
        let result = TcpListener::bind(addr).await;
        match result {
            Ok(listener) => {
                info!("Started listening on {:?}", addr);
                Ok(ConnectionListener { listener })
            }
            Err(e) => Err(ProtocolErrorKind::IoError(e)),
        }
    }
    /// Accepts connection
    pub async fn accept(&self) -> Result<ConnectionStream> {
        match self.listener.accept().await {
            Ok((stream, _addr)) => {
                info!("Connection accepted");
                Ok(ConnectionStream::new(stream))
            }
            Err(e) => Err(ProtocolErrorKind::IoError(e)),
        }
    }
}

/// Structure for communication with physical and virtual I/O devices.
pub struct DriverConnection {
    driver: Driver,
}

pub enum InputType {
    /// Relative move of cursor in x and y axis respectively
    RelativeCoordinates(i32, i32),
    /// Pressed Key code
    Key(u16),
    /// Scroll type and value
    Scroll(u16, i32),
}

impl DriverConnection {
    /// Will listen only for mouse events.
    /// The path is expected to uinput device in form "/dev/input/event*".
    pub fn with_mouse(mouse_path: impl AsRef<Path>) -> Result<Self> {
        Ok(Self {
            driver: Driver::new(mouse_path, None).map_err(ProtocolErrorKind::IoError)?,
        })
    }
    /// Will listen for mouse and keyboard events.
    /// The path is expected to uinput device in form "/dev/input/event*".
    pub fn with_mouse_and_keyboard<T: AsRef<Path>>(
        mouse_path: T,
        keyboard_path: T,
    ) -> Result<Self> {
        Ok(Self {
            driver: Driver::new(mouse_path, Some(keyboard_path))
                .map_err(ProtocolErrorKind::IoError)?,
        })
    }

    /// Sends input to virtual device.
    pub async fn send_input(&mut self, input: InputType) -> Result<()> {
        use driver::*;
        let messages = match input {
            InputType::RelativeCoordinates(x, y) => vec![
                InputEvent::new(EventType::RELATIVE, RelativeAxisType::REL_X.0, x),
                InputEvent::new(EventType::RELATIVE, RelativeAxisType::REL_Y.0, y),
            ],
            InputType::Key(code) => vec![
                InputEvent::new(EventType::KEY, code, 1),
                InputEvent::new(EventType::KEY, code, 0),
            ],
            InputType::Scroll(code, val) => vec![InputEvent::new(EventType::RELATIVE, code, val)],
        };

        self.driver
            .send_virtual_input(&messages)
            .await
            .map_err(ProtocolErrorKind::IoError)
    }

    /// Receive input from physical devices.
    pub async fn recv_input(&mut self) -> Result<InputType> {
        let ev = self
            .driver
            .read_physical_input()
            .await
            .map_err(ProtocolErrorKind::IoError)?;
        Ok(match ev.event_type() {
            driver::EventType::KEY => InputType::Key(ev.code()),
            driver::EventType::RELATIVE => match driver::RelativeAxisType(ev.code()) {
                driver::RelativeAxisType::REL_X => InputType::RelativeCoordinates(ev.value(), 0),
                driver::RelativeAxisType::REL_Y => InputType::RelativeCoordinates(0, ev.value()),
                driver::RelativeAxisType::REL_HWHEEL
                | driver::RelativeAxisType::REL_WHEEL
                | driver::RelativeAxisType::REL_HWHEEL_HI_RES
                | driver::RelativeAxisType::REL_WHEEL_HI_RES => {
                    InputType::Scroll(ev.code(), ev.value())
                }
                _ => panic!("unexpected relative event {:?}", ev.code()),
            },
            _ => panic!("unexpected event type {:?}", ev.event_type()),
        })
    }
}

#[derive(Serialize, Deserialize, PartialEq, Clone, Copy, Debug)]
pub enum Position {
    Left,
    Right,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Clone)]
pub enum CommandType {
    Start,
    ClientInfo(utils::Resolution, Position),
    Coordinates(i32, i32),
    Scroll(u16, i32),
    Key(u16),
    File(String),
    Data(Vec<u8>),
    Error,
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::{IpAddr, Ipv4Addr};

    #[tokio::test]
    async fn test_listen_connect_shutdown_correct_address() {
        let socket_addr = SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), 6969);

        let conn_listener = ConnectionListener::bind(&socket_addr).await;
        assert!(conn_listener.is_ok());

        let client_stream = ConnectionStream::connect(&socket_addr).await;
        assert!(client_stream.is_ok());

        let server_stream = conn_listener.unwrap().accept().await;
        assert!(server_stream.is_ok());
    }

    #[tokio::test]
    async fn test_stream_send_and_recv_command() {
        let socket_addr = SocketAddr::new(IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1)), 42069);

        let conn_listener = ConnectionListener::bind(&socket_addr).await;
        assert!(conn_listener.is_ok());
        let conn_listener = conn_listener.unwrap();

        let client_stream = ConnectionStream::connect(&socket_addr).await;
        assert!(client_stream.is_ok());
        let mut client_stream = client_stream.unwrap();

        let server_stream = conn_listener.accept().await;
        assert!(server_stream.is_ok());
        let mut server_stream = server_stream.unwrap();

        let command = CommandType::Coordinates(420, 69);
        let send = client_stream.send_command(command.clone()).await;
        assert!(send.is_ok());

        let recv = server_stream.recv_command().await;
        assert!(recv.is_ok());
        assert_eq!(recv.unwrap(), command);
    }
}
