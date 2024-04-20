#ifndef QCE_UTILS_H
#define QCE_UTILS_H

/*
 * Macro
 */

#define UTIL_DOT_JOIN(a, b) a.b

#ifndef QCE_RELEASE
#define qce_debug_assert(expr)                                                 \
  do {                                                                         \
    if (!(expr)) {                                                             \
      qce_fatal("assertion failed: " #expr);                                   \
    }                                                                          \
  } while (0);
#else
#define qce_debug_assert(expr)
#endif

/*
 * File system
 */

static inline void checked_dir_exists(const char *path) {
  DIR *dir = opendir(path);
  if (dir != NULL) {
    closedir(dir);
  } else {
    qce_fatal("unable to open directory %s", path);
  }
}

static inline void G_GNUC_PRINTF(1, 2) checked_mkdir(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char full_path[PATH_MAX];
  vsprintf(full_path, fmt, args);
  va_end(args);

  int rv = mkdir(full_path, 0775);
  if (unlikely(rv != 0)) {
    qce_fatal("cannot create directory %s", full_path);
  }
}

static inline G_GNUC_PRINTF(2, 3) FILE *checked_open(const char *mode,
                                                     const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char full_path[PATH_MAX];
  vsprintf(full_path, fmt, args);
  va_end(args);

  FILE *handle = fopen(full_path, mode);
  if (unlikely(handle == NULL)) {
    qce_fatal("cannot open file %s", full_path);
  }
  return handle;
}

#endif /* QCE_UTILS_H */
