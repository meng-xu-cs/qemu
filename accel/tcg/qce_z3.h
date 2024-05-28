#ifndef QEMU_QCE_Z3_H
#define QEMU_QCE_Z3_H

#include "z3.h"

// context
typedef struct {
  Z3_context ctx;
} SolverZ3;

void error_handler_z3(Z3_context ctx, Z3_error_code ec);
void error_handler_z3(Z3_context ctx, Z3_error_code ec) {
  qce_fatal("unexpected Z3 error %d: %s", ec, Z3_get_error_msg(ctx, ec));
}

/*
 * Context Lifecycle
 */

static inline void init_z3(SolverZ3 *solver) {
  Z3_config cfg = Z3_mk_config();
  Z3_context ctx = Z3_mk_context(cfg);
  Z3_set_error_handler(ctx, error_handler_z3);
  Z3_del_config(cfg);

  // assignment
  solver->ctx = ctx;
}

static inline void fini_z3(SolverZ3 *solver) {
  Z3_del_context(solver->ctx);
  solver->ctx = NULL;
}

#endif // QEMU_QCE_Z3_H
