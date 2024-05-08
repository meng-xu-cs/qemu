use std::path::PathBuf;

use log::{info, LevelFilter};
use structopt::StructOpt;

use crate::utils::{inotify_watch_for_addition, recv_str_from_guest, send_str_into_guest};

mod config;
mod qemu;
mod utils;

const VM_MONITOR_SOCKET: &str = "monitor";
const VM_IVSHMEM_FILE: &str = "ivshmem";

const MARK_READY: &str = "ready\n";

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

    // sync with guest on start-up
    let path_console = args.path_tmp.join(VM_IVSHMEM_FILE);
    let message = recv_str_from_guest(&path_console)
        .unwrap_or_else(|e| panic!("error waiting for guest to be ready: {}", e));
    if message != MARK_READY {
        panic!(
            "unexpected message from guest: {}, expect {}",
            message, MARK_READY
        );
    }
    info!("guest agent is ready");

    // save a live snapshot
    let path_monitor_socket = args.path_tmp.join(VM_MONITOR_SOCKET);
    qemu::snapshot_save(&path_monitor_socket, 0)
        .unwrap_or_else(|e| panic!("error taking a snapshot: {}", e));
    info!("live snapshot is taken");

    // release the guest
    send_str_into_guest(&path_console, MARK_READY)
        .unwrap_or_else(|e| panic!("error resuming guest: {}", e));
    info!("notified guest agent to continue");
}
