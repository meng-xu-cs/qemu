#ifndef QCE_EXPR_MOVE_H
#define QCE_EXPR_MOVE_H

#define DEFINE_CONCRETE_MOVCOND(bits)                                          \
  static inline QCEExpr *__qce_concrete_bv##bits##_movcond(                    \
      int##bits##_t lhs, int##bits##_t rhs,                                    \
      QCEExpr * val1, QCEExpr * val2, tcg_target_ulong cond) {                 \
    bool result;                                                               \
    switch ((TCGCond)cond) {                                                   \
    case TCG_COND_EQ:                                                          \
      result = (lhs == rhs);                                                   \
      break;                                                                   \
    case TCG_COND_NE:                                                          \
      result = (lhs != rhs);                                                   \
      break;                                                                   \
    case TCG_COND_LT:                                                          \
      result = (lhs < rhs);                                                    \
      break;                                                                   \
    case TCG_COND_GT:                                                          \
      result = (lhs > rhs);                                                    \
      break;                                                                   \
    case TCG_COND_LE:                                                          \
      result = (lhs <= rhs);                                                   \
      break;                                                                   \
    case TCG_COND_GE:                                                          \
      result = (lhs >= rhs);                                                   \
      break;                                                                   \
    case TCG_COND_LTU:                                                         \
      result = ((uint##bits##_t)lhs < (uint##bits##_t)rhs);                    \
      break;                                                                   \
    case TCG_COND_GTU:                                                         \
      result = ((uint##bits##_t)lhs > (uint##bits##_t)rhs);                    \
      break;                                                                   \
    case TCG_COND_LEU:                                                         \
      result = ((uint##bits##_t)lhs <= (uint##bits##_t)rhs);                   \
      break;                                                                   \
    case TCG_COND_GEU:                                                         \
      result = ((uint##bits##_t)lhs >= (uint##bits##_t)rhs);                   \
      break;                                                                   \
    case TCG_COND_TSTEQ:                                                       \
      result = (lhs & rhs) == 0;                                               \
      break;                                                                   \
    case TCG_COND_TSTNE:                                                       \
      result = (lhs & rhs) != 0;                                               \
      break;                                                                   \
    default:                                                                   \
      qce_fatal("movcond: condition not handled");                             \
    }                                                                          \
    return result ? val1: val2;                                                \
  }

#define DEFINE_CONCRETE_MOVCOND_DUAL                                           \
  DEFINE_CONCRETE_MOVCOND(32)                                                  \
  DEFINE_CONCRETE_MOVCOND(64)

DEFINE_CONCRETE_MOVCOND_DUAL

#define DEFINE_EXPR_MOVECOND(bits)                                             \
  static inline void qce_expr_movcond_i##bits(                                 \
      SolverZ3 *solver, QCEExpr *lhs, QCEExpr *rhs,                            \
      QCEExpr *val1, QCEExpr *val2, tcg_target_ulong cond, QCEExpr *result) {  \
    /* type checking */                                                        \
    qce_expr_assert_type(lhs, I##bits);                                        \
    qce_expr_assert_type(rhs, I##bits);                                        \
    qce_expr_assert_type(val1, I##bits);                                       \
    qce_expr_assert_type(val2, I##bits);                                       \
    result->type = QCE_EXPR_I##bits;                                           \
                                                                               \
    /* base assignment */                                                      \
    if (lhs->mode == QCE_EXPR_CONCRETE) {                                      \
      if (rhs->mode == QCE_EXPR_CONCRETE) {                                    \
        result = __qce_concrete_bv##bits##_movcond(lhs->v_i##bits,             \
                                                   rhs->v_i##bits,             \
                                                   val1, val2, cond);          \
      } else {                                                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_movcond(                      \
            solver, qce_smt_z3_bv##bits##_value(solver, lhs->v_i##bits),       \
            rhs->symbolic, val1->mode == QCE_EXPR_SYMBOLIC ? val1->symbolic : qce_smt_z3_bv##bits##_value(solver, val1->v_i##bits), val2->mode == QCE_EXPR_SYMBOLIC ? val2->symbolic : qce_smt_z3_bv##bits##_value(solver, val2->v_i##bits), cond);                                  \
      }                                                                        \
    } else {                                                                   \
      if (rhs->mode == QCE_EXPR_CONCRETE) {                                    \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_movcond(                      \
            solver, lhs->symbolic,                                             \
            qce_smt_z3_bv##bits##_value(solver, rhs->v_i##bits),               \
            val1->mode == QCE_EXPR_SYMBOLIC ? val1->symbolic : qce_smt_z3_bv##bits##_value(solver, val1->v_i##bits), val2->mode == QCE_EXPR_SYMBOLIC ? val2->symbolic : qce_smt_z3_bv##bits##_value(solver, val2->v_i##bits), cond);                                                 \
      } else {                                                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic =                                                     \
            qce_smt_z3_bv##bits##_movcond(solver, lhs->symbolic,               \
                                          rhs->symbolic, val1->mode == QCE_EXPR_SYMBOLIC ? val1->symbolic : qce_smt_z3_bv##bits##_value(solver, val1->v_i##bits), val2->mode == QCE_EXPR_SYMBOLIC ? val2->symbolic : qce_smt_z3_bv##bits##_value(solver, val2->v_i##bits), cond);    \
      }                                                                        \
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

#define DEFINE_EXPR_MOVECOND_DUAL                                              \
  DEFINE_EXPR_MOVECOND(32)                                                     \
  DEFINE_EXPR_MOVECOND(64)

DEFINE_EXPR_MOVECOND_DUAL

#endif /* QCE_EXPR_LD_ST_H */