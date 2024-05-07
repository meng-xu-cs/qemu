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

#define AGENT_TTY "/dev/ttyS1"
#define AGENT_MARK_READY "ready\n"

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

  // connect to a dedicated serial (tty)
  check_config_tty(AGENT_TTY);

  // signal readiness
  checked_tty_write(AGENT_TTY, AGENT_MARK_READY, strlen(AGENT_MARK_READY));

  // mark milestone
  LOG_INFO("notified host on ready");

  // wait for host to release us
  char message[MAX_LEN_OF_MESSAGE];
  checked_tty_read_line(AGENT_TTY, message, MAX_LEN_OF_MESSAGE);
  if (strcmp(message, AGENT_MARK_READY) != 0) {
    ABORT_WITH("unexpected response: %s, expecting %s", message,
               AGENT_MARK_READY);
  }
  LOG_INFO("operation resumed by host");

#ifdef HARNESS
#ifdef BLOB
  // testing mode
  LOG_INFO("entered testing mode");
  checked_exec(PATH_HARNESS, PATH_BLOB, NULL);
#else
  // fuzzing mode (TODO)
  LOG_INFO("entered fuzzing mode");
  checked_exec(PATH_HARNESS, NULL);
#endif
#else
  // shell mode
  LOG_INFO("entered shell mode");
  checked_exec(PATH_SHELL, NULL)
#endif
  return 1;
}
