#include "z3.h"

// context
typedef struct {
  Z3_context ctx;
  Z3_solver sol;

  // variables
  Z3_ast blob_addr;
  Z3_ast blob_size;
  Z3_ast blob_content;

  // utilities
  Z3_sort sort_bv32;
  Z3_sort sort_bv64;

  // variables for testing
#ifndef QCE_RELEASE
  uint64_t var_count;
#endif
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
  Z3_solver_inc_ref(ctx, sol);

  // assignment
  solver->ctx = ctx;
  solver->sol = sol;
  solver->blob_addr = blob_addr;
  solver->blob_size = blob_size;
  solver->blob_content = blob_content;
  solver->sort_bv32 = sort_bv32;
  solver->sort_bv64 = sort_bv64;
#ifndef QCE_RELEASE
  solver->var_count = 0;
#endif
}

static inline void qce_smt_z3_fini(SolverZ3 *solver) {
  // destroy the context
  Z3_solver_dec_ref(solver->ctx, solver->sol);
  Z3_del_context(solver->ctx);
}

/*
 * Paranoid checks
 */

#ifndef QCE_RELEASE
static inline void __qce_smt_z3_type_check_bool(SolverZ3 *solver, Z3_ast expr) {
  Z3_sort sort = Z3_get_sort(solver->ctx, expr);
  if (Z3_get_sort_kind(solver->ctx, sort) != Z3_BOOL_SORT) {
    qce_fatal("expression %s: expect bool, got %s",
              Z3_ast_to_string(solver->ctx, expr),
              Z3_sort_to_string(solver->ctx, sort));
  }
}

static inline void __qce_smt_z3_type_check_bv32(SolverZ3 *solver, Z3_ast expr) {
  Z3_sort sort = Z3_get_sort(solver->ctx, expr);
  if (Z3_get_sort_kind(solver->ctx, sort) != Z3_BV_SORT) {
    qce_fatal("expression %s: expect bv32, got %s",
              Z3_ast_to_string(solver->ctx, expr),
              Z3_sort_to_string(solver->ctx, sort));
  }

  unsigned int nbits = Z3_get_bv_sort_size(solver->ctx, sort);
  if (nbits != 32) {
    qce_fatal("expression %s: expect bv32, got bv%d",
              Z3_ast_to_string(solver->ctx, expr), nbits);
  }
}

static inline void __qce_smt_z3_type_check_bv64(SolverZ3 *solver, Z3_ast expr) {
  Z3_sort sort = Z3_get_sort(solver->ctx, expr);
  if (Z3_get_sort_kind(solver->ctx, sort) != Z3_BV_SORT) {
    qce_fatal("expression %s: expect bv64, got %s",
              Z3_ast_to_string(solver->ctx, expr),
              Z3_sort_to_string(solver->ctx, sort));
  }

  unsigned int nbits = Z3_get_bv_sort_size(solver->ctx, sort);
  if (nbits != 64) {
    qce_fatal("expression %s: expect bv32, got bv%d",
              Z3_ast_to_string(solver->ctx, expr), nbits);
  }
}
#else
#define __qce_smt_z3_type_check_bool(solver, expr)
#define __qce_smt_z3_type_check_bv32(solver, expr)
#define __qce_smt_z3_type_check_bv64(solver, expr)
#endif

/*
 * Simplification
 */

static inline Z3_ast __qce_smt_z3_simplify(SolverZ3 *solver, Z3_ast expr) {
  Z3_ast res = Z3_simplify(solver->ctx, expr);
#ifdef QCE_SMT_Z3_EAGER_SIMPLIFY
  // TODO: more simplification rules
#endif
  return res;
}

static inline bool __qce_smt_z3_simplify_reduce(SolverZ3 *solver, Z3_ast expr,
                                                Z3_ast *result) {
  // get an answer first
  switch (Z3_solver_check(solver->ctx, solver->sol)) {
  case Z3_L_TRUE: {
    Z3_model model = Z3_solver_get_model(solver->ctx, solver->sol);
    if (!Z3_model_eval(solver->ctx, model, expr, true, result)) {
      qce_fatal("model evaluation failed");
    }
    break;
  }
  case Z3_L_FALSE: {
    qce_fatal("model evaluation on an infeasible path");
  }
  case Z3_L_UNDEF: {
    qce_fatal("unable to determine the satisfiability of path constraints");
  }
  default: {
    __qce_unreachable__;
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
  default: {
    __qce_unreachable__;
  }
  }
}

static inline bool qce_smt_z3_probe_bool(SolverZ3 *solver, Z3_ast pred,
                                         bool *val) {
  __qce_smt_z3_type_check_bool(solver, pred);

#ifndef QCE_RELEASE
  Z3_ast_kind kind = Z3_get_ast_kind(solver->ctx, pred);
  if (kind != Z3_APP_AST) {
    qce_fatal("unexpected Z3 ast kind for bool: %d", kind);
  }
#endif

  // check satisfiability
  Z3_lbool res_positive = Z3_solver_check_assumptions(solver->ctx, solver->sol,
                                                      1, (Z3_ast[]){pred});
  if (res_positive == Z3_L_UNDEF) {
    qce_fatal("unable to establish the predicate (positive case)");
  }

  Z3_lbool res_negative = Z3_solver_check_assumptions(
      solver->ctx, solver->sol, 1, (Z3_ast[]){Z3_mk_not(solver->ctx, pred)});
  if (res_negative == Z3_L_UNDEF) {
    qce_fatal("unable to establish the predicate (negative case)");
  }

  // handle the results case by case
  if (res_positive == Z3_L_TRUE) {
    if (res_negative == Z3_L_TRUE) {
      // case 1
      return false;
    }

#ifndef QCE_RELEASE
    assert(res_negative == Z3_L_FALSE);
#endif
    // case 2
    *val = true;
    return true;
  }

#ifndef QCE_RELEASE
  assert(res_positive == Z3_L_FALSE);
#endif
  if (res_negative == Z3_L_TRUE) {
    // case 3
    *val = false;
    return true;
  }

#ifndef QCE_RELEASE
  assert(res_negative == Z3_L_FALSE);
#endif
  // case 4
  qce_fatal("logically infeasible predicate");
}

static inline bool qce_smt_z3_probe_bv32(SolverZ3 *solver, Z3_ast expr,
                                         int32_t *val) {
  __qce_smt_z3_type_check_bv32(solver, expr);

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
      Z3_get_numeral_int(solver->ctx, evaluated, val);
      return true;
    }
    return false;
  }
  default:
    qce_fatal("unexpected Z3 ast kind for bv32: %d", kind);
  }
}

static inline bool qce_smt_z3_probe_bv64(SolverZ3 *solver, Z3_ast expr,
                                         int64_t *val) {
  __qce_smt_z3_type_check_bv64(solver, expr);

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
    }
    return false;
  }
  default:
    qce_fatal("unexpected Z3 ast kind for bv32: %d", kind);
  }
}

/*
 * Template
 */

#define DEFINE_SMT_Z3_OP2(bits, name, func)                                    \
  static inline Z3_ast qce_smt_z3_bv##bits##_##name(SolverZ3 *solver,          \
                                                    Z3_ast lhs, Z3_ast rhs) {  \
    __qce_smt_z3_type_check_bv##bits(solver, lhs);                             \
    __qce_smt_z3_type_check_bv##bits(solver, rhs);                             \
    return __qce_smt_z3_simplify(solver, func(solver->ctx, lhs, rhs));         \
  }

#define DEFINE_SMT_Z3_OP2_DUAL(name, func)                                     \
  DEFINE_SMT_Z3_OP2(32, name, func)                                            \
  DEFINE_SMT_Z3_OP2(64, name, func)

#define DEFINE_SMT_Z3_OP2_EX(bits, name, func)                                 \
  static inline Z3_ast qce_smt_z3_bv##bits##_##name(SolverZ3 *solver,          \
                                                    Z3_ast lhs, Z3_ast rhs) {  \
    __qce_smt_z3_type_check_bv##bits(solver, lhs);                             \
    __qce_smt_z3_type_check_bv##bits(solver, rhs);                             \
    return __qce_smt_z3_simplify(solver,                                       \
                                 func(solver->ctx, 2, (Z3_ast[]){lhs, rhs}));  \
  }

#define DEFINE_SMT_Z3_OP2_DUAL_EX(name, func)                                  \
  DEFINE_SMT_Z3_OP2_EX(32, name, func)                                         \
  DEFINE_SMT_Z3_OP2_EX(64, name, func)

/*
 * Bit-vector
 */

static inline Z3_ast qce_smt_z3_bv32_value(SolverZ3 *solver, int32_t val) {
  return Z3_mk_int(solver->ctx, val, solver->sort_bv32);
}

static inline Z3_ast qce_smt_z3_bv64_value(SolverZ3 *solver, int64_t val) {
  return Z3_mk_int64(solver->ctx, val, solver->sort_bv64);
}

#ifndef QCE_RELEASE
static inline Z3_ast qce_smt_z3_bv32_var(SolverZ3 *solver) {
  Z3_symbol symbol = Z3_mk_int_symbol(solver->ctx, solver->var_count++);
  return Z3_mk_const(solver->ctx, symbol, solver->sort_bv32);
}

static inline Z3_ast qce_smt_z3_bv64_var(SolverZ3 *solver) {
  Z3_symbol symbol = Z3_mk_int_symbol(solver->ctx, solver->var_count++);
  return Z3_mk_const(solver->ctx, symbol, solver->sort_bv64);
}
#endif

static inline Z3_ast qce_smt_z3_bv64_extract_l(SolverZ3 *solver, Z3_ast expr) {
  __qce_smt_z3_type_check_bv64(solver, expr);
  return __qce_smt_z3_simplify(solver, Z3_mk_extract(solver->ctx, 31, 0, expr));
}

static inline Z3_ast qce_smt_z3_bv64_extract_h(SolverZ3 *solver, Z3_ast expr) {
  __qce_smt_z3_type_check_bv64(solver, expr);
  return __qce_smt_z3_simplify(solver,
                               Z3_mk_extract(solver->ctx, 63, 32, expr));
}

static inline Z3_ast qce_smt_z3_bv64_concat(SolverZ3 *solver, Z3_ast h,
                                            Z3_ast l) {
  __qce_smt_z3_type_check_bv32(solver, h);
  __qce_smt_z3_type_check_bv32(solver, l);
  return __qce_smt_z3_simplify(solver, Z3_mk_concat(solver->ctx, h, l));
}

/*
 * Arithmetics
 */

DEFINE_SMT_Z3_OP2_DUAL(add, Z3_mk_bvadd)
DEFINE_SMT_Z3_OP2_DUAL(sub, Z3_mk_bvsub)

/*
 * Comparisons
 */

DEFINE_SMT_Z3_OP2_DUAL(eq, Z3_mk_eq)
DEFINE_SMT_Z3_OP2_DUAL_EX(ne, Z3_mk_distinct)

DEFINE_SMT_Z3_OP2_DUAL(slt, Z3_mk_bvslt)
DEFINE_SMT_Z3_OP2_DUAL(sle, Z3_mk_bvsle)
DEFINE_SMT_Z3_OP2_DUAL(sge, Z3_mk_bvsge)
DEFINE_SMT_Z3_OP2_DUAL(sgt, Z3_mk_bvsgt)

DEFINE_SMT_Z3_OP2_DUAL(ult, Z3_mk_bvult)
DEFINE_SMT_Z3_OP2_DUAL(ule, Z3_mk_bvule)
DEFINE_SMT_Z3_OP2_DUAL(uge, Z3_mk_bvuge)
DEFINE_SMT_Z3_OP2_DUAL(ugt, Z3_mk_bvugt)

/*
 * Bitwise
 */

DEFINE_SMT_Z3_OP2_DUAL(bvand, Z3_mk_bvand)

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

static inline SmtZ3ProveResult
qce_smt_z3_prove_equiv(SolverZ3 *solver, Z3_ast prop1, Z3_ast prop2) {
  switch (qce_smt_z3_prove(solver, Z3_mk_implies(solver->ctx, prop1, prop2))) {
  case SMT_Z3_PROVE_REFUTED:
    return SMT_Z3_PROVE_REFUTED;
  case SMT_Z3_PROVE_UNKNOWN:
    return SMT_Z3_PROVE_UNKNOWN;
  case SMT_Z3_PROVE_PROVED:
    break;
  }

  switch (qce_smt_z3_prove(solver, Z3_mk_implies(solver->ctx, prop2, prop1))) {
  case SMT_Z3_PROVE_REFUTED:
    return SMT_Z3_PROVE_REFUTED;
  case SMT_Z3_PROVE_UNKNOWN:
    return SMT_Z3_PROVE_UNKNOWN;
  case SMT_Z3_PROVE_PROVED:
    return SMT_Z3_PROVE_PROVED;
  }

  __qce_unreachable__
}

/*
 * Testing
 */

#ifndef QCE_RELEASE

// helper macro
#define QCE_UNIT_TEST_SMT_Z3_PROLOGUE(name)                                    \
  static inline void qce_unit_test_smt_z3_##name(void) {                       \
    qce_debug("[test][z3] " #name);                                            \
    SolverZ3 solver;                                                           \
    qce_smt_z3_init(&solver);

#define QCE_UNIT_TEST_SMT_Z3_EPILOGUE                                          \
  qce_smt_z3_fini(&solver);                                                    \
  }

#define QCE_UNIT_TEST_SMT_Z3_RUN(name) qce_unit_test_smt_z3_##name()

// individual test cases
QCE_UNIT_TEST_SMT_Z3_PROLOGUE(basics)
QCE_UNIT_TEST_SMT_Z3_EPILOGUE

// collector
static inline void qce_unit_test_smt_z3(void) {
  QCE_UNIT_TEST_SMT_Z3_RUN(basics);
}
#endif
