use std::os::unix::net::UnixStream;
use std::path::PathBuf;

use log::{error, info, LevelFilter};
use structopt::StructOpt;

use crate::fuzz::Fuzzer;
use crate::qemu::{QemuProxy, VMExitMode};
use crate::utils::{inotify_watch_for_addition, Ivshmem};

mod fuzz;
mod qemu;
mod utils;

const VM_MONITOR_SOCKET: &str = "monitor";
const VM_IVSHMEM_FILE: &str = "ivshmem";
const VM_IVSHMEM_SIZE: usize = 16 * 1024 * 1024;

#[derive(StructOpt)]
struct Options {
    /// path to the temporary workspace directory
    path_tmp: PathBuf,
    /// path to the corpus directory
    #[structopt(long)]
    corpus: PathBuf,
    /// path to the output directory
    #[structopt(long)]
    output: PathBuf,
    /// check mode
    #[structopt(long)]
    check: bool,
    /// verbose mode
    #[structopt(long)]
    verbose: bool,
}

pub fn entrypoint() {
    let Options {
        path_tmp,
        corpus,
        output,
        check,
        verbose,
    } = Options::from_args();

    // logging
    env_logger::builder()
        .format_timestamp(None)
        .format_target(false)
        .format_module_path(false)
        .filter_level(if verbose {
            LevelFilter::Debug
        } else {
            LevelFilter::Info
        })
        .init();

    // initialize the fuzzer first
    let mut fuzzer = Fuzzer::new(corpus, output)
        .unwrap_or_else(|e| panic!("error initializing the fuzzer: {}", e));

    // wait for ivshmem to be created
    inotify_watch_for_addition(&path_tmp, VM_IVSHMEM_FILE)
        .unwrap_or_else(|e| panic!("error waiting for creation of ivshmem: {}", e));
    info!("QEMU is up and running");

    let path_ivshmem = path_tmp.join(VM_IVSHMEM_FILE);
    let mut ivshmem = Ivshmem::new(&path_ivshmem, VM_IVSHMEM_SIZE)
        .unwrap_or_else(|e| panic!("error mapping ivshmem: {}", e));

    let vmio = ivshmem.vmio();
    vmio.init()
        .unwrap_or_else(|e| panic!("error initializing vmio: {}", e));
    info!("vmio initialized");

    // connect to QEMU monitor
    let path_monitor_socket = path_tmp.join(VM_MONITOR_SOCKET);
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
    'fuzzing: loop {
        // put the seed into shared memory
        let seed = fuzzer.current_seed().expect("loading current seed");
        vmio.prepare_blob(&seed);

        // release the guest
        vmio.post_to_guest();
        info!("notified guest agent to continue");

        // skip if we are in checking mode (e.g., unit testing)
        if check {
            break 'fuzzing;
        }

        // wait for guest to either report completion or to (unexpectedly) stop
        'monitor: loop {
            if vmio.check_completion() {
                info!("guest agent completed successfully");

                match fuzzer.process_session_result() {
                    Ok(()) => (),
                    Err(e) => {
                        error!("unexpect error in processing session result: {}", e);
                        break 'monitor;
                    }
                };

                if fuzzer.has_pending_seeds() {
                    break 'monitor;
                } else {
                    break 'fuzzing;
                }
            }

            match qemu
                .check_guest_reset()
                .unwrap_or_else(|e| panic!("error waiting for status events from VM: {}", e))
            {
                None => continue 'monitor,
                Some(status) => match status {
                    VMExitMode::Soft => error!("guest vm resets unexpectedly"),
                    VMExitMode::Hard => error!("guest vm shuts down unexpectedly"),
                    VMExitMode::Host => error!("guest vm halted by the host unexpectedly"),
                },
                // TODO: handle VM failures
            }
            break 'monitor;
        }

        // advance fuzzer into the next session
        fuzzer.next_session();

        // refresh from a new snapshot
        qemu.snapshot_load()
            .unwrap_or_else(|e| panic!("error restoring a snapshot: {}", e));
        info!("snapshot reloaded");
    }

    if !check {
        // mark the end of fuzzing
        info!("fuzzing exited gracefully");

        // shutdown the VM
        match qemu.reset() {
            Ok(()) => info!("qemu exited gracefully"),
            Err(e) => error!("qemu exited with an unexpected error: {}", e),
        }
    }

    // drop the ivshmem at the end
    drop(ivshmem);
}
