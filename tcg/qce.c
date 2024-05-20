#include "qemu/qce.h"

#include "exec/translation-block.h"
#include "qemu/qht.h"
#include "qemu/queue.h"
#include "qemu/xxhash.h"

#define QCE_DEBUG_IR
#include "qce-debug.h"
#include "qce-ir.h"

// session
struct QCESession {
  // mark whether the session is in tracing mode
  bool tracing;
  // symbolic memory address
  tcg_target_ulong blob_addr;
  // symbolic memory size
  tcg_target_ulong size_val;
};

// cache entry
struct QCECacheEntry {
  // key for hashtable
  const TranslationBlock *tb;
  // sequence of instructions
  QCEInst *insts;
  // count of the actual number of instructions
  size_t inst_count;
};

static bool qce_cache_qht_cmp(const void *a, const void *b) {
  return ((struct QCECacheEntry *)a)->tb == ((struct QCECacheEntry *)b)->tb;
}
static bool qce_cache_qht_lookup(const void *p, const void *userp) {
  return ((struct QCECacheEntry *)p)->tb == (const TranslationBlock *)userp;
}
static void qce_cache_qht_iter_to_free(void *p, uint32_t _h, void *_userp) {
  struct QCECacheEntry *entry = p;
  if (entry->inst_count != 0) {
    g_free(entry->insts);
  }
}

// context
#define QCE_CTXT_CACHE_SIZE 1 << 24
struct QCEContext {
  // pre-allocated cache entry pool
  struct QCECacheEntry cache_pool[QCE_CTXT_CACHE_SIZE];
  // index of the next available cache entry
  size_t cache_next_entry;
  // a map of the translation block
  struct qht /* <const TranslationBlock *, QCECache> */ cache;

  // current session (i.e., execution of one snapshot)
  struct QCESession *session;

#ifdef QCE_DEBUG_IR
  // file to dump information to
  FILE *trace_file;
#endif
};

// global variable
struct QCEContext *g_qce = NULL;

void qce_init(void) {
  if (unlikely(g_qce != NULL)) {
    qce_fatal("QCE is already initialized");
  }

  // initialize the context
  g_qce = g_malloc0(sizeof(*g_qce));
  if (unlikely(g_qce == NULL)) {
    qce_fatal("unable to allocate QCE context");
  }

  g_qce->cache_next_entry = 0;
  qht_init(&g_qce->cache, qce_cache_qht_cmp, QCE_CTXT_CACHE_SIZE,
           QHT_MODE_AUTO_RESIZE);
  g_qce->session = NULL;

#ifdef QCE_DEBUG_IR
  const char *file_name = getenv("QCE_TRACE");
  if (file_name == NULL) {
    g_qce->trace_file = NULL;
  } else {
    FILE *handle = fopen(file_name, "w+");
    if (handle == NULL) {
      qce_fatal("unable to create the trace file");
    }
    g_qce->trace_file = handle;
  }
#endif

  // done
  qce_debug("initialized");
}

void qce_destroy(void) {
  CPUState *cpu;
  CPU_FOREACH(cpu) {
    // at shutdown, all CPUs should have stopped
    if (!cpu->stopped) {
      qce_fatal("vCPU still running");
    }
  }

  // destruct the QCE context
  if (unlikely(g_qce == NULL)) {
    qce_fatal("QCE is either not initialized or destroyed twice");
  }

  // ensure that we are not in the middle of a session
  struct QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("trying to shutdown QCE with no session executed");
  }
  if (session->tracing) {
    qce_fatal("trying to shutdown QCE while an active session is tracing");
  }

#ifdef QCE_DEBUG_IR
  // done with tracing
  if (g_qce->trace_file != NULL) {
    fclose(g_qce->trace_file);
  }
#endif

  // de-allocate resources
  g_free(session); // there is no need to free internal fields of session
  qht_iter(&g_qce->cache, qce_cache_qht_iter_to_free, NULL);
  qht_destroy(&g_qce->cache);
  g_free(g_qce);
  g_qce = NULL;

  // done
  qce_debug("destroyed");
}

static inline void assert_qce_initialized(void) {
  if (unlikely(g_qce == NULL)) {
    qce_fatal("QCE is not initialized yet");
  }
}

void qce_session_init(void) {
  assert_qce_initialized();

  if (g_qce->session != NULL) {
    qce_fatal("re-creating a session");
  }

  // create a new session
  struct QCESession *session = g_malloc0(sizeof(*session));
  session->tracing = false;
  session->blob_addr = 0;
  session->size_val = 0;

  g_qce->session = session;
  qce_debug("session created");
}

void qce_session_reload(void) {
  assert_qce_initialized();

  // sanity check
  struct QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("no session to reload");
  }
  if (unlikely(!session->tracing)) {
    qce_fatal("the current session is not tracing");
  }

#ifdef QCE_DEBUG_IR
  // flush the trace
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file, "\n-------- END OF SESSION --------\n\n");
    fflush(g_qce->trace_file);
  }
#endif

  // reset the tracing states
  session->tracing = false;
  session->blob_addr = 0;
  session->size_val = 0;
  qce_debug("session reloaded");
}

void qce_trace_start(tcg_target_ulong addr, tcg_target_ulong len) {
  assert_qce_initialized();

  // sanity check
  struct QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("no active session is running");
  }

  // mark that tracing should be started now
  session->blob_addr = addr;
  session->size_val = len;
  session->tracing = true;

  // log it
#ifdef QCE_DEBUG_IR
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file,
            "==== tracing started with addr 0x%lx and len %ld ====\n", addr,
            len);
  }
#endif
  qce_debug("tracing started with addr 0x%lx and len %ld", addr, len);
}

void qce_on_tcg_ir_generated(TCGContext *tcg, TranslationBlock *tb) {
  assert_qce_initialized();

  if (unlikely(tcg->gen_tb != tb)) {
    qce_fatal("TCGContext::gen_tb does not match the tb argument");
  }
}

void qce_on_tcg_ir_optimized(TCGContext *tcg) {
  assert_qce_initialized();

  struct TranslationBlock *tb = tcg->gen_tb;
#ifdef QCE_DEBUG_IR
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file, "\n[TB: 0x%p]\n", tb);
    tcg_dump_ops(tcg, g_qce->trace_file, false);
  }
#endif

  // sanity check
  if (g_qce->cache_next_entry == QCE_CTXT_CACHE_SIZE) {
    qce_fatal("cache is at capacity");
  }

  // mark the translation block
  struct QCECacheEntry *entry = &g_qce->cache_pool[g_qce->cache_next_entry];
  entry->tb = tb;
  entry->inst_count = 0;

  // insert or obtain the pointer
  void *existing;
  if (qht_insert(&g_qce->cache, (void *)entry, qemu_xxhash2((uint64_t)tb),
                 &existing)) {
    g_qce->cache_next_entry++;
  } else {
    entry = existing;
  }

  // prepare buffer to host instructions
  if (entry->inst_count != 0) {
    g_free(entry->insts);
  }

  entry->insts = g_new0(QCEInst, tcg->nb_ops);
  if (entry->insts == NULL) {
    qce_fatal("fail to allocate memory for instructions");
  }
  entry->inst_count = 0;

  // parse the translation block
  TCGOp *op;
  QTAILQ_FOREACH(op, &tcg->ops, link) {
    parse_op(tcg, op, &entry->insts[entry->inst_count]);
    entry->inst_count++;
  }
#ifdef QCE_DEBUG_IR
  g_assert(entry->inst_count == tcg->nb_ops);
#endif
}

void qce_on_tcg_tb_executed(TranslationBlock *tb, CPUState *cpu) {
  assert_qce_initialized();

  // find the cache entry
  struct QCECacheEntry *entry = qht_lookup_custom(
      &g_qce->cache, tb, qemu_xxhash2((uint64_t)tb), qce_cache_qht_lookup);
  if (entry == NULL) {
    qce_fatal("unable to find QCE entry for translation block: 0x%p", tb);
  }

#ifdef QCE_DEBUG_IR
  // mark that this TB is executed
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file, "=> TB: 0x%p\n", tb);
  }
#endif

  // TODO
  // CPUArchState *arch = cpu_env(cpu);
}
