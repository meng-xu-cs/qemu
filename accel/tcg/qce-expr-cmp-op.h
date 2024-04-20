#ifndef QCE_EXPR_CMP_OP_H
#define QCE_EXPR_CMP_OP_H

/*
 * Utilities
 */

#define DEFINE_CONCRETE_CMP_OP_SIGNED(bits, name, op)                          \
  static inline bool __qce_concrete_bv##bits##_##name(int##bits##_t lhs,       \
                                                      int##bits##_t rhs) {     \
    return lhs op rhs;                                                         \
  }

#define DEFINE_CONCRETE_CMP_OP_SIGNED_DUAL(name, op)                           \
  DEFINE_CONCRETE_CMP_OP_SIGNED(32, name, op)                                  \
  DEFINE_CONCRETE_CMP_OP_SIGNED(64, name, op)

#define DEFINE_CONCRETE_CMP_OP_UNSIGNED(bits, name, op)                        \
  static inline bool __qce_concrete_bv##bits##_##name(uint##bits##_t lhs,      \
                                                      uint##bits##_t rhs) {    \
    return lhs op rhs;                                                         \
  }

#define DEFINE_CONCRETE_CMP_OP_UNSIGNED_DUAL(name, op)                         \
  DEFINE_CONCRETE_CMP_OP_UNSIGNED(32, name, op)                                \
  DEFINE_CONCRETE_CMP_OP_UNSIGNED(64, name, op)

DEFINE_CONCRETE_CMP_OP_SIGNED_DUAL(eq, ==)
DEFINE_CONCRETE_CMP_OP_SIGNED_DUAL(ne, !=)

DEFINE_CONCRETE_CMP_OP_SIGNED_DUAL(slt, <)
DEFINE_CONCRETE_CMP_OP_SIGNED_DUAL(sle, <=)
DEFINE_CONCRETE_CMP_OP_SIGNED_DUAL(sge, >=)
DEFINE_CONCRETE_CMP_OP_SIGNED_DUAL(sgt, >)

DEFINE_CONCRETE_CMP_OP_UNSIGNED_DUAL(ult, <)
DEFINE_CONCRETE_CMP_OP_UNSIGNED_DUAL(ule, <=)
DEFINE_CONCRETE_CMP_OP_UNSIGNED_DUAL(uge, >=)
DEFINE_CONCRETE_CMP_OP_UNSIGNED_DUAL(ugt, >)

/*
 * Templates
 */

#define DEFINE_EXPR_CMP_OP(bits, name)                                         \
  static inline void qce_expr_##name##_i##bits(                                \
      SolverZ3 *solver, QCEExpr *lhs, QCEExpr *rhs, QCEPred *result) {         \
    /* type checking */                                                        \
    qce_expr_assert_type(lhs, I##bits);                                        \
    qce_expr_assert_type(rhs, I##bits);                                        \
                                                                               \
    /* base assignment */                                                      \
    if (lhs->mode == QCE_EXPR_CONCRETE) {                                      \
      if (rhs->mode == QCE_EXPR_CONCRETE) {                                    \
        result->mode = QCE_PRED_CONCRETE;                                      \
        result->concrete =                                                     \
            __qce_concrete_bv##bits##_##name(lhs->v_i##bits, rhs->v_i##bits);  \
      } else {                                                                 \
        result->mode = QCE_PRED_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_##name(                       \
            solver, qce_smt_z3_bv##bits##_value(solver, lhs->v_i##bits),       \
            rhs->symbolic);                                                    \
      }                                                                        \
    } else {                                                                   \
      if (rhs->mode == QCE_EXPR_CONCRETE) {                                    \
        result->mode = QCE_PRED_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_##name(                       \
            solver, lhs->symbolic,                                             \
            qce_smt_z3_bv##bits##_value(solver, rhs->v_i##bits));              \
      } else {                                                                 \
        result->mode = QCE_PRED_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_##name(solver, lhs->symbolic, \
                                                        rhs->symbolic);        \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* try to reduce symbolic to concrete */                                   \
    if (result->mode == QCE_PRED_SYMBOLIC) {                                   \
      bool val = false;                                                        \
      if (qce_smt_z3_probe_bool(solver, result->symbolic, &val)) {             \
        result->mode = QCE_PRED_CONCRETE;                                      \
        result->concrete = val;                                                \
      }                                                                        \
    }                                                                          \
  }

#define DEFINE_EXPR_CMP_OP_DUAL(name)                                          \
  DEFINE_EXPR_CMP_OP(32, name)                                                 \
  DEFINE_EXPR_CMP_OP(64, name)

/*
 * Comparison
 */

DEFINE_EXPR_CMP_OP_DUAL(eq)
DEFINE_EXPR_CMP_OP_DUAL(ne)

DEFINE_EXPR_CMP_OP_DUAL(slt)
DEFINE_EXPR_CMP_OP_DUAL(sle)
DEFINE_EXPR_CMP_OP_DUAL(sge)
DEFINE_EXPR_CMP_OP_DUAL(sgt)

DEFINE_EXPR_CMP_OP_DUAL(ult)
DEFINE_EXPR_CMP_OP_DUAL(ule)
DEFINE_EXPR_CMP_OP_DUAL(uge)
DEFINE_EXPR_CMP_OP_DUAL(ugt)

/*
 * Testing
 */

#ifndef QCE_RELEASE
#define QCE_UNIT_TEST_EXPR_eq(bits)                                            \
  QCE_UNIT_TEST_EXPR_PROLOGUE(eq_i##bits) {                                    \
    /* 1 == 1 is true */                                                       \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_eq_i##bits(&solver, &v1, &v1, &r);                                \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    /* 1 == -1 is false */                                                     \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_eq_i##bits(&solver, &v1, &v1m, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 == x <=> x == 0 */                                                    \
    QCEPred r1;                                                                \
    qce_expr_eq_i##bits(&solver, &v0, &x, &r1);                                \
    assert(r1.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r1.symbolic,                                           \
               qce_smt_z3_bv##bits##_eq(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
                                                                               \
    /* x == x is true */                                                       \
    QCEPred r2;                                                                \
    qce_expr_eq_i##bits(&solver, &x, &x, &r2);                                 \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(r2.concrete);                                                       \
                                                                               \
    /* x == y <==> y == x */                                                   \
    QCEPred r3;                                                                \
    qce_expr_eq_i##bits(&solver, &x, &y, &r3);                                 \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_eq(&solver, y.symbolic, x.symbolic)) ==   \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(eq)

#define QCE_UNIT_TEST_EXPR_ne(bits)                                            \
  QCE_UNIT_TEST_EXPR_PROLOGUE(ne_i##bits) {                                    \
    /* 1 != 1 is false */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_ne_i##bits(&solver, &v1, &v1, &r);                                \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    /* 1 != -1 is true */                                                      \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_ne_i##bits(&solver, &v1, &v1m, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 != x <=> x != 0 */                                                    \
    QCEPred r1;                                                                \
    qce_expr_ne_i##bits(&solver, &v0, &x, &r1);                                \
    assert(r1.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r1.symbolic,                                           \
               qce_smt_z3_bv##bits##_ne(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
                                                                               \
    /* x != x is false */                                                      \
    QCEPred r2;                                                                \
    qce_expr_ne_i##bits(&solver, &x, &x, &r2);                                 \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r2.concrete);                                                      \
                                                                               \
    /* x != y <==> y != x */                                                   \
    QCEPred r3;                                                                \
    qce_expr_ne_i##bits(&solver, &x, &y, &r3);                                 \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_ne(&solver, y.symbolic, x.symbolic)) ==   \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(ne)

#define QCE_UNIT_TEST_EXPR_slt(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(slt_i##bits) {                                   \
    /* 1 <s 1 is false */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_slt_i##bits(&solver, &v1, &v1, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    /* 1 <s -1 is false */                                                     \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_slt_i##bits(&solver, &v1, &v1m, &r);                              \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 <s x <=> x >s 0 */                                                    \
    QCEPred r1;                                                                \
    qce_expr_slt_i##bits(&solver, &v0, &x, &r1);                               \
    assert(r1.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r1.symbolic,                                           \
               qce_smt_z3_bv##bits##_sgt(                                      \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
                                                                               \
    /* x <s x is false */                                                      \
    QCEPred r2;                                                                \
    qce_expr_slt_i##bits(&solver, &x, &x, &r2);                                \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r2.concrete);                                                      \
                                                                               \
    /* x <s y <==> y >s x */                                                   \
    QCEPred r3;                                                                \
    qce_expr_slt_i##bits(&solver, &x, &y, &r3);                                \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_sgt(&solver, y.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    int##bits##_t min = ((int##bits##_t)1) << (bits - 1);                      \
    int##bits##_t max = min - 1;                                               \
    QCEExpr vmin, vmax, x;                                                     \
    qce_expr_init_v##bits(&vmin, min);                                         \
    qce_expr_init_v##bits(&vmax, max);                                         \
    qce_expr_init_s##bits(&solver, &x);                                        \
                                                                               \
    /* x <s min is false */                                                    \
    QCEPred r1;                                                                \
    qce_expr_slt_i##bits(&solver, &x, &vmin, &r1);                             \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r1.concrete);                                                      \
                                                                               \
    /* x <s max <=> x != max */                                                \
    QCEPred r2;                                                                \
    qce_expr_slt_i##bits(&solver, &x, &vmax, &r2);                             \
    assert(r2.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r2.symbolic,                                           \
               qce_smt_z3_bv##bits##_ne(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, max))) ==              \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(slt)

#define QCE_UNIT_TEST_EXPR_sle(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(sle_i##bits) {                                   \
    /* 1 <=s 1 is true */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_sle_i##bits(&solver, &v1, &v1, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    /* 1 <=s -1 is false */                                                    \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_sle_i##bits(&solver, &v1, &v1m, &r);                              \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 <=s x <=> x >=s 0 */                                                  \
    QCEPred r1;                                                                \
    qce_expr_sle_i##bits(&solver, &v0, &x, &r1);                               \
    assert(r1.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r1.symbolic,                                           \
               qce_smt_z3_bv##bits##_sge(                                      \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
                                                                               \
    /* x <=s x is true */                                                      \
    QCEPred r2;                                                                \
    qce_expr_sle_i##bits(&solver, &x, &x, &r2);                                \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(r2.concrete);                                                       \
                                                                               \
    /* x <=s y <==> y >=s x */                                                 \
    QCEPred r3;                                                                \
    qce_expr_sle_i##bits(&solver, &x, &y, &r3);                                \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_sge(&solver, y.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    int##bits##_t min = ((int##bits##_t)1) << (bits - 1);                      \
    int##bits##_t max = min - 1;                                               \
    QCEExpr vmin, vmax, x;                                                     \
    qce_expr_init_v##bits(&vmin, min);                                         \
    qce_expr_init_v##bits(&vmax, max);                                         \
    qce_expr_init_s##bits(&solver, &x);                                        \
                                                                               \
    /* x <=s max is true */                                                    \
    QCEPred r1;                                                                \
    qce_expr_sle_i##bits(&solver, &x, &vmax, &r1);                             \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(r1.concrete);                                                       \
                                                                               \
    /* x <=s min <=> x == min */                                               \
    QCEPred r2;                                                                \
    qce_expr_sle_i##bits(&solver, &x, &vmin, &r2);                             \
    assert(r2.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r2.symbolic,                                           \
               qce_smt_z3_bv##bits##_eq(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, min))) ==              \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(sle)

#define QCE_UNIT_TEST_EXPR_sge(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(sge_i##bits) {                                   \
    /* 1 >=s 1 is true */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_sge_i##bits(&solver, &v1, &v1, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    /* 1 >=s -1 is true */                                                     \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_sge_i##bits(&solver, &v1, &v1m, &r);                              \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 >=s x <=> x <=s 0 */                                                  \
    QCEPred r1;                                                                \
    qce_expr_sge_i##bits(&solver, &v0, &x, &r1);                               \
    assert(r1.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r1.symbolic,                                           \
               qce_smt_z3_bv##bits##_sle(                                      \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
                                                                               \
    /* x >=s x is true */                                                      \
    QCEPred r2;                                                                \
    qce_expr_sge_i##bits(&solver, &x, &x, &r2);                                \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(r2.concrete);                                                       \
                                                                               \
    /* x >=s y <==> y <=s x */                                                 \
    QCEPred r3;                                                                \
    qce_expr_sge_i##bits(&solver, &x, &y, &r3);                                \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_sle(&solver, y.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    int##bits##_t min = ((int##bits##_t)1) << (bits - 1);                      \
    int##bits##_t max = min - 1;                                               \
    QCEExpr vmin, vmax, x;                                                     \
    qce_expr_init_v##bits(&vmin, min);                                         \
    qce_expr_init_v##bits(&vmax, max);                                         \
    qce_expr_init_s##bits(&solver, &x);                                        \
                                                                               \
    /* x >=s min is true */                                                    \
    QCEPred r1;                                                                \
    qce_expr_sge_i##bits(&solver, &x, &vmin, &r1);                             \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(r1.concrete);                                                       \
                                                                               \
    /* x >=s max <=> x == max */                                               \
    QCEPred r2;                                                                \
    qce_expr_sge_i##bits(&solver, &x, &vmax, &r2);                             \
    assert(r2.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r2.symbolic,                                           \
               qce_smt_z3_bv##bits##_eq(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, max))) ==              \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(sge)

#define QCE_UNIT_TEST_EXPR_sgt(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(sgt_i##bits) {                                   \
    /* 1 >s 1 is false */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_sgt_i##bits(&solver, &v1, &v1, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    /* 1 >s -1 is true */                                                      \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_sgt_i##bits(&solver, &v1, &v1m, &r);                              \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 >s x <=> x <s 0 */                                                    \
    QCEPred r1;                                                                \
    qce_expr_sgt_i##bits(&solver, &v0, &x, &r1);                               \
    assert(r1.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r1.symbolic,                                           \
               qce_smt_z3_bv##bits##_slt(                                      \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
                                                                               \
    /* x >s x is false */                                                      \
    QCEPred r2;                                                                \
    qce_expr_sgt_i##bits(&solver, &x, &x, &r2);                                \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r2.concrete);                                                      \
                                                                               \
    /* x >s y <==> y <s x */                                                   \
    QCEPred r3;                                                                \
    qce_expr_sgt_i##bits(&solver, &x, &y, &r3);                                \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_slt(&solver, y.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    int##bits##_t min = ((int##bits##_t)1) << (bits - 1);                      \
    int##bits##_t max = min - 1;                                               \
    QCEExpr vmin, vmax, x;                                                     \
    qce_expr_init_v##bits(&vmin, min);                                         \
    qce_expr_init_v##bits(&vmax, max);                                         \
    qce_expr_init_s##bits(&solver, &x);                                        \
                                                                               \
    /* x >s max is false */                                                    \
    QCEPred r1;                                                                \
    qce_expr_sgt_i##bits(&solver, &x, &vmax, &r1);                             \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r1.concrete);                                                      \
                                                                               \
    /* x >s min <=> x != min */                                                \
    QCEPred r2;                                                                \
    qce_expr_sgt_i##bits(&solver, &x, &vmin, &r2);                             \
    assert(r2.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r2.symbolic,                                           \
               qce_smt_z3_bv##bits##_ne(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, min))) ==              \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(sgt)

#define QCE_UNIT_TEST_EXPR_ult(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(ult_i##bits) {                                   \
    /* 1 <u 1 is false */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_ult_i##bits(&solver, &v1, &v1, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    /* 1 <u -1 is true */                                                      \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_ult_i##bits(&solver, &v1, &v1m, &r);                              \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 <u x <=> x != 0 */                                                    \
    QCEPred r1;                                                                \
    qce_expr_ult_i##bits(&solver, &v0, &x, &r1);                               \
    assert(r1.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r1.symbolic,                                           \
               qce_smt_z3_bv##bits##_ne(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
                                                                               \
    /* x <u x is false */                                                      \
    QCEPred r2;                                                                \
    qce_expr_ult_i##bits(&solver, &x, &x, &r2);                                \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r2.concrete);                                                      \
                                                                               \
    /* x <u y <==> y >u x */                                                   \
    QCEPred r3;                                                                \
    qce_expr_ult_i##bits(&solver, &x, &y, &r3);                                \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_ugt(&solver, y.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    uint##bits##_t max = (uint##bits##_t)0 - 1;                                \
    QCEExpr v0, vmax, x;                                                       \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_v##bits(&vmax, max);                                         \
    qce_expr_init_s##bits(&solver, &x);                                        \
                                                                               \
    /* x <u 0 is false */                                                      \
    QCEPred r1;                                                                \
    qce_expr_ult_i##bits(&solver, &x, &v0, &r1);                               \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r1.concrete);                                                      \
                                                                               \
    /* x <u max <=> x != max */                                                \
    QCEPred r2;                                                                \
    qce_expr_ult_i##bits(&solver, &x, &vmax, &r2);                             \
    assert(r2.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r2.symbolic,                                           \
               qce_smt_z3_bv##bits##_ne(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, max))) ==              \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(ult)

#define QCE_UNIT_TEST_EXPR_ule(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(ule_i##bits) {                                   \
    /* 1 <=u 1 is true */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_ule_i##bits(&solver, &v1, &v1, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    /* 1 <=u -1 is true */                                                     \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_ule_i##bits(&solver, &v1, &v1m, &r);                              \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 <=u x is true */                                                      \
    QCEPred r1;                                                                \
    qce_expr_ule_i##bits(&solver, &v0, &x, &r1);                               \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(r1.concrete);                                                       \
                                                                               \
    /* x <=u x is true */                                                      \
    QCEPred r2;                                                                \
    qce_expr_ule_i##bits(&solver, &x, &x, &r2);                                \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(r2.concrete);                                                       \
                                                                               \
    /* x <=u y <==> y >=u x */                                                 \
    QCEPred r3;                                                                \
    qce_expr_ule_i##bits(&solver, &x, &y, &r3);                                \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_uge(&solver, y.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    uint##bits##_t max = (uint##bits##_t)0 - 1;                                \
    QCEExpr v0, vmax, x;                                                       \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_v##bits(&vmax, max);                                         \
    qce_expr_init_s##bits(&solver, &x);                                        \
                                                                               \
    /* x <=u max is true */                                                    \
    QCEPred r1;                                                                \
    qce_expr_ule_i##bits(&solver, &x, &vmax, &r1);                             \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(r1.concrete);                                                       \
                                                                               \
    /* x <=u 0 <=> x == 0 */                                                   \
    QCEPred r2;                                                                \
    qce_expr_ule_i##bits(&solver, &x, &v0, &r2);                               \
    assert(r2.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r2.symbolic,                                           \
               qce_smt_z3_bv##bits##_eq(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(ule)

#define QCE_UNIT_TEST_EXPR_uge(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(uge_i##bits) {                                   \
    /* 1 >=u 1 is true */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_uge_i##bits(&solver, &v1, &v1, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(r.concrete);                                                        \
  }                                                                            \
  {                                                                            \
    /* 1 >=u -1 is false */                                                    \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_uge_i##bits(&solver, &v1, &v1m, &r);                              \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 >=u x <=> x == 0 */                                                   \
    QCEPred r1;                                                                \
    qce_expr_uge_i##bits(&solver, &v0, &x, &r1);                               \
    assert(r1.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r1.symbolic,                                           \
               qce_smt_z3_bv##bits##_eq(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
                                                                               \
    /* x >=u x is true */                                                      \
    QCEPred r2;                                                                \
    qce_expr_uge_i##bits(&solver, &x, &x, &r2);                                \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(r2.concrete);                                                       \
                                                                               \
    /* x >=u y <==> y <=u x */                                                 \
    QCEPred r3;                                                                \
    qce_expr_uge_i##bits(&solver, &x, &y, &r3);                                \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_ule(&solver, y.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    uint##bits##_t max = (uint##bits##_t)0 - 1;                                \
    QCEExpr v0, vmax, x;                                                       \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_v##bits(&vmax, max);                                         \
    qce_expr_init_s##bits(&solver, &x);                                        \
                                                                               \
    /* x >=u 0 is true */                                                      \
    QCEPred r1;                                                                \
    qce_expr_uge_i##bits(&solver, &x, &v0, &r1);                               \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(r1.concrete);                                                       \
                                                                               \
    /* x >=u max <=> x == max */                                               \
    QCEPred r2;                                                                \
    qce_expr_uge_i##bits(&solver, &x, &vmax, &r2);                             \
    assert(r2.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r2.symbolic,                                           \
               qce_smt_z3_bv##bits##_eq(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, max))) ==              \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(uge)

#define QCE_UNIT_TEST_EXPR_ugt(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(ugt_i##bits) {                                   \
    /* 1 >u 1 is false */                                                      \
    QCEExpr v1;                                                                \
    qce_expr_init_v##bits(&v1, 1);                                             \
    QCEPred r;                                                                 \
    qce_expr_ugt_i##bits(&solver, &v1, &v1, &r);                               \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    /* 1 >u -1 is false */                                                     \
    QCEExpr v1, v1m;                                                           \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    QCEPred r;                                                                 \
    qce_expr_ugt_i##bits(&solver, &v1, &v1m, &r);                              \
    assert(r.mode == QCE_PRED_CONCRETE);                                       \
    assert(!r.concrete);                                                       \
  }                                                                            \
  {                                                                            \
    QCEExpr v0, x, y;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
                                                                               \
    /* 0 >u x is false */                                                      \
    QCEPred r1;                                                                \
    qce_expr_ugt_i##bits(&solver, &v0, &x, &r1);                               \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r1.concrete);                                                      \
                                                                               \
    /* x >u x is false */                                                      \
    QCEPred r2;                                                                \
    qce_expr_ugt_i##bits(&solver, &x, &x, &r2);                                \
    assert(r2.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r2.concrete);                                                      \
                                                                               \
    /* x >u y <==> y <u x */                                                   \
    QCEPred r3;                                                                \
    qce_expr_ugt_i##bits(&solver, &x, &y, &r3);                                \
    assert(r3.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r3.symbolic,                                           \
               qce_smt_z3_bv##bits##_ult(&solver, y.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    uint##bits##_t max = (uint##bits##_t)0 - 1;                                \
    QCEExpr v0, vmax, x;                                                       \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_v##bits(&vmax, max);                                         \
    qce_expr_init_s##bits(&solver, &x);                                        \
                                                                               \
    /* x >u max is false */                                                    \
    QCEPred r1;                                                                \
    qce_expr_ugt_i##bits(&solver, &x, &vmax, &r1);                             \
    assert(r1.mode == QCE_PRED_CONCRETE);                                      \
    assert(!r1.concrete);                                                      \
                                                                               \
    /* x >u 0 <=> x != 0 */                                                    \
    QCEPred r2;                                                                \
    qce_expr_ugt_i##bits(&solver, &x, &v0, &r2);                               \
    assert(r2.mode == QCE_PRED_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove_equiv(                                             \
               &solver, r2.symbolic,                                           \
               qce_smt_z3_bv##bits##_ne(                                       \
                   &solver, x.symbolic,                                        \
                   qce_smt_z3_bv##bits##_value(&solver, 0))) ==                \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(ugt)
#endif

#endif /* QCE_EXPR_CMP_OP_H */