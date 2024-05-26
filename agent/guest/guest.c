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

void fail(const char *msg, ...) {
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
  checked_mount("debug", "/sys/kernel/debug", "debugfs",
                MS_NOSUID | MS_NOEXEC | MS_NODEV);
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
  LOG_INFO("entered fuzzing mode");
  checked_exec(PATH_HARNESS, PATH_BLOB, NULL);
#endif

#ifdef MODE_Shell
  // shell mode
  LOG_INFO("entered shell mode");
  checked_exec(PATH_SHELL, NULL);
#endif

  return 1;
}
