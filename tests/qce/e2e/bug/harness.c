#include "../common.h"

int harness(char *blob, size_t size) {
  // for (int i = 0; i < 1; ++i);
  int a = 0;
  for (int i = 0; i < 1000; ++i) {
    a += 1;
  }
  return 0;
}
