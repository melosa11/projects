#![allow(dead_code)]

use thiserror::Error;

#[derive(Debug, Error)]
pub enum ProtocolErrorKind {
    #[error("Connection to server failed")]
    ServerConnection,
    #[error("Shutdown failed")]
    ShutdownFail,
    #[error("Unable to parse received command")]
    CommandError,
    #[error("There was an error with file metadata")]
    Metadata,
    #[error("Standard IO error")]
    IoError(std::io::Error),
    #[error("No command received")]
    NoCommand,
}
