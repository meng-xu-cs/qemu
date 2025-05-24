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

static inline uint32_t qce_cpu_cc_compute_all(CPUX86State *env,
                                              QCEState *state) {
  QCEExpr expr_cc_dst, expr_cc_src, expr_cc_src2, expr_cc_op;
  qce_state_env_get_i64(state, (intptr_t)&env->cc_dst, &expr_cc_dst);
  qce_state_env_get_i64(state, (intptr_t)&env->cc_src, &expr_cc_src);
  qce_state_env_get_i64(state, (intptr_t)&env->cc_src2, &expr_cc_src2);
  qce_state_env_get_i32(state, (intptr_t)&env->cc_op, &expr_cc_op);

  return helper_cc_compute_all(expr_cc_dst.v_i64, expr_cc_src.v_i64,
                               expr_cc_src2.v_i64, expr_cc_op.v_i32);
}

static inline uint32_t qce_cpu_compute_eflags(CPUX86State *env,
                                              QCEState * state) {
  QCEExpr expr_df, expr_eflags;
  qce_state_env_get_i32(state, (intptr_t)&env->df, &expr_df);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);

  uint32_t eflags = expr_eflags.v_i64;
  if (tcg_enabled()) {
    eflags |= qce_cpu_cc_compute_all(env, state) | (expr_df.v_i32 & DF_MASK);
  }
  return eflags;
}

static inline void qce_cpu_load_eflags(CPUX86State *env, QCEState * state,
                                       int eflags, int update_mask) {
  QCEExpr expr_cc_src, expr_cc_op, expr_df, expr_eflags;
  qce_state_env_get_i64(state, (intptr_t)&env->cc_src, &expr_cc_src);
  qce_state_env_get_i32(state, (intptr_t)&env->cc_op, &expr_cc_op);
  qce_state_env_get_i32(state, (intptr_t)&env->df, &expr_df);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);

  expr_cc_src.v_i64 = eflags & (CC_O | CC_S | CC_Z | CC_A | CC_P | CC_C);
  expr_cc_op.v_i32 = CC_OP_EFLAGS;
  expr_df.v_i32 = 1 - (2 * ((eflags >> 10) & 1));
  expr_eflags.v_i64 = (expr_eflags.v_i64 & ~update_mask) |
      (eflags & update_mask) | 0x2;

  qce_state_env_put_i64(state, (intptr_t)&env->cc_src, &expr_cc_src);
  qce_state_env_put_i32(state, (intptr_t)&env->cc_op, &expr_cc_op);
  qce_state_env_put_i32(state, (intptr_t)&env->df, &expr_df);
  qce_state_env_put_i64(state, (intptr_t)&env->eflags, &expr_eflags);
}

static inline void qce_cpu_x86_load_seg_cache(
    CPUX86State *env, QCEState * state, X86Seg seg_reg, unsigned int selector,
    target_ulong base, unsigned int limit, unsigned int flags) {
  QCEExpr expr_sr_selector, expr_sr_base, expr_sr_limit, expr_sr_flags,
          expr_hflags, expr_cs_flags, expr_ss_flags, expr_cr0, expr_eflags,
          expr_ds_base, expr_es_base, expr_ss_base;
  qce_state_env_get_i32(state, (intptr_t)&env->segs[seg_reg].selector,
                        &expr_sr_selector);
  qce_state_env_get_i64(state, (intptr_t)&env->segs[seg_reg].base,
                        &expr_sr_base);
  qce_state_env_get_i32(state, (intptr_t)&env->segs[seg_reg].limit,
                        &expr_sr_limit);
  qce_state_env_get_i32(state, (intptr_t)&env->segs[seg_reg].flags,
                        &expr_sr_flags);
  qce_state_env_get_i32(state, (intptr_t)&env->hflags, &expr_hflags);
  qce_state_env_get_i32(state, (intptr_t)&env->segs[R_CS].flags,
                        &expr_cs_flags);
  qce_state_env_get_i32(state, (intptr_t)&env->segs[R_SS].flags,
                        &expr_ss_flags);
  qce_state_env_get_i64(state, (intptr_t)&env->cr[0], &expr_cr0);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);
  qce_state_env_get_i64(state, (intptr_t)&env->segs[R_DS].base,
                        &expr_ds_base);
  qce_state_env_get_i64(state, (intptr_t)&env->segs[R_ES].base,
                        &expr_es_base);
  qce_state_env_get_i64(state, (intptr_t)&env->segs[R_SS].base,
                        &expr_ss_base);

  unsigned int new_hflags;

  expr_sr_selector.v_i32 = selector;
  expr_sr_base.v_i64 = base;
  expr_sr_limit.v_i32 = limit;
  expr_sr_flags.v_i32 = flags;

  /* update the hidden flags */
  {
    if (seg_reg == R_CS) {
#ifdef TARGET_X86_64
      if ((expr_hflags.v_i32 & HF_LMA_MASK) && (flags & DESC_L_MASK)) {
        /* long mode */
        expr_hflags.v_i32 |= HF_CS32_MASK | HF_SS32_MASK | HF_CS64_MASK;
        expr_hflags.v_i32 &= ~(HF_ADDSEG_MASK);
      } else
#endif
      {
        /* legacy / compatibility case */
        new_hflags = (expr_cs_flags.v_i32 & DESC_B_MASK)
            >> (DESC_B_SHIFT - HF_CS32_SHIFT);
        expr_hflags.v_i32 = (expr_hflags.v_i32 &
                             ~(HF_CS32_MASK | HF_CS64_MASK)) | new_hflags;
      }
    }
    if (seg_reg == R_SS) {
      int cpl = (flags >> DESC_DPL_SHIFT) & 3;
#if HF_CPL_MASK != 3
#error HF_CPL_MASK is hardcoded
#endif
      expr_hflags.v_i32 = (expr_hflags.v_i32 & ~HF_CPL_MASK) | cpl;
      /* Possibly switch between BNDCFGS and BNDCFGU */
      cpu_sync_bndcs_hflags(env);
    }
    new_hflags = (expr_ss_flags.v_i32 & DESC_B_MASK)
        >> (DESC_B_SHIFT - HF_SS32_SHIFT);
    if (expr_hflags.v_i32 & HF_CS64_MASK) {
      /* zero base assumed for DS, ES and SS in long mode */
    } else if (!(expr_cr0.v_i64 & CR0_PE_MASK) ||
              (expr_eflags.v_i64 & VM_MASK) ||
              !(expr_hflags.v_i32 & HF_CS32_MASK)) {
      /* XXX: try to avoid this test. The problem comes from the
         fact that is real mode or vm86 mode we only modify the
         'base' and 'selector' fields of the segment cache to go
         faster. A solution may be to force addseg to one in
         translate-i386.c. */
      new_hflags |= HF_ADDSEG_MASK;
    } else {
      new_hflags |= ((expr_ds_base.v_i64 |
                      expr_es_base.v_i64 |
                      expr_ss_base.v_i64) != 0) <<
          HF_ADDSEG_SHIFT;
    }
    expr_hflags.v_i32 = (expr_hflags.v_i32 &
                   ~(HF_SS32_MASK | HF_ADDSEG_MASK)) | new_hflags;
  }

  qce_state_env_put_i32(state, (intptr_t)&env->segs[seg_reg].selector,
                        &expr_sr_selector);
  qce_state_env_put_i64(state, (intptr_t)&env->segs[seg_reg].base,
                        &expr_sr_base);
  qce_state_env_put_i32(state, (intptr_t)&env->segs[seg_reg].limit,
                        &expr_sr_limit);
  qce_state_env_put_i32(state, (intptr_t)&env->segs[seg_reg].flags,
                        &expr_sr_flags);
  qce_state_env_put_i32(state, (intptr_t)&env->hflags, &expr_hflags);
}

static inline void qce_sym_inst_call_syscall(
    CPUArchState *env, QCEState *state, QCEVar *next_eip) {
  QCEExpr expr_next_eip;
  qce_state_get_var(env, state, next_eip, &expr_next_eip);
  /* mode checking */
  qce_expr_assert_mode(&expr_next_eip, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_next_eip, I32);

  QCEExpr expr_efer, expr_star, expr_hflags, expr_rcx, expr_eip, expr_r11,
          expr_eflags, expr_fmask, expr_lstar, expr_cstar;
  qce_state_env_get_i64(state, (intptr_t)&env->efer, &expr_efer);
  qce_state_env_get_i64(state, (intptr_t)&env->star, &expr_star);
  qce_state_env_get_i32(state, (intptr_t)&env->hflags, &expr_hflags);
  qce_state_env_get_i64(state, (intptr_t)&env->regs[R_ECX], &expr_rcx);
  qce_state_env_get_i64(state, (intptr_t)&env->eip, &expr_eip);
  qce_state_env_get_i64(state, (intptr_t)&env->regs[11], &expr_r11);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);
  qce_state_env_get_i64(state, (intptr_t)&env->fmask, &expr_fmask);
  qce_state_env_get_i64(state, (intptr_t)&env->lstar, &expr_lstar);
  qce_state_env_get_i64(state, (intptr_t)&env->cstar, &expr_cstar);

  int selector;

  if (!(expr_efer.v_i64 & MSR_EFER_SCE)) {
    qce_fatal("syscall raises an exception");
//    raise_exception_err_ra(env, EXCP06_ILLOP, 0, GETPC());
  }
  selector = ((uint64_t)expr_star.v_i64 >> 32) & 0xffff;
#ifdef TARGET_X86_64
  if (expr_hflags.v_i32 & HF_LMA_MASK) {
    int code64;

    expr_rcx.v_i64 = expr_eip.v_i64 + expr_next_eip.v_i32;
    expr_r11.v_i64 = qce_cpu_compute_eflags(env, state) & ~RF_MASK;

    code64 = expr_hflags.v_i32 & HF_CS64_MASK;

    expr_eflags.v_i64 &= ~(expr_fmask.v_i64 | RF_MASK);
    qce_cpu_load_eflags(env, state, expr_eflags.v_i64, 0);
    qce_cpu_x86_load_seg_cache(env, state, R_CS, selector & 0xfffc,
                               0, 0xffffffff,
                               DESC_G_MASK | DESC_P_MASK |
                                   DESC_S_MASK |
                                   DESC_CS_MASK | DESC_R_MASK | DESC_A_MASK |
                                   DESC_L_MASK);
    qce_cpu_x86_load_seg_cache(env, state, R_SS, (selector + 8) & 0xfffc,
                               0, 0xffffffff,
                               DESC_G_MASK | DESC_B_MASK | DESC_P_MASK |
                                  DESC_S_MASK |
                                  DESC_W_MASK | DESC_A_MASK);
    if (code64) {
      expr_eip.v_i64 = expr_lstar.v_i64;
    } else {
      expr_eip.v_i64 = expr_cstar.v_i64;
    }
  } else
#endif
  {
    expr_rcx.v_i64 = (uint32_t)(expr_eip.v_i64 + expr_next_eip.v_i32);

    expr_eflags.v_i64 &= ~(IF_MASK | RF_MASK | VM_MASK);
    qce_cpu_x86_load_seg_cache(env, state, R_CS, selector & 0xfffc,
                               0, 0xffffffff,
                               DESC_G_MASK | DESC_B_MASK | DESC_P_MASK |
                                  DESC_S_MASK |
                                  DESC_CS_MASK | DESC_R_MASK | DESC_A_MASK);
    qce_cpu_x86_load_seg_cache(env, state, R_SS, (selector + 8) & 0xfffc,
                               0, 0xffffffff,
                               DESC_G_MASK | DESC_B_MASK | DESC_P_MASK |
                                   DESC_S_MASK |
                                   DESC_W_MASK | DESC_A_MASK);
    expr_eip.v_i64 = (uint32_t)expr_star.v_i64;
  }

  qce_state_env_put_i64(state, (intptr_t)&env->regs[R_ECX], &expr_rcx);
  qce_state_env_put_i64(state, (intptr_t)&env->regs[11], &expr_r11);
  qce_state_env_put_i64(state, (intptr_t)&env->eflags, &expr_eflags);
  qce_state_env_put_i64(state, (intptr_t)&env->eip, &expr_eip);
}

#define HANDLE_SYM_INST_CALL_syscall                                           \
  case QCE_INST_CALL_syscall: {                                                \
    qce_sym_inst_call_syscall(                                                 \
        arch, &session->state, &inst->i_call_syscall.next_eip);                \
    break;                                                                     \
  }

static inline void qce_sym_inst_call_sysret(
    CPUArchState *env, QCEState *state, QCEVar *dflag) {
  QCEExpr expr_dflag;
  qce_state_get_var(env, state, dflag, &expr_dflag);
  /* mode checking */
  qce_expr_assert_mode(&expr_dflag, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_dflag, I32);

  QCEExpr expr_efer, expr_hflags, expr_cr0, expr_star,
          expr_r11, expr_eip, expr_rcx, expr_eflags;
  qce_state_env_get_i64(state, (intptr_t)&env->efer, &expr_efer);
  qce_state_env_get_i32(state, (intptr_t)&env->hflags, &expr_hflags);
  qce_state_env_get_i64(state, (intptr_t)&env->cr[0], &expr_cr0);
  qce_state_env_get_i64(state, (intptr_t)&env->star, &expr_star);
  qce_state_env_get_i64(state, (intptr_t)&env->regs[11], &expr_r11);
  qce_state_env_get_i64(state, (intptr_t)&env->eip, &expr_eip);
  qce_state_env_get_i64(state, (intptr_t)&env->regs[R_ECX], &expr_rcx);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);

  int cpl, selector;

  if (!(expr_efer.v_i64 & MSR_EFER_SCE)) {
    qce_fatal("sysret raises an exception");
//    raise_exception_err_ra(env, EXCP06_ILLOP, 0, GETPC());
  }
  cpl = expr_hflags.v_i32 & HF_CPL_MASK;
  if (!(expr_cr0.v_i64 & CR0_PE_MASK) || cpl != 0) {
    qce_fatal("sysret raises an exception");
//    raise_exception_err_ra(env, EXCP0D_GPF, 0, GETPC());
  }
  selector = ((uint64_t)expr_star.v_i64 >> 48) & 0xffff;
  if (expr_hflags.v_i32 & HF_LMA_MASK) {
    qce_cpu_load_eflags(env, state, (uint32_t)(expr_r11.v_i64), TF_MASK |
                        AC_MASK | ID_MASK | IF_MASK | IOPL_MASK | VM_MASK
                        | RF_MASK | NT_MASK);
    if (expr_dflag.v_i32 == 2) {
      qce_cpu_x86_load_seg_cache(env, state, R_CS, (selector + 16) | 3,
                                 0, 0xffffffff,
                                 DESC_G_MASK | DESC_P_MASK |
                                    DESC_S_MASK | (3 << DESC_DPL_SHIFT) |
                                    DESC_CS_MASK | DESC_R_MASK | DESC_A_MASK |
                                    DESC_L_MASK);
      expr_eip.v_i64 = expr_rcx.v_i64;
    } else {
      qce_cpu_x86_load_seg_cache(env, state, R_CS, selector | 3,
                                 0, 0xffffffff,
                                 DESC_G_MASK | DESC_B_MASK | DESC_P_MASK |
                                    DESC_S_MASK | (3 << DESC_DPL_SHIFT) |
                                    DESC_CS_MASK | DESC_R_MASK | DESC_A_MASK);
      expr_eip.v_i64 = (uint32_t)expr_rcx.v_i64;
    }
    qce_cpu_x86_load_seg_cache(env, state, R_SS, (selector + 8) | 3,
                               0, 0xffffffff,
                               DESC_G_MASK | DESC_B_MASK | DESC_P_MASK |
                                  DESC_S_MASK | (3 << DESC_DPL_SHIFT) |
                                  DESC_W_MASK | DESC_A_MASK);
  } else {
    expr_eflags.v_i64 |= IF_MASK;
    qce_state_env_put_i64(state, (intptr_t)&env->eflags, &expr_eflags);
    qce_cpu_x86_load_seg_cache(env, state, R_CS, selector | 3,
                               0, 0xffffffff,
                               DESC_G_MASK | DESC_B_MASK | DESC_P_MASK |
                                  DESC_S_MASK | (3 << DESC_DPL_SHIFT) |
                                  DESC_CS_MASK | DESC_R_MASK | DESC_A_MASK);
    expr_eip.v_i64 = (uint32_t)expr_rcx.v_i64;
    qce_cpu_x86_load_seg_cache(env, state, R_SS, (selector + 8) | 3,
                               0, 0xffffffff,
                               DESC_G_MASK | DESC_B_MASK | DESC_P_MASK |
                                  DESC_S_MASK | (3 << DESC_DPL_SHIFT) |
                                  DESC_W_MASK | DESC_A_MASK);
  }

  qce_state_env_put_i64(state, (intptr_t)&env->eip, &expr_eip);
}

#define HANDLE_SYM_INST_CALL_sysret                                            \
  case QCE_INST_CALL_sysret: {                                                 \
    qce_sym_inst_call_sysret(                                                  \
        arch, &session->state, &inst->i_call_sysret.dflag);                    \
    break;                                                                     \
  }

static inline void qce_sym_inst_call_rechecking_single_step(CPUArchState *env,
                                                            QCEState *state) {
  QCEExpr expr_eflags;
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);

  if ((expr_eflags.v_i64 & TF_MASK) != 0) {
    qce_fatal("rechecking_single_step raises an exception");
//    helper_single_step(env);
  }
}

#define HANDLE_SYM_INST_CALL_rechecking_single_step                            \
  case QCE_INST_CALL_rechecking_single_step: {                                 \
    qce_sym_inst_call_rechecking_single_step(arch, &session->state);           \
    break;                                                                     \
  }

#define SHIFT 1
#define Reg ZMMReg
#define LANE_WIDTH (SHIFT ? 16 : 8)
#define PACK_WIDTH (LANE_WIDTH / 2)
#define L(n) ZMM_L(n)
#define Q(n) ZMM_Q(n)

#define DEFINE_SYM_INST_CALL_punpck_dq_xmm(basename, base)                     \
  static inline void qce_sym_inst_call_punpck##basename##dq_xmm(               \
      CPUArchState *env, QCEState *state, QCEVar *d, QCEVar *v, QCEVar *s) {   \
    QCEExpr expr_d, expr_v, expr_s;                                            \
    qce_state_get_var(env, state, d, &expr_d);                                 \
    qce_state_get_var(env, state, v, &expr_v);                                 \
    qce_state_get_var(env, state, s, &expr_s);                                 \
    /* mode checking */                                                        \
    qce_expr_assert_mode(&expr_d, CONCRETE);                                   \
    qce_expr_assert_mode(&expr_v, CONCRETE);                                   \
    qce_expr_assert_mode(&expr_s, CONCRETE);                                   \
    /* type checking */                                                        \
    qce_expr_assert_type(&expr_d, I64);                                        \
    qce_expr_assert_type(&expr_v, I64);                                        \
    qce_expr_assert_type(&expr_s, I64);                                        \
                                                                               \
    Reg *ptr_v = (Reg *)expr_v.v_i64;                                          \
    Reg *ptr_s = (Reg *)expr_s.v_i64;                                          \
    Reg *ptr_d = (Reg *)expr_d.v_i64;                                          \
                                                                               \
    uint32_t r[PACK_WIDTH / 2];                                                \
                                                                               \
    for (int j = 0; j < 2 << SHIFT; ) {                                        \
      int k = j + base * PACK_WIDTH / 4;                                       \
      for (int i = 0; i < PACK_WIDTH / 4; i++) {                               \
        QCEExpr expr_from_v, expr_from_s;                                      \
        qce_state_env_get_i32(state, (intptr_t)&ptr_v->L(k + i), &expr_from_v);\
        qce_state_env_get_i32(state, (intptr_t)&ptr_s->L(k + i), &expr_from_s);\
        qce_expr_assert_mode(&expr_from_v, CONCRETE);                          \
        qce_expr_assert_mode(&expr_from_s, CONCRETE);                          \
        r[2 * i] = expr_from_v.v_i32;                                          \
        r[2 * i + 1] = expr_from_s.v_i32;                                      \
      }                                                                        \
      for (int i = 0; i < PACK_WIDTH / 2; i++, j++) {                          \
        QCEExpr expr_into_d;                                                   \
        qce_expr_init_v32(&expr_into_d, r[i]);                                 \
        qce_state_env_put_i32(state, (intptr_t)&ptr_d->L(j), &expr_into_d);    \
      }                                                                        \
    }                                                                          \
  }

DEFINE_SYM_INST_CALL_punpck_dq_xmm(h, 1)
DEFINE_SYM_INST_CALL_punpck_dq_xmm(l, 0)

static inline void qce_sym_inst_call_punpcklqdq_xmm(
    CPUArchState *env, QCEState *state, QCEVar *d, QCEVar *v, QCEVar *s) {
  QCEExpr expr_d, expr_v, expr_s;
  qce_state_get_var(env, state, d, &expr_d);
  qce_state_get_var(env, state, v, &expr_v);
  qce_state_get_var(env, state, s, &expr_s);
  /* mode checking */
  qce_expr_assert_mode(&expr_d, CONCRETE);
  qce_expr_assert_mode(&expr_v, CONCRETE);
  qce_expr_assert_mode(&expr_s, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_d, I64);
  qce_expr_assert_type(&expr_v, I64);
  qce_expr_assert_type(&expr_s, I64);

  Reg *ptr_v = (Reg *)expr_v.v_i64;
  Reg *ptr_s = (Reg *)expr_s.v_i64;
  Reg *ptr_d = (Reg *)expr_d.v_i64;

  for (int i = 0; i < 1 << SHIFT; i += 2) {
    QCEExpr expr_from_v, expr_from_s;
    qce_state_env_get_i64(state, (intptr_t)&ptr_v->Q(i), &expr_from_v);
    qce_state_env_get_i64(state, (intptr_t)&ptr_s->Q(i), &expr_from_s);
    qce_expr_assert_mode(&expr_from_v, CONCRETE);
    qce_expr_assert_mode(&expr_from_s, CONCRETE);
    QCEExpr expr_into_d;
    qce_expr_init_v64(&expr_into_d, expr_from_v.v_i32);
    qce_state_env_put_i64(state, (intptr_t)&ptr_d->Q(i), &expr_into_d);
    qce_expr_init_v64(&expr_into_d, expr_from_s.v_i32);
    qce_state_env_put_i64(state, (intptr_t)&ptr_d->Q(i + 1), &expr_into_d);
  }
}

#define HANDLE_SYM_INST_CALL_punpck_xmm(name)                                  \
  case QCE_INST_CALL_punpck##name##_xmm: {                                     \
    qce_sym_inst_call_punpck##name##_xmm(arch, &session->state,                \
                                         &inst->i_call_punpck##name##_xmm.d,   \
                                         &inst->i_call_punpck##name##_xmm.v,   \
                                         &inst->i_call_punpck##name##_xmm.s);  \
    break;                                                                     \
  }

static inline void qce_sym_inst_call_pshufd_xmm(
    CPUArchState *env, QCEState *state, QCEVar *d, QCEVar *s, QCEVar *order) {
  QCEExpr expr_d, expr_s, expr_order;
  qce_state_get_var(env, state, d, &expr_d);
  qce_state_get_var(env, state, s, &expr_s);
  qce_state_get_var(env, state, order, &expr_order);
  /* mode checking */
  qce_expr_assert_mode(&expr_d, CONCRETE);
  qce_expr_assert_mode(&expr_s, CONCRETE);
  qce_expr_assert_mode(&expr_order, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_d, I64);
  qce_expr_assert_type(&expr_s, I64);
  qce_expr_assert_type(&expr_order, I32);

  Reg *ptr_s = (Reg *)expr_s.v_i64;
  Reg *ptr_d = (Reg *)expr_d.v_i64;

  for (int i = 0; i < 2 << SHIFT; i += 4) {
    QCEExpr expr_from_s0, expr_from_s1, expr_from_s2, expr_from_s3;
    qce_state_env_get_i32(
        state, (intptr_t)&ptr_s->L((expr_order.v_i32 & 3) + i),
        &expr_from_s0);
    qce_state_env_get_i32(
        state, (intptr_t)&ptr_s->L(((expr_order.v_i32 >> 2) & 3) + i),
        &expr_from_s1);
    qce_state_env_get_i32(
        state, (intptr_t)&ptr_s->L(((expr_order.v_i32 >> 4) & 3) + i),
        &expr_from_s2);
    qce_state_env_get_i32(
        state, (intptr_t)&ptr_s->L(((expr_order.v_i32 >> 6) & 3) + i),
        &expr_from_s3);

    QCEExpr expr_into_d0, expr_into_d1, expr_into_d2, expr_into_d3;
    qce_expr_init_v32(&expr_into_d0, expr_from_s0.v_i32);
    qce_expr_init_v32(&expr_into_d1, expr_from_s1.v_i32);
    qce_expr_init_v32(&expr_into_d2, expr_from_s2.v_i32);
    qce_expr_init_v32(&expr_into_d3, expr_from_s3.v_i32);
    qce_state_env_put_i32(state, (intptr_t)&ptr_d->L(i), &expr_into_d0);
    qce_state_env_put_i32(state, (intptr_t)&ptr_d->L(i + 1), &expr_into_d1);
    qce_state_env_put_i32(state, (intptr_t)&ptr_d->L(i + 2), &expr_into_d2);
    qce_state_env_put_i32(state, (intptr_t)&ptr_d->L(i + 3), &expr_into_d3);
  }
}

#define HANDLE_SYM_INST_CALL_pshufd_xmm                                        \
  case QCE_INST_CALL_pshufd_xmm: {                                             \
    qce_sym_inst_call_pshufd_xmm(                                              \
        arch, &session->state, &inst->i_call_pshufd_xmm.d,                     \
        &inst->i_call_pshufd_xmm.s, &inst->i_call_pshufd_xmm.order);           \
    break;                                                                     \
  }

static void add128(uint64_t *plow, uint64_t *phigh, uint64_t a, uint64_t b) {
  *plow += a;
  /* carry test */
  if (*plow < a) {
    (*phigh)++;
  }
  *phigh += b;
}

static void neg128(uint64_t *plow, uint64_t *phigh) {
  *plow = ~*plow;
  *phigh = ~*phigh;
  add128(plow, phigh, 1, 0);
}

/* return TRUE if overflow */
static int div64(uint64_t *plow, uint64_t *phigh, uint64_t b) {
  uint64_t q, r, a1, a0;
  int i, qb, ab;

  a0 = *plow;
  a1 = *phigh;
  if (a1 == 0) {
    q = a0 / b;
    r = a0 % b;
    *plow = q;
    *phigh = r;
  } else {
    if (a1 >= b) {
      return 1;
    }
    /* XXX: use a better algorithm */
    for (i = 0; i < 64; i++) {
      ab = a1 >> 63;
      a1 = (a1 << 1) | (a0 >> 63);
      if (ab || a1 >= b) {
        a1 -= b;
        qb = 1;
      } else {
        qb = 0;
      }
      a0 = (a0 << 1) | qb;
    }

    *plow = a0;
    *phigh = a1;
  }
  return 0;
}

/* return TRUE if overflow */
static int idiv64(uint64_t *plow, uint64_t *phigh, int64_t b) {
  int sa, sb;

  sa = ((int64_t)*phigh < 0);
  if (sa) {
    neg128(plow, phigh);
  }
  sb = (b < 0);
  if (sb) {
    b = -b;
  }
  if (div64(plow, phigh, b) != 0) {
    return 1;
  }
  if (sa ^ sb) {
    if (*plow > (1ULL << 63)) {
      return 1;
    }
    *plow = -*plow;
  } else {
    if (*plow >= (1ULL << 63)) {
      return 1;
    }
  }
  if (sa) {
    *phigh = -*phigh;
  }
  return 0;
}

#define DEFINE_SYM_INST_CALL_divq_EAX(prefix)                                  \
  static inline void qce_sym_inst_call_##prefix##divq_EAX(                     \
      CPUArchState *env, QCEState *state, QCEVar *val) {                       \
    QCEExpr expr_val;                                                          \
    qce_state_get_var(env, state, val, &expr_val);                             \
    /* mode checking */                                                        \
    qce_expr_assert_mode(&expr_val, CONCRETE);                                 \
    /* type checking */                                                        \
    qce_expr_assert_type(&expr_val, I64);                                      \
                                                                               \
    QCEExpr expr_rax, expr_rdx;                                                \
    qce_state_env_get_i64(state, (intptr_t)&env->regs[R_EAX], &expr_rax);      \
    qce_state_env_get_i64(state, (intptr_t)&env->regs[R_EDX], &expr_rdx);      \
                                                                               \
    if (expr_val.v_i64 == 0) {                                                 \
      qce_fatal(#prefix"divq raises an exception");                            \
      /*raise_exception_ra(env, EXCP00_DIVZ, GETPC());*/                       \
    }                                                                          \
    if (prefix##div64((uint64_t *)&expr_rax.v_i64, (uint64_t *)&expr_rdx.v_i64,\
                      expr_val.v_i64)) {                                       \
      qce_fatal(#prefix"divq raises an exception");                            \
      /*raise_exception_ra(env, EXCP00_DIVZ, GETPC());*/                       \
    }                                                                          \
                                                                               \
    qce_state_env_put_i64(state, (intptr_t)&env->regs[R_EAX], &expr_rax);      \
    qce_state_env_put_i64(state, (intptr_t)&env->regs[R_EDX], &expr_rdx);      \
  }

DEFINE_SYM_INST_CALL_divq_EAX()
DEFINE_SYM_INST_CALL_divq_EAX(i)

#define HANDLE_SYM_INST_CALL_div_EAX(name)                                      \
  case QCE_INST_CALL_##name##_EAX: {                                            \
    qce_sym_inst_call_##name##_EAX(arch, &session->state,                       \
                                   &inst->i_call_##name##_EAX.val);             \
    break;                                                                      \
  }

static inline void qce_sym_inst_call_read_eflags(
    CPUArchState *env, QCEState *state, QCEVar *res) {
  QCEExpr expr_df, expr_eflags;
  qce_state_env_get_i32(state, (intptr_t)&env->df, &expr_df);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);

  uint32_t eflags;

  eflags = qce_cpu_cc_compute_all(env, state);
  eflags |= (expr_df.v_i32 & DF_MASK);
  eflags |= expr_eflags.v_i64 & ~(VM_MASK | RF_MASK);

  QCEExpr expr_res;
  qce_expr_init_v64(&expr_res, (target_ulong)eflags);
  qce_state_put_var(env, state, res, &expr_res);
}

#define HANDLE_SYM_INST_CALL_read_eflags                                       \
  case QCE_INST_CALL_read_eflags: {                                            \
    qce_sym_inst_call_read_eflags(arch, &session->state,                       \
                                  &inst->i_call_read_eflags.res);              \
    break;                                                                     \
  }

#endif /* QCE_SYM_CALL_H */
