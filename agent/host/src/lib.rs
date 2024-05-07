use std::path::PathBuf;

use log::{info, LevelFilter};
use structopt::StructOpt;

use crate::utils::{inotify_watch_for_addition, recv_str_from_guest, send_str_into_guest};

mod config;
mod qemu;
mod utils;

const VM_MONITOR_SOCKET: &str = "monitor";
const VM_CONSOLE_INPUT: &str = "vmio.in";
const VM_CONSOLE_OUTPUT: &str = "vmio.out";

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

    // wait for QEMU to launch first
    inotify_watch_for_addition(&args.path_tmp, VM_CONSOLE_OUTPUT)
        .unwrap_or_else(|e| panic!("error waiting for guest console to be ready: {}", e));

    // sync with guest on start-up
    let path_console_output = args.path_tmp.join(VM_CONSOLE_OUTPUT);
    let message = recv_str_from_guest(&path_console_output)
        .unwrap_or_else(|e| panic!("error waiting for guest ready: {}", e));
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
    let path_console_input = args.path_tmp.join(VM_CONSOLE_INPUT);
    send_str_into_guest(&path_console_input, MARK_READY)
        .unwrap_or_else(|e| panic!("error resuming guest: {}", e));
}
