#include "qemu/qce.h"
#include "exec/translation-block.h"
#include "qemu/xxhash.h"

#include "qemu/qht.h"

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
  // symbolic memory address
  tcg_target_ulong blob_addr;
  // symbolic memory size
  tcg_target_ulong size_val;
  // a holder of cache entries generated within this session
  QSLIST_HEAD(, QCECache) cache_entries;
};

// context
struct QCEContext {
  // a map of the translation block
  struct qht /* <const TranslationBlock *, QCECache> */ cache;
  // current QCE session
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
  if (unlikely(g_qce->session != NULL)) {
    qce_fatal("Trying to shutdown QCE while an active session is running");
  }

  // de-allocate resources
  qht_iter(&g_qce->cache, qce_cache_qht_iter_free, NULL);
  qht_destroy(&g_qce->cache);

  free(g_qce);
  g_qce = NULL;

  // done
  qce_debug("destroyed");
}

void qce_trace_start(CPUState *cpu, tcg_target_ulong addr,
                     tcg_target_ulong len) {
  if (unlikely(g_qce == NULL)) {
    qce_fatal("QCE is not initialized yet");
  }
  if (unlikely(g_qce->session != NULL)) {
    qce_fatal("QCE is already in an active session");
  }

  // prepare the session
  qce_debug("started with addr 0x%lx and len %ld", addr, len);
  struct QCESession *session = g_malloc0(sizeof(*session));
  session->blob_addr = addr;
  session->size_val = len;
  QSLIST_INIT(&session->cache_entries);
  g_qce->session = session;

  // re-purpose the kvm-related flag
  if (unlikely(cpu->vcpu_dirty)) {
    qce_fatal("vCPU already carries a session");
  }
  cpu->vcpu_dirty = true;
}

void qce_trace_try_finish(void) {
  if (unlikely(g_qce == NULL)) {
    qce_fatal("QCE is not initialized yet");
  }

  CPUState *cpu;
  CPUState *mark = NULL;

  CPU_FOREACH(cpu) {
    // only shutdown QCE when all CPUs are stopped
    if (!cpu->stopped) {
      return;
    }
    // locate the context manager
    if (cpu->vcpu_dirty) {
      if (unlikely(mark != NULL)) {
        qce_fatal("more than one vCPU carries a QCE session");
      }
      mark = cpu;
    }
  }
  // no session found, session might have finished
  if (mark == NULL) {
    if (unlikely(g_qce->session != NULL)) {
      qce_fatal("the session is running without associated vCPU");
    }
    return;
  }

  // destruct the session
  struct QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("the session is cleared without vCPU");
  }

  // clear all session-specific cache entries
  struct QCECache *entry;
  QSLIST_FOREACH(entry, &session->cache_entries, next) {
    if (!qht_remove(&g_qce->cache, (void *)entry,
                    qemu_xxhash2((uint64_t)entry->tb))) {
      qce_fatal("invalid entry to remove");
    }
  }

  // destroy the session itself
  g_free(g_qce->session);
  g_qce->session = NULL;
  mark->vcpu_dirty = false;
  qce_debug("trace session stops");
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

  // clear the CPU marker
  tcg->cpu = NULL;
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