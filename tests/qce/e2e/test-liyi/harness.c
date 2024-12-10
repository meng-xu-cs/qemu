#include "../common.h"

int harness(char *blob, size_t size) {
  if (size == 4) {
    if (blob[0] == 'o') {
      if (blob[1] == 'p') {
        if (blob[2] == 'e') {
          if (blob[3] == 'n') {
            return 0;
          }
          return 1;
        }
        return 2;
      }
      return 3;
    }
    return 4;
  } else if (size == 2) {
    if (blob[0] == 'a') {
        if (blob[1] == 'b') {
            return 5;
        }
        return 6;
    }
    return 7;
  }
  return 8;
}
