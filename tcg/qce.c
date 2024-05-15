#include "qemu/qce.h"

int qce_init(CPUState *cpu) {
  // repurpose the kvm_state (which is only used in kvm) for context
  if (cpu->kvm_state != NULL) {
    qce_fatal("kvm_state is not NULL, cannot repurpose it");
    return 1;
  }

  // initialize the context
  struct QCEContext *ctxt = malloc(sizeof(struct QCEContext));
  if (ctxt == NULL) {
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
      if (ctxt != NULL) {
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

  if (ctxt->kvm_state == NULL) {
    qce_fatal("context manager does not carry a QCE engine");
    return;
  }

  // de-allocate resources
  free(ctxt->kvm_state);
  ctxt->kvm_state = NULL;
  ctxt->vcpu_dirty = false;
  qce_debug("destroyed");
}

void qce_on_tcg_ir_generated(const TCGContext *tcg_ctx) {
  if (tcg_ctx->cpu->kvm_state == NULL) {
    return;
  }
}
