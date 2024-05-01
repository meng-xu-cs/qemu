use std::fs::OpenOptions;
use std::path::PathBuf;

use log::info;
use structopt::StructOpt;

mod config;
mod qemu;
mod utils;

/// host-guest synchronization mark
const FILE_MARK: &str = "MARK";

#[derive(StructOpt)]
struct Options {
    /// path to workspace directory
    path_workspace: PathBuf,
    /// path to QEMU monitor unix domain socket
    path_qemu_monitor: PathBuf,
}

pub fn entrypoint() {
    env_logger::builder()
        .format_timestamp(None)
        .format_target(false)
        .format_module_path(false)
        .init();
    let args = Options::from_args();

    // sync with guest on start-up
    utils::inotify_watch_for_deletion(&args.path_workspace, FILE_MARK)
        .unwrap_or_else(|e| panic!("error waiting for sync-mark deletion: {}", e));
    info!("Guest VM is ready");

    qemu::snapshot_save(&args.path_qemu_monitor)
        .unwrap_or_else(|e| panic!("error taking a snapshot: {}", e));

    // release the guest
    OpenOptions::new()
        .create_new(true)
        .write(true)
        .open(args.path_workspace.join(FILE_MARK))
        .unwrap_or_else(|e| panic!("error re-creating the sync-mark: {}", e));
}
