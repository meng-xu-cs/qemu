#ifndef QCE_EXPR_UNI_OP_H
#define QCE_EXPR_UNI_OP_H

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

#endif /* QCE_EXPR_UNI_OP_H */