#ifndef AGENT_COMMON_H
#define AGENT_COMMON_H

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

/*
 * Project Constants
 */

// host-guest synchronization mark
#define FILE_MARK "MARK"

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

/*
 * IO-utilities
 */

// create a new file at directory
static inline int touch(const char *dir, const char *file) {
  int fd;

  int dir_fd = open(dir, O_DIRECTORY | O_RDONLY);
  if ((fd = openat(dir_fd, file, O_CREAT | S_IRUSR | S_IWUSR)) < 0) {
    fprintf(stderr, "unable to touch %s/%s: %s\n", dir, file, strerror(errno));
    close(dir_fd);
    return -1;
  }

  close(fd);
  close(dir_fd);
  return 0;
}

// recv on a socket, expecting recv to happen within timeout
static inline ssize_t recv_blocking(int socket, char *buf, size_t bufsize,
                                    int timeout) {
  int nfds;
  ssize_t len;

  // poll for events
  struct pollfd poll_fds[] = {{
      .fd = socket,
      .events = POLLIN,
      .revents = 0,
  }};
  if ((nfds = poll(poll_fds, 1, timeout)) < 0) {
    fprintf(stderr, "unable to poll socket %d: %s\n", socket, strerror(errno));
    return -1;
  }
  if (nfds == 0) {
    fprintf(stderr, "socket %d is not ready for recv\n", socket);
    return -1;
  }

  // initial attempt on getting the data into buffer
  if ((len = recv(socket, buf, bufsize, 0)) < 0) {
    fprintf(stderr, "unable to recv on socket %d: %s\n", socket,
            strerror(errno));
    return -1;
  }

  // in case there are more data to receive
  ssize_t rv;
  while (true) {
    char *cur = buf + len;
    size_t remaining = bufsize - len;

    if (remaining == 0) {
      fprintf(stderr, "more data to read than expected in fd %d\n", socket);
      return -1;
    }

    if ((rv = recv(socket, cur, remaining, MSG_DONTWAIT)) < 0) {
      // exit when there is no more data to consume
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      // in other cases, there is an error
      fprintf(stderr, "unable to recv (no-wait) on socket %d: %s\n", socket,
              strerror(errno));
      return -1;
    }

    len += rv;
  }

  // return number of bytes received
  return len;
}

// send the entire string to a socket
static inline ssize_t send_string(int socket, const char *msg) {
  ssize_t len;

  size_t msg_len = strlen(msg);
  fprintf(stderr, "about to send string [%ld] %s\n", msg_len, msg);
  if ((len = send(socket, msg, msg_len, 0)) < 0) {
    fprintf(stderr, "unable to send to socket %d: %s\n", socket,
            strerror(errno));
    return -1;
  }

  if (len < msg_len) {
    fprintf(stderr, "more data to send than expected\n");
    return -1;
  }

  // return number of bytes written
  return len;
}

/*
 * String utilities
 */

static inline bool str_prefix(const char *msg, const char *prefix) {
  return strncmp(msg, prefix, strlen(prefix)) == 0;
}

static inline bool str_suffix(const char *msg, const char *suffix) {
  size_t len = strlen(msg);
  size_t suffix_len = strlen(suffix);
  return strncmp(&msg[len - suffix_len - 1], suffix, suffix_len) == 0;
}

#endif // AGENT_COMMON_H
