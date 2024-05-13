#ifndef QEMU_QCE_H
#define QEMU_QCE_H

// logging utilities
#ifndef QCE_RELEASE
#include "qemu/error-report.h"
#define qce_debug(fmt, ...) warn_report("[QCE] " fmt, ##__VA_ARGS__)
#define qce_fatal(fmt, ...) warn_report("[!!!] " fmt, ##__VA_ARGS__)
#else
#define qce_debug(fmt, ...)
#define qce_fatal(fmt, ...)
#endif

// command selector
#define SGX_EDBGWR 0x05

#endif /* QEMU_QCE_H */