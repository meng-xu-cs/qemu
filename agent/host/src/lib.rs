use std::os::unix::net::UnixStream;
use std::path::PathBuf;

use log::{error, info, LevelFilter};
use structopt::StructOpt;

use crate::qemu::{QemuProxy, VMExitMode};
use crate::utils::{inotify_watch_for_addition, Ivshmem};

mod qemu;
mod utils;

const VM_MONITOR_SOCKET: &str = "monitor";
const VM_IVSHMEM_FILE: &str = "ivshmem";
const VM_IVSHMEM_SIZE: usize = 16 * 1024 * 1024;

#[derive(StructOpt)]
struct Options {
    /// path to the temporary workspace directory
    path_tmp: PathBuf,
    /// check mode
    #[structopt(short, long)]
    check: bool,
    /// verbose mode
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

    // connect to QEMU monitor
    let path_monitor_socket = args.path_tmp.join(VM_MONITOR_SOCKET);
    let stream = UnixStream::connect(path_monitor_socket)
        .unwrap_or_else(|e| panic!("error connecting to QEMU monitor socket: {}", e));
    let mut qemu = QemuProxy::new(&stream)
        .unwrap_or_else(|e| panic!("error negotiating with the QEMU-QMP: {}", e));
    info!("QEMU-QMP connected");

    // sync with guest on start-up
    vmio.wait_on_host();
    info!("guest agent is ready");

    // save a live snapshot
    qemu.snapshot_save()
        .unwrap_or_else(|e| panic!("error taking a snapshot: {}", e));
    info!("live snapshot is taken");

    // fuzzing loop
    loop {
        // release the guest
        vmio.post_to_guest();
        info!("notified guest agent to continue");

        // skip if we are checking (e.g., unit testing)
        if args.check {
            break;
        }

        // wait for guest to either report completion or to (unexpectedly) stop
        loop {
            if vmio.check_completion() {
                info!("guest agent completed successfully");
                break;
            }

            match qemu
                .check_guest_reset()
                .unwrap_or_else(|e| panic!("error waiting for status events from VM: {}", e))
            {
                None => continue,
                Some(status) => match status {
                    VMExitMode::Soft => error!("guest vm resets unexpectedly"),
                    VMExitMode::Hard => error!("guest vm shuts down unexpectedly"),
                    VMExitMode::Host => error!("guest vm halted by the host unexpectedly"),
                },
            }
            break;
        }

        // refresh from a new snapshot
        qemu.snapshot_load()
            .unwrap_or_else(|e| panic!("error restoring a snapshot: {}", e));
        info!("snapshot reloaded");
    }

    // drop the ivshmem at the end
    drop(ivshmem);
}
