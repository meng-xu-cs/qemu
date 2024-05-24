#include "utils.h"

/*
 * Project Constants
 */

#if defined(MODE_Fuzz) || defined(MODE_Test)
#define PATH_HARNESS "/root/harness"
#define PATH_BLOB "/root/blob"
#endif
#ifdef MODE_Shell
#define PATH_SHELL "/bin/sh"
#endif

#ifdef VIRTME
#define BIN_UDEV "/lib/systemd/systemd-udevd"
#else
#define BIN_MDEV "/bin/mdev"
#endif

#define IVSHMEM_SIZE (16 * 1024 * 1024)

#define KCOV_COVER_SIZE (256 << 10)
#define KCOV_TRACE_PC 0
#define KCOV_INIT_TRACE64 _IOR('c', 1, uint64_t)
#define KCOV_ENABLE _IO('c', 100)

void fail(const char* msg, ...) {
	int e = errno;
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, " (errno %d)\n", e);
	_exit(1);
}

/*
 * Entrypoint
 */

int main(int argc, char *argv[]) {
  // sanity check
  if (argc != 1) {
    ABORT_WITH("unexpected arguments for %s", argv[0]);
  }
  LOG_INFO("guest agent started");

  // configure environment variables
  if (setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin", 1) != 0) {
    ABORT_WITH_ERRNO("unable to configure environment variable");
  }

  // mount filesystems
  checked_mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("sys", "/sys", "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("tmp", "/tmp", "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("run", "/run", "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("dev", "/dev", "devtmpfs", MS_NOSUID | MS_NOEXEC);

  checked_mount("debug", "/sys/kernel/debug", "debugfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);

  checked_mkdir("dev/pts");
  checked_mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC);

  checked_mount_tmpfs("/dev/shm");
  checked_mount_tmpfs("/var/cache");
  checked_mount_tmpfs("/var/log");
  checked_mount_tmpfs("/var/tmp");
  checked_mkdir("/run/dbus");

  sync();
  LOG_INFO("filesystems mounted");

  // setup devices
#ifdef VIRTME
  // setup udev
  const char *sys_helper = "/sys/kernel/uevent_helper";
  if (checked_exists(sys_helper)) {
    checked_trunc(sys_helper);
  }
  checked_exec(BIN_UDEV, "--daemon", "--resolve-names=never", NULL);
  checked_exec("udevadm", "trigger", "--type=subsystems", "--action=add", NULL);
  checked_exec("udevadm", "trigger", "--type=devices", "--action=add", NULL);
  checked_exec("udevadm", "settle", NULL);
#else
  // setup mdev (an alternative to udev)
  const char *sys_hotplug = "/proc/sys/kernel/hotplug";
  if (checked_exists(sys_hotplug)) {
    checked_write_or_create(sys_hotplug, BIN_MDEV, strlen(BIN_MDEV));
  }
  checked_exec(BIN_MDEV, "-s", NULL);
#endif
  LOG_INFO("devices ready");

  // setup network
  checked_exec("ip", "link", "set", "dev", "lo", "up", NULL);
  LOG_INFO("network ready");

#ifdef MODE_Test
  // testing mode
  LOG_INFO("entered testing mode");
  checked_exec(PATH_HARNESS, PATH_BLOB, NULL);
#endif

#ifdef MODE_Fuzz
  // fuzzing mode
  checked_write_or_create(PATH_BLOB, "X", 1);

  // connect to the ivshmem device
  struct ivshmem pack;
  probe_ivshmem(&pack, IVSHMEM_SIZE);
  LOG_INFO("ivshmem ready");


  // wait for host to be ready
  struct vmio *vmio = pack.addr;
  while (atomic_load(&vmio->flag) == 0) {
    // do nothing, this (unexpected) busy waiting is intentional
  }

  // check and enable kcov
  int kcov = open("/sys/kernel/debug/kcov", O_RDWR);
	if (kcov == -1)
		fail("open of /sys/kernel/debug/kcov failed");
	if (ioctl(kcov, KCOV_INIT_TRACE64, KCOV_COVER_SIZE))
		fail("init trace write failed");
	uint64_t* kcov_data = (uint64_t*)mmap(NULL, KCOV_COVER_SIZE * sizeof(kcov_data[0]),
				    PROT_READ | PROT_WRITE, MAP_SHARED, kcov, 0);
	if (kcov_data == MAP_FAILED)
		fail("mmap failed");
	if (ioctl(kcov, KCOV_ENABLE, KCOV_TRACE_PC))
		fail("enable write trace failed");
	close(kcov);

  atomic_store(&vmio->spin_guest, 1);
  atomic_store(&vmio->spin_host, 0);
  LOG_INFO("notified host on ready");

  // wait for host to release us
  while (atomic_load(&vmio->spin_guest)) {
    // busy wait
  }
  LOG_INFO("operation resumed by host");

  if (vmio->size > 0) {
    // get the blob from the host
    checked_write_or_create(PATH_BLOB, vmio->buf, vmio->size);
    LOG_INFO("get blob from host");

    // debug
    LOG_INFO("blob size: %d", vmio->size);
    for (int i = 0; i < vmio->size; i++) {
      LOG_INFO("blob[%d]: %c", i, vmio->buf[i]);
    }
  }

  // unmap the memory before proceeding with any actions
  unmap_ivshmem(&pack);

  LOG_INFO("entered fuzzing mode");

  // start kcov
  __atomic_store_n(&kcov_data[0], 0, __ATOMIC_RELAXED);

  checked_exec(PATH_HARNESS, PATH_BLOB, NULL);

  // get kcov data & send kcov data to the host
  uint64_t ncov = __atomic_load_n(&kcov_data[0], __ATOMIC_RELAXED);
  LOG_INFO("kcov data length: %llu", ncov);
	if (ncov >= KCOV_COVER_SIZE)
		fail("too much cover: %llu", ncov);
  LOG_INFO("store kcov len");
  atomic_store(&vmio->size, ncov * 8);
  LOG_INFO("store kcov data");
	for (uint64_t i = 0; i < ncov; i++) {
		uint64_t pc = __atomic_load_n(&kcov_data[i + 1], __ATOMIC_RELAXED);
		atomic_store(&vmio->buf[i * 8], pc);
    // LOG_INFO("kcov data[%llu]: %llu", i, pc);
	}

#endif

#ifdef MODE_Shell
  // shell mode
  LOG_INFO("entered shell mode");
  checked_exec(PATH_SHELL, NULL);
#endif

  return 1;
}
