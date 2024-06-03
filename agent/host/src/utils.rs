use std::ffi::{c_void, CString};
use std::os::unix::ffi::OsStrExt;
use std::path::Path;
use std::sync::atomic;
use std::sync::atomic::AtomicU64;
use std::{io, ptr};

use inotify::{Inotify, WatchMask};
use libc::c_int;
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
    loop {
        // if the file exists, return immediately
        if dir.join(name).exists() {
            return Ok(());
        }
    } 
    // inotify_watch(dir, name, true)
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
    __pad: AtomicU64,
    size: u64,
    data: [u8; 1048000],
}

impl Vmio {
    pub fn init(&mut self) -> io::Result<()> {
        // initialize atomics
        self.spin_host.store(1, atomic::Ordering::SeqCst);
        self.spin_guest.store(0, atomic::Ordering::SeqCst);

        // set size to be 0
        self.size = 0;

        // mark flag as 1 as the last step
        self.flag.store(1, atomic::Ordering::SeqCst);

        // done
        Ok(())
    }

    pub fn init_ipc(&mut self) -> io::Result<()> {
        Ok(())
    }

    pub fn wait_on_host(&mut self) {
        while self.spin_host.load(atomic::Ordering::SeqCst) == 1 {}
    }

    pub fn post_to_guest(&mut self) {
        self.spin_guest.store(0, atomic::Ordering::SeqCst);
    }

    pub fn send_fuzz_input(&mut self, data: &[u8], size: usize) {
        // set size
        self.size = size as u64;

        // copy data
        self.data[..size].copy_from_slice(data);

        // notify guest
        self.spin_guest.store(0, atomic::Ordering::SeqCst);
    }

    pub fn recv_data(&self) -> Vec<u8> {
        let size = self.size as usize;
        self.data[..size].to_vec()
    }

    pub fn get_kcov_info(&self) -> Vec<u64>{
        let mut result = Vec::new();
        let size = self.size as usize;
        // get a u64 ptr of data
        let data = self.data.as_ptr() as *const u64;
        // iterate over data
        for i in 0..size / 8 {
            let value = unsafe { *data.add(i) };
            // copy value to a vector
            result.push(value);
        }
        result
    }

    pub fn send_kcov_info(&mut self, data: &[u64]) {
        let size = data.len();
        // get a u64 ptr of data
        let dest = self.data.as_mut_ptr() as *mut u64;
        // iterate over data
        for i in 0..size {
            unsafe {
                *dest.add(i) = data[i];
            }
        }
        // set size
        self.size = (size * 8) as u64;
        self.spin_guest.store(0, atomic::Ordering::SeqCst);
    }

    pub fn check_success(&self) -> bool {
        self.spin_guest.load(atomic::Ordering::SeqCst) == 2
    }
}
