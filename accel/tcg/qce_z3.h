#include "z3.h"

#define QCE_SMT_Z3_EAGER_SIMPLIFY

// context
typedef struct {
  Z3_context ctx;

  // variables
  Z3_ast blob_addr;
  Z3_ast blob_size;
  Z3_ast blob_content;

  // utilities
  Z3_sort sort_bv32;
  Z3_sort sort_bv64;
} SolverZ3;

void qce_error_handler_z3(Z3_context ctx, Z3_error_code ec);
void qce_error_handler_z3(Z3_context ctx, Z3_error_code ec) {
  qce_fatal("unexpected Z3 error %d: %s", ec, Z3_get_error_msg(ctx, ec));
}

/*
 * Context Lifecycle
 */

static inline void qce_smt_z3_init(SolverZ3 *solver) {
  // initialize the context
  Z3_config cfg = Z3_mk_config();
  Z3_context ctx = Z3_mk_context(cfg);
  Z3_set_error_handler(ctx, qce_error_handler_z3);
  Z3_del_config(cfg);

  // populate the utility types
  Z3_sort sort_bv8 = Z3_mk_bv_sort(ctx, 8);
  Z3_sort sort_bv32 = Z3_mk_bv_sort(ctx, 32);
  Z3_sort sort_bv64 = Z3_mk_bv_sort(ctx, 64);

  // pre-define inputs
  Z3_ast blob_addr =
      Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, "addr"), sort_bv64);
  Z3_ast blob_size =
      Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, "size"), sort_bv64);

  Z3_sort sort_blob = Z3_mk_array_sort(ctx, sort_bv64, sort_bv8);
  Z3_ast blob_content =
      Z3_mk_const(ctx, Z3_mk_string_symbol(ctx, "blob"), sort_blob);

  // assignment
  solver->ctx = ctx;
  solver->blob_addr = blob_addr;
  solver->blob_size = blob_size;
  solver->blob_content = blob_content;
  solver->sort_bv32 = sort_bv32;
  solver->sort_bv64 = sort_bv64;
}

static inline void qce_smt_z3_fini(SolverZ3 *solver) {
  // destroy the context
  Z3_del_context(solver->ctx);
}

/*
 * Simplification
 */

static inline Z3_ast qce_smt_z3_simplify(SolverZ3 *solver, Z3_ast expr) {
  Z3_ast res = Z3_simplify(solver->ctx, expr);
#ifdef QCE_SMT_Z3_EAGER_SIMPLIFY
  // TODO: more simplification rules
#endif
  return res;
}

/*
 * Bit-vector
 */

static inline Z3_ast qce_smt_z3_bv32_value(SolverZ3 *solver, int32_t val) {
  return Z3_mk_int(solver->ctx, val, solver->sort_bv32);
}

static inline Z3_ast qce_smt_z3_bv64_value(SolverZ3 *solver, int64_t val) {
  return Z3_mk_int64(solver->ctx, val, solver->sort_bv64);
}

static inline Z3_ast qce_smt_z3_bv64_extract_t(SolverZ3 *solver, Z3_ast expr) {
  return qce_smt_z3_simplify(solver, Z3_mk_extract(solver->ctx, 63, 32, expr));
}

static inline Z3_ast qce_smt_z3_bv64_extract_b(SolverZ3 *solver, Z3_ast expr) {
  return qce_smt_z3_simplify(solver, Z3_mk_extract(solver->ctx, 31, 0, expr));
}

static inline Z3_ast qce_smt_z3_bv64_concat(SolverZ3 *solver, Z3_ast expr_t,
                                            Z3_ast expr_b) {
  return qce_smt_z3_simplify(solver, Z3_mk_concat(solver->ctx, expr_t, expr_b));
}
