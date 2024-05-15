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
  return 0;
}

void qce_shutdown(CPUState *cpu) {
  // only applicable to QCE context
  if (cpu->kvm_state == NULL) {
    return;
  }

  // de-allocate resources
  cpu->vcpu_dirty = false;
  free(cpu->kvm_state);
}

void qce_on_tcg_ir_generated(const TCGContext *tcg_ctx) {
  if (tcg_ctx->cpu->kvm_state == NULL) {
    return;
  }
}
