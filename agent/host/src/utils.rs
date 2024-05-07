use std::fs::File;
use std::io::{BufRead, BufReader};
use std::os::fd::{AsFd, AsRawFd};
use std::path::Path;
use std::{fs, io};

use inotify::{Inotify, WatchMask};
use polling::{Event, Events, Poller};

const VMIO_POLLING_KEY: usize = 42;

/// Receive one line from the guest agent
pub fn recv_str_from_guest(path_console_output: &Path) -> io::Result<String> {
    let f = File::open(path_console_output)?;
    let fd = f.as_raw_fd();

    let mut message = String::new();

    let poller = Poller::new()?;
    unsafe {
        poller.add(fd, Event::readable(VMIO_POLLING_KEY))?;
    }

    // event loop
    let mut reader = BufReader::new(f);
    let mut events = Events::new();
    'event: loop {
        poller.wait(&mut events, None)?;
        for ev in events.iter() {
            if ev.key != VMIO_POLLING_KEY {
                continue;
            }
            // ready to read
            reader.read_line(&mut message)?;
            if message.ends_with('\n') || message.ends_with("\n\0") {
                break 'event;
            }
            // next round of polling
            poller.modify(reader.get_ref().as_fd(), Event::readable(VMIO_POLLING_KEY))?;
        }
        events.clear();
    }

    // done
    Ok(message)
}

/// Send one line to the guest agent
pub fn send_str_into_guest(path_console_input: &Path, message: &str) -> io::Result<()> {
    fs::write(path_console_input, message)
}

/// block until a specific file is created or deleted in the watched directory
fn inotify_watch(dir: &Path, name: &str, is_create: bool) -> io::Result<()> {
    let target = dir.join(name);

    let mut inotify = Inotify::init()?;
    inotify.watches().add(
        dir,
        if is_create {
            WatchMask::CREATE
        } else {
            WatchMask::DELETE
        },
    )?;

    let mut buffer = [0; 1024];
    loop {
        let existed = target.exists();
        if existed == is_create {
            return Ok(());
        }

        let events = match inotify.read_events(&mut buffer) {
            Ok(items) => items,
            Err(e) if e.kind() == io::ErrorKind::WouldBlock => {
                inotify.read_events_blocking(&mut buffer)?
            }
            Err(e) => return Err(e),
        };
        for event in events {
            if event.name.map_or(false, |n| n == name) {
                return Ok(());
            }
        }
    }
}

/// block until a specific file is deleted in the watched directory
pub fn inotify_watch_for_deletion(dir: &Path, name: &str) -> io::Result<()> {
    inotify_watch(dir, name, false)
}

/// block until a specific file is deleted in the watched directory
pub fn inotify_watch_for_addition(dir: &Path, name: &str) -> io::Result<()> {
    inotify_watch(dir, name, true)
}
