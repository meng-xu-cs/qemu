#include "utils.h"

/*
 * Project Constants
 */

#ifdef HARNESS
#define PATH_HARNESS "/root/harness"

#ifdef BLOB
#define PATH_BLOB "/root/blob"
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
  // NOTE: (device already populated)
  // checked_mount("dev", "/dev", "devtmpfs", MS_NOSUID | MS_NOEXEC);

  checked_mkdir("dev/pts");
  checked_mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC);

  checked_mount_tmpfs("/dev/shm");
  checked_mount_tmpfs("/var/cache");
  checked_mount_tmpfs("/var/log");
  checked_mount_tmpfs("/var/tmp");
  checked_mkdir("/run/dbus");
  LOG_INFO("filesystems mounted");

  // setup devices
#ifdef VIRTME
  // setup udev
  const char *sys_helper = "/sys/kernel/uevent_helper";
  if (checked_exists(sys_helper)) {
    checked_trunc(sys_helper);
  }
  checked_exec(BIN_UDEV, "--daemon", "--resolve-names=never");
  checked_exec("udevadm", "trigger", "--type=subsystems", "--action=add");
  checked_exec("udevadm", "trigger", "--type=devices", "--action=add");
  checked_exec("udevadm", "settle");
#else
  // setup mdev (an alternative to udev)
  const char *sys_hotplug = "/proc/sys/kernel/hotplug";
  if (checked_exists(sys_hotplug)) {
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
  LOG_INFO("notified host on ready");

#ifdef HARNESS
#ifdef BLOB
  // testing mode
  LOG_INFO("entered testing mode");
  checked_exec(PATH_HARNESS, PATH_BLOB);
#else
  // fuzzing mode (TODO)
  LOG_INFO("entered fuzzing mode");
  checked_exec(PATH_HARNESS);
#endif
#else
  // shell mode
  LOG_INFO("entered shell mode");
  checked_exec(PATH_SHELL)
#endif
  return 1;
}
