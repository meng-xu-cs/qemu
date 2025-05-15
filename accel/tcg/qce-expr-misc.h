#ifndef QCE_EXPR_MISC_H
#define QCE_EXPR_MISC_H

/*
 * Utilities
 */

#define DEFINE_CONCRETE_deposit(bits)                                          \
  static inline int##bits##_t __qce_concrete_bv##bits##_deposit(               \
      int##bits##_t into, int##bits##_t from, tcg_target_ulong pos,            \
      tcg_target_ulong len) {                                                  \
    int##bits##_t mask = len < bits ? ((1 << len) - 1) << pos : -1;            \
    return (into & ~mask) | ((from << pos) & mask);                            \
  }

DEFINE_CONCRETE_deposit(32)
DEFINE_CONCRETE_deposit(64)

#define DEFINE_CONCRETE_extract2(bits)                                         \
  static inline int##bits##_t __qce_concrete_bv##bits##_extract2(              \
      int##bits##_t v_b, int##bits##_t v_t, tcg_target_ulong pos) {            \
    return (v_t << (bits-pos)) | ((uint##bits##_t)v_b >> pos);                 \
  }

DEFINE_CONCRETE_extract2(32)
DEFINE_CONCRETE_extract2(64)

/*
 * Templates
 */

#define DEFINE_EXPR_deposit(bits)                                              \
  static inline void qce_expr_deposit_i##bits(                                 \
      SolverZ3 *solver, QCEExpr *into, QCEExpr *from, tcg_target_ulong pos,    \
      tcg_target_ulong len, QCEExpr *result) {                                 \
    /* type checking */                                                        \
    qce_expr_assert_type(into, I##bits);                                       \
    qce_expr_assert_type(from, I##bits);                                       \
    result->type = QCE_EXPR_I##bits;                                           \
                                                                               \
    /* base assignment */                                                      \
    if (into->mode == QCE_EXPR_CONCRETE) {                                     \
      if (from->mode == QCE_EXPR_CONCRETE) {                                   \
        result->mode = QCE_EXPR_CONCRETE;                                      \
        result->v_i##bits = __qce_concrete_bv##bits##_deposit(                 \
            into->v_i##bits, from->v_i##bits, pos, len);                       \
      } else {                                                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_deposit(                      \
            solver, qce_smt_z3_bv##bits##_value(solver, into->v_i##bits),      \
            from->symbolic, pos, len);                                         \
      }                                                                        \
    } else {                                                                   \
      if (from->mode == QCE_EXPR_CONCRETE) {                                   \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_deposit(                      \
            solver, into->symbolic,                                            \
            qce_smt_z3_bv##bits##_value(solver, from->v_i##bits), pos, len);   \
      } else {                                                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_deposit(                      \
            solver, into->symbolic, from->symbolic, pos, len);                 \
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

DEFINE_EXPR_deposit(32)
DEFINE_EXPR_deposit(64)

#define DEFINE_EXPR_extract2(bits)                                             \
  static inline void qce_expr_extract2_i##bits(                                \
      SolverZ3 *solver, QCEExpr *v_low, QCEExpr *v_high, tcg_target_ulong pos, \
      QCEExpr *result) {                                                       \
    /* type checking */                                                        \
    qce_expr_assert_type(v_low, I##bits);                                      \
    qce_expr_assert_type(v_high, I##bits);                                     \
    result->type = QCE_EXPR_I##bits;                                           \
                                                                               \
    /* base assignment */                                                      \
    if (v_low->mode == QCE_EXPR_CONCRETE) {                                    \
      if (v_high->mode == QCE_EXPR_CONCRETE) {                                 \
        result->mode = QCE_EXPR_CONCRETE;                                      \
        result->v_i##bits = __qce_concrete_bv##bits##_extract2(                \
            v_low->v_i##bits, v_high->v_i##bits, pos);                         \
      } else {                                                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_extract2(                     \
            solver, qce_smt_z3_bv##bits##_value(solver, v_low->v_i##bits),     \
            v_high->symbolic, pos);                                            \
      }                                                                        \
    } else {                                                                   \
      if (v_high->mode == QCE_EXPR_CONCRETE) {                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_extract2(                     \
            solver, v_low->symbolic,                                           \
            qce_smt_z3_bv##bits##_value(solver, v_high->v_i##bits), pos);      \
      } else {                                                                 \
        result->mode = QCE_EXPR_SYMBOLIC;                                      \
        result->symbolic = qce_smt_z3_bv##bits##_extract2(                     \
            solver, v_low->symbolic, v_high->symbolic, pos);                   \
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

DEFINE_EXPR_extract2(32)
DEFINE_EXPR_extract2(64)

/*
 * Testing
 */

#ifndef QCE_RELEASE
#define QCE_UNIT_TEST_EXPR_deposit(bits)                                       \
  QCE_UNIT_TEST_EXPR_PROLOGUE(deposit_i##bits) {                               \
    /* deposit(-1, 2, 8, 4) == 0xf...f2ff */                                   \
    QCEExpr v1m, v2, r;                                                        \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v2, 2);                                             \
    tcg_target_ulong pos = 8, len = 4;                                         \
    qce_expr_deposit_i##bits(&solver, &v1m, &v2, pos, len, &r);                \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == (int##bits##_t)(int16_t)0xf2ff);                     \
  }                                                                            \
  {                                                                            \
    /* deposit(-1, 2, 0, bits) == 2 */                                         \
    QCEExpr v1m, v2, r;                                                        \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    qce_expr_init_v##bits(&v2, 2);                                             \
    tcg_target_ulong pos = 0, len = bits;                                      \
    qce_expr_deposit_i##bits(&solver, &v1m, &v2, pos, len, &r);                \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 2);                                                  \
  }                                                                            \
  {                                                                            \
    /* deposit(a, a, 0, 4) == a */                                             \
    QCEExpr a, r;                                                              \
    qce_expr_init_s##bits(&solver, &a);                                        \
    tcg_target_ulong pos = 0, len = 4;                                         \
    qce_expr_deposit_i##bits(&solver, &a, &a, pos, len, &r);                   \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r.symbolic, a.symbolic)) ==           \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* deposit(a, b, 0, bits) == b */                                          \
    QCEExpr a, b, r;                                                           \
    qce_expr_init_s##bits(&solver, &a);                                        \
    qce_expr_init_s##bits(&solver, &b);                                        \
    tcg_target_ulong pos = 0, len = bits;                                      \
    qce_expr_deposit_i##bits(&solver, &a, &b, pos, len, &r);                   \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r.symbolic, b.symbolic)) ==           \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(deposit)

#define QCE_UNIT_TEST_EXPR_extract2(bits)                                      \
  QCE_UNIT_TEST_EXPR_PROLOGUE(extract2_i##bits) {                              \
    /* extract2(-1, -1, 8) == -1 */                                            \
    QCEExpr v1m, r;                                                            \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    tcg_target_ulong pos = 8;                                                  \
    qce_expr_extract2_i##bits(&solver, &v1m, &v1m, pos, &r);                   \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -1);                                                 \
  }                                                                            \
  {                                                                            \
    /* extract2(0x12345678, 0, 8) == 0x123456 */                               \
    QCEExpr v_b, v_t, r;                                                       \
    qce_expr_init_v##bits(&v_b, 0x12345678);                                   \
    qce_expr_init_v##bits(&v_t, 0);                                            \
    tcg_target_ulong pos = 8;                                                  \
    qce_expr_extract2_i##bits(&solver, &v_b, &v_t, pos, &r);                   \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x123456);                                           \
  }                                                                            \
  {                                                                            \
    /* extract2(0xABCDEF, 0x123456, 24) == 0x123456 << (bits - 24)  */         \
    QCEExpr v_b, v_t, r;                                                       \
    qce_expr_init_v##bits(&v_b, 0xABCDEF);                                     \
    qce_expr_init_v##bits(&v_t, 0x123456);                                     \
    tcg_target_ulong pos = 24;                                                 \
    qce_expr_extract2_i##bits(&solver, &v_b, &v_t, pos, &r);                   \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == (int##bits##_t)0x123456<<(bits-24));                 \
  }                                                                            \
  {                                                                            \
    /* extract2(a, a, 15) == rotr(a, 15) */                                    \
    QCEExpr a, r;                                                              \
    qce_expr_init_s##bits(&solver, &a);                                        \
    tcg_target_ulong pos = 15;                                                 \
    qce_expr_extract2_i##bits(&solver, &a, &a, pos, &r);                       \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, r.symbolic,                           \
                                qce_smt_z3_bv##bits##_rotr(                    \
                                    &solver, a.symbolic,                       \
                                    qce_smt_z3_bv##bits##_value(&solver,       \
                                                                pos)))) ==     \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(extract2)
#endif

#endif /* QCE_EXPR_MISC_H */
