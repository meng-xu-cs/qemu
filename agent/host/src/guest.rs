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
    pub fn new(id: usize, ivshmem: Ivshmem, qemu: QemuProxy<'a>) -> Self {
        Self {
            id,
            ivshmem,
            qemu,
            status: GuestStatus::Waiting,
            seed_id: 0,
            session_id: 0,
        }
    }

    pub fn next_session(&mut self) {
        self.session_id += 1
    }
}
