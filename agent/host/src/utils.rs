use std::fs::File;
use std::io::{BufRead, BufReader};
use std::path::Path;
use std::{fs, io};

use inotify::{Inotify, WatchMask};

/// Receive one line from the guest agent
pub fn recv_str_from_guest(path_console_output: &Path) -> io::Result<String> {
    let f = File::open(path_console_output)?;
    let mut reader = BufReader::new(f);

    let mut buffer = String::new();
    let mut message = String::new();
    loop {
        let num_bytes = reader.read_line(&mut buffer)?;
        if num_bytes == 0 {
            // TODO: might opt for a polling-based solution?
            continue;
        }
        if message.ends_with('\n') {
            message.push_str(&buffer[0..num_bytes - 1]);
            break;
        }
        message.push_str(&buffer);
        buffer.clear();
    }

    Ok(message)
}

/// Send one line to the guest agent
pub fn send_str_into_guest(path_console_input: &Path, message: &str) -> io::Result<()> {
    fs::write(path_console_input, format!("{}\n", message))
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
