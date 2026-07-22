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

/*
 * resuming into a preserved output directory should not restart session
 * numbering from 0, or checked_mkdir() will fatal on a directory that
 * already exists from a previous run
 */
static inline long session_id_init(const char *output_dir) {
  DIR *dir = opendir(output_dir);
  if (dir == NULL) {
    qce_fatal("unable to open directory %s", output_dir);
  }

  long start_id = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    char *end;
    long id = strtol(entry->d_name, &end, 10);
    if (end != entry->d_name && *end == '\0' && id + 1 > start_id) {
      start_id = id + 1;
    }
  }
  closedir(dir);
  return start_id;
}

/*
 * QEMU moved this static function definition from internal-target.h to
 * cpu-exec.c in commit e07788a, so we define our own copy in QCE.
 */
static vaddr log_pc(CPUState *cpu, const TranslationBlock *tb)
{
  if (tb_cflags(tb) & CF_PCREL) {
    return cpu->cc->get_pc(cpu);
  } else {
    return tb->pc;
  }
}

#endif /* QCE_UTILS_H */
