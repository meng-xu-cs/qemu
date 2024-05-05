#ifndef GUEST_UTILS_H
#define GUEST_UTILS_H

#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_EXEC_ARGS 128

/*
 * Logging Utility
 */

#define LOG_INFO(M, ...)                                                       \
  do {                                                                         \
    fprintf(stderr, "[harness-fuzz] |info| " M "\n", ##__VA_ARGS__);           \
  } while (0)

#define ABORT_WITH(M, ...)                                                     \
  do {                                                                         \
    fprintf(stderr, "[harness-fuzz] |critical| " M "\n", ##__VA_ARGS__);       \
    exit(1);                                                                   \
  } while (0)

#define ABORT_WITH_ERRNO(M, ...)                                               \
  do {                                                                         \
    fprintf(stderr, "[harness-fuzz] |critical| " M ": %s\n", ##__VA_ARGS__,    \
            strerror(errno));                                                  \
    exit(1);                                                                   \
  } while (0)

/*
 * Filesystem Utility
 */

static inline void checked_mount(const char *source, const char *target,
                                 const char *type, unsigned long flags) {
  if (mount(source, target, type, flags, NULL) != 0) {
    ABORT_WITH_ERRNO("failed to mount %s", target);
  }
}

static inline void checked_mkdir(const char *path) {
  if (mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IROTH) != 0) {
    ABORT_WITH_ERRNO("failed to mkdir %s", path);
  }
}

static inline void checked_mount_tmpfs(const char *path) {
  checked_mkdir(path);
  checked_mount("tmpfs", path, "tmpfs", MS_NOSUID | MS_NODEV);
}

static inline void checked_exec(const char *bin, ...) {
  const char *argv[MAX_EXEC_ARGS] = {NULL};

  // fork, exec, and wait
  pid_t pid = fork();
  if (pid == 0) {
    // child process

    // prepare for arguments
    argv[0] = bin;

    va_list vlist;
    int i = 1;
    const char *arg;
    va_start(vlist, bin);
    while ((arg = va_arg(vlist, const char *)) != NULL) {
      argv[i++] = arg;
    }
    va_end(vlist);

    argv[i] = NULL;
    execvp(bin, argv);
    ABORT_WITH_ERRNO("failed to run %s", bin);
  } else if (pid > 0) {
    // parent process
    int status;
    if (waitpid(pid, &status, 0) < 0) {
      ABORT_WITH_ERRNO("failed to wait for child %s", bin);
    }
    if (status != 0) {
      ABORT_WITH_ERRNO("child execution failed %s", bin);
    }
  } else {
    ABORT_WITH_ERRNO("failed to fork");
  }
}

static inline void checked_write(const char *path, const char *buf,
                                 size_t len) {
  int fd;
  if ((fd = open(path, O_CREAT | O_RDWR | O_TRUNC)) < 0) {
    ABORT_WITH_ERRNO("unable to open file %s", path);
  }

  ssize_t written = 0;
  do {
    ssize_t rv;
    rv = write(fd, buf + written, len);
    if (rv < 0) {
      ABORT_WITH_ERRNO("unable to write to file %s", path);
    }
    written += rv;
    if (written == len) {
      return;
    }
  } while (true);
}

static inline void list_dir(const char *path) {
  DIR *dir = opendir(path);
  if (dir == NULL) {
    ABORT_WITH_ERRNO("failed to open dir: %s", path);
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    fprintf(stderr, "%s\n", entry->d_name);
  }

  if (closedir(dir) != 0) {
    ABORT_WITH_ERRNO("failed to close dir: %s", path);
  }
}

/*
 * inotify-based Synchronization Utility
 */

// dnotify notification signal
#define DNOTIFY_SIGNAL (SIGRTMIN + 1)

// enumerate list of FDs to poll
enum { FD_POLL_SIGNAL = 0, FD_POLL_MAX };

static inline int __inotify_initialize_signals(void) {
  int signal_fd;
  sigset_t sigmask;

  // handle SIGINT, SIGTERM and SIGRTMIN+1 in the signal_fd
  sigemptyset(&sigmask);
  sigaddset(&sigmask, SIGINT);
  sigaddset(&sigmask, SIGTERM);
  sigaddset(&sigmask, DNOTIFY_SIGNAL);

  if (sigprocmask(SIG_BLOCK, &sigmask, NULL) < 0) {
    fprintf(stderr, "cannot block signals: '%s'\n", strerror(errno));
    return -1;
  }

  // get new FD to read signals from it
  if ((signal_fd = signalfd(-1, &sigmask, 0)) < 0) {
    fprintf(stderr, "cannot setup signal fd: '%s'\n", strerror(errno));
    return -1;
  }

  return signal_fd;
}

static inline int __dnotify_initialize(const char *path, int mask) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "cannot open directory %s: %s\n", path, strerror(errno));
    return -1;
  }

  // init directory notifications using DNOTIFY_SIGNAL
  if (fcntl(fd, F_SETSIG, DNOTIFY_SIGNAL) < 0 ||
      fcntl(fd, F_NOTIFY, mask) < 0) {
    fprintf(stderr, "cannot setup directory notifications in %s: %s\n", path,
            strerror(errno));
    close(fd);
    return -1;
  }

  return fd;
}

// block until a specific file is created or deleted in the watched directory
static inline int dnotify_watch(const char *dir, const char *file,
                                bool expect_create) {
  int signal_fd, dir_fd;
  struct pollfd fds[FD_POLL_MAX];

  // setup
  if ((signal_fd = __inotify_initialize_signals()) < 0) {
    return -1;
  }
  if ((dir_fd = __dnotify_initialize(dir, expect_create ? DN_CREATE
                                                        : DN_DELETE)) < 0) {
    close(signal_fd);
    return -1;
  }

  // check existence, skip waiting if existence (or nonexistence) is expected
  bool exists = faccessat(dir_fd, file, F_OK, 0) == 0;
  if (expect_create == exists) {
    goto shutdown;
  }

  // setup polling
  fds[FD_POLL_SIGNAL].fd = signal_fd;
  fds[FD_POLL_SIGNAL].events = POLLIN;

  // block until there is something to read
  if (poll(fds, FD_POLL_MAX, -1) < 0) {
    fprintf(stderr, "cannot poll(): '%s'\n", strerror(errno));
    goto exception;
  }

  // signal received?
  if (fds[FD_POLL_SIGNAL].revents & POLLIN) {
    struct signalfd_siginfo fdsi;

    if (read(fds[FD_POLL_SIGNAL].fd, &fdsi, sizeof(fdsi)) != sizeof(fdsi)) {
      fprintf(stderr, "cannot read signal, wrong size read\n");
      goto exception;
    }

    // break loop if we got the expected signal
    if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
      fprintf(stderr, "terminating by signal\n");
      exit(1);
    }

    // process event if dnotify signal received
    if (fdsi.ssi_signo == DNOTIFY_SIGNAL) {
      if (fdsi.ssi_fd != dir_fd) {
        fprintf(stderr, "unexpect dnotify event received\n");
        goto exception;
      }

      bool exists_on_event = faccessat(dir_fd, file, F_OK, 0) == 0;
      if (expect_create == exists_on_event) {
        goto shutdown;
      } else {
        goto exception;
      }
    }

    // other signals are not expected
    fprintf(stderr, "received unexpected signal\n");
    goto exception;
  }

  // shutdown
shutdown:
  close(dir_fd);
  close(signal_fd);
  return 0;

  // exception
exception:
  close(dir_fd);
  close(signal_fd);
  return -1;
}

#endif // GUEST_UTILS_H
