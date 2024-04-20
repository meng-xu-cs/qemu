#include "../common.h"

int harness(char *blob, size_t size) {
  if (size == 1) {
    if (blob[0] == 'a') {
      return 0;
    }
    return 1;
  }
  return 2;
}
