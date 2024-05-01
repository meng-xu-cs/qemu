use std::io;
use std::path::Path;

use inotify::{Inotify, WatchMask};

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
