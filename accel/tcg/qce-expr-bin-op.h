#ifndef QCE_EXPR_BIN_OP_H
#define QCE_EXPR_BIN_OP_H

/*
 * Utilities
 */

#define DEFINE_CONCRETE_BIN_OP_SIGNED(bits, name, op)                          \
  static inline int##bits##_t __qce_concrete_bv##bits##_##name(                \
      int##bits##_t lhs, int##bits##_t rhs) {                                  \
    return lhs op rhs;                                                         \
  }

#define DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(name, op)                           \
  DEFINE_CONCRETE_BIN_OP_SIGNED(32, name, op)                                  \
  DEFINE_CONCRETE_BIN_OP_SIGNED(64, name, op)

#define DEFINE_CONCRETE_BIN_OP_UNSIGNED(bits, name, op)                        \
  static inline uint##bits##_t __qce_concrete_bv##bits##_##name(               \
      uint##bits##_t lhs, uint##bits##_t rhs) {                                \
    return lhs op rhs;                                                         \
  }

#define DEFINE_CONCRETE_BIN_OP_UNSIGNED_DUAL(name, op)                         \
  DEFINE_CONCRETE_BIN_OP_UNSIGNED(32, name, op)                                \
  DEFINE_CONCRETE_BIN_OP_UNSIGNED(64, name, op)

DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(add, +)
DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(sub, -)
DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(mul, *)
DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(div, /)

DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(bvand, &)
DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(bvor, |)
DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(bvxor, ^)

DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(shl, <<)
DEFINE_CONCRETE_BIN_OP_UNSIGNED_DUAL(shr, >>)
DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(sar, >>)

#define DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP(bits, name, op1, op2, op3)          \
  static inline int##bits##_t __qce_concrete_bv##bits##_##name(                \
      int##bits##_t lhs, int##bits##_t rhs) {                                  \
    return op1 lhs op2 op3 rhs;                                                \
  }

#define DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP_DUAL(name, op1, op2, op3)           \
  DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP(32, name, op1, op2, op3)                  \
  DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP(64, name, op1, op2, op3)

DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP_DUAL(bvandc, +, &, ~)
DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP_DUAL(bvorc, +, ^, ~)
DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP_DUAL(bvnand, ~, |, ~)
DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP_DUAL(bvnor, ~, &, ~)
DEFINE_CONCRETE_BIN_OP_SIGNED_TEMP_DUAL(bveqv, +, |, ~)

#define DEFINE_CONCRETE_BIN_OP_muls2(bits, name, op)                           \
  static inline void __qce_concrete_bv##bits##_##name(                         \
      int##bits##_t lhs, int##bits##_t rhs,                                    \
      int##bits##_t *res_low, int##bits##_t *res_high) {                       \
    union {                                                                    \
      int64_t type_i32;                                                        \
      __int128 type_i64;                                                       \
    } v1, v2, res;                                                             \
    v1.type_i##bits = lhs;                                                     \
    v2.type_i##bits = rhs;                                                     \
    res.type_i##bits = v1.type_i##bits op v2.type_i##bits;                     \
    *res_low = res.type_i##bits;                                               \
    *res_high = res.type_i##bits>>bits;                                        \
  }

#define DEFINE_CONCRETE_BIN_OP_muls2_DUAL(name, op)                            \
  DEFINE_CONCRETE_BIN_OP_muls2(32, name, op)                                   \
  DEFINE_CONCRETE_BIN_OP_muls2(64, name, op)

// DEFINE_CONCRETE_BIN_OP_mulu2_DUAL(mulu2, *)
DEFINE_CONCRETE_BIN_OP_muls2_DUAL(muls2, *)

#define DEFINE_CONCRETE_QUAD_OP_add2(bits, name, op)                           \
  static inline void __qce_concrete_bv##bits##_##name(                         \
      int##bits##_t lhs_low, int##bits##_t lhs_high,                           \
      int##bits##_t rhs_low, int##bits##_t rhs_high,                           \
      int##bits##_t *res_low, int##bits##_t *res_high) {                       \
    *res_low = lhs_low op rhs_low;                                             \
    int##bits##_t carry = 0;                                                   \
    uint##bits##_t ures_low =                                                  \
      (uint##bits##_t)lhs_low op (uint##bits##_t)rhs_low;                      \
    if (ures_low < (uint##bits##_t)lhs_low ||                                  \
        ures_low < (uint##bits##_t)rhs_low)                                    \
      carry = 1;                                                               \
    *res_high = lhs_high op rhs_high op carry;                                 \
  }      

#define DEFINE_CONCRETE_QUAD_OP_add2_DUAL(name, op)                            \
  DEFINE_CONCRETE_QUAD_OP_add2(32, name, op)                                   \
  DEFINE_CONCRETE_QUAD_OP_add2(64, name, op)

#define DEFINE_CONCRETE_QUAD_OP_sub2(bits, name, op)                           \
  static inline void __qce_concrete_bv##bits##_##name(                         \
      int##bits##_t lhs_low, int##bits##_t lhs_high,                           \
      int##bits##_t rhs_low, int##bits##_t rhs_high,                           \
      int##bits##_t *res_low, int##bits##_t *res_high) {                       \
    *res_low = lhs_low op rhs_low;                                             \
    int##bits##_t borrow =                                                     \
      (uint##bits##_t)lhs_low < (uint##bits##_t)rhs_low ? 1 : 0;               \
    *res_high = lhs_high op rhs_high op borrow;                                \
  }      

#define DEFINE_CONCRETE_QUAD_OP_sub2_DUAL(name, op)                            \
  DEFINE_CONCRETE_QUAD_OP_sub2(32, name, op)                                   \
  DEFINE_CONCRETE_QUAD_OP_sub2(64, name, op)

DEFINE_CONCRETE_QUAD_OP_add2_DUAL(add2, +)
DEFINE_CONCRETE_QUAD_OP_sub2_DUAL(sub2, -)

/*
 * Templates
 */

#define DEFINE_EXPR_BIN_OP(bits, name)                                         \
  static inline void qce_expr_##name##_i##bits(                                \
      SolverZ3 *solver, QCEExpr *lhs, QCEExpr *rhs, QCEExpr *result) {         \
    /* type checking */                                                        \
    qce_expr_assert_type(lhs, I##bits);                                        \
    qce_expr_assert_type(rhs, I##bits);                                        \
    result->type = QCE_EXPR_I##bits;                                           \
                                                                               \
    /* base assignment */                                                      \
    if (lhs->mode == QCE_EXPR_CONCRETE) {                                      \
      if (rhs->mode == QCE_EXPR_CONCRETE) {                                    \
        result->mode = QCE_EXPR_CONCRETE;                                      \
        result->v_i##bits =                                                    \
            __qce_concrete_bv##bits##_##name(lhs->v_i##bits, rhs->v_i##bits);  \
      } else {                                                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_##name(                       \
            solver, qce_smt_z3_bv##bits##_value(solver, lhs->v_i##bits),       \
            rhs->symbolic);                                                    \
      }                                                                        \
    } else {                                                                   \
      if (rhs->mode == QCE_EXPR_CONCRETE) {                                    \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_##name(                       \
            solver, lhs->symbolic,                                             \
            qce_smt_z3_bv##bits##_value(solver, rhs->v_i##bits));              \
      } else {                                                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_##name(solver, lhs->symbolic, \
                                                        rhs->symbolic);        \
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

#define DEFINE_EXPR_BIN_OP_DUAL(name)                                          \
  DEFINE_EXPR_BIN_OP(32, name)                                                 \
  DEFINE_EXPR_BIN_OP(64, name)

#define DEFINE_EXPR_BIN_OP_BIN_RES(bits, name)                                 \
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
                                        &res_low->v_i##bits,                   \
                                        &res_high->v_i##bits);                 \
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

#define DEFINE_EXPR_BIN_OP_BIN_RES_DUAL(name)                                  \
  DEFINE_EXPR_BIN_OP_BIN_RES(32, name)                                         \
  DEFINE_EXPR_BIN_OP_BIN_RES(64, name)

#define DEFINE_EXPR_QUAD_OP(bits, name)                                        \
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

#define DEFINE_EXPR_QUAD_OP_DUAL(name)                                         \
  DEFINE_EXPR_QUAD_OP(32, name)                                                \
  DEFINE_EXPR_QUAD_OP(64, name)

/*
 * Arithmetics
 */

DEFINE_EXPR_BIN_OP_DUAL(add)
DEFINE_EXPR_BIN_OP_DUAL(sub)
DEFINE_EXPR_BIN_OP_DUAL(mul)
DEFINE_EXPR_BIN_OP_DUAL(div)

DEFINE_EXPR_QUAD_OP_DUAL(add2)
DEFINE_EXPR_QUAD_OP_DUAL(sub2)
// DEFINE_EXPR_BIN_OP_BIN_RES_DUAL(mulu2)
DEFINE_EXPR_BIN_OP_BIN_RES_DUAL(muls2)

DEFINE_EXPR_BIN_OP_DUAL(shl)
DEFINE_EXPR_BIN_OP_DUAL(shr)
DEFINE_EXPR_BIN_OP_DUAL(sar)

/*
 * Bitwise
 */

DEFINE_EXPR_BIN_OP_DUAL(bvand)
DEFINE_EXPR_BIN_OP_DUAL(bvor)
DEFINE_EXPR_BIN_OP_DUAL(bvxor)

DEFINE_EXPR_BIN_OP_DUAL(bvandc)
DEFINE_EXPR_BIN_OP_DUAL(bvorc)
DEFINE_EXPR_BIN_OP_DUAL(bvnand)
DEFINE_EXPR_BIN_OP_DUAL(bvnor)
DEFINE_EXPR_BIN_OP_DUAL(bveqv)

/*
 * Testing
 */

#ifndef QCE_RELEASE
#define QCE_UNIT_TEST_EXPR_add(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(add_i##bits) {                                   \
    /* 1 + 2 == 3 */                                                           \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_add_i##bits(&solver, &v1, &v2, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 3);                                                  \
  }                                                                            \
  {                                                                            \
    /* -1 + 3 == 2 */                                                          \
    QCEExpr v1m, v3, r;                                                        \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v3, 3);                                             \
    qce_expr_add_i##bits(&solver, &v1m, &v3, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 2);                                                  \
  }                                                                            \
  {                                                                            \
    /* a + b == b + a */                                                       \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_add_i##bits(&solver, &a, &b, &r);                                 \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r.symbolic,                           \
                                qce_smt_z3_bv##bits##_add(&solver, b.symbolic, \
                                                          a.symbolic))) ==     \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a + 0 == a */                                                           \
    QCEExpr a, v0, r;                                                          \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_add_i##bits(&solver, &a, &v0, &r);                                \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r.symbolic, a.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(add)

#define QCE_UNIT_TEST_EXPR_sub(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(sub_i##bits) {                                   \
    /* 1 - 2 == -1 */                                                          \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_sub_i##bits(&solver, &v1, &v2, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -1);                                                 \
  }                                                                            \
  {                                                                            \
    /* -1 - (-3) == 2 */                                                       \
    QCEExpr v1m, v3m, r;                                                       \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v3m, -3);                                           \
    qce_expr_sub_i##bits(&solver, &v1m, &v3m, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 2);                                                  \
  }                                                                            \
  {                                                                            \
    /* a - b == a - b */                                                       \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_sub_i##bits(&solver, &a, &b, &r);                                 \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r.symbolic,                           \
                                qce_smt_z3_bv##bits##_sub(&solver, a.symbolic, \
                                                          b.symbolic))) ==     \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a - 0 == a */                                                           \
    QCEExpr a, v0, r;                                                          \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_sub_i##bits(&solver, &a, &v0, &r);                                \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r.symbolic, a.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a - a == 0 */                                                           \
    QCEExpr a, r;                                                              \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_sub_i##bits(&solver, &a, &a, &r);                                 \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(sub)

#define QCE_UNIT_TEST_EXPR_mul(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(mul_i##bits) {                                   \
    /* 1 * 2 == 2 */                                                           \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_mul_i##bits(&solver, &v1, &v2, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 2);                                                  \
  }                                                                            \
  {                                                                            \
    /* -1 * 3 == -3 */                                                         \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, -1);                                            \
    qce_expr_init_v##bits(&v2, 3);                                             \
    qce_expr_mul_i##bits(&solver, &v1, &v2, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -3);                                                 \
  }                                                                            \
  {                                                                            \
    /* a * b == b * a */                                                       \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_mul_i##bits(&solver, &a, &b, &r);                                 \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r.symbolic,                           \
                                qce_smt_z3_bv##bits##_mul(&solver, b.symbolic, \
                                                          a.symbolic))) ==     \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a * 0 == 0 */                                                           \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_v##bits(&b, 0);                                              \
    qce_expr_mul_i##bits(&solver, &a, &b, &r);                                 \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(mul)

#define QCE_UNIT_TEST_EXPR_div(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(div_i##bits) {                                   \
    /* 2 / 1 == 2 */                                                           \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, 2);                                             \
    qce_expr_init_v##bits(&v2, 1);                                             \
    qce_expr_div_i##bits(&solver, &v1, &v2, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 2);                                                  \
  }                                                                            \
  {                                                                            \
    /* -1 / 3 == 0 */                                                          \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, -1);                                            \
    qce_expr_init_v##bits(&v2, 3);                                             \
    qce_expr_div_i##bits(&solver, &v1, &v2, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  {                                                                            \
    /* a / b == a / b */                                                       \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_div_i##bits(&solver, &a, &b, &r);                                 \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r.symbolic,                           \
                                qce_smt_z3_bv##bits##_div(&solver, a.symbolic, \
                                                          b.symbolic))) ==     \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(div)

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
    /* 2 / 1 == 2 */                                                           \
    QCEExpr v1, v2, res_t, res_b;                                              \
    qce_expr_init_v##bits(&v1, INT##bits##_MAX);                               \
    qce_expr_init_v##bits(&v2, INT##bits##_MAX);                               \
    qce_expr_muls2_i##bits(&solver, &v1, &v2, &res_t, &res_b);                 \
    if (bits == 32) {                                                          \
      assert(res_t.v_i##bits == 1);                                            \
      assert(res_b.v_i##bits == 1073741823);                                   \
    } else if (bits == 64) {                                                   \
      assert(res_t.v_i##bits == 1);                                            \
      assert(res_b.v_i##bits == 4611686018427387903);                          \
    }                                                                          \
    assert(res_t.type == QCE_EXPR_I##bits);                                    \
    assert(res_b.type == QCE_EXPR_I##bits);                                    \
    assert(res_t.mode == QCE_EXPR_CONCRETE);                                   \
    assert(res_b.mode == QCE_EXPR_CONCRETE);                                   \
  }                                                                            \
  {                                                                            \
    /* a * b == b * a */                                                       \
    QCEExpr a, b, r_low, r_high;                                               \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_muls2_i##bits(&solver, &a, &b, &r_low, &r_high);                  \
    assert(r_low.type == QCE_EXPR_I##bits);                                    \
    assert(r_high.type == QCE_EXPR_I##bits);                                   \
    assert(r_low.mode == QCE_EXPR_SYMBOLIC);                                   \
    assert(r_high.mode == QCE_EXPR_SYMBOLIC);                                  \
    QCEExpr r2_low, r2_high;                                                   \
    qce_smt_z3_bv##bits##_muls2(&solver, b.symbolic, a.symbolic,               \
                                &r2_low.symbolic, &r2_high.symbolic);          \
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
    /* a * 0 == [0, 0] */                                                      \
    QCEExpr a, b, res_t, res_b;                                                \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_v##bits(&b, 0);                                              \
    qce_expr_muls2_i##bits(&solver, &a, &b, &res_t, &res_b);                   \
    assert(res_t.type == QCE_EXPR_I##bits);                                    \
    assert(res_b.type == QCE_EXPR_I##bits);                                    \
    assert(res_t.mode == QCE_EXPR_CONCRETE);                                   \
    assert(res_b.mode == QCE_EXPR_CONCRETE);                                   \
    assert(res_t.v_i##bits == 0);                                              \
    assert(res_b.v_i##bits == 0);                                              \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(muls2)

#define QCE_UNIT_TEST_EXPR_special_a_add_then_sub(bits)                        \
  QCE_UNIT_TEST_EXPR_PROLOGUE(special_a_add_then_sub_i##bits) {                \
    /* a + 1 - 3 == a - 2 */                                                   \
    QCEExpr a, v1, v3, r1, r2;                                                 \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v3, 3);                                             \
    qce_expr_add_i##bits(&solver, &a, &v1, &r1);                               \
    qce_expr_sub_i##bits(&solver, &r1, &v3, &r2);                              \
    assert(r2.type == QCE_EXPR_I##bits);                                       \
    assert(r2.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    QCEExpr v2, r3;                                                            \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_sub_i##bits(&solver, &a, &v2, &r3);                               \
    assert(r3.type == QCE_EXPR_I##bits);                                       \
    assert(r3.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r2.symbolic, r3.symbolic))); \
  }                                                                            \
  {                                                                            \
    /* a + (-1) - (-3) == a + 2 */                                             \
    QCEExpr a, v1m, v3m, r1, r2;                                               \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v3m, -3);                                           \
    qce_expr_add_i##bits(&solver, &a, &v1m, &r1);                              \
    qce_expr_sub_i##bits(&solver, &r1, &v3m, &r2);                             \
    assert(r2.type == QCE_EXPR_I##bits);                                       \
    assert(r2.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    QCEExpr v2, r3;                                                            \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_add_i##bits(&solver, &a, &v2, &r3);                               \
    assert(r3.type == QCE_EXPR_I##bits);                                       \
    assert(r3.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r2.symbolic, r3.symbolic))); \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(special_a_add_then_sub)

#define QCE_UNIT_TEST_EXPR_bvand(bits)                                         \
  QCE_UNIT_TEST_EXPR_PROLOGUE(bvand_i##bits) {                                 \
    /* 1 & 2 == 0 */                                                           \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_bvand_i##bits(&solver, &v1, &v2, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  {                                                                            \
    /* -1 & (-3) == -3 */                                                      \
    QCEExpr v1m, v3m, r;                                                       \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v3m, -3);                                           \
    qce_expr_bvand_i##bits(&solver, &v1m, &v3m, &r);                           \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -3);                                                 \
  }                                                                            \
  {                                                                            \
    /* a & b == b & a */                                                       \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_bvand_i##bits(&solver, &a, &b, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(                                                   \
               &solver, qce_smt_z3_bv##bits##_eq(                              \
                            &solver, r.symbolic,                               \
                            qce_smt_z3_bv##bits##_bvand(&solver, b.symbolic,   \
                                                        a.symbolic))) ==       \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a & 0 == 0 */                                                           \
    QCEExpr v0, a, r;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvand_i##bits(&solver, &a, &v0, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  {                                                                            \
    /* a & -1 == a */                                                          \
    QCEExpr v1m, a, r;                                                         \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvand_i##bits(&solver, &a, &v1m, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r.symbolic, a.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a & a == a */                                                           \
    QCEExpr a, r;                                                              \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvand_i##bits(&solver, &a, &a, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r.symbolic, a.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(bvand)

#define QCE_UNIT_TEST_EXPR_bvor(bits)                                          \
  QCE_UNIT_TEST_EXPR_PROLOGUE(bvor_i##bits) {                                  \
    /* 1 | 2 == 3 */                                                           \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_bvor_i##bits(&solver, &v1, &v2, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 3);                                                  \
  }                                                                            \
  {                                                                            \
    /* -1 | (-3) == -1 */                                                      \
    QCEExpr v1m, v3m, r;                                                       \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v3m, -3);                                           \
    qce_expr_bvor_i##bits(&solver, &v1m, &v3m, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -1);                                                 \
  }                                                                            \
  {                                                                            \
    /* a | b == b | a */                                                       \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_bvor_i##bits(&solver, &a, &b, &r);                                \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(                                                   \
               &solver, qce_smt_z3_bv##bits##_eq(                              \
                            &solver, r.symbolic,                               \
                            qce_smt_z3_bv##bits##_bvor(&solver, b.symbolic,    \
                                                       a.symbolic))) ==        \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a | 0 == a */                                                           \
    QCEExpr v0, a, r;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvor_i##bits(&solver, &a, &v0, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r.symbolic, a.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a | -1 == -1 */                                                         \
    QCEExpr v1m, a, r;                                                         \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvor_i##bits(&solver, &a, &v1m, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -1);                                                 \
  }                                                                            \
  {                                                                            \
    /* a | a == a */                                                           \
    QCEExpr a, r;                                                              \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvor_i##bits(&solver, &a, &a, &r);                                \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r.symbolic, a.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(bvor)

#define QCE_UNIT_TEST_EXPR_bvxor(bits)                                         \
  QCE_UNIT_TEST_EXPR_PROLOGUE(bvxor_i##bits) {                                 \
    /* 1 ^ 2 == 3 */                                                           \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_bvxor_i##bits(&solver, &v1, &v2, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 3);                                                  \
  }                                                                            \
  {                                                                            \
    /* -1 ^ (-3) == 2 */                                                       \
    QCEExpr v1m, v3m, r;                                                       \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v3m, -3);                                           \
    qce_expr_bvxor_i##bits(&solver, &v1m, &v3m, &r);                           \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 2);                                                  \
  }                                                                            \
  {                                                                            \
    /* a ^ b == b ^ a */                                                       \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_bvxor_i##bits(&solver, &a, &b, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(                                                   \
               &solver, qce_smt_z3_bv##bits##_eq(                              \
                            &solver, r.symbolic,                               \
                            qce_smt_z3_bv##bits##_bvxor(&solver, b.symbolic,   \
                                                        a.symbolic))) ==       \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a ^ 0 == a */                                                           \
    QCEExpr v0, a, r;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvxor_i##bits(&solver, &a, &v0, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r.symbolic, a.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a ^ -1 == ~a */                                                         \
    QCEExpr v1m, a, r;                                                         \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvxor_i##bits(&solver, &a, &v1m, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(                                                                    \
        qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                    \
                                      &solver, r.symbolic,                     \
                                      Z3_mk_bvnot(solver.ctx, a.symbolic))) == \
        SMT_Z3_PROVE_PROVED);                                                  \
  }                                                                            \
  {                                                                            \
    /* a ^ a == 0 */                                                           \
    QCEExpr a, r;                                                              \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvxor_i##bits(&solver, &a, &a, &r);                               \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(bvxor)

#define QCE_UNIT_TEST_EXPR_bvnand(bits)                                        \
  QCE_UNIT_TEST_EXPR_PROLOGUE(bvnand_i##bits) {                                \
    /* 1 & 2 == -1 */                                                           \
    QCEExpr v1, v2, r;                                                         \
    qce_expr_init_v##bits(&v1, 1);                                             \
    qce_expr_init_v##bits(&v2, 2);                                             \
    qce_expr_bvnand_i##bits(&solver, &v1, &v2, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -1);                                                  \
  }                                                                            \
  {                                                                            \
    /* -1 & (-3) == 2 */                                                       \
    QCEExpr v1m, v3m, r;                                                       \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v3m, -3);                                           \
    qce_expr_bvnand_i##bits(&solver, &v1m, &v3m, &r);                          \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 2);                                                  \
  }                                                                            \
  {                                                                            \
    /* a & b == b & a */                                                       \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    qce_expr_bvnand_i##bits(&solver, &a, &b, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(                                                   \
               &solver, qce_smt_z3_bv##bits##_eq(                              \
                            &solver, r.symbolic,                               \
                            qce_smt_z3_bv##bits##_bvand(&solver, b.symbolic,   \
                                                        a.symbolic))) ==       \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a & 0 == -1 */                                                          \
    QCEExpr v0, a, r;                                                          \
    qce_expr_init_v##bits(&v0, 0);                                             \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvnand_i##bits(&solver, &a, &v0, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  {                                                                            \
    /* a & -1 == -a - 1 */                                                     \
    QCEExpr v1m, a, r;                                                         \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvnand_i##bits(&solver, &a, &v1m, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(&solver, r.symbolic,      \
                              qce_smt_z3_bv##bits##_add(&solver,               \
                                qce_smt_z3_bv##bits##_mul(&solver,             \
                                a.symbolic, qce_smt_z3_bv##bits##_value(&solver, v1m.v_i##bits)),\
                                qce_smt_z3_bv##bits##_value(&solver, v1m.v_i##bits)))) ==              \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* a & a == -a - 1 */                                                      \
    QCEExpr v1m, a, r;                                                         \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_bvnand_i##bits(&solver, &a, &a, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(&solver, r.symbolic,      \
                              qce_smt_z3_bv##bits##_add(&solver,               \
                                qce_smt_z3_bv##bits##_mul(&solver,             \
                                a.symbolic, qce_smt_z3_bv##bits##_value(&solver, v1m.v_i##bits)),\
                                qce_smt_z3_bv##bits##_value(&solver, v1m.v_i##bits)))) ==              \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(bvnand)
#endif

#endif /* QCE_EXPR_BIN_OP_H */