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
  if (setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin", 1) < 0) {
    ABORT_WITH_ERRNO("unable to configure environment variable");
  }

  // mount filesystems
  checked_mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("sys", "/sys", "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("tmp", "/tmp", "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("run", "/run", "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV);
  checked_mount("dev", "/dev", "devtmpfs", MS_NOSUID | MS_NOEXEC);

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
