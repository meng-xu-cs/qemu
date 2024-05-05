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
    fprintf(stderr, "unexpected arguments for %s\n", argv[0]);
    exit(1);
  }

  // TODO
  fprintf(stderr, "[harness-fuzz] notified host on VM ready\n");

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
