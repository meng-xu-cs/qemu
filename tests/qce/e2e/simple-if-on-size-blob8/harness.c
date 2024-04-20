#include "../common.h"

int harness(char *blob, size_t size) {
  if (size == 8) {
    if (blob[0] == 'o') {
      if (blob[1] == 'p') {
        if (blob[2] == 'e') {
          if (blob[3] == 'n') {
            if (blob[4] == '5') {
              if (blob[5] == 'g') {
                if (blob[6] == 's') {
                  if (blob[7] == '!') {
                    return 0;
                  }
                  return 1;
                }
                return 2;
              }
              return 3;
            }
            return 4;
          }
          return 5;
        }
        return 6;
      }
      return 7;
    }
    return 8;
  }
  return 9;
}
