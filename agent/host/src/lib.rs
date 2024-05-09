use std::path::PathBuf;

use log::{info, LevelFilter};
use structopt::StructOpt;

use crate::qemu::{QemuProxy, VMStatus};
use crate::utils::{inotify_watch_for_addition, Ivshmem};

mod config;
mod qemu;
mod utils;

const VM_MONITOR_SOCKET: &str = "monitor";
const VM_IVSHMEM_FILE: &str = "ivshmem";
const VM_IVSHMEM_SIZE: usize = 16 * 1024 * 1024;

#[derive(StructOpt)]
struct Options {
    /// path to the temporary workspace directory
    path_tmp: PathBuf,
    /// path to QEMU monitor unix domain socket
    #[structopt(short, long)]
    verbose: bool,
}

pub fn entrypoint() {
    let args = Options::from_args();
    env_logger::builder()
        .format_timestamp(None)
        .format_target(false)
        .format_module_path(false)
        .filter_level(if args.verbose {
            LevelFilter::Debug
        } else {
            LevelFilter::Info
        })
        .init();

    // wait for ivshmem to be created
    inotify_watch_for_addition(&args.path_tmp, VM_IVSHMEM_FILE)
        .unwrap_or_else(|e| panic!("error waiting for creation of ivshmem: {}", e));
    info!("QEMU is up and running");

    let path_ivshmem = args.path_tmp.join(VM_IVSHMEM_FILE);
    let mut ivshmem = Ivshmem::new(&path_ivshmem, VM_IVSHMEM_SIZE)
        .unwrap_or_else(|e| panic!("error mapping ivshmem: {}", e));

    let vmio = ivshmem.vmio();
    vmio.init()
        .unwrap_or_else(|e| panic!("error initializing vmio: {}", e));
    info!("vmio initialized");

    // sync with guest on start-up
    vmio.wait_on_host();
    info!("guest agent is ready");

    // save a live snapshot
    let path_monitor_socket = args.path_tmp.join(VM_MONITOR_SOCKET);
    let mut qemu_proxy = QemuProxy::new(path_monitor_socket);
    qemu_proxy
        .snapshot_save()
        .unwrap_or_else(|e| panic!("error taking a snapshot: {}", e));
    info!("live snapshot is taken");

    // fuzzing loop
    'fuzz: loop {
        // always refresh from a new snapshot
        qemu_proxy
            .snapshot_load()
            .unwrap_or_else(|e| panic!("error restoring a snapshot: {}", e));

        // release the guest
        vmio.post_to_guest();
        info!("notified guest agent to continue");

        // wait for guest to stop
        'wait: loop {
            match qemu_proxy
                .query_status()
                .unwrap_or_else(|e| panic!("error querying status of the VM: {}", e))
            {
                VMStatus::Running => (),
                VMStatus::Stopped => {
                    break 'wait;
                }
                VMStatus::Error => {
                    break 'fuzz;
                }
            }
        }
    }

    // drop the ivshmem at the end
    drop(ivshmem);
}
