use std::ffi::{c_void, CString};
use std::os::unix::ffi::OsStrExt;
use std::path::Path;
use std::sync::atomic;
use std::sync::atomic::AtomicU64;
use std::{io, ptr};

use inotify::{Inotify, WatchMask};
use libc::{c_int, memcpy};
use log::error;

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
#[allow(dead_code)]
pub fn inotify_watch_for_deletion(dir: &Path, name: &str) -> io::Result<()> {
    inotify_watch(dir, name, false)
}

/// block until a specific file is deleted in the watched directory
pub fn inotify_watch_for_addition(dir: &Path, name: &str) -> io::Result<()> {
    inotify_watch(dir, name, true)
}

pub struct Ivshmem {
    fd: c_int,
    size: usize,
    addr: *mut c_void,
}

impl Ivshmem {
    pub fn new(path: &Path, size: usize) -> io::Result<Self> {
        let c_path = CString::new(path.as_os_str().as_bytes())?;
        let null = ptr::null_mut();
        let (fd, addr) = unsafe {
            let fd = libc::open(c_path.as_ptr(), libc::O_RDWR);
            if fd < 0 {
                return Err(io::Error::last_os_error());
            }
            let addr = libc::mmap(
                null,
                size,
                libc::PROT_READ | libc::PROT_WRITE,
                libc::MAP_SHARED,
                fd,
                0,
            );
            if addr == null {
                return Err(io::Error::last_os_error());
            }
            (fd, addr)
        };
        Ok(Self { fd, addr, size })
    }

    pub fn vmio(&mut self) -> &mut Vmio {
        unsafe { &mut *(self.addr as *mut Vmio) }
    }
}

impl Drop for Ivshmem {
    fn drop(&mut self) {
        let mut err = None;
        unsafe {
            if libc::munmap(self.addr, self.size) < 0 {
                err = Some(io::Error::last_os_error());
            }
            libc::close(self.fd);
        }
        if let Some(e) = err {
            error!("unexpected error in dropping the ivshmem: {}", e);
        }
    }
}

#[repr(C)]
pub struct Vmio {
    flag: AtomicU64,
    spin_host: AtomicU64,
    spin_guest: AtomicU64,
    completed: AtomicU64,
    size: u64,
}

impl Vmio {
    pub fn init(&mut self) -> io::Result<()> {
        // initialize atomics
        self.spin_host.store(1, atomic::Ordering::SeqCst);
        self.spin_guest.store(0, atomic::Ordering::SeqCst);
        self.completed.store(0, atomic::Ordering::SeqCst);

        // set size to be 0
        self.size = 0;

        // mark flag as 1 as the last step
        self.flag.store(1, atomic::Ordering::SeqCst);

        // done
        Ok(())
    }

    pub fn wait_on_host(&mut self) {
        while self.spin_host.load(atomic::Ordering::SeqCst) == 1 {}
    }

    pub fn post_to_guest(&mut self) {
        self.spin_guest.store(0, atomic::Ordering::SeqCst);
    }

    pub fn check_completion(&mut self) -> bool {
        self.completed
            .compare_exchange(1, 0, atomic::Ordering::SeqCst, atomic::Ordering::SeqCst)
            .is_ok()
    }

    pub fn prepare_blob(&mut self, blob: &[u8]) {
        self.size = blob.len() as u64;
        unsafe {
            let base = ptr::addr_of_mut!(self.size).add(1) as *mut c_void;
            memcpy(base, blob.as_ptr() as *const c_void, blob.len());
        };
    }
}
