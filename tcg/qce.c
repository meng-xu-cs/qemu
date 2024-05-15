#include "qemu/qce.h"
#include "exec/translation-block.h"
#include "qemu/xxhash.h"

#include "qemu/qht.h"

// context
struct QCEContext {
  // a map of the translation block
  struct qht /* <TranslationBlock, vaddr> */ tb_map;
};

#define QCE_CTXT_TB_MAP_SIZE 1 << 16
static bool qce_ctxt_tb_map_cmp(const void *a, const void *b) {
  const TranslationBlock *lhs = a;
  const TranslationBlock *rhs = b;
  return lhs->pc == rhs->pc;
}
static inline void qce_ctxt_register_tb(struct QCEContext *qce,
                                        const TranslationBlock *tb) {
  if (!qht_insert(&qce->tb_map, (void *)tb, qemu_xxhash2(tb->pc), NULL)) {
    qce_fatal("duplicate translation block for PC 0x%lx", tb->pc);
  }
}

int qce_init(CPUState *cpu) {
  // repurpose the kvm_state (which is only used in kvm) for context
  if (unlikely(cpu->kvm_state != NULL)) {
    qce_fatal("kvm_state is not NULL, cannot repurpose it");
    return 1;
  }

  // initialize the context
  struct QCEContext *ctxt = malloc(sizeof(struct QCEContext));
  if (unlikely(ctxt == NULL)) {
    qce_fatal("unable to allocate QCE context");
    return 1;
  }

  qht_init(&ctxt->tb_map, qce_ctxt_tb_map_cmp, QCE_CTXT_TB_MAP_SIZE,
           QHT_MODE_AUTO_RESIZE);

  // done
  cpu->kvm_state = (struct KVMState *)ctxt;
  cpu->vcpu_dirty = true; // mark this vcpu as the main context manager
  qce_debug("initialized");
  return 0;
}

void qce_try_shutdown(void) {
  CPUState *cpu;
  CPUState *ctxt = NULL;

  CPU_FOREACH(cpu) {
    // only shutdown QCE when all CPUs are stopped
    if (!cpu->stopped) {
      return;
    }
    // locate the context manager
    if (cpu->vcpu_dirty) {
      if (unlikely(ctxt != NULL)) {
        qce_fatal("more than one context manager for QCE");
        return;
      }
      ctxt = cpu;
    }
  }

  // no manager found, QCE has been destroyed
  if (ctxt == NULL) {
    return;
  }

  // destruct the QCE context
  struct QCEContext *qce = (struct QCEContext *)ctxt->kvm_state;
  if (unlikely(qce == NULL)) {
    qce_fatal("context manager does not carry a QCE engine");
    return;
  }

  qht_destroy(&qce->tb_map);

  // de-allocate resources
  free(qce);
  ctxt->kvm_state = NULL;
  ctxt->vcpu_dirty = false;
  qce_debug("destroyed");
}

void qce_on_tcg_ir_generated(TCGContext *tcg, CPUState *cpu,
                             TranslationBlock *tb) {
  if (cpu->kvm_state == NULL) {
    return;
  }
  if (unlikely(tcg->gen_tb != tb)) {
    qce_fatal("TCGContext::gen_tb does not match the tb argument");
  }
  tcg->cpu = cpu;
}

void qce_on_tcg_ir_optimized(TCGContext *tcg) {
  if (tcg->cpu == NULL) {
    return;
  }

  // obtain the context manager
  struct QCEContext *qce = (struct QCEContext *)tcg->cpu->kvm_state;
  if (unlikely(qce == NULL)) {
    qce_fatal("TCG context does not carry a QCE engine");
    return;
  }

  // go over the operators
  TCGOp *op;
  QTAILQ_FOREACH(op, &tcg->ops, link) {
    // TODO
  }

  // mark the translation block
  qce_ctxt_register_tb(qce, tcg->gen_tb);

  // clear the CPU marker
  tcg->cpu = NULL;
}

void qce_on_tcg_tb_executed(TranslationBlock *tb, CPUState *cpu) {
  struct QCEContext *qce = (struct QCEContext *)cpu->kvm_state;
  if (qce == NULL) {
    return;
  }
  // CPUArchState *arch = cpu_env(cpu);
}