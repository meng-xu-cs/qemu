use std::fs;
use std::io;
use std::path::Path;

use crate::utils::Ivshmem;
use crate::qemu::QemuProxy;

pub enum GuestStatus {
    Ready,
    Running,
    Completed,
    Waiting,
}

pub struct Guest<'a> {
    pub id: usize,
    pub ivshmem: Ivshmem,
    pub qemu: QemuProxy<'a>,
    pub status: GuestStatus,
    pub seed_id: usize,
    pub session_id: usize,
}

impl<'a> Guest<'a> {
    pub fn new(id: usize, ivshmem: Ivshmem, qemu: QemuProxy<'a>, path_output: &Path) -> io::Result<Self> {
        Ok(Self {
            id,
            ivshmem,
            qemu,
            status: GuestStatus::Waiting,
            seed_id: 0,
            session_id: Self::session_id_init(&path_output.join(format!("guest{}", id)))?,
        })
    }

    // resuming a preserved output directory should not restart session
    // numbering from 0, or QCE will crash trying to re-create a directory
    // that already exists from a previous run
    fn session_id_init(path_output: &Path) -> io::Result<usize> {
        let mut start_id = 0usize;
        for item in fs::read_dir(path_output)? {
            let item = item?;
            if !item.file_type()?.is_dir() {
                continue;
            }
            if let Some(id) = item.file_name().to_str().and_then(|s| s.parse::<usize>().ok()) {
                start_id = start_id.max(id + 1);
            }
        }
        Ok(start_id)
    }

    pub fn next_session(&mut self) {
        self.session_id += 1
    }
}
