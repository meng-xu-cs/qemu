#include "common.h"

#define MONITOR_BUFSIZE 4096
#define MONITOR_TIMEOUT 60 * 1000
#define MONITOR_VM_TAG "qce"

static inline ssize_t __qmp_interact(int sock, const char *cmd, char *buf,
                                     size_t buflen) {
  ssize_t len;

  // send command
  if (send_string(sock, cmd) < 0) {
    fprintf(stderr, "unable to send to QMP: %s\n", strerror(errno));
    return -1;
  }
  fprintf(stderr, "command sent: %s\n", cmd);

  // recv result
  if ((len = recv_blocking(sock, buf, buflen, MONITOR_TIMEOUT)) < 0) {
    return -1;
  }
  buf[len] = '\0';

  // return number of bytes received
  return len;
}

static inline int __qmp_handshake(int sock) {
  ssize_t len;
  char buf[MONITOR_BUFSIZE];

  // recv welcome message
  if ((len = recv_blocking(sock, buf, MONITOR_BUFSIZE, MONITOR_TIMEOUT)) < 0) {
    return -1;
  }
  buf[len] = '\0';
  fprintf(stderr, "[host] connected to QEMU monitor: %s", buf);

  // negotiate capabilities
  if ((len = __qmp_interact(sock,
                            "{"
                            "\"execute\":\"qmp_capabilities\""
                            "}",
                            buf, MONITOR_BUFSIZE)) < 0) {
    return -1;
  }
  // TODO: parse the result
  fprintf(stderr, "received: %s", buf);

  // negotiate capabilities
  if ((len = __qmp_interact(sock,
                            "{"
                            "\"execute\":\"query-block\""
                            "}",
                            buf, MONITOR_BUFSIZE)) < 0) {
    return -1;
  }
  // TODO: parse the result
  fprintf(stderr, "%s", buf);

  while (true) {
    if ((len = recv_blocking(sock, buf, MONITOR_BUFSIZE, MONITOR_TIMEOUT)) <
        0) {
      return -1;
    }
    buf[len] = '\0';
    // TODO: parse the result
    fprintf(stderr, "%s", buf);
  }

  return 0;
}

static inline int __qmp_snapshot_save(int sock) {
  ssize_t len;
  char buf[MONITOR_BUFSIZE];

  if ((len = __qmp_interact(sock,
                            "{"
                            "\"execute\":\"snapshot-save\","
                            "\"arguments\": {"
                            "\"job-id\":\"job0\", "
                            "\"tag\":\"" MONITOR_VM_TAG "\","
                            "\"vmstate\":\"disk0\","
                            "\"devices\":[]"
                            "}"
                            "}",
                            buf, MONITOR_BUFSIZE) < 0)) {
    return -1;
  }
  // TODO: parse the result
  fprintf(stderr, "received: %s", buf);

  while (true) {
    if ((len = recv_blocking(sock, buf, MONITOR_BUFSIZE, MONITOR_TIMEOUT)) <
        0) {
      return -1;
    }
    buf[len] = '\0';
    // TODO: parse the result
    fprintf(stderr, "status: %s", buf);
  }

  return 0;
}

int take_snapshot(const char *mon) {
  int sock;
  struct sockaddr_un addr;

  // connect to monitor
  if ((sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "unable to create a unix socket: %s\n", strerror(errno));
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, mon);
  if (connect(sock, &addr, sizeof(addr)) < 0) {
    fprintf(stderr, "unable to connect to socket at %s: %s\n", mon,
            strerror(errno));
    goto cleanup;
  }

  // handshake
  if (__qmp_handshake(sock) < 0) {
    goto cleanup;
  }

  // snapshot
  if (__qmp_snapshot_save(sock) < 0) {
    goto cleanup;
  }

  // shutdown
  close(sock);
  return 0;

cleanup:
  close(sock);
  return -1;
}

int main(int argc, char *argv[]) {
  // sanity check
  if (argc != 3) {
    fprintf(stderr, "expect one and only one argument\n");
    exit(1);
  }
  const char *wks = argv[1];
  const char *mon = argv[2];

  // wait for mark to disappear
  dnotify_watch(wks, FILE_MARK, false);
  fprintf(stderr, "[host] guest virtual machine is ready\n");

  // take snapshot on the guest
  take_snapshot(mon);

  // re-create the mark
  touch(wks, FILE_MARK);
}
