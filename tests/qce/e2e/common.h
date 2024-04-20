#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int harness(char *blob, size_t size) /* */;

int main(int argc, char *argv[]) {
  struct stat st;
  int fd;
  char *blob;
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

  blob = malloc(st.st_size);
  if (blob == NULL) {
    printf("Failed to allocate blob\n");
    return -1;
  }

  size = read(fd, blob, st.st_size);
  if (size != st.st_size) {
    printf("Failed to read file\n");
    return -1;
  }
  close(fd);

  return harness(blob, size);
}
