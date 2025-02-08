#include "qemu/osdep.h"

#define XXH_INLINE_ALL
#include "xxhash.h"
#define QCE_SKIP_QEMU_XXH64
#include "qemu/xxhash.h"

#include "qemu/qht.h"
#include "qemu/qtree.h"
#include "qemu/queue.h"

#include "exec/cpu_ldst.h"
#include "exec/exec-all.h"
#include "internal-target.h"

#include "tcg/tcg-internal.h"
#include "tcg/tcg.h"

#include "qemu/qce.h"

#include "qce-debug.h"
#include "qce-utils.h"

#include "qce-ir.h"

#include "qce-z3.h"

#include "qce-expr.h"

#include "qce-state.h"

typedef enum {
  QCE_Tracing_NotStarted,
  QCE_Tracing_Kicked,
  QCE_Tracing_Capturing,
  QCE_Tracing_Running,
  QCE_Tracing_StopPending,
  QCE_Tracing_Stopped,
} QCETracingMode;

// session
typedef struct {
  // unique identifier
  size_t id;

  // mark whether the session is in tracing mode
  QCETracingMode mode;

  // information about the blob
  tcg_target_ulong blob_addr;
  tcg_target_ulong blob_size;
  uint8_t *blob_content;

  // state
  QCEState state;
  // number of new seeds generated
  size_t seed_count;

  // coverage prior to this execution
  GArray *database;
  // coverage on this execution
  GArray *coverage;
  // hash of the coverage trace
  XXH64_state_t cov_hash;
} QCESession;

// cache entry
typedef struct {
  // key for hashtable
  const TranslationBlock *tb;
  // sequence of instructions
  QCEInst *insts;
  // count of the actual number of instructions
  size_t inst_count;
  // label to index mapping
  size_t *labels;
  // count of the actual number of labels
  size_t label_count;
} QCECacheEntry;

static bool qce_cache_qht_cmp(const void *a, const void *b) {
  return ((QCECacheEntry *)a)->tb == ((QCECacheEntry *)b)->tb;
}
static bool qce_cache_qht_lookup(const void *p, const void *userp) {
  return ((QCECacheEntry *)p)->tb == (const TranslationBlock *)userp;
}
static void qce_cache_qht_iter_to_free(void *p, uint32_t _h, void *_userp) {
  QCECacheEntry *entry = p;

  if (entry->insts != NULL) {
    g_free(entry->insts);
  }
  entry->insts = NULL;
  entry->inst_count = 0;

  if (entry->labels != NULL) {
    g_free(entry->labels);
  }
  entry->labels = NULL;
  entry->label_count = 0;
}

// context
#define QCE_CTXT_CACHE_SIZE 1 << 24
struct QCEContext {
  // corpus directory
  const char *corpus_dir;
  // output directory
  const char *output_dir;
#ifdef QCE_DEBUG_IR
  // file to dump debug information
  FILE *trace_file;
#endif

  // pre-allocated cache entry pool
  QCECacheEntry cache_pool[QCE_CTXT_CACHE_SIZE];
  // index of the next available cache entry
  size_t cache_next_entry;
  // a map of the translation block
  struct qht /* <const TranslationBlock *, QCECache> */ cache;

  // current session (i.e., execution of one snapshot)
  QCESession *session;
};

// global variable
struct QCEContext *g_qce = NULL;

// coverage and symbolization of instructions have to be here
#include "qce-cov.h"
#include "qce-sym.h"

void qce_init(void) {
  if (unlikely(g_qce != NULL)) {
    qce_fatal("QCE is already initialized");
  }

  // initialize the context
  g_qce = g_malloc0(sizeof(*g_qce));
  if (unlikely(g_qce == NULL)) {
    qce_fatal("unable to allocate QCE context");
  }

  // check on corpus dir
  const char *corpus_dir = getenv("QCE_CORPUS");
  checked_dir_exists(corpus_dir);
  g_qce->corpus_dir = corpus_dir;

  // check on output dir
  const char *output_dir = getenv("QCE_OUTPUT");
  checked_dir_exists(output_dir);
  g_qce->output_dir = output_dir;

  // prepare for trace file
#ifdef QCE_DEBUG_IR
  const char *trace_val = getenv("QCE_TRACE");
  if (trace_val == NULL) {
    g_qce->trace_file = NULL;
  } else if (strcmp(trace_val, "1") == 0) {
    g_qce->trace_file = checked_open("w+", "%s/trace", g_qce->output_dir);
  } else {
    qce_fatal("invalid value for QCE_TRACE environment variable");
  }
#endif

  // initialize the context
  g_qce->cache_next_entry = 0;
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
  __qce_free_cov_db(session->database);
  g_array_free(session->coverage, true);
  // there is no need to free other internal fields of session
  g_free(session);

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
  session->id = 0;
  session->mode = QCE_Tracing_NotStarted;

  session->seed_count = 0;
  /* session->blob_* and session->state do not need initialization */

  FILE *cov_file = checked_open("r", "%s/total_cov", g_qce->corpus_dir);
  session->database = __qce_load_cov_db(cov_file);
  fclose(cov_file);

  session->coverage = g_array_new(false, false, sizeof(vaddr));
  XXH64_reset(&session->cov_hash, QEMU_XXHASH_SEED);

  // this session is fixed to the QCE context
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
  if (session->blob_content != NULL) {
    g_free(session->blob_content);
  }
  session->blob_content = NULL;

  // re-load the coverage database (after clean-up)
  __qce_free_cov_db(session->database);
  FILE *cov_file = checked_open("r", "%s/total_cov", g_qce->corpus_dir);
  session->database = __qce_load_cov_db(cov_file);
  fclose(cov_file);

  // reset the tracing states
  session->mode = QCE_Tracing_NotStarted;
  session->id++;
  qce_debug("session reloaded");
}

void qce_trace_start(tcg_target_ulong addr, tcg_target_ulong size,
                     uint8_t *blob) {
  assert_qce_initialized();

  // sanity check
  QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("no active session exists");
  }
  if (unlikely(session->mode != QCE_Tracing_NotStarted)) {
    qce_fatal("the current session is already tracing");
  }

  // mark that tracing should be started now
  session->mode = QCE_Tracing_Kicked;

  // record blob information
  session->blob_addr = addr;
  session->blob_size = size;
  session->blob_content = blob;

  // initialize symbolic states
  qce_state_init(&session->state);

  // prepare the output directory
  checked_mkdir("%s/%ld", g_qce->output_dir, session->id);
  checked_mkdir("%s/%ld/seeds", g_qce->output_dir, session->id);
  session->seed_count = 0;

  // reset coverage tracking
  session->coverage->len = 0;
  XXH64_reset(&session->cov_hash, QEMU_XXHASH_SEED);

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

void qce_trace_stop(tcg_target_ulong addr, tcg_target_ulong size,
                    uint8_t *blob) {
  assert_qce_initialized();

  // sanity check
  QCESession *session = g_qce->session;
  if (unlikely(session == NULL)) {
    qce_fatal("no active session exists");
  }
  if (unlikely(session->mode != QCE_Tracing_StopPending)) {
    qce_fatal("the current session is not pending for stop");
  }
  if (unlikely(session->blob_addr != addr)) {
    qce_fatal("mismatched blob_addr on stop");
  }
  if (unlikely(session->blob_size != size)) {
    qce_fatal("mismatched blob_size on stop");
  }
  for (size_t i = 0; i < size; i++) {
    if (blob[i] != session->blob_content[i]) {
      qce_fatal("mismatched blob_content on stop");
    }
  }
  g_free(blob);

  // mark that tracing should be ended
  session->mode = QCE_Tracing_Stopped;

  // dump the coverage
  FILE *handle =
      checked_open("w+", "%s/%ld/cov", g_qce->output_dir, session->id);
  size_t count = fwrite(session->coverage->data, sizeof(vaddr),
                        session->coverage->len, handle);
  if (unlikely(count != session->coverage->len)) {
    qce_fatal("error on writing cov information");
  }
  fclose(handle);

  // log it
  uint64_t cov_hash = XXH64_digest(&session->cov_hash);
#ifdef QCE_DEBUG_IR
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file,
            "==== tracing stopped with coverage hash %016lx ====\n", cov_hash);
    fflush(g_qce->trace_file);
  }
#endif
  qce_debug("tracing stopped with coverage hash %016lx", cov_hash);
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
    fprintf(g_qce->trace_file, "\n[TB: %p]\n", tb);
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
  if (qht_insert(&g_qce->cache, entry, qemu_xxhash2((uint64_t)tb), &existing)) {
    g_qce->cache_next_entry++;
  } else {
    entry = existing;
  }

  // prepare buffer to host instructions
  if (entry->insts != NULL) {
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

  // collect the labels
  if (entry->labels != NULL) {
    g_free(entry->labels);
  }
  if (tcg->nb_labels == 0) {
    entry->labels = NULL;
  } else {
    entry->labels = g_new0(size_t, tcg->nb_labels);
    if (entry->labels == NULL) {
      qce_fatal("fail to allocate memory for labels");
    }
  }
  entry->label_count = 0;

  for (size_t i = 0; i < entry->inst_count; i++) {
    QCEInst *inst = &entry->insts[i];
    if (inst->kind == QCE_INST_SET_LABEL) {
#ifdef QCE_DEBUG_IR
      g_assert(i + 1 < entry->inst_count);
#endif
      uint16_t label_id = inst->i_set_label.label.id;
#ifdef QCE_DEBUG_IR
      g_assert(label_id < tcg->nb_labels);
      if (entry->labels[label_id] != 0) {
        qce_fatal("label already exists");
      }
#endif
      entry->labels[label_id] = i + 1;
      entry->label_count++;
    }
  }
#ifdef QCE_DEBUG_IR
  // TODO: not sure why there are missing labels
  g_assert(entry->label_count <= tcg->nb_labels);
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
    fprintf(g_qce->trace_file, "\n{TB: %p} @ %016lx\n", tb, log_pc(cpu, tb));
  }
#endif

  // short-circuit if we are not in a session or are not in tracing mode
  QCESession *session = g_qce->session;
  if (session == NULL) {
    return;
  }

  if (session->mode == QCE_Tracing_NotStarted ||
      session->mode == QCE_Tracing_Stopped) {
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
    qce_state_env_put_symbolic_i64(state, (intptr_t)&arch->regs[R_EDI],
                                   state->solver_z3.blob_addr);
    qce_state_env_put_symbolic_i64(state, (intptr_t)&arch->regs[R_ESI],
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

#ifdef QCE_DEBUG_IR
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file, ">>>> \n");
  }
#endif

  // dual-mode (symbolic + concrete) emulation
  size_t cursor = 0;
  vaddr last_pc = 0;
  uint64_t pc_offset = 0;

  while (true) {
#ifdef QCE_DEBUG_IR
    g_assert(cursor < entry->inst_count);
#endif
    QCEInst *inst = &entry->insts[cursor];

    // dump the IR to be emulated
#ifdef QCE_DEBUG_IR
    if (g_qce->trace_file != NULL) {
      qce_debug_print_inst(g_qce->trace_file, inst);
    }
#endif

    switch (inst->kind) {
      /* ignored */
    case QCE_INST_DISCARD:
      break;

      /* markers */
    case QCE_INST_START: {
      if (unlikely(last_pc == 0)) {
        last_pc = log_pc(cpu, tb);
        pc_offset = last_pc - inst->i_start.pc;
      } else {
        last_pc = inst->i_start.pc + pc_offset;
      }
      break;
    }

      /* tb chaining */
    case QCE_INST_GOTO_TB: {
      uintptr_t next_tb = qatomic_read(&tb->jmp_dest[inst->i_goto_tb.idx]);
      if (next_tb == 0 || next_tb & 1) {
        // goto the next instruction if
        // 1) jump target is NULL, or
        // 2) this TB is invalidated
        // TODO: validate this understanding
        break;
      }
      goto end_of_loop;
    }
    case QCE_INST_EXIT_TB: {
      // TODO: what about the return value? i.e., inst->i_exit_tb.idx
      goto end_of_loop;
    }
    case QCE_INST_CALL_lookup_tb_ptr: {
#ifdef QCE_DEBUG_IR
      g_assert(cursor + 1 < entry->inst_count);
      g_assert(entry->insts[cursor + 1].kind == QCE_INST_GOTO_PTR);
#endif
      goto end_of_loop;
    }

      /* QCE control (via sgx instructions) */
    case QCE_INST_CALL_sgx: {
      // when we hit an SGX instruction, we know tracing is about to finish
      session->mode = QCE_Tracing_StopPending;
      goto end_of_loop;
    }

      /* assignment */
      HANDLE_SYM_INST_mov(32);
      HANDLE_SYM_INST_mov(64);

      /* sign/zero extend */
      HANDLE_SYM_INST_ext(32, 8, u, U);
      HANDLE_SYM_INST_ext(32, 8, s, S);
      HANDLE_SYM_INST_ext(32, 16, u, U);
      HANDLE_SYM_INST_ext(32, 16, s, S);
      HANDLE_SYM_INST_ext(64, 8, u, U);
      HANDLE_SYM_INST_ext(64, 8, s, S);
      HANDLE_SYM_INST_ext(64, 16, u, U);
      HANDLE_SYM_INST_ext(64, 16, s, S);
      HANDLE_SYM_INST_ext(64, 32, u, U);
      HANDLE_SYM_INST_ext(64, 32, s, S);

      /* arithmetics */
      HANDLE_SYM_INST_BIN_OP(ADD, add, 32);
      HANDLE_SYM_INST_BIN_OP(ADD, add, 64);

      HANDLE_SYM_INST_BIN_OP(SUB, sub, 32);
      HANDLE_SYM_INST_BIN_OP(SUB, sub, 64);

      HANDLE_SYM_INST_BIN_OP(MUL, mul, 32);
      HANDLE_SYM_INST_BIN_OP(MUL, mul, 64);

      /* multiword arithmetic */
      HANDLE_SYM_INST_QUAD_OP(ADD2, add2, 32);
      HANDLE_SYM_INST_QUAD_OP(ADD2, add2, 64);
      
      HANDLE_SYM_INST_QUAD_OP(SUB2, sub2, 32);
      HANDLE_SYM_INST_QUAD_OP(SUB2, sub2, 64);

      HANDLE_SYM_INST_BIN_OP_BIN_RES(MULS2, muls2, 32);
      HANDLE_SYM_INST_BIN_OP_BIN_RES(MULS2, muls2, 64);
      // HANDLE_SYM_INST_BIN_OP_BIN_RES(MULU2, mulu2, 32);
      // HANDLE_SYM_INST_BIN_OP_BIN_RES(MULU2, mulu2, 64);

      /* bitwise */
      HANDLE_SYM_INST_BIN_OP_BV(AND, and, 32);
      HANDLE_SYM_INST_BIN_OP_BV(AND, and, 64);

      HANDLE_SYM_INST_BIN_OP_BV(OR, or, 32);
      HANDLE_SYM_INST_BIN_OP_BV(OR, or, 64);

      HANDLE_SYM_INST_BIN_OP_BV(XOR, xor, 32);
      HANDLE_SYM_INST_BIN_OP_BV(XOR, xor, 64);

      HANDLE_SYM_INST_BIN_OP_BV(ANDC, andc, 32);
      HANDLE_SYM_INST_BIN_OP_BV(ANDC, andc, 64);

      HANDLE_SYM_INST_BIN_OP_BV(ORC, orc, 32);
      HANDLE_SYM_INST_BIN_OP_BV(ORC, orc, 64);

      HANDLE_SYM_INST_BIN_OP_BV(NAND, nand, 32);
      HANDLE_SYM_INST_BIN_OP_BV(NAND, nand, 64);

      HANDLE_SYM_INST_BIN_OP_BV(NOR, nor, 32);
      HANDLE_SYM_INST_BIN_OP_BV(NOR, nor, 64);

      HANDLE_SYM_INST_BIN_OP_BV(EQV, eqv, 32);
      HANDLE_SYM_INST_BIN_OP_BV(EQV, eqv, 64);

      /* load and store */
      HANDLE_SYM_INST_ld(LD8U, ld8u, 32);
      HANDLE_SYM_INST_ld(LD8S, ld8s, 32);
      HANDLE_SYM_INST_ld(LD16U, ld16u, 32);
      HANDLE_SYM_INST_ld(LD16S, ld16s, 32);
      HANDLE_SYM_INST_ld(LD, ld, 32);

      HANDLE_SYM_INST_ld(LD8U, ld8u, 64);
      HANDLE_SYM_INST_ld(LD8S, ld8s, 64);
      HANDLE_SYM_INST_ld(LD16U, ld16u, 64);
      HANDLE_SYM_INST_ld(LD16S, ld16s, 64);
      HANDLE_SYM_INST_ld(LD32U, ld32u, 64);
      HANDLE_SYM_INST_ld(LD32S, ld32s, 64);
      HANDLE_SYM_INST_ld(LD, ld, 64);

      HANDLE_SYM_INST_st(ST8, st8, 32);
      HANDLE_SYM_INST_st(ST16, st16, 32);
      HANDLE_SYM_INST_st(ST, st, 32);

      HANDLE_SYM_INST_st(ST8, st8, 64);
      HANDLE_SYM_INST_st(ST16, st16, 64);
      HANDLE_SYM_INST_st(ST32, st32, 64);
      HANDLE_SYM_INST_st(ST, st, 64);

      /* guest load and store */
      HANDLE_SYM_INST_qemu_ld(32);
      HANDLE_SYM_INST_qemu_ld(64);
      HANDLE_SYM_INST_qemu_st(32);
      HANDLE_SYM_INST_qemu_st(64);

      /* branch */
      HANDLE_SYM_INST_brcond(32);
      HANDLE_SYM_INST_brcond(64);

      /* all others */
    default: {
#ifdef QCE_DEBUG_IR
      qce_debug_print_inst(stderr, inst);
#endif
      // TODO: change to fatal
      // qce_fatal("emulation not supported yet");
    }
    }

    // for branch instructions, pc is already updated
    cursor += 1;
  }

end_of_loop:
#ifdef QCE_DEBUG_IR
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file, "<<<<\n");
  }
#endif
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
