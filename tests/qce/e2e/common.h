#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>

int harness(const uint8_t *blob, size_t size) /* */;

int main(int argc, char *argv[]) {
  struct stat st;
  int fd;
  uint8_t *blob;
  size_t size;

  if (argc < 2) {
    printf("Need file\n");
    return -1;
  }
  if (stat(argv[1], &st) != 0) {
    printf("Failed to stat file\n");
    return -1;
  }

  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf("Failed to open file\n");
    return -1;
  }

  uint64_t bound;
  size = read(fd, &bound, sizeof(uint64_t));
  if (size != sizeof(uint64_t)) {
    printf("Failed to read file\n");
    return -1;
  }

  blob = (uint8_t *)malloc(st.st_size-sizeof(uint64_t));
  if (blob == NULL) {
    printf("Failed to allocate blob\n");
    return -1;
  }

  size = read(fd, (char *)blob, st.st_size-sizeof(uint64_t));
  if (size != st.st_size-sizeof(uint64_t)) {
    printf("Failed to read file\n");
    return -1;
  }
  close(fd);

  return harness(blob, size);
}
