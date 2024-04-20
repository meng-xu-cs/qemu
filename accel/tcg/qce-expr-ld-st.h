#ifndef QCE_EXPR_LD_ST_H
#define QCE_EXPR_LD_ST_H

/*
 * Utilities
 */

#define DEFINE_CONCRETE_BIN_OP_st(bits, n)                                     \
  static inline int##bits##_t __qce_concrete_bv##bits##_st##n(                 \
      int##bits##_t src, int##bits##_t dst) {                                  \
    *(uint##n##_t *)&dst = (uint##n##_t)src;                                   \
    return dst;                                                                \
  }

#define DEFINE_CONCRETE_UNI_OP_ld_u(bits, n)                                   \
  static inline uint##bits##_t __qce_concrete_bv##bits##_ld##n##u(             \
      uint##bits##_t val) {                                                    \
    return (uint##n##_t)val;                                                   \
  }

#define DEFINE_CONCRETE_UNI_OP_ld_s(bits, n)                                   \
  static inline int##bits##_t __qce_concrete_bv##bits##_ld##n##s(              \
      int##bits##_t val) {                                                     \
    return (int##n##_t)val;                                                    \
  }

DEFINE_CONCRETE_BIN_OP_st(32, 8);
DEFINE_CONCRETE_BIN_OP_st(32, 16);
DEFINE_CONCRETE_BIN_OP_st(64, 8);
DEFINE_CONCRETE_BIN_OP_st(64, 16);
DEFINE_CONCRETE_BIN_OP_st(64, 32);

DEFINE_CONCRETE_UNI_OP_ld_u(32, 8);
DEFINE_CONCRETE_UNI_OP_ld_s(32, 8);
DEFINE_CONCRETE_UNI_OP_ld_u(32, 16);
DEFINE_CONCRETE_UNI_OP_ld_s(32, 16);
DEFINE_CONCRETE_UNI_OP_ld_u(64, 8);
DEFINE_CONCRETE_UNI_OP_ld_s(64, 8);
DEFINE_CONCRETE_UNI_OP_ld_u(64, 16);
DEFINE_CONCRETE_UNI_OP_ld_s(64, 16);
DEFINE_CONCRETE_UNI_OP_ld_u(64, 32);
DEFINE_CONCRETE_UNI_OP_ld_s(64, 32);

/*
 * Partial load and store
 */

DEFINE_EXPR_BIN_OP_DUAL(st8)
DEFINE_EXPR_BIN_OP_DUAL(st16)
DEFINE_EXPR_BIN_OP(64, st32)

DEFINE_EXPR_UNI_OP_DUAL(ld8u)
DEFINE_EXPR_UNI_OP_DUAL(ld8s)
DEFINE_EXPR_UNI_OP_DUAL(ld16u)
DEFINE_EXPR_UNI_OP_DUAL(ld16s)
DEFINE_EXPR_UNI_OP(64, ld32u)
DEFINE_EXPR_UNI_OP(64, ld32s)

/*
 * Testing
 */

#ifndef QCE_RELEASE
#define QCE_UNIT_TEST_EXPR_st8(bits)                                           \
  QCE_UNIT_TEST_EXPR_PROLOGUE(st8_i##bits) {                                   \
    /* st 0x1 to 0x2 == 0x1 */                                                 \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x1);                                          \
    qce_expr_init_v##bits(&dst, 0x2);                                          \
    qce_expr_st8_i##bits(&solver, &src, &dst, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x1);                                                \
  }                                                                            \
  {                                                                            \
    /* st 0x2 to 0x100 == 0x102 */                                             \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x2);                                          \
    qce_expr_init_v##bits(&dst, 0x100);                                        \
    qce_expr_st8_i##bits(&solver, &src, &dst, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x102);                                              \
  }                                                                            \
  {                                                                            \
    /* st -1 to 0x100 == 0x1FF */                                              \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, -1);                                           \
    qce_expr_init_v##bits(&dst, 0x100);                                        \
    qce_expr_st8_i##bits(&solver, &src, &dst, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x1FF);                                              \
  }                                                                            \
  {                                                                            \
    /* st 0 to -1 == 0xFF...00 */                                              \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0);                                            \
    qce_expr_init_v##bits(&dst, -1);                                           \
    qce_expr_st8_i##bits(&solver, &src, &dst, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -(1l << 8));                                         \
  }                                                                            \
  {                                                                            \
    /* st x to 0x100 |r1|, then st 0xF to r1 == 0x10F */                       \
    QCEExpr x, v0x100, r1;                                                     \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_v##bits(&v0x100, 0x100);                                     \
    qce_expr_st8_i##bits(&solver, &x, &v0x100, &r1);                           \
    assert(r1.type == QCE_EXPR_I##bits);                                       \
    assert(r1.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    QCEExpr v0xF, r2;                                                          \
    qce_expr_init_v##bits(&v0xF, 0xF);                                         \
    qce_expr_st8_i##bits(&solver, &v0xF, &r1, &r2);                            \
    assert(r2.type == QCE_EXPR_I##bits);                                       \
    assert(r2.mode == QCE_EXPR_CONCRETE);                                      \
    assert(r2.v_i##bits == 0x10F);                                             \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(st8)

#define QCE_UNIT_TEST_EXPR_st16(bits)                                          \
  QCE_UNIT_TEST_EXPR_PROLOGUE(st16_i##bits) {                                  \
    /* st 0x1 to 0x2 == 0x1 */                                                 \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x1);                                          \
    qce_expr_init_v##bits(&dst, 0x2);                                          \
    qce_expr_st16_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x1);                                                \
  }                                                                            \
  {                                                                            \
    /* st 0x2 to 0x100 == 0x2 */                                               \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x2);                                          \
    qce_expr_init_v##bits(&dst, 0x100);                                        \
    qce_expr_st16_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x2);                                                \
  }                                                                            \
  {                                                                            \
    /* st 0x3 to 0x40000 == 0x40003 */                                         \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x3);                                          \
    qce_expr_init_v##bits(&dst, 0x40000);                                      \
    qce_expr_st16_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x40003);                                            \
  }                                                                            \
  {                                                                            \
    /* st -1 to 0x10000 == 0x1FFFF */                                          \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, -1);                                           \
    qce_expr_init_v##bits(&dst, 0x10000);                                      \
    qce_expr_st16_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x1FFFF);                                            \
  }                                                                            \
  {                                                                            \
    /* st 0 to -1 == 0xFF...0000 */                                            \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0);                                            \
    qce_expr_init_v##bits(&dst, -1);                                           \
    qce_expr_st16_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -(1l << 16));                                        \
  }                                                                            \
  {                                                                            \
    /* st x to 0x10000 |r1|, then st 0xF to r1 == 0x1000F */                   \
    QCEExpr x, v0x10000, r1;                                                   \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_v##bits(&v0x10000, 0x10000);                                 \
    qce_expr_st16_i##bits(&solver, &x, &v0x10000, &r1);                        \
    assert(r1.type == QCE_EXPR_I##bits);                                       \
    assert(r1.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    QCEExpr v0xF, r2;                                                          \
    qce_expr_init_v##bits(&v0xF, 0xF);                                         \
    qce_expr_st16_i##bits(&solver, &v0xF, &r1, &r2);                           \
    assert(r2.type == QCE_EXPR_I##bits);                                       \
    assert(r2.mode == QCE_EXPR_CONCRETE);                                      \
    assert(r2.v_i##bits == 0x1000F);                                           \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_DEF_DUAL(st16)

#define QCE_UNIT_TEST_EXPR_st32(bits)                                          \
  QCE_UNIT_TEST_EXPR_PROLOGUE(st32_i##bits) {                                  \
    /* st 0x1 to 0x2 == 0x1 */                                                 \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x1);                                          \
    qce_expr_init_v##bits(&dst, 0x2);                                          \
    qce_expr_st32_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x1);                                                \
  }                                                                            \
  {                                                                            \
    /* st 0x2 to 0x100 == 0x2 */                                               \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x2);                                          \
    qce_expr_init_v##bits(&dst, 0x100);                                        \
    qce_expr_st32_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x2);                                                \
  }                                                                            \
  {                                                                            \
    /* st 0x3 to 0x40000 == 0x3 */                                             \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x3);                                          \
    qce_expr_init_v##bits(&dst, 0x40000);                                      \
    qce_expr_st32_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x3);                                                \
  }                                                                            \
  {                                                                            \
    /* st 0x4 to 0x300000000 == 0x300000004 */                                 \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0x4);                                          \
    qce_expr_init_v##bits(&dst, 0x300000000);                                  \
    qce_expr_st32_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x300000004);                                        \
  }                                                                            \
  {                                                                            \
    /* st -1 to 0x100000000 == 0x1FFFFFFFF */                                  \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, -1);                                           \
    qce_expr_init_v##bits(&dst, 0x100000000);                                  \
    qce_expr_st32_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x1FFFFFFFF);                                        \
  }                                                                            \
  {                                                                            \
    /* st 0 to -1 == 0xFF...00000000 */                                        \
    QCEExpr src, dst, r;                                                       \
    qce_expr_init_v##bits(&src, 0);                                            \
    qce_expr_init_v##bits(&dst, -1);                                           \
    qce_expr_st32_i##bits(&solver, &src, &dst, &r);                            \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -(1l << 32));                                        \
  }                                                                            \
  {                                                                            \
    /* st x to 0x100000000 |r1|, then st 0xF to r1 == 0x10000000F */           \
    QCEExpr x, v0x100000000, r1;                                               \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_v##bits(&v0x100000000, 0x100000000);                         \
    qce_expr_st32_i##bits(&solver, &x, &v0x100000000, &r1);                    \
    assert(r1.type == QCE_EXPR_I##bits);                                       \
    assert(r1.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    QCEExpr v0xF, r2;                                                          \
    qce_expr_init_v##bits(&v0xF, 0xF);                                         \
    qce_expr_st32_i##bits(&solver, &v0xF, &r1, &r2);                           \
    assert(r2.type == QCE_EXPR_I##bits);                                       \
    assert(r2.mode == QCE_EXPR_CONCRETE);                                      \
    assert(r2.v_i##bits == 0x10000000F);                                       \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_st32(64);

#define QCE_UNIT_TEST_EXPR_st_symbolic(bits, n)                                \
  QCE_UNIT_TEST_EXPR_PROLOGUE(st##n##_symbolic_i##bits) {                      \
    /* st x to x == x */                                                       \
    QCEExpr x, r;                                                              \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_st##n##_i##bits(&solver, &x, &x, &r);                             \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_SYMBOLIC);                                       \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r.symbolic, x.symbolic)) ==  \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* st x to y |r1|, then st y to r1 == y */                                 \
    QCEExpr x, y, r1;                                                          \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
    qce_expr_st##n##_i##bits(&solver, &x, &y, &r1);                            \
    assert(r1.type == QCE_EXPR_I##bits);                                       \
    assert(r1.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    QCEExpr r2;                                                                \
    qce_expr_st##n##_i##bits(&solver, &y, &r1, &r2);                           \
    assert(r2.type == QCE_EXPR_I##bits);                                       \
    assert(r2.mode == QCE_EXPR_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r2.symbolic, y.symbolic)) == \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  {                                                                            \
    /* st x to y |r1|, then st r1 to x == x */                                 \
    QCEExpr x, y, r1;                                                          \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_init_s##bits(&solver, &y);                                        \
    qce_expr_st##n##_i##bits(&solver, &x, &y, &r1);                            \
    assert(r1.type == QCE_EXPR_I##bits);                                       \
    assert(r1.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    QCEExpr r2;                                                                \
    qce_expr_st##n##_i##bits(&solver, &r1, &x, &r2);                           \
    assert(r2.type == QCE_EXPR_I##bits);                                       \
    assert(r2.mode == QCE_EXPR_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r2.symbolic, x.symbolic)) == \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_st_symbolic(32, 8);
QCE_UNIT_TEST_EXPR_st_symbolic(32, 16);
QCE_UNIT_TEST_EXPR_st_symbolic(64, 8);
QCE_UNIT_TEST_EXPR_st_symbolic(64, 16);
QCE_UNIT_TEST_EXPR_st_symbolic(64, 32);

#define QCE_UNIT_TEST_EXPR_ld_common(bits, n, sign)                            \
  QCE_UNIT_TEST_EXPR_PROLOGUE(ld##n##sign##_common_i##bits) {                  \
    /* ld 0 == 0 */                                                            \
    QCEExpr val, r;                                                            \
    qce_expr_init_v##bits(&val, 0);                                            \
    qce_expr_ld##n##sign##_i##bits(&solver, &val, &r);                         \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0);                                                  \
  }                                                                            \
  {                                                                            \
    /* ld 0x1 == 0x1 */                                                        \
    QCEExpr val, r;                                                            \
    qce_expr_init_v##bits(&val, 0x1);                                          \
    qce_expr_ld##n##sign##_i##bits(&solver, &val, &r);                         \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == 0x1);                                                \
  }                                                                            \
  {                                                                            \
    /* ld (1 << (n-1)) - 1 == self */                                          \
    QCEExpr val, r;                                                            \
    qce_expr_init_v##bits(&val, (1l << (n - 1)) - 1);                          \
    qce_expr_ld##n##sign##_i##bits(&solver, &val, &r);                         \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == val.v_i##bits);                                      \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_ld_common(32, 8, u);
QCE_UNIT_TEST_EXPR_ld_common(32, 8, s);
QCE_UNIT_TEST_EXPR_ld_common(32, 16, u);
QCE_UNIT_TEST_EXPR_ld_common(32, 16, s);
QCE_UNIT_TEST_EXPR_ld_common(64, 8, u);
QCE_UNIT_TEST_EXPR_ld_common(64, 8, s);
QCE_UNIT_TEST_EXPR_ld_common(64, 16, u);
QCE_UNIT_TEST_EXPR_ld_common(64, 16, s);
QCE_UNIT_TEST_EXPR_ld_common(64, 32, u);
QCE_UNIT_TEST_EXPR_ld_common(64, 32, s);

#define QCE_UNIT_TEST_EXPR_ld_u(bits, n)                                       \
  QCE_UNIT_TEST_EXPR_PROLOGUE(ld##n##u_i##bits) {                              \
    /* ld_u -1 == (1 << n) - 1 */                                              \
    QCEExpr val, r;                                                            \
    qce_expr_init_v##bits(&val, -1);                                           \
    qce_expr_ld##n##u_i##bits(&solver, &val, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == (1l << n) - 1);                                      \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_ld_u(32, 8);
QCE_UNIT_TEST_EXPR_ld_u(32, 16);
QCE_UNIT_TEST_EXPR_ld_u(64, 8);
QCE_UNIT_TEST_EXPR_ld_u(64, 16);
QCE_UNIT_TEST_EXPR_ld_u(64, 32);

#define QCE_UNIT_TEST_EXPR_ld_s(bits, n)                                       \
  QCE_UNIT_TEST_EXPR_PROLOGUE(ld##n##s_i##bits) {                              \
    /* ld_s -1 == -1 */                                                        \
    QCEExpr val, r;                                                            \
    qce_expr_init_v##bits(&val, -1);                                           \
    qce_expr_ld##n##s_i##bits(&solver, &val, &r);                              \
    assert(r.type == QCE_EXPR_I##bits);                                        \
    assert(r.mode == QCE_EXPR_CONCRETE);                                       \
    assert(r.v_i##bits == -1);                                                 \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_ld_s(32, 8);
QCE_UNIT_TEST_EXPR_ld_s(32, 16);
QCE_UNIT_TEST_EXPR_ld_s(64, 8);
QCE_UNIT_TEST_EXPR_ld_s(64, 16);
QCE_UNIT_TEST_EXPR_ld_s(64, 32);

#define QCE_UNIT_TEST_EXPR_special_ld_then_st(bits, n, sign)                   \
  QCE_UNIT_TEST_EXPR_PROLOGUE(special_ld_then_st##n##sign##_i##bits) {         \
    /* ld x |r1|; st r1 to x == x */                                           \
    QCEExpr x, r1;                                                             \
    qce_expr_init_s##bits(&solver, &x);                                        \
    qce_expr_ld##n##sign##_i##bits(&solver, &x, &r1);                          \
    assert(r1.type == QCE_EXPR_I##bits);                                       \
    assert(r1.mode == QCE_EXPR_SYMBOLIC);                                      \
                                                                               \
    QCEExpr r2;                                                                \
    qce_expr_st##n##_i##bits(&solver, &r1, &x, &r2);                           \
    assert(r2.type == QCE_EXPR_I##bits);                                       \
    assert(r2.mode == QCE_EXPR_SYMBOLIC);                                      \
    assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv##bits##_eq(                 \
                                         &solver, r2.symbolic, x.symbolic)) == \
           SMT_Z3_PROVE_PROVED);                                               \
  }                                                                            \
  QCE_UNIT_TEST_EXPR_EPILOGUE
QCE_UNIT_TEST_EXPR_special_ld_then_st(32, 8, u);
QCE_UNIT_TEST_EXPR_special_ld_then_st(32, 8, s);
QCE_UNIT_TEST_EXPR_special_ld_then_st(32, 16, u);
QCE_UNIT_TEST_EXPR_special_ld_then_st(32, 16, s);
QCE_UNIT_TEST_EXPR_special_ld_then_st(64, 8, u);
QCE_UNIT_TEST_EXPR_special_ld_then_st(64, 8, s);
QCE_UNIT_TEST_EXPR_special_ld_then_st(64, 16, u);
QCE_UNIT_TEST_EXPR_special_ld_then_st(64, 16, s);
QCE_UNIT_TEST_EXPR_special_ld_then_st(64, 32, u);
QCE_UNIT_TEST_EXPR_special_ld_then_st(64, 32, s);
#endif

#endif /* QCE_EXPR_LD_ST_H */