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

DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(bvand, &)
DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(bvor, |)
DEFINE_CONCRETE_BIN_OP_SIGNED_DUAL(bvxor, ^)

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

/*
 * Arithmetics
 */

DEFINE_EXPR_BIN_OP_DUAL(add)
DEFINE_EXPR_BIN_OP_DUAL(sub)

/*
 * Bitwise
 */

DEFINE_EXPR_BIN_OP_DUAL(bvand)
DEFINE_EXPR_BIN_OP_DUAL(bvor)
DEFINE_EXPR_BIN_OP_DUAL(bvxor)

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
#endif

#endif /* QCE_EXPR_BIN_OP_H */