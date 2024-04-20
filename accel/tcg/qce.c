#include "qemu/osdep.h"

#include "exec/exec-all.h"
#include "qemu/qht.h"
#include "qemu/qtree.h"
#include "qemu/queue.h"
#include "qemu/xxhash.h"
#include "tcg/tcg-internal.h"
#include "tcg/tcg.h"

#include "qemu/qce.h"

#include "qce-debug.h"

#include "qce-ir.h"

#include "qce-z3.h"

#include "qce-state.h"

#include "qce-sym.h"

typedef enum {
  QCE_Tracing_NotStarted,
  QCE_Tracing_Kicked,
  QCE_Tracing_Capturing,
  QCE_Tracing_Running,
} QCETracingMode;

// session
typedef struct {
  // mark whether the session is in tracing mode
  QCETracingMode mode;

  // information about the blob
  tcg_target_ulong blob_addr;
  tcg_target_ulong blob_size;

  // state
  QCEState state;
} QCESession;

// cache entry
typedef struct {
  // key for hashtable
  const TranslationBlock *tb;
  // sequence of instructions
  QCEInst *insts;
  // count of the actual number of instructions
  size_t inst_count;
} QCECacheEntry;

static bool qce_cache_qht_cmp(const void *a, const void *b) {
  return ((QCECacheEntry *)a)->tb == ((QCECacheEntry *)b)->tb;
}
static bool qce_cache_qht_lookup(const void *p, const void *userp) {
  return ((QCECacheEntry *)p)->tb == (const TranslationBlock *)userp;
}
static void qce_cache_qht_iter_to_free(void *p, uint32_t _h, void *_userp) {
  QCECacheEntry *entry = p;
  if (entry->inst_count != 0) {
    g_free(entry->insts);
  }
}

// context
#define QCE_CTXT_CACHE_SIZE 1 << 24
struct QCEContext {
  // pre-allocated cache entry pool
  QCECacheEntry cache_pool[QCE_CTXT_CACHE_SIZE];
  // index of the next available cache entry
  size_t cache_next_entry;
  // a map of the translation block
  struct qht /* <const TranslationBlock *, QCECache> */ cache;

  // current session (i.e., execution of one snapshot)
  QCESession *session;

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
  QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("trying to shutdown QCE with no session executed");
  }
  if (session->mode != QCE_Tracing_NotStarted) {
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

#ifndef QCE_RELEASE
void qce_on_panic(void) {
  if (g_qce != NULL && g_qce->trace_file != NULL) {
    fflush(g_qce->trace_file);
  }
}
#endif

void qce_session_init(void) {
#ifdef QCE_RELEASE
  assert_qce_initialized();
#else
  if (g_qce == NULL) {
    return;
  }
#endif

  if (g_qce->session != NULL) {
    qce_fatal("re-creating a session");
  }

  // create a new session
  QCESession *session = g_malloc0(sizeof(*session));
  session->mode = QCE_Tracing_NotStarted;

  g_qce->session = session;
  qce_debug("session created");
}

void qce_session_reload(void) {
#ifdef QCE_RELEASE
  assert_qce_initialized();
#else
  if (g_qce == NULL) {
    return;
  }
#endif

  // sanity check
  QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("no session to reload");
  }
  if (unlikely(session->mode == QCE_Tracing_NotStarted)) {
    qce_fatal("the current session is not tracing");
  }

#ifdef QCE_DEBUG_IR
  // flush the trace
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file, "\n-------- END OF SESSION --------\n\n");
    fflush(g_qce->trace_file);
  }
#endif

  // finish up the symbolic state
  qce_state_fini(&session->state);
  session->blob_addr = 0;
  session->blob_size = 0;

  // reset the tracing states
  session->mode = QCE_Tracing_NotStarted;
  qce_debug("session reloaded");
}

void qce_trace_start(tcg_target_ulong addr, tcg_target_ulong size) {
  assert_qce_initialized();

  // sanity check
  QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("no active session is running");
  }
  if (unlikely(session->mode != QCE_Tracing_NotStarted)) {
    qce_fatal("the current session is already tracing");
  }

  // mark that tracing should be started now
  session->mode = QCE_Tracing_Kicked;
  session->blob_addr = addr;
  session->blob_size = size;
  qce_state_init(&session->state);

  // log it
#ifdef QCE_DEBUG_IR
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file,
            "==== tracing started with addr 0x%lx and size %ld ====\n", addr,
            size);
  }
#endif
  qce_debug("tracing started with addr 0x%lx and size %ld", addr, size);
}

void qce_on_tcg_ir_generated(TCGContext *tcg, TranslationBlock *tb) {
  assert_qce_initialized();

  if (unlikely(tcg->gen_tb != tb)) {
    qce_fatal("TCGContext::gen_tb does not match the tb argument");
  }
}

void qce_on_tcg_ir_optimized(TCGContext *tcg) {
  assert_qce_initialized();

  TranslationBlock *tb = tcg->gen_tb;
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
  QCECacheEntry *entry = &g_qce->cache_pool[g_qce->cache_next_entry];
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
    parse_op(tcg, op, &entry->insts[entry->inst_count++]);
  }
#ifdef QCE_DEBUG_IR
  g_assert(entry->inst_count == tcg->nb_ops);
#endif
}

void qce_on_tcg_tb_executed(TranslationBlock *tb, CPUState *cpu) {
  assert_qce_initialized();

  // find the cache entry
  const QCECacheEntry *entry = qht_lookup_custom(
      &g_qce->cache, tb, qemu_xxhash2((uint64_t)tb), qce_cache_qht_lookup);
  if (entry == NULL) {
    qce_fatal("unable to find QCE entry for translation block: 0x%p", tb);
  }

#ifdef QCE_DEBUG_IR
  // mark that this TB is executed
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file, "-> TB: 0x%p\n", tb);
  }
#endif

  // short-circuit if we are not in a session or are not in tracing mode
  QCESession *session = g_qce->session;
  if (session == NULL) {
    return;
  }

  if (session->mode == QCE_Tracing_NotStarted) {
    return;
  }

  // look for a TB which jumps to the called function
  if (session->mode == QCE_Tracing_Kicked) {
    // filter out empty TB
    if (entry->inst_count == 0) {
      return;
    }

    // look for the needle backwards
    size_t i = entry->inst_count;
    do {
      i--;

      QCEInst *inst = &entry->insts[i];
      if (inst->kind == QCE_INST_START) {
        // reached end of basic block, found nothing
        break;
      }

      if (inst->kind == QCE_INST_ADD_I64) {
        const QCEVar *res = &inst->i_add_i64.res;
        if (res->kind == QCE_VAR_GLOBAL_DIRECT &&
            strcmp(res->v_global_direct.name, "rip") == 0) {
          // found our confirmation, enter the capturing mode
          // TODO: also check the offset (can be pre-calculated via objdump)
          session->mode = QCE_Tracing_Capturing;
          qce_debug("about to jump to the target function");
          break;
        }
      }
    } while (i != 0);

    // in case we did not find anything, report an error instead of being fatal
    if (session->mode != QCE_Tracing_Capturing) {
      qce_error("failed to find the needle at TB 0x%p after kickstart", tb);
    }
    return;
  }

  // we need this arch state in the rest of the execution
  CPUArchState *arch = cpu_env(cpu);

  // validate that we have caught the right values
  if (session->mode == QCE_Tracing_Capturing) {
    if (session->blob_addr != arch->regs[R_EDI] ||
        session->blob_size != arch->regs[R_ESI]) {
      qce_error("session value mismatch at TB 0x%p", tb);
      return;
    }

    // initialize symbolic states
    QCEState *state = &session->state;
    qce_state_env_put_symbolic_i64(state, offsetof(CPUArchState, regs[R_EDI]),
                                   state->solver_z3.blob_addr);
    qce_state_env_put_symbolic_i64(state, offsetof(CPUArchState, regs[R_ESI]),
                                   state->solver_z3.blob_size);
    session->mode = QCE_Tracing_Running;
    qce_debug("target function confirmed, start tracing");
  }
#ifdef QCE_DEBUG_IR
  else {
    g_assert(session->mode == QCE_Tracing_Running);
  }
#endif

#ifndef QCE_RELEASE
  // run the unit test at the first hooked basic block
  if (getenv("QCE_CHECK") != NULL) {
    qce_unit_test(arch);
    _exit(0);
  }
#endif

  // dual-mode (symbolic + concrete) emulation
  for (size_t i = 0; i < entry->inst_count; i++) {
    QCEInst *inst = &entry->insts[i];

    // dump the IR to be emulated
#ifdef QCE_DEBUG_IR
    if (g_qce->trace_file != NULL) {
      qce_debug_print_inst(g_qce->trace_file, inst);
    }
#endif

    switch (inst->kind) {
    case QCE_INST_LD_I32: {
      qce_sym_inst_ld_i32(arch, &session->state, &inst->i_ld_i32.addr,
                          inst->i_ld_i32.offset, &inst->i_ld_i32.res);
      break;
    case QCE_INST_ADD_I32: {
      qce_sym_inst_add_i32(arch, &session->state, &inst->i_add_i32.v1,
                           &inst->i_add_i32.v2, &inst->i_add_i32.res);
      break;
    }
    }
    default: {
#ifdef QCE_DEBUG_IR
      qce_debug_print_inst(stderr, inst);
#endif
      // TODO: change to fatal
      qce_fatal("emulation not supported yet");
    }
    }
  }
}

#ifndef QCE_RELEASE
void qce_unit_test(CPUArchState *env) {
  qce_debug("start unit testing");
  qce_unit_test_smt_z3();
  qce_unit_test_expr();
  qce_unit_test_state(env);
  qce_debug("unit testing completed");
}
#endif
