#ifndef QCE_EXPR_UNI_OP_H
#define QCE_EXPR_UNI_OP_H

/*
 * Utilities
 */

#define DEFINE_CONCRETE_UNI_OP_SIGNED(bits, name, op)                          \
  static inline int##bits##_t __qce_concrete_bv##bits##_##name(                \
      int##bits##_t operand) {                                                 \
    return op operand;                                                         \
}

#define DEFINE_CONCRETE_UNI_OP_SIGNED_DUAL(name, op)                           \
  DEFINE_CONCRETE_UNI_OP_SIGNED(32, name, op)                                  \
  DEFINE_CONCRETE_UNI_OP_SIGNED(64, name, op)

DEFINE_CONCRETE_UNI_OP_SIGNED_DUAL(neg, -)

/*
 * Templates
 */

#define DEFINE_EXPR_UNI_OP(bits, name)                                         \
  static inline void qce_expr_##name##_i##bits(SolverZ3 *solver, QCEExpr *opv, \
                                               QCEExpr *result) {              \
    /* type checking */                                                        \
    qce_expr_assert_type(opv, I##bits);                                        \
    result->type = QCE_EXPR_I##bits;                                           \
                                                                               \
    /* base assignment */                                                      \
    if (opv->mode == QCE_EXPR_CONCRETE) {                                      \
      result->mode = QCE_EXPR_CONCRETE;                                        \
      result->v_i##bits = __qce_concrete_bv##bits##_##name(opv->v_i##bits);    \
    } else {                                                                   \
      result->mode = QCE_EXPR_SYMBOLIC;                                        \
      result->symbolic = qce_smt_z3_bv##bits##_##name(solver, opv->symbolic);  \
    }                                                                          \
                                                                               \
    /* try to reduce symbolic to concrete */                                   \
    if (result->mode == QCE_EXPR_SYMBOLIC) {                                   \
      uint##bits##_t val = 0;                                                  \
      if (qce_smt_z3_probe_bv##bits(solver, result->symbolic, &val)) {         \
        result->mode = QCE_EXPR_CONCRETE;                                      \
        result->v_i##bits = val;                                               \
      }                                                                        \
    }                                                                          \
  }

#define DEFINE_EXPR_UNI_OP_DUAL(name)                                          \
  DEFINE_EXPR_UNI_OP(32, name)                                                 \
  DEFINE_EXPR_UNI_OP(64, name)

DEFINE_EXPR_UNI_OP_DUAL(neg)

/*
 * Testing
 */

#ifndef QCE_RELEASE
#define QCE_UNIT_TEST_EXPR_neg(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(neg_i##bits) {                                   \
    /* -(-1) = 1 */                                                            \
    QCEExpr v1m, r;                                                            \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_neg_i##bits(&solver, &v1m, &r);                                   \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 1);                                                  \
  }                                                                            \
  {                                                                            \
    /* 0 - a == -(a) */                                                        \
    Z3_ast s0 = qce_smt_z3_bv##bits##_value(&solver, 0);                       \
    QCEExpr a, r;                                                              \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_neg_i##bits(&solver, &a, &r);                                     \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r.symbolic,                           \
                                qce_smt_z3_bv##bits##_sub(&solver, s0,         \
                                                          a.symbolic))) ==     \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a + -(a) == 0 */                                                        \
    QCEExpr a, r;                                                              \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_neg_i##bits(&solver, &a, &r);                                     \
    qce_expr_add_i##bits(&solver, &a, &r, &r);                                 \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(neg)
#endif

#endif /* QCE_EXPR_UNI_OP_H */