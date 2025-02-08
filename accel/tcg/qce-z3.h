#ifndef QCE_Z3_H
#define QCE_Z3_H

#include "z3.h"

#define BLOB_SIZE_MAX 4096

// context
typedef struct {
  Z3_context ctx;
  Z3_solver sol;

  // variables
  Z3_ast blob_addr;
  Z3_ast blob_size;
  Z3_ast blob_content;

  // utilities
  Z3_sort sort_bv8;
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
 * Context lifecycle
 */

static inline void qce_smt_z3_init(SolverZ3 *solver) {
  // initialize the context
  Z3_config cfg = Z3_mk_config();
  Z3_context ctx = Z3_mk_context(cfg);
  Z3_set_error_handler(ctx, qce_error_handler_z3);
  Z3_del_config(cfg);

  // create the solver
  Z3_solver sol = Z3_mk_solver(ctx);
  Z3_solver_inc_ref(ctx, sol);

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

  // constraints on the pre-defined inputs
  // - blob_addr != NULL
  Z3_solver_assert(
      ctx, sol,
      Z3_mk_distinct(ctx, 2,
                     (Z3_ast[]){blob_addr, Z3_mk_int(ctx, 0, sort_bv64)}));
  // - 0 < blob_size <= BLOB_SIZE_MAX
  Z3_solver_assert(ctx, sol,
                   Z3_mk_bvugt(ctx, blob_size, Z3_mk_int(ctx, 0, sort_bv64)));
  Z3_solver_assert(
      ctx, sol,
      Z3_mk_bvule(ctx, blob_size, Z3_mk_int(ctx, BLOB_SIZE_MAX, sort_bv64)));

  // assignment
  solver->ctx = ctx;
  solver->sol = sol;
  solver->blob_addr = blob_addr;
  solver->blob_size = blob_size;
  solver->blob_content = blob_content;
  solver->sort_bv8 = sort_bv8;
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
                                         uint32_t *val) {
  __qce_smt_z3_type_check_bv32(solver, expr);

  Z3_ast_kind kind = Z3_get_ast_kind(solver->ctx, expr);
  switch (kind) {
  case Z3_NUMERAL_AST: {
#ifndef QCE_RELEASE
    bool probed =
#endif
        Z3_get_numeral_uint(solver->ctx, expr, val);
#ifndef QCE_RELEASE
    assert(probed);
#endif
    return true;
  }
  case Z3_APP_AST: {
    Z3_ast evaluated;
    if (__qce_smt_z3_simplify_reduce(solver, expr, &evaluated)) {
#ifndef QCE_RELEASE
      assert(Z3_get_ast_kind(solver->ctx, evaluated) == Z3_NUMERAL_AST);
#endif
#ifndef QCE_RELEASE
      bool probed =
#endif
          Z3_get_numeral_uint(solver->ctx, evaluated, val);
#ifndef QCE_RELEASE
      assert(probed);
#endif
      return true;
    }
    return false;
  }
  default:
    qce_fatal("unexpected Z3 ast kind for bv32: %d", kind);
  }
}

static inline bool qce_smt_z3_probe_bv64(SolverZ3 *solver, Z3_ast expr,
                                         uint64_t *val) {
  __qce_smt_z3_type_check_bv64(solver, expr);

  Z3_ast_kind kind = Z3_get_ast_kind(solver->ctx, expr);
  switch (kind) {
  case Z3_NUMERAL_AST: {
#ifndef QCE_RELEASE
    bool probed =
#endif
        Z3_get_numeral_uint64(solver->ctx, expr, val);
#ifndef QCE_RELEASE
    assert(probed);
#endif
    return true;
  }
  case Z3_APP_AST: {
    Z3_ast evaluated;
    if (__qce_smt_z3_simplify_reduce(solver, expr, &evaluated)) {
#ifndef QCE_RELEASE
      assert(Z3_get_ast_kind(solver->ctx, evaluated) == Z3_NUMERAL_AST);
#endif
#ifndef QCE_RELEASE
      bool probed =
#endif
          Z3_get_numeral_uint64(solver->ctx, expr, val);
#ifndef QCE_RELEASE
      assert(probed);
#endif
      return true;
    }
    return false;
  }
  default:
    qce_fatal("unexpected Z3 ast kind for bv32: %d", kind);
  }
}

/*
 * Concretize
 */

static inline bool qce_smt_z3_concretize_bool(SolverZ3 *solver,
                                              target_ulong addr,
                                              target_ulong size, uint8_t *blob,
                                              Z3_ast pred) {
  // build the clauses
  Z3_ast *clauses = g_alloca(sizeof(Z3_ast) * (size + 3));
  for (target_ulong i = 0; i < size; i++) {
    clauses[i] =
        Z3_mk_eq(solver->ctx, Z3_mk_int(solver->ctx, blob[i], solver->sort_bv8),
                 Z3_mk_select(solver->ctx, solver->blob_content,
                              Z3_mk_int64(solver->ctx, i, solver->sort_bv64)));
  }
  clauses[size + 0] =
      Z3_mk_eq(solver->ctx, solver->blob_addr,
               Z3_mk_int64(solver->ctx, addr, solver->sort_bv64));
  clauses[size + 1] =
      Z3_mk_eq(solver->ctx, solver->blob_size,
               Z3_mk_int64(solver->ctx, size, solver->sort_bv64));
  clauses[size + 2] = pred;

  // check the assumptions
  Z3_lbool result =
      Z3_solver_check_assumptions(solver->ctx, solver->sol, size + 3, clauses);
  if (result == Z3_L_UNDEF) {
    qce_fatal("unable to determine the satisfiability of concretization");
  }

  // positive case
  if (result == Z3_L_TRUE) {
#ifndef QCE_RELEASE
    // evaluate the predicate
    Z3_ast eval = NULL;
    Z3_model model = Z3_solver_get_model(solver->ctx, solver->sol);
    if (!Z3_model_eval(solver->ctx, model, pred, true, &eval)) {
      qce_fatal("model evaluation failed");
    }

    bool concrete = false;
    if (!qce_smt_z3_probe_bool(solver, eval, &concrete)) {
      qce_fatal("unable to probe the bool out of concretization");
    }
    if (!concrete) {
      qce_fatal("probed bool does not match with concretization");
    }
#endif
    return true;
  }

  // negative case
#ifndef QCE_RELEASE
  assert(result == Z3_L_FALSE);

  // also try the negation
  clauses[size + 2] = Z3_mk_not(solver->ctx, pred);
  result =
      Z3_solver_check_assumptions(solver->ctx, solver->sol, size + 3, clauses);

  if (result == Z3_L_UNDEF) {
    qce_fatal(
        "unable to determine the satisfiability of negated concretization");
  }
  if (result == Z3_L_FALSE) {
    qce_fatal("unsat for both sides of concretization");
  }
  assert(result == Z3_L_TRUE);
#endif
  return false;
}

static inline size_t qce_smt_z3_solve_for(SolverZ3 *solver, Z3_ast cond,
                                          uint8_t *output) {
  // solve for model
  Z3_lbool result = Z3_solver_check_assumptions(solver->ctx, solver->sol, 1,
                                                (Z3_ast[]){cond});
  if (result != Z3_L_TRUE) {
    qce_fatal("expect SAT for an already-concretized condition");
  }
  Z3_model model = Z3_solver_get_model(solver->ctx, solver->sol);

  // evaluate blob size
  Z3_ast eval_size = NULL;
  if (!Z3_model_eval(solver->ctx, model, solver->blob_size, true, &eval_size)) {
    qce_fatal("model evaluation on blob_size failed");
  }

  uint64_t output_size = 0;
#ifndef QCE_RELEASE
  g_assert(Z3_get_ast_kind(solver->ctx, eval_size) == Z3_NUMERAL_AST);
  bool probed_size =
#endif
      Z3_get_numeral_uint64(solver->ctx, eval_size, &output_size);
#ifndef QCE_RELEASE
  g_assert(probed_size);
  g_assert(output_size != 0 && output_size <= BLOB_SIZE_MAX);
#endif

  // evaluate each blob cell
  for (size_t i = 0; i < output_size; i++) {
    Z3_ast cell = Z3_mk_select(solver->ctx, solver->blob_content,
                               Z3_mk_int64(solver->ctx, i, solver->sort_bv64));

    Z3_ast eval_cell = NULL;
    if (!Z3_model_eval(solver->ctx, model, cell, true, &eval_cell)) {
      qce_fatal("model evaluation on blob_content[%ld] failed", i);
    }

    uint64_t output_cell = 0;
#ifndef QCE_RELEASE
    g_assert(Z3_get_ast_kind(solver->ctx, eval_cell) == Z3_NUMERAL_AST);
    bool probed_cell =
#endif
        Z3_get_numeral_uint64(solver->ctx, eval_cell, &output_cell);
#ifndef QCE_RELEASE
    g_assert(probed_cell);
    g_assert(output_cell <= UINT8_MAX);
#endif
    output[i] = output_cell;
  }

  return output_size;
}

static void qce_Z3_mk_bvadd2(Z3_context ctx,
                             Z3_ast t1_low, Z3_ast t1_high,
                             Z3_ast t2_low, Z3_ast t2_high,
                             Z3_ast *t0_low, Z3_ast *t0_high) {
  unsigned int nbits = Z3_get_bv_sort_size(ctx, Z3_get_sort(ctx, t1_low));

  Z3_ast t1 = Z3_mk_concat(ctx, t1_high, t1_low);
  Z3_ast t2 = Z3_mk_concat(ctx, t2_high, t2_low);
  Z3_ast t0 = Z3_mk_bvadd(ctx, t1, t2);

  *t0_low = Z3_mk_extract(ctx, nbits-1, 0, t0);
  *t0_high = Z3_mk_extract(ctx, 2*nbits-1, nbits, t0);
}

static void qce_Z3_mk_bvsub2(Z3_context ctx,
                             Z3_ast t1_low, Z3_ast t1_high,
                             Z3_ast t2_low, Z3_ast t2_high,
                             Z3_ast *t0_low, Z3_ast *t0_high) {
  unsigned int nbits = Z3_get_bv_sort_size(ctx, Z3_get_sort(ctx, t1_low));

  Z3_ast t1 = Z3_mk_concat(ctx, t1_high, t1_low);
  Z3_ast t2 = Z3_mk_concat(ctx, t2_high, t2_low);
  Z3_ast t0 = Z3_mk_bvsub(ctx, t1, t2);

  *t0_low = Z3_mk_extract(ctx, nbits-1, 0, t0);
  *t0_high = Z3_mk_extract(ctx, 2*nbits-1, nbits, t0);
}

static void qce_Z3_mk_bvmuls2(Z3_context ctx, Z3_ast t1, Z3_ast t2,
                              Z3_ast *t0_low, Z3_ast *t0_high) {
  unsigned int nbits = Z3_get_bv_sort_size(ctx, Z3_get_sort(ctx, t1));

  t1 = Z3_mk_sign_ext(ctx, nbits, t1);
  t2 = Z3_mk_sign_ext(ctx, nbits, t2);

  Z3_ast result = Z3_mk_bvmul(ctx, t1, t2);

  *t0_low = Z3_mk_extract(ctx, nbits-1, 0, result);
  *t0_high = Z3_mk_extract(ctx, 2*nbits-1, nbits, result);
}

// static void qce_Z3_mk_bvmulu2(Z3_context ctx, Z3_ast t1, Z3_ast t2, Z3_ast t0[]) {
//   unsigned int nbits = Z3_get_bv_sort_size(ctx, Z3_get_sort(ctx, t1));

//   t1 = Z3_mk_sign_ext(ctx, nbits, t1);
//   t2 = Z3_mk_sign_ext(ctx, nbits, t2);

//   Z3_ast result = Z3_mk_bvmul(ctx, t1, t2);

//   t0[0] = Z3_mk_extract(ctx, nbits-1, 0, result);
//   t0[1] = Z3_mk_extract(ctx, 2*nbits-1, nbits, result);
// }

static Z3_ast qce_Z3_mk_bvandc(Z3_context ctx, Z3_ast t1, Z3_ast t2) {
  return Z3_mk_bvand(ctx, t1, Z3_mk_bvnot(ctx, t2));
}

static Z3_ast qce_Z3_mk_bvorc(Z3_context ctx, Z3_ast t1, Z3_ast t2) {
  return Z3_mk_bvor(ctx, t1, Z3_mk_bvnot(ctx, t2));
}

static Z3_ast qce_Z3_mk_bveqv(Z3_context ctx, Z3_ast t1, Z3_ast t2) {
  return Z3_mk_bvxor(ctx, t1, Z3_mk_bvnot(ctx, t2));
}

/*
 * Template
 */

#define DEFINE_SMT_Z3_OP1(bits, name, func)                                    \
  static inline Z3_ast qce_smt_z3_bv##bits##_##name(SolverZ3 *solver,          \
                                                    Z3_ast opv) {              \
    __qce_smt_z3_type_check_bv##bits(solver, opv);                             \
    return __qce_smt_z3_simplify(solver, func(solver->ctx, opv));              \
  }

#define DEFINE_SMT_Z3_OP1_DUAL(name, func)                                     \
  DEFINE_SMT_Z3_OP1(32, name, func)                                            \
  DEFINE_SMT_Z3_OP1(64, name, func)

#define DEFINE_SMT_Z3_OP1_EX(bits, name, func)                                 \
  static inline Z3_ast qce_smt_z3_bv##bits##_##name(SolverZ3 *solver,          \
                                                    Z3_ast opv) {              \
    __qce_smt_z3_type_check_bv##bits(solver, opv);                             \
    return __qce_smt_z3_simplify(solver,                                       \
                                 func(solver->ctx, 1, (Z3_ast[]){lhs}));       \
  }

#define DEFINE_SMT_Z3_OP1_DUAL_EX(name, func)                                  \
  DEFINE_SMT_Z3_OP1_EX(32, name, func)                                         \
  DEFINE_SMT_Z3_OP1_EX(64, name, func)

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

#define DEFINE_SMT_Z3_OP2_mul2(bits, name, func)                               \
  static inline void qce_smt_z3_bv##bits##_##name(SolverZ3 *solver,            \
                                                  Z3_ast lhs, Z3_ast rhs,      \
                                                  Z3_ast *res_low,             \
                                                  Z3_ast *res_high) {          \
    __qce_smt_z3_type_check_bv##bits(solver, lhs);                             \
    __qce_smt_z3_type_check_bv##bits(solver, rhs);                             \
    func(solver->ctx, lhs, rhs, res_low, res_high);                            \
    *res_low = __qce_smt_z3_simplify(solver, *res_low);                        \
    *res_high = __qce_smt_z3_simplify(solver, *res_high);                      \
  }

#define DEFINE_SMT_Z3_OP2_mul2_DUAL(name, func)                                \
  DEFINE_SMT_Z3_OP2_mul2(32, name, func)                                       \
  DEFINE_SMT_Z3_OP2_mul2(64, name, func)

#define DEFINE_SMT_Z3_OP4(bits, name, func)                                    \
  static inline void qce_smt_z3_bv##bits##_##name(SolverZ3 *solver,            \
      Z3_ast lhs_low, Z3_ast lhs_high, Z3_ast rhs_low, Z3_ast rhs_high,        \
      Z3_ast *res_low, Z3_ast *res_high) {                                     \
    __qce_smt_z3_type_check_bv##bits(solver, lhs_low);                         \
    __qce_smt_z3_type_check_bv##bits(solver, lhs_high);                        \
    __qce_smt_z3_type_check_bv##bits(solver, rhs_low);                         \
    __qce_smt_z3_type_check_bv##bits(solver, rhs_high);                        \
    func(solver->ctx, lhs_low, lhs_high, rhs_low, rhs_high, res_low, res_high);\
    *res_low = __qce_smt_z3_simplify(solver, *res_low);                        \
    *res_high = __qce_smt_z3_simplify(solver, *res_high);                      \
  }

#define DEFINE_SMT_Z3_OP4_DUAL(name, func)                                     \
  DEFINE_SMT_Z3_OP4(32, name, func)                                            \
  DEFINE_SMT_Z3_OP4(64, name, func)

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

#define DEFINE_SMT_Z3_OP2_st(bits, n)                                          \
  static inline Z3_ast qce_smt_z3_bv##bits##_st##n(SolverZ3 *solver,           \
                                                   Z3_ast src, Z3_ast dst) {   \
    __qce_smt_z3_type_check_bv##bits(solver, src);                             \
    __qce_smt_z3_type_check_bv##bits(solver, dst);                             \
    Z3_ast l = Z3_mk_extract(solver->ctx, n - 1, 0, src);                      \
    Z3_ast h = Z3_mk_extract(solver->ctx, bits - 1, n, dst);                   \
    return __qce_smt_z3_simplify(solver, Z3_mk_concat(solver->ctx, h, l));     \
  }

DEFINE_SMT_Z3_OP2_st(32, 8);
DEFINE_SMT_Z3_OP2_st(32, 16);
DEFINE_SMT_Z3_OP2_st(64, 8);
DEFINE_SMT_Z3_OP2_st(64, 16);
DEFINE_SMT_Z3_OP2_st(64, 32);

#define DEFINE_SMT_Z3_OP1_ld_u(bits, n)                                        \
  static inline Z3_ast qce_smt_z3_bv##bits##_ld##n##u(SolverZ3 *solver,        \
                                                      Z3_ast src) {            \
    __qce_smt_z3_type_check_bv##bits(solver, src);                             \
    Z3_ast part = Z3_mk_extract(solver->ctx, n - 1, 0, src);                   \
    return __qce_smt_z3_simplify(solver,                                       \
                                 Z3_mk_zero_ext(solver->ctx, bits - n, part)); \
  }

#define DEFINE_SMT_Z3_OP1_ld_s(bits, n)                                        \
  static inline Z3_ast qce_smt_z3_bv##bits##_ld##n##s(SolverZ3 *solver,        \
                                                      Z3_ast src) {            \
    __qce_smt_z3_type_check_bv##bits(solver, src);                             \
    Z3_ast part = Z3_mk_extract(solver->ctx, n - 1, 0, src);                   \
    return __qce_smt_z3_simplify(solver,                                       \
                                 Z3_mk_sign_ext(solver->ctx, bits - n, part)); \
  }

DEFINE_SMT_Z3_OP1_ld_u(32, 8);
DEFINE_SMT_Z3_OP1_ld_s(32, 8);
DEFINE_SMT_Z3_OP1_ld_u(32, 16);
DEFINE_SMT_Z3_OP1_ld_s(32, 16);
DEFINE_SMT_Z3_OP1_ld_u(64, 8);
DEFINE_SMT_Z3_OP1_ld_s(64, 8);
DEFINE_SMT_Z3_OP1_ld_u(64, 16);
DEFINE_SMT_Z3_OP1_ld_s(64, 16);
DEFINE_SMT_Z3_OP1_ld_u(64, 32);
DEFINE_SMT_Z3_OP1_ld_s(64, 32);

/*
 * Arithmetics
 */

DEFINE_SMT_Z3_OP2_DUAL(add, Z3_mk_bvadd)
DEFINE_SMT_Z3_OP2_DUAL(sub, Z3_mk_bvsub)
DEFINE_SMT_Z3_OP2_DUAL(mul, Z3_mk_bvmul)
DEFINE_SMT_Z3_OP2_DUAL(div, Z3_mk_bvsdiv)
DEFINE_SMT_Z3_OP2_DUAL(smod, Z3_mk_bvsrem)
DEFINE_SMT_Z3_OP2_DUAL(umod, Z3_mk_bvurem)

DEFINE_SMT_Z3_OP4_DUAL(add2, qce_Z3_mk_bvadd2)
DEFINE_SMT_Z3_OP4_DUAL(sub2, qce_Z3_mk_bvsub2)
DEFINE_SMT_Z3_OP2_mul2_DUAL(muls2, qce_Z3_mk_bvmuls2)

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

DEFINE_SMT_Z3_OP1_DUAL(bvnot, Z3_mk_bvnot)

DEFINE_SMT_Z3_OP2_DUAL(bvand, Z3_mk_bvand)
DEFINE_SMT_Z3_OP2_DUAL(bvor, Z3_mk_bvor)
DEFINE_SMT_Z3_OP2_DUAL(bvxor, Z3_mk_bvxor)

DEFINE_SMT_Z3_OP2_DUAL(bvandc, qce_Z3_mk_bvandc)
DEFINE_SMT_Z3_OP2_DUAL(bvorc, qce_Z3_mk_bvorc)
DEFINE_SMT_Z3_OP2_DUAL(bvnand, Z3_mk_bvnand)
DEFINE_SMT_Z3_OP2_DUAL(bvnor, Z3_mk_bvnor)
DEFINE_SMT_Z3_OP2_DUAL(bveqv, qce_Z3_mk_bveqv)

/*
 * Array
 */

#define __STMT_SMT_BLOB_LOAD_AND_CONCAT(idx)                                   \
  result = Z3_mk_concat(                                                       \
      solver->ctx,                                                             \
      Z3_mk_select(solver->ctx, solver->blob_content,                          \
                   qce_smt_z3_bv64_add(solver, offset,                         \
                                       qce_smt_z3_bv64_value(solver, idx))),   \
      result)

static inline Z3_ast qce_smt_z3_blob_ld32(SolverZ3 *solver, Z3_ast addr) {
  Z3_ast offset = qce_smt_z3_bv64_sub(solver, addr, solver->blob_addr);
  Z3_ast result = Z3_mk_select(solver->ctx, solver->blob_content, offset);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(1);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(2);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(3);
  return __qce_smt_z3_simplify(solver, result);
}

static inline Z3_ast qce_smt_z3_blob_ld64(SolverZ3 *solver, Z3_ast addr) {
  Z3_ast offset = qce_smt_z3_bv64_sub(solver, addr, solver->blob_addr);
  Z3_ast result = Z3_mk_select(solver->ctx, solver->blob_content, offset);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(1);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(2);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(3);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(4);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(5);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(6);
  __STMT_SMT_BLOB_LOAD_AND_CONCAT(7);
  return __qce_smt_z3_simplify(solver, result);
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

#endif /* QCE_Z3_H */