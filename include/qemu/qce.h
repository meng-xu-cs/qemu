#ifndef QEMU_QCE_H
#define QEMU_QCE_H

#include "hw/core/cpu.h"
#include "qemu/thread.h"
#include "tcg/tcg.h"

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

// context
struct QCEContext {
  QemuSpin lock;
};

// context management
int qce_init(CPUState *cpu);
void qce_shutdown(CPUState *cpu);

// callback on TCG IR is first generated
void qce_on_tcg_ir_generated(TCGContext *cpu);

#endif /* QEMU_QCE_H */
