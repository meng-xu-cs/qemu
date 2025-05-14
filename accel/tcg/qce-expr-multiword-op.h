#ifndef QCE_EXPR_MULTIWORD_OP_H
#define QCE_EXPR_MULTIWORD_OP_H

/*
 * Utilities
 */

#define DEFINE_CONCRETE_MULTIWORD_OP_add2(bits)                                \
  static inline void __qce_concrete_bv##bits##_add2(                           \
      int##bits##_t lhs_low, int##bits##_t lhs_high,                           \
      int##bits##_t rhs_low, int##bits##_t rhs_high,                           \
      int##bits##_t *res_low, int##bits##_t *res_high) {                       \
    *res_low = lhs_low + rhs_low;                                              \
    int##bits##_t carry = 0;                                                   \
    uint##bits##_t ures_low =                                                  \
      (uint##bits##_t)lhs_low + (uint##bits##_t)rhs_low;                       \
    if (ures_low < (uint##bits##_t)lhs_low ||                                  \
        ures_low < (uint##bits##_t)rhs_low)                                    \
      carry = 1;                                                               \
    *res_high = lhs_high + rhs_high + carry;                                   \
}

DEFINE_CONCRETE_MULTIWORD_OP_add2(32)
DEFINE_CONCRETE_MULTIWORD_OP_add2(64)

#define DEFINE_CONCRETE_MULTIWORD_OP_sub2(bits)                                \
  static inline void __qce_concrete_bv##bits##_sub2(                           \
      int##bits##_t lhs_low, int##bits##_t lhs_high,                           \
      int##bits##_t rhs_low, int##bits##_t rhs_high,                           \
      int##bits##_t *res_low, int##bits##_t *res_high) {                       \
    *res_low = lhs_low - rhs_low;                                              \
    int##bits##_t borrow =                                                     \
      (uint##bits##_t)lhs_low < (uint##bits##_t)rhs_low ? 1 : 0;               \
    *res_high = lhs_high - rhs_high - borrow;                                  \
}

DEFINE_CONCRETE_MULTIWORD_OP_sub2(32)
DEFINE_CONCRETE_MULTIWORD_OP_sub2(64)

#define DEFINE_CONCRETE_MULTIWORD_OP_muls2(bits)                               \
  static inline void __qce_concrete_bv##bits##_muls2(                          \
      int##bits##_t lhs, int##bits##_t rhs,                                    \
      int##bits##_t *res_low, int##bits##_t *res_high) {                       \
    union {                                                                    \
      int64_t type_i32;                                                        \
      __int128 type_i64;                                                       \
    } v1, v2, res;                                                             \
    v1.type_i##bits = lhs;                                                     \
    v2.type_i##bits = rhs;                                                     \
    res.type_i##bits = v1.type_i##bits * v2.type_i##bits;                      \
    *res_low = res.type_i##bits;                                               \
    *res_high = res.type_i##bits>>bits;                                        \
}

DEFINE_CONCRETE_MULTIWORD_OP_muls2(32)
DEFINE_CONCRETE_MULTIWORD_OP_muls2(64)

/*
 * Templates
 */

#define DEFINE_EXPR_MULTIWORD_OP(bits, name)                                   \
  static inline void qce_expr_##name##_i##bits(                                \
      SolverZ3 *solver, QCEExpr *lhs_low, QCEExpr *lhs_high,                   \
      QCEExpr *rhs_low, QCEExpr *rhs_high,                                     \
      QCEExpr *res_low, QCEExpr *res_high) {                                   \
    /* type checking */                                                        \
    qce_expr_assert_type(lhs_low, I##bits);                                    \
    qce_expr_assert_type(lhs_high, I##bits);                                   \
    qce_expr_assert_type(rhs_low, I##bits);                                    \
    qce_expr_assert_type(rhs_high, I##bits);                                   \
    res_low->type = QCE_EXPR_I##bits;                                          \
    res_high->type = QCE_EXPR_I##bits;                                         \
                                                                               \
    /* base assignment */                                                      \
    if (lhs_low->mode == QCE_EXPR_CONCRETE) {                                  \
      if (rhs_low->mode == QCE_EXPR_CONCRETE) {                                \
        res_low->mode = QCE_EXPR_CONCRETE;                                     \
        res_high->mode = QCE_EXPR_CONCRETE;                                    \
        __qce_concrete_bv##bits##_##name(                                      \
            lhs_low->v_i##bits, lhs_high->v_i##bits,                           \
            rhs_low->v_i##bits, rhs_high->v_i##bits,                           \
            &res_low->v_i##bits, &res_high->v_i##bits);                        \
      } else {                                                                 \
        res_low->mode = QCE_EXPR_SYMBOLIC;                                     \
        res_high->mode = QCE_EXPR_SYMBOLIC;                                    \
        qce_smt_z3_bv##bits##_##name(solver,                                   \
            qce_smt_z3_bv##bits##_value(solver, lhs_low->v_i##bits),           \
            qce_smt_z3_bv##bits##_value(solver, lhs_high->v_i##bits),          \
            rhs_low->symbolic, rhs_high->symbolic,                             \
            &res_low->symbolic, &res_high->symbolic);                          \
      }                                                                        \
    } else {                                                                   \
      if (rhs_low->mode == QCE_EXPR_CONCRETE) {                                \
        res_low->mode = QCE_EXPR_SYMBOLIC;                                     \
        res_high->mode = QCE_EXPR_SYMBOLIC;                                    \
        qce_smt_z3_bv##bits##_##name(solver,                                   \
            lhs_low->symbolic, lhs_high->symbolic,                             \
            qce_smt_z3_bv##bits##_value(solver, rhs_low->v_i##bits),           \
            qce_smt_z3_bv##bits##_value(solver, rhs_high->v_i##bits),          \
            &res_low->symbolic, &res_high->symbolic);                          \
      } else {                                                                 \
        res_low->mode = QCE_EXPR_SYMBOLIC;                                     \
        res_high->mode = QCE_EXPR_SYMBOLIC;                                    \
        qce_smt_z3_bv##bits##_##name(solver,                                   \
                                     lhs_low->symbolic, lhs_high->symbolic,    \
                                     rhs_low->symbolic, rhs_high->symbolic,    \
                                     &res_low->symbolic, &res_high->symbolic); \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* try to reduce symbolic to concrete */                                   \
    if (res_low->mode == QCE_EXPR_SYMBOLIC) {                                  \
      uint##bits##_t val = 0;                                                  \
      if (qce_smt_z3_probe_bv##bits(solver, res_low->symbolic, &val)) {        \
        res_low->mode = QCE_EXPR_CONCRETE;                                     \
        res_low->v_i##bits = val;                                              \
      }                                                                        \
    }                                                                          \
    if (res_high->mode == QCE_EXPR_SYMBOLIC) {                                 \
      uint##bits##_t val = 0;                                                  \
      if (qce_smt_z3_probe_bv##bits(solver, res_high->symbolic, &val)) {       \
        res_high->mode = QCE_EXPR_CONCRETE;                                    \
        res_high->v_i##bits = val;                                             \
      }                                                                        \
    }                                                                          \
  }

#define DEFINE_EXPR_MULTIWORD_OP_DUAL(name)                                    \
  DEFINE_EXPR_MULTIWORD_OP(32, name)                                           \
  DEFINE_EXPR_MULTIWORD_OP(64, name)

#define DEFINE_EXPR_MULTIWORD_OP2(bits, name)                                  \
  static inline void qce_expr_##name##_i##bits(                                \
      SolverZ3 *solver, QCEExpr *lhs, QCEExpr *rhs,                            \
      QCEExpr *res_low, QCEExpr *res_high) {                                   \
    /* type checking */                                                        \
    qce_expr_assert_type(lhs, I##bits);                                        \
    qce_expr_assert_type(rhs, I##bits);                                        \
    res_low->type = QCE_EXPR_I##bits;                                          \
    res_high->type = QCE_EXPR_I##bits;                                         \
                                                                               \
    /* base assignment */                                                      \
    if (lhs->mode == QCE_EXPR_CONCRETE) {                                      \
      if (rhs->mode == QCE_EXPR_CONCRETE) {                                    \
        res_low->mode = QCE_EXPR_CONCRETE;                                     \
        res_high->mode = QCE_EXPR_CONCRETE;                                    \
        __qce_concrete_bv##bits##_##name(lhs->v_i##bits, rhs->v_i##bits,       \
                                         &res_low->v_i##bits,                  \
                                         &res_high->v_i##bits);                \
      } else {                                                                 \
        res_low->mode = QCE_EXPR_SYMBOLIC;                                     \
        res_high->mode = QCE_EXPR_SYMBOLIC;                                    \
        qce_smt_z3_bv##bits##_##name(                                          \
            solver, qce_smt_z3_bv##bits##_value(solver, lhs->v_i##bits),       \
            rhs->symbolic, &res_low->symbolic, &res_high->symbolic);           \
      }                                                                        \
    } else {                                                                   \
      if (rhs->mode == QCE_EXPR_CONCRETE) {                                    \
        res_low->mode = QCE_EXPR_SYMBOLIC;                                     \
        res_high->mode = QCE_EXPR_SYMBOLIC;                                    \
        qce_smt_z3_bv##bits##_##name(                                          \
            solver, lhs->symbolic,                                             \
            qce_smt_z3_bv##bits##_value(solver, rhs->v_i##bits),               \
            &res_low->symbolic, &res_high->symbolic);                          \
      } else {                                                                 \
        res_low->mode = QCE_EXPR_SYMBOLIC;                                     \
        res_high->mode = QCE_EXPR_SYMBOLIC;                                    \
        qce_smt_z3_bv##bits##_##name(solver, lhs->symbolic, rhs->symbolic,     \
                                     &res_low->symbolic, &res_high->symbolic); \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* try to reduce symbolic to concrete */                                   \
    if (res_low->mode == QCE_EXPR_SYMBOLIC) {                                  \
      uint##bits##_t val = 0;                                                  \
      if (qce_smt_z3_probe_bv##bits(solver, res_low->symbolic, &val)) {        \
        res_low->mode = QCE_EXPR_CONCRETE;                                     \
        res_low->v_i##bits = val;                                              \
      }                                                                        \
    }                                                                          \
    if (res_high->mode == QCE_EXPR_SYMBOLIC) {                                 \
      uint##bits##_t val = 0;                                                  \
      if (qce_smt_z3_probe_bv##bits(solver, res_high->symbolic, &val)) {       \
        res_high->mode = QCE_EXPR_CONCRETE;                                    \
        res_high->v_i##bits = val;                                             \
      }                                                                        \
    }                                                                          \
  }

#define DEFINE_EXPR_MULTIWORD_OP2_DUAL(name)                                   \
  DEFINE_EXPR_MULTIWORD_OP2(32, name)                                          \
  DEFINE_EXPR_MULTIWORD_OP2(64, name)

/*
 * Multiword Arithmetics
 */

DEFINE_EXPR_MULTIWORD_OP_DUAL(add2)
DEFINE_EXPR_MULTIWORD_OP_DUAL(sub2)
// DEFINE_EXPR_MULTIWORD_OP2_DUAL(mulu2)
DEFINE_EXPR_MULTIWORD_OP2_DUAL(muls2)

/*
 * Testing
 */

#define QCE_UNIT_TEST_EXPR_add2(bits)                                          \
  QCE_UNIT_TEST_EXPR_PROLOGUE(add2_i##bits) {                                  \
    /* INT_MAX + 1 -> high == 0, low == INT_MIN  */                            \
    QCEExpr v1_low, v1_high, v2_low, v2_high, r_low, r_high;                   \
    qce_expr_init_v##bits(&v1_low, INT##bits##_MAX);                           \
    qce_expr_init_v##bits(&v1_high, 0);                                        \
    qce_expr_init_v##bits(&v2_low, 1);                                         \
    qce_expr_init_v##bits(&v2_high, 0);                                        \
    qce_expr_add2_i##bits(&solver, &v1_low, &v1_high, &v2_low, &v2_high,       \
                          &r_low, &r_high);                                    \
    assert(r_low.v_i##bits == INT##bits##_MIN);                                \
    assert(r_high.v_i##bits == 0);                                             \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_CONCRETE);                                   \
    assert(r_high.mode == QCE_EXPR_CONCRETE);                                  \
  }                                                                            \
  {                                                                            \
    /* INT_MIN + INT_MIN -> high == 1, low == 0 */                             \
    QCEExpr v1_low, v1_high, v2_low, v2_high, r_low, r_high;                   \
    qce_expr_init_v##bits(&v1_low, INT##bits##_MIN);                           \
    qce_expr_init_v##bits(&v1_high, 0);                                        \
    qce_expr_init_v##bits(&v2_low, INT##bits##_MIN);                           \
    qce_expr_init_v##bits(&v2_high, 0);                                        \
    qce_expr_add2_i##bits(&solver, &v1_low, &v1_high, &v2_low, &v2_high,       \
                          &r_low, &r_high);                                    \
      assert(r_low.v_i##bits == 0);                                            \
      assert(r_high.v_i##bits == 1);                                           \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_CONCRETE);                                   \
    assert(r_high.mode == QCE_EXPR_CONCRETE);                                  \
  }                                                                            \
  {                                                                            \
    /* 1 + 2 == 3 */                                                           \
    QCEExpr v1_low, v1_high, v2_low, v2_high, r_low, r_high;                   \
    qce_expr_init_v##bits(&v1_low, 1);                                         \
    qce_expr_init_v##bits(&v1_high, 0);                                        \
    qce_expr_init_v##bits(&v2_low, 2);                                         \
    qce_expr_init_v##bits(&v2_high, 0);                                        \
    qce_expr_add2_i##bits(&solver, &v1_low, &v1_high, &v2_low, &v2_high,       \
                          &r_low, &r_high);                                    \
    assert(r_low.v_i##bits == 3);                                              \
    assert(r_high.v_i##bits == 0);                                             \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_CONCRETE);                                   \
    assert(r_high.mode == QCE_EXPR_CONCRETE);                                  \
  }                                                                            \
  {                                                                            \
    /* a + b == b + a */                                                       \
    QCEExpr a_low, a_high, b_low, b_high, r_low, r_high;                       \
    qce_expr_init_s##bits(&solver, &a_low);                                    \
    qce_expr_init_s##bits(&solver, &a_high);                                   \
    qce_expr_init_s##bits(&solver, &b_low);                                    \
    qce_expr_init_s##bits(&solver, &b_high);                                   \
    qce_expr_add2_i##bits(&solver, &a_low, &a_high, &b_low, &b_high,           \
                          &r_low, &r_high);                                    \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_SYMBOLIC);                                   \
    assert(r_high.mode == QCE_EXPR_SYMBOLIC);                                  \
    QCEExpr r2_low, r2_high;                                                   \
    qce_smt_z3_bv##bits##_add2(&solver,                                        \
                               b_low.symbolic, b_high.symbolic,                \
                               a_low.symbolic, a_high.symbolic,                \
                               &r2_low.symbolic, &r2_high.symbolic);           \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_low.symbolic, r2_low.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_high.symbolic, r2_high.symbolic)) ==\
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a + 0 == a */                                                           \
    QCEExpr a_low, a_high, b_low, b_high, r_low, r_high;                       \
    qce_expr_init_s##bits(&solver, &a_low);                                    \
    qce_expr_init_s##bits(&solver, &a_high);                                   \
    qce_expr_init_v##bits(&b_low, 0);                                          \
    qce_expr_init_v##bits(&b_high, 0);                                         \
    qce_expr_add2_i##bits(&solver, &a_low, &a_high, &b_low, &b_high,           \
                          &r_low, &r_high);                                    \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_SYMBOLIC);                                   \
    assert(r_high.mode == QCE_EXPR_SYMBOLIC);                                  \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_low.symbolic, a_low.symbolic)) ==   \
           SMT_Z3_PROVE_PROVED);                                               \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_high.symbolic, a_high.symbolic)) == \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(add2)

#define QCE_UNIT_TEST_EXPR_sub2(bits)                                          \
  QCE_UNIT_TEST_EXPR_PROLOGUE(sub2_i##bits) {                                  \
    /* 2 - 1 == 1 */                                                           \
    QCEExpr v1_low, v1_high, v2_low, v2_high, r_low, r_high;                   \
    qce_expr_init_v##bits(&v1_low, 2);                                         \
    qce_expr_init_v##bits(&v1_high, 0);                                        \
    qce_expr_init_v##bits(&v2_low, 1);                                         \
    qce_expr_init_v##bits(&v2_high, 0);                                        \
    qce_expr_sub2_i##bits(&solver, &v1_low, &v1_high, &v2_low, &v2_high,       \
                          &r_low, &r_high);                                    \
    assert(r_low.v_i##bits == 1);                                              \
    assert(r_high.v_i##bits == 0);                                             \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_CONCRETE);                                   \
    assert(r_high.mode == QCE_EXPR_CONCRETE);                                  \
  }                                                                            \
  {                                                                            \
    /* a - b == a - b */                                                       \
    QCEExpr a_low, a_high, b_low, b_high, r_low, r_high;                       \
    qce_expr_init_s##bits(&solver, &a_low);                                    \
    qce_expr_init_s##bits(&solver, &a_high);                                   \
    qce_expr_init_s##bits(&solver, &b_low);                                    \
    qce_expr_init_s##bits(&solver, &b_high);                                   \
    qce_expr_sub2_i##bits(&solver, &a_low, &a_high, &b_low, &b_high,           \
                          &r_low, &r_high);                                    \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_SYMBOLIC);                                   \
    assert(r_high.mode == QCE_EXPR_SYMBOLIC);                                  \
    QCEExpr r2_low, r2_high;                                                   \
    qce_smt_z3_bv##bits##_sub2(&solver,                                        \
                               a_low.symbolic, a_high.symbolic,                \
                               b_low.symbolic, b_high.symbolic,                \
                               &r2_low.symbolic, &r2_high.symbolic);           \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_low.symbolic, r2_low.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_high.symbolic, r2_high.symbolic)) ==\
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a - a == 0 */                                                           \
    QCEExpr a_low, a_high, r_low, r_high;                                      \
    qce_expr_init_s##bits(&solver, &a_low);                                    \
    qce_expr_init_s##bits(&solver, &a_high);                                   \
    qce_expr_sub2_i##bits(&solver, &a_low, &a_high, &a_low, &a_high,           \
                          &r_low, &r_high);                                    \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_CONCRETE);                                   \
    assert(r_high.mode == QCE_EXPR_CONCRETE);                                  \
    assert(r_low.v_i##bits == 0);                                              \
    assert(r_high.v_i##bits == 0);                                             \
  }                                                                            \
  {                                                                            \
    /* a - 0 == a */                                                           \
    QCEExpr a_low, a_high, v_low, v_high, r_low, r_high;                       \
    qce_expr_init_s##bits(&solver, &a_low);                                    \
    qce_expr_init_s##bits(&solver, &a_high);                                   \
    qce_expr_init_v##bits(&v_low, 0);                                          \
    qce_expr_init_v##bits(&v_high, 0);                                         \
    qce_expr_sub2_i##bits(&solver, &a_low, &a_high, &v_low, &v_high,           \
                          &r_low, &r_high);                                    \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_SYMBOLIC);                                   \
    assert(r_high.mode == QCE_EXPR_SYMBOLIC);                                  \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_low.symbolic, a_low.symbolic)) ==   \
           SMT_Z3_PROVE_PROVED);                                               \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_high.symbolic, a_high.symbolic)) == \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(sub2)

#define QCE_UNIT_TEST_EXPR_muls2(bits)                                         \
  QCE_UNIT_TEST_EXPR_PROLOGUE(muls2_i##bits) {                                 \
    /* 1 * 2 == 2 */                                                           \
    QCEExpr v1, v2, r_b, r_t;                                                  \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_muls2_i##bits(&solver, &v1, &v2, &r_b, &r_t);                     \
    assert(r_b.type == QCE_EXPR_I##bits);                                      \
    assert(r_t.type == QCE_EXPR_I##bits);                                      \
    assert(r_b.mode == QCE_EXPR_CONCRETE);                                     \
    assert(r_t.mode == QCE_EXPR_CONCRETE);                                     \
    assert(r_b.v_i##bits == 2);                                                \
    assert(r_t.v_i##bits == 0);                                                \
  }                                                                            \
  {                                                                            \
    /* INT##bits##_MAX * INT##bits##_MAX == 0x3f...f0...01 */                  \
    QCEExpr vmax, r_b, r_t;                                                    \
    qce_expr_init_v##bits(&vmax, INT##bits##_MAX);                             \
    qce_expr_muls2_i##bits(&solver, &vmax, &vmax, &r_b, &r_t);                 \
    assert(r_b.type == QCE_EXPR_I##bits);                                      \
    assert(r_t.type == QCE_EXPR_I##bits);                                      \
    assert(r_b.mode == QCE_EXPR_CONCRETE);                                     \
    assert(r_t.mode == QCE_EXPR_CONCRETE);                                     \
    if (bits == 32) {                                                          \
      assert(r_b.v_i##bits == 1);                                              \
      assert(r_t.v_i##bits == 0x3fffffff);                                     \
    } else if (bits == 64) {                                                   \
      assert(r_b.v_i##bits == 1);                                              \
      assert(r_t.v_i##bits == 0x3fffffffffffffff);                             \
    }                                                                          \
  }                                                                            \
  {                                                                            \
    /* a * b == b * a */                                                       \
    QCEExpr a, b, r_b, r_t;                                                    \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_muls2_i##bits(&solver, &a, &b, &r_b, &r_t);                       \
    assert(r_b.type == QCE_EXPR_I##bits);                                      \
    assert(r_t.type == QCE_EXPR_I##bits);                                      \
    assert(r_b.mode == QCE_EXPR_SYMBOLIC);                                     \
    assert(r_t.mode == QCE_EXPR_SYMBOLIC);                                     \
    QCEExpr r2_b, r2_t;                                                        \
    qce_smt_z3_bv##bits##_muls2(&solver, b.symbolic, a.symbolic,               \
                                &r2_b.symbolic, &r2_t.symbolic);               \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_b.symbolic, r2_b.symbolic)) ==      \
           SMT_Z3_PROVE_PROVED);                                               \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r_t.symbolic, r2_t.symbolic)) ==      \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a * 0 == 0 */                                                           \
    QCEExpr a, v0, r_b, r_t;                                                   \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_muls2_i##bits(&solver, &a, &v0, &r_b, &r_t);                      \
    assert(r_b.type == QCE_EXPR_I##bits);                                      \
    assert(r_t.type == QCE_EXPR_I##bits);                                      \
    assert(r_b.mode == QCE_EXPR_CONCRETE);                                     \
    assert(r_t.mode == QCE_EXPR_CONCRETE);                                     \
    assert(r_b.v_i##bits == 0);                                                \
    assert(r_t.v_i##bits == 0);                                                \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(muls2)

#endif /* QCE_EXPR_MULTIWORD_H */
