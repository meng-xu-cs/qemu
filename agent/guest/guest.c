#include "utils.h"

/*
 * Project Constants
 */

#if defined(MODE_Fuzz) || defined(MODE_Test) || defined(MODE_Check)
#define PATH_HARNESS "/root/harness"
#define PATH_BLOB "/root/blob"
#endif
#ifdef MODE_Shell
#define PATH_SHELL "/bin/sh"
#endif

#ifdef SETUP_Simple
#define BIN_MDEV "/bin/mdev"
#endif
#ifdef SETUP_Virtme
#define BIN_UDEV "/lib/systemd/systemd-udevd"
#endif

#define IVSHMEM_SIZE (16 * 1024 * 1024)
#define FUZZING_TIME (24 * 60 * 60)

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
#if defined(SETUP_Simple) || defined(SETUP_Virtme)
  if (setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin", 1) != 0) {
    ABORT_WITH_ERRNO("unable to configure environment variable");
  }
#endif

  // mount filesystems
#ifdef SETUP_Bare
  checked_mount("sys", "/sys", "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
#endif
#if defined(SETUP_Simple) || defined(SETUP_Virtme)
  checked_mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("sys", "/sys", "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("tmp", "/tmp", "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("run", "/run", "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("dev", "/dev", "devtmpfs", MS_NOSUID | MS_NOEXEC);

  checked_mkdir("dev/pts");
  checked_mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC);

  checked_mount_tmpfs("/dev/shm");
  checked_mount_tmpfs("/var/cache");
  checked_mount_tmpfs("/var/log");
  checked_mount_tmpfs("/var/tmp");
  checked_mkdir("/run/dbus");
#endif
  sync();
  LOG_INFO("filesystems mounted");

  // setup devices
#ifdef SETUP_Simple
  // using mdev
  const char *sys_hotplug = "/proc/sys/kernel/hotplug";
  if (checked_exists(sys_hotplug)) {
    checked_write_or_create(sys_hotplug, BIN_MDEV, strlen(BIN_MDEV));
  }
  checked_exec(BIN_MDEV, "-s", NULL);
  LOG_INFO("devices ready");
#endif
#ifdef SETUP_Virtme
  // using udev
  const char *sys_helper = "/sys/kernel/uevent_helper";
  if (checked_exists(sys_helper)) {
    checked_trunc(sys_helper);
  }
  checked_exec(BIN_UDEV, "--daemon", "--resolve-names=never", NULL);
  checked_exec("udevadm", "trigger", "--type=subsystems", "--action=add", NULL);
  checked_exec("udevadm", "trigger", "--type=devices", "--action=add", NULL);
  checked_exec("udevadm", "settle", NULL);
  LOG_INFO("devices ready");
#endif

#if defined(SETUP_Simple) || defined(SETUP_Virtme)
  // setup network
  checked_exec("ip", "link", "set", "dev", "lo", "up", NULL);
  LOG_INFO("network ready");
#endif

#if defined(MODE_Fuzz) || defined(MODE_Test) || defined(MODE_Check)
#ifdef MODE_Fuzz
  LOG_INFO("entered fuzzing mode");
#endif
#ifdef MODE_Test
  LOG_INFO("entered testing mode");
#endif
#ifdef MODE_Check
  LOG_INFO("entered checking mode");
#endif

  // connect to the ivshmem device
  struct ivshmem pack;
  probe_ivshmem(&pack, IVSHMEM_SIZE);
  LOG_INFO("ivshmem ready");
  struct vmio *vmio = pack.addr;

#if defined(MODE_Fuzz) || defined(MODE_Check)
  // wait for host to be ready
  while (atomic_load(&vmio->flag) == 0) {
    // do nothing, this (unexpected) busy waiting is intentional
  }
  atomic_store(&vmio->spin_guest, 1);
  LOG_INFO("notified host on ready");
  atomic_store(&vmio->spin_host, 0);

  // wait for host to release us
  while (atomic_load(&vmio->spin_guest)) {
    // busy wait
  }
  LOG_INFO("operation resumed by host");

  // prepare the blob file
  uint64_t blob_size = atomic_load(&vmio->size);
  if (blob_size == 0) {
    ABORT_WITH("received a zero-sized blob");
  }
  checked_write_or_create(PATH_BLOB, vmio->buf, blob_size);
#endif

  // invoke harness
  int status = unchecked_exec(PATH_HARNESS, PATH_BLOB, NULL);
  LOG_INFO("harness terminated with status %d", status);

#ifdef MODE_Fuzz
  // inform the host on completion
  atomic_store(&vmio->completed, 1);

  // put into (almost) infinite sleep
  sleep(FUZZING_TIME);
#endif

#ifdef MODE_Test
  // save the return status into vmio
  atomic_store(&vmio->flag, status);
#endif

  // unmap the memory in the end
  unmap_ivshmem(&pack);
  LOG_INFO("ivshmem unmapped");
#endif

#ifdef MODE_Shell
  // shell mode
  LOG_INFO("entered shell mode");
  checked_exec(PATH_SHELL, NULL);
#endif

  // reboot before exiting
  reboot(RB_AUTOBOOT);
  return 1;
}
