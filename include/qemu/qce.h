#ifndef QEMU_QCE_H
#define QEMU_QCE_H

#include "hw/core/cpu.h"

// logging utilities
#ifndef QCE_RELEASE
#include "qemu/error-report.h"
#define qce_debug(fmt, ...) info_report("[QCE] " fmt, ##__VA_ARGS__)
#define qce_fatal(fmt, ...) error_report("[!!!] " fmt, ##__VA_ARGS__)
#else
#define qce_debug(fmt, ...)
#define qce_fatal(fmt, ...)
#endif

// command selector
#define SGX_EDBGWR 0x05

// initialize the context
static inline int qce_init(CPUState *cpu) {
  // repurpose the kvm_state (which is only used in kvm) for context
  if (cpu->kvm_state != NULL) {
    qce_fatal("kvm_state is not NULL, cannot repurpose it");
    return 1;
  }
  return 0;
}

// destroy the context
static inline void qce_shutdown(CPUState *cpu) {
  // only applicable to QCE context
  if (cpu->kvm_state == NULL) {
    return;
  }
}

#endif /* QEMU_QCE_H */