#ifndef QEMU_QCE_H
#define QEMU_QCE_H

#include "hw/core/cpu.h"
#include "qemu/thread.h"
#include "tcg/tcg.h"

// logging utilities
#ifndef QCE_RELEASE
#include "qemu/error-report.h"
#define qce_debug(fmt, ...) info_report("[QCE] " fmt, ##__VA_ARGS__)
#define qce_error(fmt, ...) info_report("[!!!] " fmt, ##__VA_ARGS__)
#define qce_fatal(fmt, ...)                                                    \
  do {                                                                         \
    error_report("[!!!] " fmt, ##__VA_ARGS__);                                 \
    killpg(0, SIGKILL);                                                        \
  } while (0);
#else
#define qce_debug(fmt, ...)
#define qce_error(fmt, ...)
#define qce_fatal(fmt, ...) killpg(0, SIGKILL)
#endif

// command selector
#define SGX_EDBGWR 0x05

// exposed type
struct QCEContext;

// context management
int qce_init(CPUState *cpu);
void qce_destroy(void);

// tracing
void qce_trace_start(CPUState *cpu, tcg_target_ulong addr,
                     tcg_target_ulong len);
void qce_trace_try_finish(void);

// callback on TCG IR is first generated
void qce_on_tcg_ir_generated(TCGContext *tcg, CPUState *cpu,
                             TranslationBlock *tb);

// callback on TCG IR is fully optimized
void qce_on_tcg_ir_optimized(TCGContext *tcg);

// callback on TCG translation block being executed
void qce_on_tcg_tb_executed(TranslationBlock *tb, CPUState *cpu);

#endif /* QEMU_QCE_H */
