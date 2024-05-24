#![no_main]

use libfuzzer_sys::{fuzz_mutator, fuzz_target};
use std::os::unix::net::UnixStream;
use std::path::PathBuf;

use log::{info, LevelFilter};

use std::env;

use qce_agent_host::qemu::{QemuProxy, VMExitMode};
use qce_agent_host::utils::{inotify_watch_for_addition, Ivshmem, Vmio};

static mut INITED: bool = false;

const VM_MONITOR_SOCKET: &str = "monitor";
const VM_IVSHMEM_FILE: &str = "ivshmem";
const VM_IVSHMEM_SIZE: usize = 16 * 1024 * 1024;

static mut VMIO: Option<&mut Vmio> = None;
static mut QEMU: Option<QemuProxy> = None;
static mut IVSHMEM: Option<Ivshmem> = None;

fuzz_target!(|data: &[u8]| {

    unsafe {
        if !INITED {
            // fuzzed code goes here
            // let args = Options::from_args();
            let verbose = env::var("AIXCC_KERNEL_FUZZ_VERBOSE").is_ok();
            let path_tmp_var = env::var("AIXCC_KERNEL_FUZZ_TMP").unwrap_or_else(|_| "/tmp/aixcc_kernel_fuzz".to_string());
            let path_tmp = PathBuf::from(path_tmp_var);
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

            // wait for ivshmem to be created
            info!("ivshmem file will create on {}", path_tmp.display());
            inotify_watch_for_addition(&path_tmp, VM_IVSHMEM_FILE)
                .unwrap_or_else(|e| panic!("error waiting for creation of ivshmem: {}", e));
            info!("QEMU is up and running");

            let path_ivshmem = path_tmp.join(VM_IVSHMEM_FILE);
            IVSHMEM = Some(Ivshmem::new(&path_ivshmem, VM_IVSHMEM_SIZE)
                .unwrap_or_else(|e| panic!("error mapping ivshmem: {}", e)));
            let ivshmem = IVSHMEM.as_mut().unwrap();

            VMIO = Some(ivshmem.vmio()); 
            let vmio = VMIO.as_deref_mut().unwrap();
            vmio.init()
                .unwrap_or_else(|e| panic!("error initializing vmio: {}", e));
            info!("vmio initialized");

            // connect to QEMU monitor
            let path_monitor_socket = path_tmp.join(VM_MONITOR_SOCKET);
            let stream = Box::new(UnixStream::connect(path_monitor_socket)
                .unwrap_or_else(|e| panic!("error connecting to QEMU monitor socket: {}", e)));
            QEMU = Some(QemuProxy::new(Box::leak(stream))
                .unwrap_or_else(|e| panic!("error negotiating with the QEMU-QMP: {}", e)));
            info!("QEMU-QMP connected");
            let qemu = QEMU.as_mut().unwrap();

            // sync with guest on start-up
            vmio.wait_on_host();
            info!("guest agent is ready");

            // save a live snapshot
            qemu.snapshot_save()
                .unwrap_or_else(|e| panic!("error taking a snapshot: {}", e));
            info!("live snapshot is taken");
            INITED = true;
        }
    

    // fuzzing loop
    // release the guest
    let vmio = VMIO.as_deref_mut().unwrap();

    // send data to guest
    vmio.send_fuzz_input(data, data.len());

    // debug 
    info!("fuzz input sent to guest: {:?}", data);

    // TODO: use FFI to convert proto data -> raw data

    // vmio.post_to_guest();
    info!("notified guest agent to continue");

    // wait for guest to stop
    let qemu = QEMU.as_mut().unwrap();
    match qemu
        .wait_for_guest_reset()
        .unwrap_or_else(|e| panic!("error waiting for status events from VM: {}", e))
    {
        VMExitMode::Soft => (),
        VMExitMode::Hard | VMExitMode::Host => panic!("guest vm stopped"),
    }
    info!("guest vm stopped");

    // get kcov info from vm 
    let kcov_info = vmio
        .get_kcov_info();

    // debug: output kcov array, as hex array
    info!("kcov info: {:?}", kcov_info);

    // always refresh from a new snapshot
    qemu.snapshot_load()
        .unwrap_or_else(|e| panic!("error restoring a snapshot: {}", e));
    info!("snapshot reloaded");

    }
    // }

    // technically we should never reach here
    // error!("guest vm is forced to exit in an unexpected way");

    // drop the ivshmem at the end
    // drop(ivshmem);
});


// fuzz_mutator!(
//     |data: &mut [u8], size: usize, max_size: usize, _seed: u32| {
            // TODO: use FFI to call libprotobuf-mutator
//     }
// );