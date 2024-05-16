#include "qemu/qce.h"
#include "exec/translation-block.h"
#include "qemu/xxhash.h"

#include "qemu/qht.h"

// exec-graph
struct QCEBlock {
  const TranslationBlock *tb;
};

static bool qce_block_qht_cmp(const void *a, const void *b) {
  return ((struct QCEBlock *)a)->tb == ((struct QCEBlock *)b)->tb;
}
static bool qce_block_qht_lookup(const void *p, const void *userp) {
  return ((struct QCEBlock *)p)->tb == (const TranslationBlock *)userp;
}
static void qce_block_qht_iter_free(void *p, uint32_t _h, void *_userp) {
  g_free(p);
}

// context
struct QCEContext {
  // a map of the translation block
  struct qht /* <const TranslationBlock *, QCEBlock> */ tb_map;
};

#define QCE_CTXT_TB_MAP_SIZE 1 << 16
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

  qht_init(&ctxt->tb_map, qce_block_qht_cmp, QCE_CTXT_TB_MAP_SIZE,
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

  qht_iter(&qce->tb_map, qce_block_qht_iter_free, NULL);
  qht_destroy(&qce->tb_map);

  // de-allocate resources
  free(qce);
  ctxt->kvm_state = NULL;
  ctxt->vcpu_dirty = false;
  qce_debug("destroyed");
}

void qce_trace_start(void) {}

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

  // mark the translation block
  struct TranslationBlock *tb = tcg->gen_tb;
  struct QCEBlock *blk = g_malloc0(sizeof(*blk));
  blk->tb = tb;

  // go over the operators
  TCGOp *op;
  QTAILQ_FOREACH(op, &tcg->ops, link) {
    // TODO
  }

  // insert it
  if (!qht_insert(&qce->tb_map, (void *)blk, qemu_xxhash2((uint64_t)tb),
                  NULL)) {
    qce_fatal("duplicate translation block: 0x%p", tb);
  }

  // clear the CPU marker
  tcg->cpu = NULL;
}

void qce_on_tcg_tb_executed(TranslationBlock *tb, CPUState *cpu) {
  struct QCEContext *qce = (struct QCEContext *)cpu->kvm_state;
  if (qce == NULL) {
    return;
  }

  // find the block
  struct QCEBlock *blk = qht_lookup_custom(
      &qce->tb_map, tb, qemu_xxhash2((uint64_t)tb), qce_block_qht_lookup);
  if (blk == NULL) {
    qce_fatal("unable to find QCE block for translation block: 0x%p", tb);
  }

  // CPUArchState *arch = cpu_env(cpu);
}