#include "utils.h"

/*
 * Project Constants
 */

#ifdef HARNESS
#define PATH_HARNESS "/home/harness"

#ifdef BLOB
#define PATH_BLOB "/home/blob"
#else
#define PATH_SHELL "/bin/sh"
#endif
#endif

#ifdef VIRTME
#define BIN_UDEV "/lib/systemd/systemd-udevd"
#else
#define BIN_MDEV "/bin/mdev"
#endif

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

  checked_mkdir("dev/pts");
  checked_mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC);

  checked_mount_tmpfs("/dev/shm");
  checked_mount_tmpfs("/var/cache");
  checked_mount_tmpfs("/var/log");
  checked_mount_tmpfs("/var/tmp");
  LOG_INFO("filesystems mounted");

  // setup devices
#ifdef VIRTME
  // setup udev
  checked_exec(BIN_UDEV, "--daemon", "--resolve-names=never");
  checked_exec("/bin/udevadm", "trigger", "--type=subsystems", "--action=add");
  checked_exec("/bin/udevadm", "trigger", "--type=devices", "--action=add");
  checked_exec("/bin/udevadm", "settle");
#else
  // setup mdev (an alternative to udev)
  const char *sys_hotplug = "/proc/sys/kernel/hotplug";
  if (access(sys_hotplug, F_OK) == 0) {
    checked_write(sys_hotplug, BIN_MDEV, strlen(BIN_MDEV));
    checked_exec(BIN_MDEV, "-s");
  }
#endif
  LOG_INFO("devices ready");

  // connect to a dedicated serial device
  list_dir("/dev");
  fprintf(stderr, "--------\n");
  list_dir("/dev/pts");

  // mark milestone
  LOG_INFO("[harness-fuzz] notified host on ready");

#ifdef HARNESS
#ifdef BLOB
  // testing mode
  return execl(PATH_HARNESS, PATH_HARNESS, PATH_BLOB);
#else
  // fuzzing mode (TODO)
  return execl(PATH_HARNESS, PATH_HARNESS);
#endif
#else
  // shell mode
  return execl(PATH_SHELL, PATH_SHELL)
#endif
}
