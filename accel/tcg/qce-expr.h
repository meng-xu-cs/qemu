#ifndef QCE_EXPR_H
#define QCE_EXPR_H

// dual-mode representation of an expression
typedef struct {
  enum {
    QCE_EXPR_CONCRETE,
    QCE_EXPR_SYMBOLIC,
  } mode;
  enum {
    QCE_EXPR_I32,
    QCE_EXPR_I64,
  } type;
  union {
    int32_t v_i32;
    int64_t v_i64;
    Z3_ast symbolic;
  };
} QCEExpr;

#define qce_expr_assert_type(expr, ty)                                         \
  if (expr->type != QCE_EXPR_##ty) {                                           \
    qce_fatal("[expr] type mismatch: expect " #ty ", actual %d", expr->type);  \
  }

// dual-mode representation of a predicate
typedef struct {
  enum {
    QCE_PRED_CONCRETE,
    QCE_PRED_SYMBOLIC,
  } mode;
  union {
    bool concrete;
    Z3_ast symbolic;
  };
} QCEPred;

/*
 * Initialization
 */

static inline void qce_expr_init_v32(QCEExpr *expr, int32_t val) {
  expr->mode = QCE_EXPR_CONCRETE;
  expr->type = QCE_EXPR_I32;
  expr->v_i32 = val;
}

#ifndef QCE_RELEASE
static inline void qce_expr_init_s32(SolverZ3 *solver, QCEExpr *expr) {
  expr->mode = QCE_EXPR_SYMBOLIC;
  expr->type = QCE_EXPR_I32;
  expr->symbolic = qce_smt_z3_bv32_var(solver);
}
#endif

static inline void qce_expr_init_v64(QCEExpr *expr, int64_t val) {
  expr->mode = QCE_EXPR_CONCRETE;
  expr->type = QCE_EXPR_I64;
  expr->v_i64 = val;
}

#ifndef QCE_RELEASE
static inline void qce_expr_init_s64(SolverZ3 *solver, QCEExpr *expr) {
  expr->mode = QCE_EXPR_SYMBOLIC;
  expr->type = QCE_EXPR_I64;
  expr->symbolic = qce_smt_z3_bv64_var(solver);
}
#endif

/*
 * Testing
 */

#ifndef QCE_RELEASE
// helper macro
#define QCE_UNIT_TEST_EXPR_PROLOGUE(name)                                      \
  static inline void qce_unit_test_expr_##name(void) {                         \
    qce_debug("[test][expr] " #name);                                          \
    SolverZ3 solver;                                                           \
    qce_smt_z3_init(&solver);

#define QCE_UNIT_TEST_EXPR_EPILOGUE                                            \
  qce_smt_z3_fini(&solver);                                                    \
  }

#define QCE_UNIT_TEST_EXPR_RUN(name) qce_unit_test_expr_##name()

#define QCE_UNIT_TEST_EXPR_DEF_DUAL(name)                                      \
  QCE_UNIT_TEST_EXPR_##name(32);                                               \
  QCE_UNIT_TEST_EXPR_##name(64);

#define QCE_UNIT_TEST_EXPR_RUN_DUAL(name)                                      \
  qce_unit_test_expr_##name##_i32();                                           \
  qce_unit_test_expr_##name##_i64();

// individual test cases
QCE_UNIT_TEST_EXPR_PROLOGUE(basics)
QCE_UNIT_TEST_EXPR_EPILOGUE
#endif

/*
 * Operations
 */

#include "qce-expr-bin-op.h"
#include "qce-expr-cmp-op.h"
#include "qce-expr-uni-op.h"

#include "qce-expr-ld-st.h"

#ifndef QCE_RELEASE
static inline void qce_unit_test_expr(void) {
  QCE_UNIT_TEST_EXPR_RUN(basics);

  QCE_UNIT_TEST_EXPR_RUN_DUAL(add);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(sub);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(mul);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(div);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(add2);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(sub2);
  // QCE_UNIT_TEST_EXPR_RUN_DUAL(mulu2);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(muls2);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(special_a_add_then_sub);

  QCE_UNIT_TEST_EXPR_RUN_DUAL(eq);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ne);

  QCE_UNIT_TEST_EXPR_RUN_DUAL(slt);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(sle);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(sge);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(sgt);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ult);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ule);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(uge);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ugt);

  QCE_UNIT_TEST_EXPR_RUN_DUAL(bvand);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(bvor);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(bvxor);
  // QCE_UNIT_TEST_EXPR_RUN_DUAL(bvandc);
  // QCE_UNIT_TEST_EXPR_RUN_DUAL(bvorc);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(bvnand);
  // QCE_UNIT_TEST_EXPR_RUN_DUAL(bvnor);
  // QCE_UNIT_TEST_EXPR_RUN_DUAL(eqv);

  QCE_UNIT_TEST_EXPR_RUN_DUAL(st8);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(st8_symbolic);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(st16);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(st16_symbolic);
  QCE_UNIT_TEST_EXPR_RUN(st32_i64);
  QCE_UNIT_TEST_EXPR_RUN(st32_symbolic_i64);

  QCE_UNIT_TEST_EXPR_RUN_DUAL(ld8u_common);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ld8u);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ld8s_common);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ld8s);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ld16u_common);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ld16u);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ld16s_common);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(ld16s);
  QCE_UNIT_TEST_EXPR_RUN(ld32u_common_i64);
  QCE_UNIT_TEST_EXPR_RUN(ld32u_i64);
  QCE_UNIT_TEST_EXPR_RUN(ld32s_common_i64);
  QCE_UNIT_TEST_EXPR_RUN(ld32s_i64);

  QCE_UNIT_TEST_EXPR_RUN_DUAL(special_ld_then_st8u);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(special_ld_then_st8s);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(special_ld_then_st16u);
  QCE_UNIT_TEST_EXPR_RUN_DUAL(special_ld_then_st16s);
  QCE_UNIT_TEST_EXPR_RUN(special_ld_then_st32u_i64);
  QCE_UNIT_TEST_EXPR_RUN(special_ld_then_st32s_i64);
}
#endif

#endif /* QCE_EXPR_H */