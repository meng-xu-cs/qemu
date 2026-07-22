use std::os::unix::net::UnixStream;
use std::path::PathBuf;

use log::{error, info, LevelFilter};
use structopt::StructOpt;

use crate::fuzz::Fuzzer;
use crate::qemu::{QemuProxy, VMExitMode};
use crate::guest::{Guest, GuestStatus};
use crate::utils::{inotify_watch_for_addition, Ivshmem};

mod fuzz;
mod qemu;
mod guest;
mod utils;

const VM_MONITOR_SOCKET_PREFIX: &str = "monitor";
const VM_IVSHMEM_FILE_PREFIX: &str = "ivshmem";
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
    /// number of workers
    #[structopt(long)]
    workers: usize,
}

pub fn entrypoint() {
    let Options {
        path_tmp,
        corpus,
        output,
        check,
        verbose,
        workers,
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
    let mut fuzzer = Fuzzer::new(corpus, output.clone())
        .unwrap_or_else(|e| panic!("error initializing the fuzzer: {}", e));

    let mut guests: Vec<Guest> = Vec::new();
    for id in 0..workers {
        // wait for ivshmem to be created
        inotify_watch_for_addition(&path_tmp, &format!("{}{}", VM_IVSHMEM_FILE_PREFIX, id))
            .unwrap_or_else(|e| panic!("error waiting for creation of ivshmem{}: {}", id, e));
        info!("QEMU is up and running");

        let path_ivshmem = path_tmp.join(&format!("{}{}", VM_IVSHMEM_FILE_PREFIX, id));
        let mut ivshmem = Ivshmem::new(&path_ivshmem, VM_IVSHMEM_SIZE)
            .unwrap_or_else(|e| panic!("error mapping ivshmem: {}", e));

        let vmio = ivshmem.vmio();
        vmio.init()
            .unwrap_or_else(|e| panic!("error initializing vmio: {}", e));
        info!("vmio initialized");

        // connect to QEMU monitor
        let path_monitor_socket = path_tmp.join(&format!("{}{}", VM_MONITOR_SOCKET_PREFIX, id));
        let stream = Box::leak(Box::new(UnixStream::connect(path_monitor_socket)
            .unwrap_or_else(|e| panic!("error connecting to QEMU monitor socket: {}", e))));
        let mut qemu = QemuProxy::new(stream)
            .unwrap_or_else(|e| panic!("error negotiating with the QEMU-QMP: {}", e));
        info!("QEMU-QMP connected");

        // sync with guest on start-up
        vmio.wait_on_host();
        info!("guest agent is ready");

        // save a live snapshot
        qemu.snapshot_save()
            .unwrap_or_else(|e| panic!("error taking a snapshot: {}", e));
        info!("live snapshot is taken");

        // construct a guest struct
        guests.push(
            Guest::new(id, ivshmem, qemu, &output)
                .unwrap_or_else(|e| panic!("error initializing guest {}: {}", id, e)),
        );
    }

    let mut waiting_guests = workers;
    let mut index = 0;
    'fuzzing: loop {
        let id = index % workers;
        let guest = &mut guests[id];
        index += 1;

        'monitor: loop {
            match guest.status {
                GuestStatus::Ready => {
                    // put the seed into shared memory
                    let (seed, seed_id) = fuzzer.current_seed().expect("loading current seed");
                    guest.seed_id = seed_id;
                    guest.ivshmem.vmio().prepare_blob(&seed);

                    // release the guest
                    guest.ivshmem.vmio().post_to_guest();
                    info!("notified guest agent {} to continue, seed ID: {}", id, seed_id);

                    // skip if we are in checking mode (e.g., unit testing)
                    if check {
                        break 'fuzzing;
                    }

                    guest.status = GuestStatus::Running;
                }
                GuestStatus::Running => {
                    if guest.ivshmem.vmio().check_completion() {
                        info!("guest agent completed successfully");

                        guest.status = GuestStatus::Completed;
                    } else {
                        match guest.qemu
                            .check_guest_reset()
                            .unwrap_or_else(|e| panic!("error waiting for status events from VM {}: {}", id, e))
                        {
                            None => break 'monitor,
                            Some(status) => match status {
                                VMExitMode::Soft => error!("guest vm resets unexpectedly"),
                                VMExitMode::Hard => error!("guest vm shuts down unexpectedly"),
                                VMExitMode::Host => error!("guest vm halted by the host unexpectedly"),
                            },
                            // TODO: handle VM failures
                        }
                    }
                }
                GuestStatus::Completed => {
                    match fuzzer.process_session_result(guest.id, guest.session_id, guest.seed_id) {
                        Ok(()) => (),
                        Err(e) => {
                            error!("unexpect error in processing session result: {}", e);
                            break 'fuzzing;
                        }
                    };

                    // advance fuzzer into the next session
                    fuzzer.next_session();

                    // advance guest into the next session
                    guest.next_session();

                    // refresh from a new snapshot
                    guest.qemu.snapshot_load()
                        .unwrap_or_else(|e| panic!("error restoring a snapshot: {}", e));
                    info!("snapshot reloaded");

                    waiting_guests += 1;
                    guest.status = GuestStatus::Waiting;
                }
                GuestStatus::Waiting => {
                    if fuzzer.has_pending_seeds() {
                        waiting_guests -= 1;
                        guest.status = GuestStatus::Ready;
                    } else {
                        if waiting_guests == workers {
                            break 'fuzzing;
                        }
                        break 'monitor;
                    }
                }
            }
        }
    }

    if !check {
        // mark the end of fuzzing
        info!("fuzzing exited gracefully");

        // shutdown the VM
        for id in 0..workers {
            match guests[id].qemu.reset() {
                Ok(()) => info!("qemu{} exited gracefully", id),
                Err(e) => error!("qemu{} exited with an unexpected error: {}", id, e),
            }
        }
    }
}
