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
#include <sys/mman.h>
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

static inline size_t checked_read_line_from_fd(int fd, char *buf,
                                               size_t bufsize) {
  size_t cur = 0;
  do {
    ssize_t len = read(fd, buf + cur, bufsize - cur);
    if (len < 0) {
      ABORT_WITH_ERRNO("unable to read from fd");
    }
    if (len == 0) {
      continue;
    }

    cur += len;
    if (buf[cur - 1] == '\n') {
      buf[cur - 1] = '\0';
      // return the strlen, with \n stripped
      return cur - 1;
    }

    if (cur == bufsize) {
      ABORT_WITH("buffer size too small for read");
    }
  } while (true);
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
  if ((fd = open(path, O_RDONLY | O_SYNC)) < 0) {
    ABORT_WITH_ERRNO("unable to open file %s for read", path);
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
  close(fd);
}

static inline void check_config_tty(const char *path) {
  int fd;
  if ((fd = open(path, O_RDWR | O_NOCTTY | O_SYNC)) < 0) {
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

  // baud rates
  cfsetospeed(&attrs, B9600);
  cfsetispeed(&attrs, B9600);

  attrs.c_cflag &= ~(PARENB | PARODD); // disable parity bit
  attrs.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls
  attrs.c_cflag &= ~CSIZE;             // 8-bit characters: step 1
  attrs.c_cflag |= CS8;                // 8-bit characters: step 2
  attrs.c_cflag &= ~CSTOPB;            // only need 1 stop bit
  attrs.c_cflag &= ~CRTSCTS;           // disable hardware flow control

  // setup for non-canonical mode
  attrs.c_lflag &= ~ICANON;
  attrs.c_lflag &= ~(ECHO | ECHONL | ECHOE);
  attrs.c_lflag &= ~ISIG;
  attrs.c_lflag &= ~IEXTEN;

  attrs.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
  attrs.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                     ICRNL); // disable any special handling of received bytes

  attrs.c_oflag &= ~OPOST; // disable special interpretation of output bytes
  attrs.c_oflag &= ~(ONLCR | OCRNL); // disable conversion between NL and CR
  attrs.c_oflag &= ~OFILL;           // do not use fill characters for delay

  // fetch bytes as they become available
  attrs.c_cc[VMIN] = 1;
  attrs.c_cc[VTIME] = 0;

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
  if ((fd = open(path, O_RDWR | O_NOCTTY | O_SYNC)) < 0) {
    ABORT_WITH_ERRNO("unable to open tty %s for write", path);
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

  // drain the message before closing the fd
  tcdrain(fd);
  close(fd);
}

#ifndef RELEASE
static inline void list_dir_recursive(const char *path, int target_depth,
                                      int current_depth) {
  DIR *dir = opendir(path);
  if (dir == NULL) {
    ABORT_WITH_ERRNO("failed to open dir: %s", path);
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // ignore the links
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // probe for the items
    struct stat stats;
    if (fstatat(dirfd(dir), entry->d_name, &stats, AT_NO_AUTOMOUNT) != 0) {
      ABORT_WITH_ERRNO("failed to stat dir entry: %s/%s", path, entry->d_name);
    }
    fprintf(stderr, "%*s", current_depth * 2, "");
    fprintf(stderr, "%s | mode: %o, user: %d:%d\n", entry->d_name,
            stats.st_mode, stats.st_uid, stats.st_gid);

    // check whether to list more directories
    if (target_depth == current_depth) {
      continue;
    }
    if (S_ISDIR(stats.st_mode)) {
      size_t sub_path_len = strlen(path) + strlen(entry->d_name) + 2;
      char sub_path[sub_path_len];
      if (snprintf(sub_path, sub_path_len, "%s/%s", path, entry->d_name) < 0) {
        ABORT_WITH("failed to allocate buffer for %s/%s", path, entry->d_name);
      }
      list_dir_recursive(sub_path, target_depth, current_depth + 1);
    }
  }

  if (closedir(dir) != 0) {
    ABORT_WITH_ERRNO("failed to close dir: %s", path);
  }
}
static inline void list_dir(const char *path, int target_depth) {
  list_dir_recursive(path, target_depth, 0);
}
#endif

/*
 * Device Probing
 */

#define MAX_DENTRY_NAME_SIZE 256
#define MAX_PCI_IDENT_SIZE 64

#define IVSHMEM_VENDOR_ID "0x1af4"
#define IVSHMEM_DEVICE_ID "0x1110"
#define IVSHMEM_REVISION_ID "0x01"

#define IVSHMEM_SIZE 16 * 1024 * 1024

static inline bool __check_pci_ident(int dirfd, const char *kind,
                                     const char *expected) {
  char buf[MAX_PCI_IDENT_SIZE];
  if (faccessat(dirfd, kind, R_OK, 0) != 0) {
    return false;
  }

  int fd;
  if ((fd = openat(dirfd, kind, O_RDONLY | O_CLOEXEC)) < 0) {
    ABORT_WITH_ERRNO("failed to open PCI entry %s", kind);
  }
  checked_read_line_from_fd(fd, buf, MAX_PCI_IDENT_SIZE);
  close(fd);

  return strcmp(buf, expected) == 0;
}

static inline void *probe_for_ivshmem(void) {
  // indicator on whether we have found the ivshmem
  void *mem = NULL;

  // scan over the entries
  DIR *dir = opendir("/sys/bus/pci/devices/");
  if (dir == NULL) {
    ABORT_WITH_ERRNO("failed to open PCI device tree");
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    const char *name = entry->d_name;
    size_t name_len = strnlen(name, MAX_DENTRY_NAME_SIZE);
    if (name_len == 0 || name_len == MAX_DENTRY_NAME_SIZE) {
      // unexpected length
      continue;
    }

    // filter links
    if (name[0] == '.') {
      if (name_len == 1) {
        continue;
      }
      if (name[1] == '.' && name_len == 2) {
        continue;
      }
    }

    // filter non-directories
    struct stat stats;
    if (fstatat(dirfd(dir), name, &stats, AT_NO_AUTOMOUNT) != 0) {
      ABORT_WITH_ERRNO("failed to stat PCI entry: %s", name);
    }
    if (!S_ISDIR(stats.st_mode)) {
      continue;
    }

    // now we have a valid entry
    int dev_dir_fd;
    if ((dev_dir_fd = openat(dirfd(dir), name,
                             O_RDONLY | O_CLOEXEC | O_DIRECTORY)) < 0) {
      ABORT_WITH_ERRNO("failed to open PCI entry: %s", name);
    }

    // check device identity
    if (!__check_pci_ident(dev_dir_fd, "vendor", IVSHMEM_VENDOR_ID)) {
      goto skip;
    }
    if (!__check_pci_ident(dev_dir_fd, "device", IVSHMEM_DEVICE_ID)) {
      goto skip;
    }
    if (!__check_pci_ident(dev_dir_fd, "revision", IVSHMEM_REVISION_ID)) {
      goto skip;
    }

    // found ivshmem, and make sure there is only one device
    if (mem != NULL) {
      ABORT_WITH("more than one ivshmem device found");
    }

    // check for resources
    int dev_bar2_fd;
    if ((dev_bar2_fd = openat(dev_dir_fd, "resource2", O_RDWR) < 0)) {
      ABORT_WITH_ERRNO("unable to open BAR2 of ivshmem");
    }

    // map the memory
    mem = mmap(NULL, IVSHMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
               dev_bar2_fd, 0);
    if (mem == NULL) {
      ABORT_WITH_ERRNO("unable to mmap ivshmem");
    }

    // done with the initialization
    close(dev_bar2_fd);

  skip:
    // done with this entry
    close(dev_dir_fd);
  }

  if (closedir(dir) != 0) {
    ABORT_WITH_ERRNO("failed to close PCI device tree");
  }

  // check we do have a memory
  if (mem == NULL) {
    ABORT_WITH("unable to find the ivshmem device");
  }
  return mem;
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
