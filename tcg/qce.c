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
  return 0;
}

void qce_shutdown(CPUState *cpu) {
  // only applicable to QCE context
  if (cpu->kvm_state == NULL) {
    return;
  }

  // free memory usage
  free(cpu->kvm_state);
}

void qce_on_tcg_ir_generated(CPUState *cpu) {
  if (cpu->kvm_state == NULL) {
    return;
  }
}