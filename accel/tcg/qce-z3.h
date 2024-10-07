#include "z3.h"

#define QCE_SMT_Z3_EAGER_SIMPLIFY

// context
typedef struct {
  Z3_context ctx;
  Z3_solver sol;
  Z3_model model;

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

  // create the solver
  Z3_solver sol = Z3_mk_solver(ctx);

  // assignment
  solver->ctx = ctx;
  solver->blob_addr = blob_addr;
  solver->blob_size = blob_size;
  solver->blob_content = blob_content;
  solver->sort_bv32 = sort_bv32;
  solver->sort_bv64 = sort_bv64;

  solver->sol = sol;
  solver->model = NULL;
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

static inline bool __qce_smt_z3_simplify_reduce(SolverZ3 *solver, Z3_ast expr,
                                                Z3_ast *result) {
#ifndef QCE_RELEASE
  Z3_app app = Z3_to_app(solver->ctx, expr);
  assert(Z3_get_app_num_args(solver->ctx, app) != 0);
#endif

  // get an answer first
  switch (Z3_solver_check(solver->ctx, solver->sol)) {
  case Z3_L_TRUE: {
    Z3_model model = Z3_solver_get_model(solver->ctx, solver->sol);
    if (!Z3_model_eval(solver->ctx, model, expr, true, result)) {
      qce_fatal("model evaluation failed");
    }
  }
  case Z3_L_FALSE: {
    qce_fatal("model evaluation on an infeasible path");
  }
  case Z3_L_UNDEF: {
    qce_fatal("unable to determine the satisfiability of path constraints");
  }
  }

  // check if the answer is the only possibility
  Z3_ast proposition =
      Z3_mk_distinct(solver->ctx, 2, (Z3_ast[]){expr, *result});
  switch (Z3_solver_check_assumptions(solver->ctx, solver->sol, 1,
                                      (Z3_ast[]){proposition})) {
  case Z3_L_TRUE: {
    return false;
  }
  case Z3_L_FALSE: {
    return true;
  }
  case Z3_L_UNDEF: {
    qce_fatal("unable to determine the feasibility of a unique model");
  }
  }
}

static inline bool qce_smt_z3_probe_bv32(SolverZ3 *solver, Z3_ast expr,
                                         int32_t *val) {
#ifndef QCE_RELEASE
  Z3_sort sort = Z3_get_sort(solver->ctx, expr);
  assert(Z3_get_sort_kind(solver->ctx, sort) == Z3_BV_SORT);
  assert(Z3_get_bv_sort_size(solver->ctx, sort) == 32);
#endif

  Z3_ast_kind kind = Z3_get_ast_kind(solver->ctx, expr);
  switch (kind) {
  case Z3_NUMERAL_AST: {
    Z3_get_numeral_int(solver->ctx, expr, val);
    return true;
  }
  case Z3_APP_AST: {
    Z3_ast evaluated;
    if (__qce_smt_z3_simplify_reduce(solver, expr, &evaluated)) {
#ifndef QCE_RELEASE
      assert(Z3_get_ast_kind(solver->ctx, evaluated) == Z3_NUMERAL_AST);
#endif
      Z3_get_numeral_int(solver->ctx, expr, val);
      return true;
    } else {
      return false;
    }
  }
  default:
    qce_fatal("unexpected Z3 ast kind: %d", kind);
  }
}

static inline bool qce_smt_z3_probe_bv64(SolverZ3 *solver, Z3_ast expr,
                                         int64_t *val) {
#ifndef QCE_RELEASE
  Z3_sort sort = Z3_get_sort(solver->ctx, expr);
  assert(Z3_get_sort_kind(solver->ctx, sort) == Z3_BV_SORT);
  assert(Z3_get_bv_sort_size(solver->ctx, sort) == 64);
#endif

  Z3_ast_kind kind = Z3_get_ast_kind(solver->ctx, expr);
  switch (kind) {
  case Z3_NUMERAL_AST: {
    Z3_get_numeral_int64(solver->ctx, expr, val);
    return true;
  }
  case Z3_APP_AST: {
    Z3_ast evaluated;
    if (__qce_smt_z3_simplify_reduce(solver, expr, &evaluated)) {
#ifndef QCE_RELEASE
      assert(Z3_get_ast_kind(solver->ctx, evaluated) == Z3_NUMERAL_AST);
#endif
      Z3_get_numeral_int64(solver->ctx, expr, val);
      return true;
    } else {
      return false;
    }
  }
  default:
    qce_fatal("unexpected Z3 ast kind: %d", kind);
  }
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

static inline Z3_ast qce_smt_z3_bv64_extract_l(SolverZ3 *solver, Z3_ast expr) {
  return qce_smt_z3_simplify(solver, Z3_mk_extract(solver->ctx, 31, 0, expr));
}

static inline Z3_ast qce_smt_z3_bv64_extract_h(SolverZ3 *solver, Z3_ast expr) {
  return qce_smt_z3_simplify(solver, Z3_mk_extract(solver->ctx, 63, 32, expr));
}

static inline Z3_ast qce_smt_z3_bv64_concat(SolverZ3 *solver, Z3_ast expr_h,
                                            Z3_ast expr_l) {
  return qce_smt_z3_simplify(solver, Z3_mk_concat(solver->ctx, expr_h, expr_l));
}

/*
 * Arithmetics
 */

static inline Z3_ast qce_smt_z3_bv32_add(SolverZ3 *solver, Z3_ast expr_h,
                                         Z3_ast expr_l) {
  return qce_smt_z3_simplify(solver, Z3_mk_bvadd(solver->ctx, expr_h, expr_l));
}

static inline Z3_ast qce_smt_z3_bv64_add(SolverZ3 *solver, Z3_ast expr_h,
                                         Z3_ast expr_l) {
  return qce_smt_z3_simplify(solver, Z3_mk_bvadd(solver->ctx, expr_h, expr_l));
}

/*
 * Equality
 */

static inline Z3_ast qce_smt_z3_mk_eq(SolverZ3 *solver, Z3_ast lhs,
                                      Z3_ast rhs) {
  return qce_smt_z3_simplify(solver, Z3_mk_eq(solver->ctx, lhs, rhs));
}

/*
 * Proving
 */

typedef enum {
  SMT_Z3_PROVE_REFUTED = -1,
  SMT_Z3_PROVE_UNKNOWN = 0,
  SMT_Z3_PROVE_PROVED = 1,
} SmtZ3ProveResult;

static inline SmtZ3ProveResult qce_smt_z3_prove(SolverZ3 *solver,
                                                Z3_ast proposition) {
  Z3_lbool r1 = Z3_solver_check_assumptions(solver->ctx, solver->sol, 1,
                                            (Z3_ast[]){proposition});
  if (r1 == Z3_L_UNDEF) {
    return SMT_Z3_PROVE_UNKNOWN;
  }
  if (r1 == Z3_L_FALSE) {
    // the claim itself is inconsistent
    return SMT_Z3_PROVE_REFUTED;
  }
#ifndef QCE_RELEASE
  assert(r1 == Z3_L_TRUE);
#endif

  Z3_lbool r2 = Z3_solver_check_assumptions(
      solver->ctx, solver->sol, 1,
      (Z3_ast[]){Z3_mk_not(solver->ctx, proposition)});
  if (r2 == Z3_L_UNDEF) {
    return SMT_Z3_PROVE_UNKNOWN;
  }
  if (r2 == Z3_L_FALSE) {
    // the negation of the claim is inconsistent, the claim is proved
    return SMT_Z3_PROVE_PROVED;
  }
#ifndef QCE_RELEASE
  assert(r2 == Z3_L_TRUE);
#endif
  return SMT_Z3_PROVE_REFUTED;
}

/*
 * Testing
 */

#ifndef QCE_RELEASE
static inline void qce_unit_test_z3(void) {
  // setup
  SolverZ3 solver;
  qce_smt_z3_init(&solver);

  // tear-down
  qce_smt_z3_fini(&solver);
}
#endif
