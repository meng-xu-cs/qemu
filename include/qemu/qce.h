#ifndef QEMU_QCE_H
#define QEMU_QCE_H

#include "qemu/qce-config.h"

#include "hw/core/cpu.h"
#include "qemu/thread.h"
#include "tcg/tcg.h"

// logging utilities
#ifndef QCE_RELEASE
#include "qemu/error-report.h"
#define qce_debug(fmt, ...) info_report("[QCE] " fmt, ##__VA_ARGS__)
#define qce_error(fmt, ...)                                                    \
  info_report("[!!!] " fmt " ... something wrong?", ##__VA_ARGS__)
#define qce_fatal(fmt, ...)                                                    \
  do {                                                                         \
    error_report("[!!!] " fmt, ##__VA_ARGS__);                                 \
    qce_on_panic();                                                            \
    killpg(0, SIGKILL);                                                        \
  } while (0);                                                                 \
  assert(0);
#else
#define qce_debug(fmt, ...)
#define qce_error(fmt, ...)
#define qce_fatal(fmt, ...)                                                    \
  killpg(0, SIGKILL);                                                          \
  assert(0);
#endif
#define __qce_unreachable__                                                    \
  killpg(0, SIGKILL);                                                          \
  assert(0);

// command selector
#define SGX_EDBGWR 0x05

// exposed type
struct QCEContext;
extern struct QCEContext *g_qce;

// context management
void qce_init(void);
void qce_destroy(void);

#ifndef QCE_RELEASE
void qce_on_panic(void);
#endif

// session management (a session is from save/load vm -> next load vm)
void qce_session_init(void);
void qce_session_reload(void);

// tracing
void qce_trace_start(tcg_target_ulong addr, tcg_target_ulong size);

// callback on TCG IR is first generated
void qce_on_tcg_ir_generated(TCGContext *tcg, TranslationBlock *tb);

// callback on TCG IR is fully optimized
void qce_on_tcg_ir_optimized(TCGContext *tcg);

// callback on TCG translation block being executed
void qce_on_tcg_tb_executed(TranslationBlock *tb, CPUState *cpu);

#ifndef QCE_RELEASE
void qce_unit_test(CPUArchState *env);
#endif

#endif /* QEMU_QCE_H */
