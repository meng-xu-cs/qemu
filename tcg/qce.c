#include "qemu/qce.h"
#include "exec/translation-block.h"
#include "qemu/xxhash.h"

#include "qemu/qht.h"

// cache
struct QCECache {
  const TranslationBlock *tb;  // key for hashtable
  QSLIST_ENTRY(QCECache) next; // part of the linked list
};

static bool qce_cache_qht_cmp(const void *a, const void *b) {
  return ((struct QCECache *)a)->tb == ((struct QCECache *)b)->tb;
}
static bool qce_cache_qht_lookup(const void *p, const void *userp) {
  return ((struct QCECache *)p)->tb == (const TranslationBlock *)userp;
}
static void qce_cache_qht_iter_free(void *p, uint32_t _h, void *_userp) {
  g_free(p);
}

// session
struct QCESession {
  // a holder of cache entries generated within this session
  QSLIST_HEAD(, QCECache) cache_entries;

  // mark whether the session is in tracing mode
  bool tracing;
  // symbolic memory address
  tcg_target_ulong blob_addr;
  // symbolic memory size
  tcg_target_ulong size_val;
};

// context
struct QCEContext {
  // a map of the translation block
  struct qht /* <const TranslationBlock *, QCECache> */ cache;
  // current session (i.e., execution of one snapshot)
  struct QCESession *session;
};

// global variable
struct QCEContext *g_qce = NULL;

#define QCE_CTXT_CACHE_SIZE 1 << 16
void qce_init(void) {
  // repurpose the kvm_state (which is only used in kvm) for context
  if (unlikely(g_qce != NULL)) {
    qce_fatal("QCE is already initialized");
  }

  // initialize the context
  g_qce = g_malloc0(sizeof(*g_qce));
  if (unlikely(g_qce == NULL)) {
    qce_fatal("unable to allocate QCE context");
  }

  qht_init(&g_qce->cache, qce_cache_qht_cmp, QCE_CTXT_CACHE_SIZE,
           QHT_MODE_AUTO_RESIZE);
  g_qce->session = NULL;

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

  // de-allocate resources
  g_free(session); // there is no need to free internal fields of session
  qht_iter(&g_qce->cache, qce_cache_qht_iter_free, NULL);
  qht_destroy(&g_qce->cache);
  g_free(g_qce);
  g_qce = NULL;

  // done
  qce_debug("destroyed");
}

void qce_session_init(void) {
  if (unlikely(g_qce == NULL)) {
    qce_fatal("QCE is not initialized yet");
  }
  if (g_qce->session != NULL) {
    qce_fatal("re-creating a session");
  }

  // create a new session
  struct QCESession *session = g_malloc0(sizeof(*session));
  session->tracing = false;
  session->blob_addr = 0;
  session->size_val = 0;
  QSLIST_INIT(&session->cache_entries);

  g_qce->session = session;
  qce_debug("session created");
}

void qce_session_reload(void) {
  if (unlikely(g_qce == NULL)) {
    qce_fatal("QCE is not initialized yet");
  }

  // sanity check
  struct QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("no session to reload");
  }
  if (unlikely(!session->tracing)) {
    qce_fatal("the current session is not tracing");
  }

  // clean up cache
  struct QCECache *entry;
  QSLIST_FOREACH(entry, &session->cache_entries, next) {
    if (!qht_remove(&g_qce->cache, (void *)entry,
                    qemu_xxhash2((uint64_t)entry->tb))) {
      qce_fatal("invalid QCE cache entry to be removed");
    }
  }
  struct QCECache *tmp;
  QSLIST_FOREACH_SAFE(entry, &session->cache_entries, next, tmp) {
    g_free(tmp);
  }

  // reset the cache list
  QSLIST_INIT(&session->cache_entries);

  // reset the tracing states
  session->tracing = false;
  session->blob_addr = 0;
  session->size_val = 0;
  qce_debug("session reloaded");
}

void qce_trace_start(tcg_target_ulong addr, tcg_target_ulong len) {
  // sanity checks
  if (unlikely(g_qce == NULL)) {
    qce_fatal("QCE is not initialized yet");
  }

  struct QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("no active session is running");
  }

  // mark that tracing should be started now
  session->blob_addr = addr;
  session->size_val = len;
  session->tracing = true;

  // done
  qce_debug("tracing started with addr 0x%lx and len %ld", addr, len);
}

void qce_on_tcg_ir_generated(TCGContext *tcg, TranslationBlock *tb) {
  if (unlikely(tcg->gen_tb != tb)) {
    qce_fatal("TCGContext::gen_tb does not match the tb argument");
  }
}

void qce_on_tcg_ir_optimized(TCGContext *tcg) {
  if (g_qce == NULL) {
    return;
  }

  // mark the translation block
  struct TranslationBlock *tb = tcg->gen_tb;
  struct QCECache *entry = g_malloc0(sizeof(*entry));
  entry->tb = tb;

  // go over the operators
  TCGOp *op;
  QTAILQ_FOREACH(op, &tcg->ops, link) {
    // TODO
  }

  // insert it
  if (!qht_insert(&g_qce->cache, (void *)entry, qemu_xxhash2((uint64_t)tb),
                  NULL)) {
    qce_fatal("duplicate translation block: 0x%p", tb);
  }

  struct QCESession *session = g_qce->session;
  if (session != NULL) {
    QSLIST_INSERT_HEAD(&session->cache_entries, entry, next);
  }
}

void qce_on_tcg_tb_executed(TranslationBlock *tb, CPUState *cpu) {
  struct QCEContext *qce = (struct QCEContext *)cpu->kvm_state;
  if (qce == NULL) {
    return;
  }

  // find the cache entry
  struct QCECache *entry = qht_lookup_custom(
      &g_qce->cache, tb, qemu_xxhash2((uint64_t)tb), qce_cache_qht_lookup);
  if (entry == NULL) {
    qce_fatal("unable to find QCE entry for translation block: 0x%p", tb);
  }

  // TODO
  // CPUArchState *arch = cpu_env(cpu);
}