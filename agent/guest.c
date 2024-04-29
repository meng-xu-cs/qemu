#include "common.h"

#define PATH_WKS "/tmp/wks"

int main(int argc, char *argv[]) {
  // sanity check
  if (argc != 1) {
    fprintf(stderr, "expect one and only one argument\n");
    exit(1);
  }

  // remove the mark to signal that the guest is ready
  if (unlink(PATH_WKS "/" FILE_MARK) != 0) {
    fprintf(stderr, FILE_MARK " does not exist in workspace\n");
    exit(1);
  }

  // wait for mark to re-appear
  dnotify_watch(PATH_WKS, FILE_MARK, true);

  // transfer control to harness
  return execl(HARNESS, HARNESS, argv[1]);
}
