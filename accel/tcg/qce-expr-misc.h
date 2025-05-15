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

#define DEFINE_CONCRETE_extract(bits)                                          \
  static inline int##bits##_t __qce_concrete_bv##bits##_extract(               \
      int##bits##_t val, tcg_target_ulong pos, tcg_target_ulong len) {         \
    return (uint##bits##_t)val << (bits-pos-len) >> (bits-len);                \
  }

#define DEFINE_CONCRETE_sextract(bits)                                         \
  static inline int##bits##_t __qce_concrete_bv##bits##_sextract(              \
      int##bits##_t val, tcg_target_ulong pos, tcg_target_ulong len) {         \
    return (int##bits##_t)val << (bits-pos-len) >> (bits-len);                 \
  }

#define DEFINE_CONCRETE_extract2(bits)                                         \
  static inline int##bits##_t __qce_concrete_bv##bits##_extract2(              \
      int##bits##_t v_b, int##bits##_t v_t, tcg_target_ulong pos) {            \
    return (v_t << (bits-pos)) | ((uint##bits##_t)v_b >> pos);                 \
  }

DEFINE_CONCRETE_extract(32)
DEFINE_CONCRETE_extract(64)
DEFINE_CONCRETE_sextract(32)
DEFINE_CONCRETE_sextract(64)
DEFINE_CONCRETE_extract2(32)
DEFINE_CONCRETE_extract2(64)

static inline int32_t __qce_concrete_extrl_i64_i32(int64_t val) {
    return (int32_t)val;
}

static inline int32_t __qce_concrete_extrh_i64_i32(int64_t val) {
  return (int32_t)(val >> 32);
}

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

#define DEFINE_EXPR_extract(name, bits)                                        \
  static inline void qce_expr_##name##_i##bits(                                \
      SolverZ3 *solver, QCEExpr *opv, tcg_target_ulong pos,                    \
      tcg_target_ulong len, QCEExpr *result) {                                 \
    /* type checking */                                                        \
    qce_expr_assert_type(opv, I##bits);                                        \
    result->type = QCE_EXPR_I##bits;                                           \
                                                                               \
    /* base assignment */                                                      \
    if (opv->mode == QCE_EXPR_CONCRETE) {                                      \
      result->mode = QCE_EXPR_CONCRETE;                                        \
      result->v_i##bits =                                                      \
          __qce_concrete_bv##bits##_##name(opv->v_i##bits, pos, len);          \
    } else {                                                                   \
      result->mode = QCE_EXPR_SYMBOLIC;                                        \
      result->symbolic =                                                       \
          qce_smt_z3_bv##bits##_##name(solver, opv->symbolic, pos, len);       \
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

DEFINE_EXPR_extract(extract, 32)
DEFINE_EXPR_extract(extract, 64)
DEFINE_EXPR_extract(sextract, 32)
DEFINE_EXPR_extract(sextract, 64)
DEFINE_EXPR_extract2(32)
DEFINE_EXPR_extract2(64)

#define DEFINE_EXPR_extr_i64_i32(side)                                         \
  static inline void qce_expr_extr##side##_i64_i32(                            \
      SolverZ3 *solver, QCEExpr *opv, QCEExpr *result) {                       \
    /* type checking */                                                        \
    qce_expr_assert_type(opv, I64);                                            \
    result->type = QCE_EXPR_I32;                                               \
                                                                               \
    /* base assignment */                                                      \
    if (opv->mode == QCE_EXPR_CONCRETE) {                                      \
      result->mode = QCE_EXPR_CONCRETE;                                        \
      result->v_i32 = __qce_concrete_extr##side##_i64_i32(opv->v_i64);         \
    } else {                                                                   \
      result->mode = QCE_EXPR_SYMBOLIC;                                        \
      result->symbolic =                                                       \
          qce_smt_z3_extr##side##_i64_i32(solver, opv->symbolic);              \
    }                                                                          \
                                                                               \
    /* try to reduce symbolic to concrete */                                   \
    if (result->mode == QCE_EXPR_SYMBOLIC) {                                   \
      uint32_t val = 0;                                                        \
      if (qce_smt_z3_probe_bv32(solver, result->symbolic, &val)) {             \
        result->mode = QCE_EXPR_CONCRETE;                                      \
        result->v_i32 = val;                                                   \
      }                                                                        \
    }                                                                          \
  }

DEFINE_EXPR_extr_i64_i32(l)
DEFINE_EXPR_extr_i64_i32(h)

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


#define QCE_UNIT_TEST_EXPR_extract(bits)                                       \
  QCE_UNIT_TEST_EXPR_PROLOGUE(extract_i##bits) {                               \
    /* extract(-1, 8, 4) == 0xF */                                             \
    QCEExpr v1m, r;                                                            \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    tcg_target_ulong pos = 8, len = 4;                                         \
    qce_expr_extract_i##bits(&solver, &v1m, pos, len, &r);                     \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0xF);                                                \
  }                                                                            \
  {                                                                            \
    /* extract(0xF123ABC, 12, 12) == 0x123 */                                  \
    QCEExpr v, r;                                                              \
    qce_expr_init_v##bits(&v, 0xF123ABC);                                      \
    tcg_target_ulong pos = 12, len = 12;                                       \
    qce_expr_extract_i##bits(&solver, &v, pos, len, &r);                       \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x123);                                              \
  }                                                                            \
  {                                                                            \
    /* extract(0xFFF, 4, 4) == 0xF */                                          \
    QCEExpr v, r;                                                              \
    qce_expr_init_v##bits(&v, 0xFFF);                                          \
    tcg_target_ulong pos = 4, len = 4;                                         \
    qce_expr_extract_i##bits(&solver, &v, pos, len, &r);                       \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0xF);                                                \
  }                                                                            \
  {                                                                            \
    /* extract(a, bits/2, bits/2) << bits/2 + extract(a, 0, bits/2) == a */    \
    QCEExpr a, r_t, r_b;                                                       \
    qce_expr_init_s##bits(&solver, &a);                                        \
    tcg_target_ulong pos_t = bits/2, pos_b = 0, len = pos_t;                   \
    qce_expr_extract_i##bits(&solver, &a, pos_t, len, &r_t);                   \
    qce_expr_extract_i##bits(&solver, &a, pos_b, len, &r_b);                   \
    assert(r_t.type == QCE_EXPR_I##bits);                                      \
    assert(r_b.type == QCE_EXPR_I##bits);                                      \
    assert(r_t.mode == QCE_EXPR_SYMBOLIC);                                     \
    assert(r_b.mode == QCE_EXPR_SYMBOLIC);                                     \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, a.symbolic,                           \
                                qce_smt_z3_bv##bits##_add(&solver,             \
                                    qce_smt_z3_bv##bits##_shl(                 \
                                        &solver, r_t.symbolic,                 \
                                        qce_smt_z3_bv##bits##_value(&solver,   \
                                                                    pos_t)),   \
                                    r_b.symbolic))) ==                         \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(extract)

#define QCE_UNIT_TEST_EXPR_sextract(bits)                                      \
  QCE_UNIT_TEST_EXPR_PROLOGUE(sextract_i##bits) {                              \
    /* sextract(-1, 8, 4) == -1 */                                             \
    QCEExpr v1m, r;                                                            \
    qce_expr_init_v##bits(&v1m, -1);                                           \
    tcg_target_ulong pos = 8, len = 4;                                         \
    qce_expr_sextract_i##bits(&solver, &v1m, pos, len, &r);                    \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -1);                                                 \
  }                                                                            \
  {                                                                            \
    /* sextract(0xF123ABC, 12, 12) == 0x123 */                                 \
    QCEExpr v, r;                                                              \
    qce_expr_init_v##bits(&v, 0xF123ABC);                                      \
    tcg_target_ulong pos = 12, len = 12;                                       \
    qce_expr_sextract_i##bits(&solver, &v, pos, len, &r);                      \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x123);                                              \
  }                                                                            \
  {                                                                            \
    /* sextract(0xFF, 4, 4) == -1 */                                           \
    QCEExpr v, r;                                                              \
    qce_expr_init_v##bits(&v, 0xFF);                                           \
    tcg_target_ulong pos = 4, len = 4;                                         \
    qce_expr_sextract_i##bits(&solver, &v, pos, len, &r);                      \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -1);                                                 \
  }                                                                            \
  {                                                                            \
    /* sextract(a, bits/2, bits/2) << bits/2 + extract(a, 0, bits/2) == a */   \
    QCEExpr a, r_t, r_b;                                                       \
    qce_expr_init_s##bits(&solver, &a);                                        \
    tcg_target_ulong pos_t = bits/2, pos_b = 0, len = pos_t;                   \
    qce_expr_sextract_i##bits(&solver, &a, pos_t, len, &r_t);                  \
    qce_expr_extract_i##bits(&solver, &a, pos_b, len, &r_b);                   \
    assert(r_t.type == QCE_EXPR_I##bits);                                      \
    assert(r_b.type == QCE_EXPR_I##bits);                                      \
    assert(r_t.mode == QCE_EXPR_SYMBOLIC);                                     \
    assert(r_b.mode == QCE_EXPR_SYMBOLIC);                                     \
    assert(qce_smt_z3_prove(&solver,                                           \
                            qce_smt_z3_bv##bits##_eq(                          \
                                &solver, a.symbolic,                           \
                                qce_smt_z3_bv##bits##_add(&solver,             \
                                    qce_smt_z3_bv##bits##_shl(                 \
                                        &solver, r_t.symbolic,                 \
                                        qce_smt_z3_bv##bits##_value(&solver,   \
                                                                    pos_t)),   \
                                    r_b.symbolic))) ==                         \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(sextract)

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

QCE_UNIT_TEST_EXPR_PROLOGUE(extr_i64_i32) {
  /* extrl(-1) == -1 */
  /* extrh(-1) == -1 */
  QCEExpr v1m, r_l, r_h;
  qce_expr_init_v64(&v1m, -1);
  qce_expr_extrl_i64_i32(&solver, &v1m, &r_l);
  qce_expr_extrh_i64_i32(&solver, &v1m, &r_h);
  assert(r_l.type == QCE_EXPR_I32);
  assert(r_l.mode == QCE_EXPR_CONCRETE);
  assert(r_l.v_i32 == -1);
  assert(r_h.type == QCE_EXPR_I32);
  assert(r_h.mode == QCE_EXPR_CONCRETE);
  assert(r_h.v_i32 == -1);
}
{
  /* extrl(0xFFFFFFFF12345678) == 0x12345678 */
  /* extrr(0xFFFFFFFF12345678) == -1 */
  QCEExpr v, r_l, r_h;
  qce_expr_init_v64(&v, 0xFFFFFFFF12345678);
  qce_expr_extrl_i64_i32(&solver, &v, &r_l);
  qce_expr_extrh_i64_i32(&solver, &v, &r_h);
  assert(r_l.type == QCE_EXPR_I32);
  assert(r_l.mode == QCE_EXPR_CONCRETE);
  assert(r_l.v_i32 == 0x12345678);
  assert(r_h.type == QCE_EXPR_I32);
  assert(r_h.mode == QCE_EXPR_CONCRETE);
  assert(r_h.v_i32 == -1);
}
{
  /* concat(extrh(a), extrl(a)) == a */
  QCEExpr a, r_l, r_h;
  qce_expr_init_s64(&solver, &a);
  qce_expr_extrl_i64_i32(&solver, &a, &r_l);
  qce_expr_extrh_i64_i32(&solver, &a, &r_h);
  assert(r_l.type == QCE_EXPR_I32);
  assert(r_l.mode == QCE_EXPR_SYMBOLIC);
  assert(r_h.type == QCE_EXPR_I32);
  assert(r_h.mode == QCE_EXPR_SYMBOLIC);
  assert(qce_smt_z3_prove(&solver,
                          qce_smt_z3_bv64_eq(
                              &solver, a.symbolic,
                              qce_smt_z3_bv64_concat(&solver,r_h.symbolic,
                                                     r_l.symbolic))) ==
         SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

#endif

#endif /* QCE_EXPR_MISC_H */
