#ifndef QCE_SYM_CALL_H
#define QCE_SYM_CALL_H

#include "exec/helper-proto.h"
#include "tcg/tcg-gvec-desc.h"

#define DEFINE_SYM_INST_CALL_cc_compute(name)                                  \
  static inline void qce_sym_inst_call_cc_compute_##name(                      \
        CPUArchState *env, QCEState *state, QCEVar *dst, QCEVar *src1,         \
        QCEVar *src2, QCEVar *opc, QCEVar *res) {                              \
    QCEExpr expr_dst, expr_src1, expr_src2, expr_opc;                          \
    qce_state_get_var(env, state, dst, &expr_dst);                             \
    qce_state_get_var(env, state, src1, &expr_src1);                           \
    qce_state_get_var(env, state, src2, &expr_src2);                           \
    qce_state_get_var(env, state, opc, &expr_opc);                             \
    /* mode checking */                                                        \
    qce_expr_assert_mode(&expr_dst, CONCRETE);                                 \
    qce_expr_assert_mode(&expr_src1, CONCRETE);                                \
    qce_expr_assert_mode(&expr_src2, CONCRETE);                                \
    qce_expr_assert_mode(&expr_opc, CONCRETE);                                 \
    /* type checking */                                                        \
    qce_expr_assert_type(&expr_dst, I64);                                      \
    qce_expr_assert_type(&expr_src1, I64);                                     \
    qce_expr_assert_type(&expr_src2, I64);                                     \
    qce_expr_assert_type(&expr_opc, I32);                                      \
                                                                               \
    QCEExpr expr_res;                                                          \
    expr_res.type = QCE_EXPR_I64;                                              \
    expr_res.mode = QCE_EXPR_CONCRETE;                                         \
    expr_res.v_i64 = helper_cc_compute_##name(expr_dst.v_i64, expr_src1.v_i64, \
                                              expr_src2.v_i64, expr_opc.v_i32);\
    qce_state_put_var(env, state, res, &expr_res);                             \
  }

DEFINE_SYM_INST_CALL_cc_compute(all)
DEFINE_SYM_INST_CALL_cc_compute(c)

#define HANDLE_SYM_INST_CALL_cc_compute(name)                                  \
  case QCE_INST_CALL_cc_compute_##name: {                                      \
    qce_sym_inst_call_cc_compute_##name(                                       \
        arch, &session->state,                                                 \
        &inst->i_call_cc_compute_##name.dst,                                   \
        &inst->i_call_cc_compute_##name.src1,                                  \
        &inst->i_call_cc_compute_##name.src2,                                  \
        &inst->i_call_cc_compute_##name.opc,                                   \
        &inst->i_call_cc_compute_##name.res);                                  \
    break;                                                                     \
}

static inline void qce_sym_inst_call_cc_compute_nz(
    CPUArchState *env, QCEState *state, QCEVar *dst, QCEVar *src,
    QCEVar *opc, QCEVar *res) {
  QCEExpr expr_dst, expr_src, expr_opc;
  qce_state_get_var(env, state, dst, &expr_dst);
  qce_state_get_var(env, state, src, &expr_src);
  qce_state_get_var(env, state, opc, &expr_opc);
  /* mode checking */
  qce_expr_assert_mode(&expr_dst, CONCRETE);
  qce_expr_assert_mode(&expr_src, CONCRETE);
  qce_expr_assert_mode(&expr_opc, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_dst, I64);
  qce_expr_assert_type(&expr_src, I64);
  qce_expr_assert_type(&expr_opc, I32);

  QCEExpr expr_res;
  expr_res.type = QCE_EXPR_I64;
  expr_res.mode = QCE_EXPR_CONCRETE;
  expr_res.v_i64 = helper_cc_compute_nz(expr_dst.v_i64, expr_src.v_i64,
                                        expr_opc.v_i32);
  qce_state_put_var(env, state, res, &expr_res);
}

#define HANDLE_SYM_INST_CALL_cc_compute_nz                                     \
  case QCE_INST_CALL_cc_compute_nz: {                                          \
    qce_sym_inst_call_cc_compute_nz(                                           \
        arch, &session->state, &inst->i_call_cc_compute_nz.dst,                \
        &inst->i_call_cc_compute_nz.src, &inst->i_call_cc_compute_nz.opc,      \
        &inst->i_call_cc_compute_nz.res);                                      \
    break;                                                                     \
}

static inline void qce_sym_inst_call_ld_i128(
    CPUArchState *env, QCEState *state, QCEVar *addr, QCEVar *flag,
    QCEVar *out_b, QCEVar *out_t) {
  QCEExpr expr_addr, expr_flag;
  qce_state_get_var(env, state, addr, &expr_addr);
  qce_state_get_var(env, state, flag, &expr_flag);
  /* mode checking */
  qce_expr_assert_mode(&expr_addr, CONCRETE);
  qce_expr_assert_mode(&expr_flag, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_addr, I64);
  qce_expr_assert_type(&expr_flag, I32);

  QCEExpr expr_out_b, expr_out_t;
  Int128 out = helper_ld_i128(env, expr_addr.v_i64, expr_flag.v_i32);
  qce_expr_init_v64(&expr_out_b, int128_getlo(out));
  qce_expr_init_v64(&expr_out_t, int128_gethi(out));
  qce_state_put_var(env, state, out_b, &expr_out_b);
  qce_state_put_var(env, state, out_t, &expr_out_t);
}

#define HANDLE_SYM_INST_CALL_ld_i128                                           \
  case QCE_INST_CALL_ld_i128: {                                                \
    qce_sym_inst_call_ld_i128(                                                 \
        arch, &session->state,                                                 \
        &inst->i_call_ld_i128.base, &inst->i_call_ld_i128.offset,              \
        &inst->i_call_ld_i128.out_b, &inst->i_call_ld_i128.out_t);             \
    break;                                                                     \
  }

static inline void qce_sym_inst_call_st_i128(
    CPUArchState *env, QCEState *state, QCEVar *addr, QCEVar *flag,
    QCEVar *in_b, QCEVar *in_t) {
  QCEExpr expr_addr, expr_flag, expr_in_t, expr_in_b;
  qce_state_get_var(env, state, addr, &expr_addr);
  qce_state_get_var(env, state, flag, &expr_flag);
  qce_state_get_var(env, state, in_b, &expr_in_b);
  qce_state_get_var(env, state, in_t, &expr_in_t);
  /* mode checking */
  qce_expr_assert_mode(&expr_addr, CONCRETE);
  qce_expr_assert_mode(&expr_flag, CONCRETE);
  qce_expr_assert_mode(&expr_in_b, CONCRETE);
  qce_expr_assert_mode(&expr_in_t, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_addr, I64);
  qce_expr_assert_type(&expr_flag, I32);
  qce_expr_assert_type(&expr_in_b, I64);
  qce_expr_assert_type(&expr_in_t, I64);

  qce_debug_assert((get_memop(expr_flag.v_i32) & MO_SIZE) == MO_128);
  unsigned mmu_idx = get_mmuidx(expr_flag.v_i32);
  qce_state_mem_put_i64(env, state, expr_addr.v_i64, mmu_idx, &expr_in_b);
  qce_state_mem_put_i64(env, state, expr_addr.v_i64 + 8, mmu_idx, &expr_in_t);
}

#define HANDLE_SYM_INST_CALL_st_i128                                           \
  case QCE_INST_CALL_st_i128: {                                                \
    qce_sym_inst_call_st_i128(                                                 \
        arch, &session->state,                                                 \
        &inst->i_call_st_i128.base, &inst->i_call_st_i128.offset,              \
        &inst->i_call_st_i128.in_b, &inst->i_call_st_i128.in_t);               \
    break;                                                                     \
  }

#define DEFINE_SYM_INST_CALL_gvec(name, op)                                    \
  static inline void qce_sym_inst_call_gvec_##name(                            \
      CPUArchState *env, QCEState *state, QCEVar *d, QCEVar *a,                \
      QCEVar *b, QCEVar *desc) {                                               \
    QCEExpr expr_d, expr_a, expr_b, expr_desc;                                 \
    qce_state_get_var(env, state, d, &expr_d);                                 \
    qce_state_get_var(env, state, a, &expr_a);                                 \
    qce_state_get_var(env, state, b, &expr_b);                                 \
    qce_state_get_var(env, state, desc, &expr_desc);                           \
    /* mode checking */                                                        \
    qce_expr_assert_mode(&expr_d, CONCRETE);                                   \
    qce_expr_assert_mode(&expr_a, CONCRETE);                                   \
    qce_expr_assert_mode(&expr_b, CONCRETE);                                   \
    qce_expr_assert_mode(&expr_desc, CONCRETE);                                \
    /* type checking */                                                        \
    qce_expr_assert_type(&expr_d, I64);                                        \
    qce_expr_assert_type(&expr_a, I64);                                        \
    qce_expr_assert_type(&expr_b, I64);                                        \
    qce_expr_assert_type(&expr_desc, I32);                                     \
                                                                               \
    intptr_t oprsz = simd_oprsz(expr_desc.v_i32);                              \
    g_assert(oprsz % 4 == 0);                                                  \
    for (intptr_t i = 0; i < oprsz; i += 4 * sizeof(uint8_t)) {                \
      QCEExpr expr_into_d, expr_from_a, expr_from_b;                           \
      qce_state_env_get_i32(state, expr_a.v_i64 + i, &expr_from_a);            \
      qce_state_env_get_i32(state, expr_b.v_i64 + i, &expr_from_b);            \
      qce_expr_assert_mode(&expr_from_a, CONCRETE);                            \
      qce_expr_assert_mode(&expr_from_b, CONCRETE);                            \
      for (uint8_t byte = 0; byte < 4; ++byte) {                               \
        ((uint8_t *)&expr_into_d.v_i32)[byte] =                                \
            -(((uint8_t *)&expr_from_a.v_i32)[byte] op                         \
              ((uint8_t *)&expr_from_b.v_i32)[byte]);                          \
      }                                                                        \
      expr_into_d.mode = QCE_EXPR_CONCRETE;                                    \
      expr_into_d.type = QCE_EXPR_I32;                                         \
      qce_state_env_put_i32(state, expr_d.v_i64 + i, &expr_into_d);            \
    }                                                                          \
    intptr_t maxsz = simd_maxsz(expr_desc.v_i32);                              \
    if (unlikely(maxsz > oprsz)) {                                             \
      for (intptr_t i = oprsz; i < maxsz; i += sizeof(uint64_t)) {             \
        QCEExpr expr_into_d;                                                   \
        qce_expr_init_v64(&expr_into_d, 0);                                    \
        qce_state_env_put_i64(state, expr_d.v_i64 + i, &expr_into_d);          \
      }                                                                        \
    }                                                                          \
  }

DEFINE_SYM_INST_CALL_gvec(eq8, ==)
DEFINE_SYM_INST_CALL_gvec(lt8, <)

#define HANDLE_SYM_INST_CALL_gvec(name)                                        \
  case QCE_INST_CALL_gvec_##name: {                                            \
    qce_sym_inst_call_gvec_##name(                                             \
        arch, &session->state, &inst->i_call_gvec_##name.d,                    \
        &inst->i_call_gvec_##name.a, &inst->i_call_gvec_##name.b,              \
        &inst->i_call_gvec_##name.desc);                                       \
    break;                                                                     \
  }

#endif /* QCE_SYM_CALL_H */
