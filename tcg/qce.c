#include "qemu/qce.h"

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
  qemu_spin_init(&ctxt->lock);

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

  if (unlikely(ctxt->kvm_state == NULL)) {
    qce_fatal("context manager does not carry a QCE engine");
    return;
  }

  // de-allocate resources
  free(ctxt->kvm_state);
  ctxt->kvm_state = NULL;
  ctxt->vcpu_dirty = false;
  qce_debug("destroyed");
}

void qce_on_tcg_ir_generated(TCGContext *tcg, CPUState *cpu,
                             TranslationBlock *tb) {
  if (cpu->kvm_state == NULL) {
    return;
  }
  if (tcg->gen_tb != tb) {
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