pub use evdev::{EventType, InputEvent, Key, RelativeAxisType};

use evdev::{
    uinput::{VirtualDevice, VirtualDeviceBuilder},
    AttributeSet, Device, EventStream,
};
use std::io;
use std::path::Path;

pub struct Driver {
    virtual_device: VirtualDevice,
    mouse_event_stream: EventStream,
    keyboard_event_stream: Option<EventStream>,
}

impl Driver {
    pub fn new<T: AsRef<Path>>(mouse_path: T, keyboard_path: Option<T>) -> io::Result<Self> {
        Ok(Self {
            virtual_device: create_virtual_device()?,
            mouse_event_stream: create_event_stream(mouse_path)?,
            keyboard_event_stream: match keyboard_path {
                Some(path) => Some(create_event_stream(path)?),
                None => None,
            },
        })
    }

    pub async fn read_physical_input(&mut self) -> io::Result<InputEvent> {
        match &mut self.keyboard_event_stream {
            None => self.mouse_event_stream.next_event().await,
            Some(keyboard_event_stream) => {
                tokio::select! {
                    ev = self.mouse_event_stream.next_event() => ev,
                    ev = keyboard_event_stream.next_event() => ev,
                }
            }
        }
    }

    pub async fn send_virtual_input(&mut self, messages: &[InputEvent]) -> io::Result<()> {
        self.virtual_device.emit(messages)
    }
}

fn create_event_stream(path: impl AsRef<Path>) -> io::Result<EventStream> {
    Device::open(path)?.into_event_stream()
}

fn create_virtual_device() -> io::Result<VirtualDevice> {
    let mut keys = AttributeSet::<Key>::new();

    // register all keys
    for code in 1..0x2e7 {
        if code == Key::KEY_POWER.code()
            || (Key::KEY_MICMUTE.code() < code && code < Key::BTN_0.code())
        {
            continue;
        }
        keys.insert(Key::new(code));
    }

    let mut relatives = AttributeSet::<RelativeAxisType>::new();
    relatives.insert(RelativeAxisType::REL_X);
    relatives.insert(RelativeAxisType::REL_Y);
    relatives.insert(RelativeAxisType::REL_HWHEEL);
    relatives.insert(RelativeAxisType::REL_WHEEL);
    relatives.insert(RelativeAxisType::REL_HWHEEL_HI_RES);
    relatives.insert(RelativeAxisType::REL_WHEEL_HI_RES);

    VirtualDeviceBuilder::new()?
        .name("Virtual Keyboard & Mouse")
        .with_keys(&keys)?
        .with_relative_axes(&relatives)?
        .build()
}
