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
#include <sys/epoll.h>
#include <sys/mount.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#define MAX_EXEC_ARGS 16
#define MAX_LEN_OF_MESSAGE 16

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

static inline bool checked_exists(const char *path) {
  return access(path, F_OK) == 0;
}

static inline void checked_trunc(const char *path) {
  int fd;
  if ((fd = open(path, O_CREAT | O_RDWR | O_TRUNC)) < 0) {
    ABORT_WITH_ERRNO("unable to open file %s", path);
  }
  close(fd);
}

static inline void checked_write_or_create(const char *path, const char *buf,
                                           size_t len) {
  int fd;
  if ((fd = open(path, O_CREAT | O_RDWR | O_TRUNC)) < 0) {
    ABORT_WITH_ERRNO("unable to open file %s for write", path);
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

  fsync(fd);
  close(fd);
}

static inline void checked_tty_read_line(const char *path, char *buf,
                                         size_t size) {
  int fd;
  if ((fd = open(path, O_RDONLY | O_NONBLOCK)) < 0) {
    ABORT_WITH_ERRNO("unable to open file %s for read", path);
  }
  if (fcntl(fd, F_SETFL, 0) < 0) {
    ABORT_WITH_ERRNO("failed to fcntl on the tty at %s", path);
  }

  int flags;
  if (ioctl(fd, TIOCMGET, &flags) < 0) {
    ABORT_WITH_ERRNO("failed to ioctl(TIOCMGET) on the tty at %s", path);
  }

  // set dtr
  flags |= TIOCM_DTR;
  if (ioctl(fd, TIOCMSET, &flags) < 0) {
    ABORT_WITH_ERRNO("failed to set DTR on the tty at %s", path);
  }

  // wait
  int epoll_fd;
  if ((epoll_fd = epoll_create1(0)) < 0) {
    ABORT_WITH_ERRNO("unable epoll");
  }

  struct epoll_event event = {.events = EPOLLIN, .data.fd = fd};
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) != 0) {
    ABORT_WITH_ERRNO("epoll add failed");
  }

  size_t len = 0;
  struct epoll_event event_received;
  do {
    int count;
    if ((count = epoll_wait(epoll_fd, &event_received, 1, -1)) < 0) {
      ABORT_WITH_ERRNO("epoll wait failed");
    }
    if (count == 0) {
      // TODO: handle timeout
      ABORT_WITH("epoll timed out");
    }
    if (count != 1) {
      ABORT_WITH("expecting one and only one event");
    }
    if (event_received.data.fd != fd) {
      ABORT_WITH("expecting an event related to the monitored fd");
    }

    ssize_t rv;
    if ((rv = read(fd, buf + len, size - len)) < 0) {
      ABORT_WITH_ERRNO("failed to read %s", path);
    }
    len += rv;
    if (len == size) {
      ABORT_WITH("buffer size too small");
    }

    if (len >= 1 && buf[len - 1] == '\n') {
      buf[len] = '\0';
      break;
    } else if (len >= 2 && buf[len - 1] == '\0' && buf[len - 2] == '\n') {
      break;
    }
  } while (true);
  close(epoll_fd);

  // clear dtr
  flags &= ~TIOCM_DTR;
  if (ioctl(fd, TIOCMSET, &flags) < 0) {
    ABORT_WITH_ERRNO("failed to clear RTS on the tty at %s", path);
  }
  close(fd);
}

static inline void check_config_tty(const char *path) {
  int fd;
  if ((fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
    ABORT_WITH_ERRNO("unable to open tty at %s", path);
  }
  if (!isatty(fd)) {
    ABORT_WITH_ERRNO("tty opened at %s is not a tty", path);
  }

  // mark exclusive access
  if (ioctl(fd, TIOCEXCL, NULL) < 0) {
    ABORT_WITH_ERRNO("failed to set exclusive access with the tty at %s", path);
  }

  // configure the tty
  struct termios attrs;
  if (tcgetattr(fd, &attrs) < 0) {
    ABORT_WITH_ERRNO("unable to get attributes of the tty at %s", path);
  }

  // turn off input processing
  attrs.c_iflag &= ~(IGNBRK | BRKINT // convert break to null byte
                     | ICRNL         // no CR to NL translation
                     | INLCR         // no NL to CR translation
                     | PARMRK        // don't mark parity errors or breaks
                     | INPCK         // no input parity check
                     | ISTRIP        // don't strip high bit off
                     | IXON          // no XON/XOFF software flow control
  );
  // turn off output processing
  attrs.c_oflag &= ~(OCRNL    // no CR to NL translation
                     | ONLCR  // no NL to CR-NL translation
                     | ONLRET // no NL to CR translation
                     | ONOCR  // no column 0 CR suppression
                     | OFILL  // no fill characters
                     | OLCUC  // no case mapping
                     | OPOST  // no local output processing
  );
  // turn off line processing
  attrs.c_lflag &= ~(ICANON   // canonical mode off
                     | ECHO   // echo off
                     | ECHONL // echo newline off
                     | IEXTEN // extended input processing off
                     | ISIG   // signal chars off
  );
  // turn off character processing
  attrs.c_cflag &= ~(CSIZE | // clear current char size mask
                     PARENB  // no parity checking
  );
  attrs.c_cflag |= CREAD | CS8 | CLOCAL; // force 8 bit input

  // other configs
  // - one input byte is enough to return from read()
  // - inter-character timer off
  attrs.c_cc[VMIN] = 1;
  attrs.c_cc[VTIME] = 0;

  // baud rates
  cfsetospeed(&attrs, B115200);
  cfsetispeed(&attrs, B115200);

  // apply the configuration
  if (tcsetattr(fd, TCSANOW, &attrs) < 0) {
    ABORT_WITH_ERRNO("unable to configure the tty at %s", path);
  }

  // done
  close(fd);
}

static inline void checked_tty_write(const char *path, const char *buf,
                                     size_t len) {
  int fd;
  if ((fd = open(path, O_RDWR)) < 0) {
    ABORT_WITH_ERRNO("unable to open tty %s for write", path);
  }
  if (fcntl(fd, F_SETFL, 0) < 0) {
    ABORT_WITH_ERRNO("failed to fcntl on the tty at %s", path);
  }

  int flags;
  if (ioctl(fd, TIOCMGET, &flags) < 0) {
    ABORT_WITH_ERRNO("failed to ioctl(TIOCMGET) on the tty at %s", path);
  }

  // set rts
  flags |= TIOCM_RTS;
  if (ioctl(fd, TIOCMSET, &flags) < 0) {
    ABORT_WITH_ERRNO("failed to set RTS on the tty at %s", path);
  }

  // transmit
  ssize_t written = 0;
  do {
    ssize_t rv;
    rv = write(fd, buf + written, len);
    if (rv < 0) {
      ABORT_WITH_ERRNO("unable to write to tty %s", path);
    }
    written += rv;
    if (written == len) {
      return;
    }
  } while (true);
  tcdrain(fd);

  // clear rts
  flags &= ~TIOCM_RTS;
  if (ioctl(fd, TIOCMSET, &flags) < 0) {
    ABORT_WITH_ERRNO("failed to clear RTS on the tty at %s", path);
  }
  close(fd);
}

static inline void list_dir(const char *path) {
  DIR *dir = opendir(path);
  if (dir == NULL) {
    ABORT_WITH_ERRNO("failed to open dir: %s", path);
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    fprintf(stderr, "%s\n", entry->d_name);

    struct stat stats;
    if (fstatat(dirfd(dir), entry->d_name, &stats, AT_NO_AUTOMOUNT) != 0) {
      ABORT_WITH_ERRNO("failed to stat dir entry: %s/%s", path, entry->d_name);
    }
    fprintf(stderr, "mode: %o, user: %d:%d\n", stats.st_mode, stats.st_uid,
            stats.st_gid);
  }

  if (closedir(dir) != 0) {
    ABORT_WITH_ERRNO("failed to close dir: %s", path);
  }
}

/*
 * Subprocess Utility
 */

static inline void checked_exec(const char *bin, ...) {
  // prepare for arguments
  char const *argv[MAX_EXEC_ARGS] = {NULL};
  argv[0] = bin;

  va_list vlist;
  va_start(vlist, bin);

  int i = 1;
  const char *arg;
  while ((arg = va_arg(vlist, const char *)) != NULL) {
    argv[i++] = arg;
    if (i == MAX_EXEC_ARGS) {
      ABORT_WITH("exec has more than %d arguments", MAX_EXEC_ARGS);
    }
  }

  va_end(vlist);
  argv[i] = NULL;

  // fork, exec, and wait
  pid_t pid = fork();
  if (pid == 0) {
    // child process
    execvp(bin, (char *const *)argv);
    ABORT_WITH_ERRNO("failed to run %s", bin);
  } else if (pid > 0) {
    // parent process
    int status;
    if (waitpid(pid, &status, 0) < 0) {
      ABORT_WITH_ERRNO("failed to wait for child %s", bin);
    }
    if (status != 0) {
      ABORT_WITH("child execution failed %s with status %d", bin, status);
    }
  } else {
    ABORT_WITH_ERRNO("failed to fork");
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
