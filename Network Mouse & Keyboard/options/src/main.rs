#![allow(deprecated)]

use clap::{clap_app, ArgMatches};
use log::{error, info};
use std::net::SocketAddr;

enum Direction {
    Left,
    Right,
}

struct Options {
    // Run as server
    server: bool,

    // Path to mouse
    mouse_path: Option<String>,

    // Path to keyboard
    keyboard_path: Option<String>,

    // Relative position of screen
    position: Option<Direction>,

    // Port on which we want to connect
    port: Option<i32>,

    // Connection address
    address: Option<String>,
}

fn parse_direction(direction: Option<&str>) -> Option<Direction> {
    match direction {
        Some("left") => Some(Direction::Left),
        Some("right") => Some(Direction::Right),
        _ => None,
    }
}

fn get_options() -> ArgMatches {
    clap_app!(myapp =>
        (version: "1.0")
        (author: "")
        (about: "Linux driver for sharing mouse and keyboard between computers")
        (@arg server: -s --server)
        (@arg mouse: -m --mouse +required +takes_value)
        (@arg keyboard: -k --keyboard +required +takes_value)
        (@arg position: -n --position +required +takes_value)
        (@arg port: -p --port +required +takes_value)
        (@arg address: -a --address +required +takes_value)
    )
    .get_matches()
}

fn get_arguments() -> Options {
    let options = get_options();

    let server = options.is_present("server");
    let mouse_path = options.value_of("mouse").map(|x| x.to_string());
    let keyboard_path = options.value_of("keyboard").map(|x| x.to_string());
    let position = parse_direction(options
        .value_of("position"));
    let port = options
        .value_of("port")
        .map(|x| x.parse::<i32>().expect("Port is not a number"));
    let address = options
        .value_of("address")
        .map(|x| x.parse().expect("Not a valid IP address format"));

    Options {
        server,
        mouse_path,
        keyboard_path,
        position,
        port,
        address,
    }
}

fn main() {
    let options = get_arguments();

    println!("{}", options.mouse_path.unwrap());
    println!("{}", options.port.unwrap());
    match options.position {
        Some(Direction::Left) => info!("Left position was chosen"),
        Some(Direction::Right) => info!("Right position was chosen"),
        _ => error!("Invalid direction"),
    }
}
