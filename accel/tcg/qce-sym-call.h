#ifndef QCE_SYM_CALL_H
#define QCE_SYM_CALL_H

#include "exec/helper-proto.h"
#include "tcg/tcg-gvec-desc.h"

static inline void qce_compute_pf(QCEState *state, QCEExpr *x,
                                  QCEExpr *result) {
  qce_expr_extract_i64(&state->solver_z3, x, 0, 8, result);
  qce_expr_extrl_i64_i32(&state->solver_z3, result, result);
  qce_expr_parity_i32(&state->solver_z3, result, result);
  QCEPred pred;
  QCEExpr expr_v0;
  qce_expr_init_v32(&expr_v0, 0);
  qce_expr_eq_i32(&state->solver_z3, result, &expr_v0, &pred);
  qce_expr_init_from_pred_i32(&state->solver_z3, result, &pred);
  QCEExpr expr_cc_p;
  qce_expr_init_v32(&expr_cc_p, CC_P);
  qce_expr_mul_i32(&state->solver_z3, result, &expr_cc_p, result);
}

static inline void qce_lshift(QCEState *state, QCEExpr *x,
                              int n, QCEExpr *result) {
  if (n >= 0) {
    QCEExpr expr_n;
    qce_expr_init_v64(&expr_n, n);
    qce_expr_shl_i64(&state->solver_z3, x, &expr_n, result);
  } else {
    QCEExpr expr_nm;
    qce_expr_init_v64(&expr_nm, -n);
    qce_expr_sar_i64(&state->solver_z3, x, &expr_nm, result);
  }
}

#define DEFINE_CC_COMPUTE_ALL_HELPERS(suffix, bits)                            \
  static inline void compute_all_mul##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
                                                                               \
    uint32_t af;                                                               \
    QCEExpr expr_cf, expr_pf, expr_zf, expr_sf, expr_of;                       \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0;                                                           \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_ne_i64(&state->solver_z3, expr_src1, &expr_v0, &pred);            \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_cf, &pred);           \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    af = 0;                                                                    \
                                                                               \
    QCEExpr expr_cc_z;                                                         \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    QCEExpr expr_cc_o;                                                         \
    qce_expr_init_v32(&expr_cc_o, CC_O);                                       \
    qce_expr_mul_i32(&state->solver_z3, &expr_cf, &expr_cc_o, &expr_of);       \
                                                                               \
    qce_expr_init_v32(expr_res, af);                                           \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_add##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    QCEExpr expr_cf, expr_pf, expr_af, expr_zf, expr_sf, expr_of;              \
                                                                               \
    QCEExpr expr_src2;                                                         \
    qce_expr_sub_i64(&state->solver_z3, expr_dst, expr_src1, &expr_src2);      \
    qce_expr_extract_i64(&state->solver_z3, &expr_src2, 0, bits, &expr_src2);  \
                                                                               \
    QCEPred pred;                                                              \
    qce_expr_ult_i64(&state->solver_z3, expr_dst, expr_src1, &pred);           \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_cf, &pred);           \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    QCEExpr expr_cc_a;                                                         \
    qce_expr_init_v64(&expr_cc_a, CC_A);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, expr_dst, expr_src1, &expr_af);      \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_af, &expr_src2, &expr_af);     \
    qce_expr_bvand_i64(&state->solver_z3, &expr_af, &expr_cc_a, &expr_af);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_af, &expr_af);             \
                                                                               \
    QCEExpr expr_v0, expr_cc_z;                                                \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    QCEExpr expr_v1m, expr_cc_o, expr_tmp;                                     \
    qce_expr_init_v64(&expr_v1m, -1);                                          \
    qce_expr_init_v64(&expr_cc_o, CC_O);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, expr_src1, &expr_src2, &expr_of);    \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_of, &expr_v1m, &expr_of);      \
    qce_expr_bvxor_i64(&state->solver_z3, expr_src1, expr_dst, &expr_tmp);     \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_tmp, &expr_of);      \
    qce_lshift(state, &expr_of, 12-bits, &expr_of);                            \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_cc_o, &expr_of);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_of, &expr_of);             \
                                                                               \
    qce_expr_init_v32(expr_res, 0);                                            \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_af, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_adc##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_src3, QCEExpr *expr_res) {                                 \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
    qce_expr_extract_i64(&state->solver_z3, expr_src3, 0, bits, expr_src3);    \
                                                                               \
    QCEExpr expr_cf, expr_pf, expr_af, expr_zf, expr_sf, expr_of;              \
                                                                               \
    QCEExpr expr_src13, expr_src2;                                             \
    qce_expr_add_i64(&state->solver_z3, expr_src1, expr_src3, &expr_src13);    \
    qce_expr_sub_i64(&state->solver_z3, expr_dst, &expr_src13, &expr_src2);    \
    qce_expr_extract_i64(&state->solver_z3, &expr_src2, 0, bits, &expr_src2);  \
                                                                               \
    QCEPred pred;                                                              \
    qce_expr_ult_i64(&state->solver_z3, expr_dst, &expr_src13, &pred);         \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_cf, &pred);           \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    QCEExpr expr_v0x10;                                                        \
    qce_expr_init_v64(&expr_v0x10, 0x10);                                      \
    qce_expr_bvxor_i64(&state->solver_z3, expr_dst, expr_src1, &expr_af);      \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_af, &expr_src2, &expr_af);     \
    qce_expr_bvand_i64(&state->solver_z3, &expr_af, &expr_v0x10, &expr_af);    \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_af, &expr_af);             \
                                                                               \
    QCEExpr expr_v0, expr_v6;                                                  \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_v6, 6);                                            \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_shl_i32(&state->solver_z3, &expr_zf, &expr_v6, &expr_zf);         \
                                                                               \
    QCEExpr expr_v0x80;                                                        \
    qce_expr_init_v64(&expr_v0x80, 0x80);                                      \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_v0x80, &expr_sf);    \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    QCEExpr expr_v1m, expr_cc_o, expr_tmp;                                     \
    qce_expr_init_v64(&expr_v1m, -1);                                          \
    qce_expr_init_v64(&expr_cc_o, CC_O);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, expr_src1, &expr_src2, &expr_of);    \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_of, &expr_v1m, &expr_of);      \
    qce_expr_bvxor_i64(&state->solver_z3, expr_src1, expr_dst, &expr_tmp);     \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_tmp, &expr_of);      \
    qce_lshift(state, &expr_of, 12-bits, &expr_of);                            \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_cc_o, &expr_of);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_of, &expr_of);             \
                                                                               \
    qce_expr_init_v32(expr_res, 0);                                            \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_af, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_sub##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src2,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src2, 0, bits, expr_src2);    \
                                                                               \
    QCEExpr expr_cf, expr_pf, expr_af, expr_zf, expr_sf, expr_of;              \
                                                                               \
    QCEExpr expr_src1;                                                         \
    qce_expr_add_i64(&state->solver_z3, expr_dst, expr_src2, &expr_src1);      \
    qce_expr_extract_i64(&state->solver_z3, &expr_src1, 0, bits, &expr_src1);  \
                                                                               \
    QCEPred pred;                                                              \
    qce_expr_ult_i64(&state->solver_z3, &expr_src1, expr_src2, &pred);         \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_cf, &pred);           \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    QCEExpr expr_cc_a;                                                         \
    qce_expr_init_v64(&expr_cc_a, CC_A);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, expr_dst, &expr_src1, &expr_af);     \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_af, expr_src2, &expr_af);      \
    qce_expr_bvand_i64(&state->solver_z3, &expr_af, &expr_cc_a, &expr_af);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_af, &expr_af);             \
                                                                               \
    QCEExpr expr_v0, expr_cc_z;                                                \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v32(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
    qce_expr_bvand_i32(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
                                                                               \
    QCEExpr expr_cc_o, expr_tmp;                                               \
    qce_expr_init_v64(&expr_cc_o, CC_O);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_src1, expr_src2, &expr_of);    \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_src1, expr_dst, &expr_tmp);    \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_tmp, &expr_of);      \
    qce_lshift(state, &expr_of, 12-bits, &expr_of);                            \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_cc_o, &expr_of);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_of, &expr_of);             \
                                                                               \
    qce_expr_init_v32(expr_res, 0);                                            \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_af, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_sbb##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src2,                  \
      QCEExpr *expr_src3, QCEExpr *expr_res) {                                 \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src2, 0, bits, expr_src2);    \
    qce_expr_extract_i64(&state->solver_z3, expr_src3, 0, bits, expr_src3);    \
                                                                               \
    QCEExpr expr_cf, expr_pf, expr_af, expr_zf, expr_sf, expr_of;              \
                                                                               \
    QCEExpr expr_src23, expr_src1;                                             \
    qce_expr_add_i64(&state->solver_z3, expr_src2, expr_src3, &expr_src23);    \
    qce_expr_add_i64(&state->solver_z3, expr_dst, &expr_src23, &expr_src1);    \
    qce_expr_extract_i64(&state->solver_z3, &expr_src1, 0, bits, &expr_src1);  \
                                                                               \
    QCEPred pred;                                                              \
    qce_expr_ult_i64(&state->solver_z3, &expr_src1, &expr_src23, &pred);       \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_cf, &pred);           \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    QCEExpr expr_v0x10;                                                        \
    qce_expr_init_v64(&expr_v0x10, 0x10);                                      \
    qce_expr_bvxor_i64(&state->solver_z3, expr_dst, &expr_src1, &expr_af);     \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_af, expr_src2, &expr_af);      \
    qce_expr_bvand_i64(&state->solver_z3, &expr_af, &expr_v0x10, &expr_af);    \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_af, &expr_af);             \
                                                                               \
    QCEExpr expr_v0, expr_v6;                                                  \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_v6, 6);                                            \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_shl_i32(&state->solver_z3, &expr_zf, &expr_v6, &expr_zf);         \
                                                                               \
    QCEExpr expr_v0x80;                                                        \
    qce_expr_init_v64(&expr_v0x80, 0x80);                                      \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_v0x80, &expr_sf);    \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    QCEExpr expr_cc_o, expr_tmp;                                               \
    qce_expr_init_v64(&expr_cc_o, CC_O);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_src1, expr_src2, &expr_of);    \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_src1, expr_dst, &expr_tmp);    \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_tmp, &expr_of);      \
    qce_lshift(state, &expr_of, 12-bits, &expr_of);                            \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_cc_o, &expr_of);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_of, &expr_of);             \
                                                                               \
    qce_expr_init_v32(expr_res, 0);                                            \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_af, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_logic##suffix(                                \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    uint32_t cf, af, of;                                                       \
    QCEExpr expr_pf, expr_zf, expr_sf;                                         \
                                                                               \
    cf = 0;                                                                    \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    af = 0;                                                                    \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0, expr_cc_z;                                                \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    of = 0;                                                                    \
                                                                               \
    qce_expr_init_v32(expr_res, cf+af+of);                                     \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_inc##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    QCEExpr expr_cf, expr_pf, expr_af, expr_zf, expr_sf, expr_of;              \
                                                                               \
    qce_expr_extrl_i64_i32(&state->solver_z3, expr_src1, &expr_cf);            \
                                                                               \
    QCEExpr expr_v1, expr_src2;                                                \
    qce_expr_init_v64(&expr_v1, 1);                                            \
    qce_expr_sub_i64(&state->solver_z3, expr_dst, &expr_v1, expr_src1);        \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
    qce_expr_init_v64(&expr_src2, 1);                                          \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    QCEExpr expr_cc_a;                                                         \
    qce_expr_init_v64(&expr_cc_a, CC_A);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, expr_dst, expr_src1, &expr_af);      \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_af, &expr_src2, &expr_af);     \
    qce_expr_bvand_i64(&state->solver_z3, &expr_af, &expr_cc_a, &expr_af);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_af, &expr_af);             \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0, expr_cc_z;                                                \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    QCEExpr expr_sign_mask, expr_cc_o;                                         \
    qce_expr_init_v64(&expr_sign_mask, ((uint##bits##_t)1) << (bits- 1));      \
    qce_expr_init_v32(&expr_cc_o, CC_O);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_sign_mask, &pred);      \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_of, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_of, &expr_cc_o, &expr_of);       \
                                                                               \
    qce_expr_init_v32(expr_res, 0);                                            \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_af, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_dec##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    QCEExpr expr_cf, expr_pf, expr_af, expr_zf, expr_sf, expr_of;              \
                                                                               \
    qce_expr_extrl_i64_i32(&state->solver_z3, expr_src1, &expr_cf);            \
                                                                               \
    QCEExpr expr_v1, expr_src2;                                                \
    qce_expr_init_v64(&expr_v1, 1);                                            \
    qce_expr_add_i64(&state->solver_z3, expr_dst, &expr_v1, expr_src1);        \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
    qce_expr_init_v64(&expr_src2, 1);                                          \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    QCEExpr expr_cc_a;                                                         \
    qce_expr_init_v64(&expr_cc_a, CC_A);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, expr_dst, expr_src1, &expr_af);      \
    qce_expr_bvxor_i64(&state->solver_z3, &expr_af, &expr_src2, &expr_af);     \
    qce_expr_bvand_i64(&state->solver_z3, &expr_af, &expr_cc_a, &expr_af);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_af, &expr_af);             \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0, expr_cc_z;                                                \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    QCEExpr expr_sign_mask, expr_cc_o;                                         \
    qce_expr_init_v64(&expr_sign_mask, (((uint##bits##_t)1) << (bits- 1)) - 1);\
    qce_expr_init_v32(&expr_cc_o, CC_O);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_sign_mask, &pred);      \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_of, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_of, &expr_cc_o, &expr_of);       \
                                                                               \
    qce_expr_init_v32(expr_res, 0);                                            \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_af, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_shl##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    uint32_t af;                                                               \
    QCEExpr expr_cf, expr_pf, expr_zf, expr_sf, expr_of;                       \
                                                                               \
    QCEExpr expr_v, expr_cc_c;                                                 \
    qce_expr_init_v64(&expr_v, bits-1);                                        \
    qce_expr_init_v64(&expr_cc_c, CC_C);                                       \
    qce_expr_shr_i64(&state->solver_z3, expr_src1, &expr_v, &expr_cf);         \
    qce_expr_bvand_i64(&state->solver_z3, &expr_cf, &expr_cc_c, &expr_cf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_cf, &expr_cf);             \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    af = 0;                                                                    \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0, expr_cc_z;                                                \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    QCEExpr expr_cc_o;                                                         \
    qce_expr_init_v64(&expr_cc_o, CC_O);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, expr_src1, expr_dst, &expr_of);      \
    qce_lshift(state, &expr_of, 12-bits, &expr_of);                            \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_cc_o, &expr_of);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_of, &expr_of);             \
                                                                               \
    qce_expr_init_v32(expr_res, af);                                           \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_sar##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    uint32_t af;                                                               \
    QCEExpr expr_cf, expr_pf, expr_zf, expr_sf, expr_of;                       \
                                                                               \
    QCEExpr expr_v1;                                                           \
    qce_expr_init_v64(&expr_v1, 1);                                            \
    qce_expr_bvand_i64(&state->solver_z3, expr_src1, &expr_v1, &expr_cf);      \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_cf, &expr_cf);             \
                                                                               \
    qce_compute_pf(state, expr_dst, &expr_pf);                                 \
                                                                               \
    af = 0;                                                                    \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0, expr_cc_z;                                                \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    QCEExpr expr_cc_o;                                                         \
    qce_expr_init_v64(&expr_cc_o, CC_O);                                       \
    qce_expr_bvxor_i64(&state->solver_z3, expr_src1, expr_dst, &expr_of);      \
    qce_lshift(state, &expr_of, 12-bits, &expr_of);                            \
    qce_expr_bvand_i64(&state->solver_z3, &expr_of, &expr_cc_o, &expr_of);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_of, &expr_of);             \
                                                                               \
    qce_expr_init_v32(expr_res, af);                                           \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_pf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_of, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_bmilg##suffix(                                \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    uint32_t pf, af, of;                                                       \
    QCEExpr expr_cf, expr_zf, expr_sf;                                         \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0;                                                           \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_eq_i64(&state->solver_z3, expr_src1, &expr_v0, &pred);            \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_cf, &pred);           \
                                                                               \
    pf = 0;                                                                    \
    af = 0;                                                                    \
                                                                               \
    QCEExpr expr_cc_z;                                                         \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    of = 0;                                                                    \
                                                                               \
    qce_expr_init_v32(expr_res, pf+af+of);                                     \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }                                                                            \
                                                                               \
  static inline void compute_all_blsi##suffix(                                 \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    uint32_t pf, af, of;                                                       \
    QCEExpr expr_cf, expr_zf, expr_sf;                                         \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0;                                                           \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_ne_i64(&state->solver_z3, expr_src1, &expr_v0, &pred);            \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_cf, &pred);           \
                                                                               \
    pf = 0;                                                                    \
    af = 0;                                                                    \
                                                                               \
    QCEExpr expr_cc_z;                                                         \
    qce_expr_init_v32(&expr_cc_z, CC_Z);                                       \
    qce_expr_eq_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);             \
    qce_expr_init_from_pred_i32(&state->solver_z3, &expr_zf, &pred);           \
    qce_expr_mul_i32(&state->solver_z3, &expr_zf, &expr_cc_z, &expr_zf);       \
                                                                               \
    QCEExpr expr_cc_s;                                                         \
    qce_expr_init_v64(&expr_cc_s, CC_S);                                       \
    qce_lshift(state, expr_dst, 8-bits, &expr_sf);                             \
    qce_expr_bvand_i64(&state->solver_z3, &expr_sf, &expr_cc_s, &expr_sf);     \
    qce_expr_extrl_i64_i32(&state->solver_z3, &expr_sf, &expr_sf);             \
                                                                               \
    of = 0;                                                                    \
                                                                               \
    qce_expr_init_v32(expr_res, pf+af+of);                                     \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_cf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_zf, expr_res);         \
    qce_expr_add_i32(&state->solver_z3, expr_res, &expr_sf, expr_res);         \
    qce_expr_extu_i32_i64(&state->solver_z3, expr_res, expr_res);              \
  }

DEFINE_CC_COMPUTE_ALL_HELPERS(b, 8)
DEFINE_CC_COMPUTE_ALL_HELPERS(w, 16)
DEFINE_CC_COMPUTE_ALL_HELPERS(l, 32)
DEFINE_CC_COMPUTE_ALL_HELPERS(q, 64)

static inline void qce_helper_cc_compute_all(
    QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,
    QCEExpr *expr_src2, QCEExpr *expr_opc, QCEExpr *expr_res) {
  switch (expr_opc->v_i32) {
  default: /* should never happen */ {
    qce_expr_init_v64(expr_res, 0);
    break;
  }

  case CC_OP_EFLAGS: {
    memcpy(expr_res, expr_src1, sizeof(QCEExpr));
    break;
  }
  case CC_OP_POPCNT: {
    QCEExpr expr_v0, expr_cc_z;
    qce_expr_init_v64(&expr_v0, 0);
    qce_expr_init_v64(&expr_cc_z, CC_Z);
    QCEPred pred;
    qce_expr_ne_i64(&state->solver_z3, expr_dst, &expr_v0, &pred);
    qce_expr_ite_i64(&state->solver_z3, &pred, &expr_v0, &expr_cc_z, expr_res);
    break;
  }

  case CC_OP_MULB:
    compute_all_mulb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_MULW:
    compute_all_mulw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_MULL:
    compute_all_mull(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_MULQ:
    compute_all_mulq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_ADDB:
    compute_all_addb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_ADDW:
    compute_all_addw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_ADDL:
    compute_all_addl(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_ADDQ:
    compute_all_addq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_ADCB:
    compute_all_adcb(state, expr_dst, expr_src1, expr_src2, expr_res);
    break;
  case CC_OP_ADCW:
    compute_all_adcw(state, expr_dst, expr_src1, expr_src2, expr_res);
    break;
  case CC_OP_ADCL:
    compute_all_adcl(state, expr_dst, expr_src1, expr_src2, expr_res);
    break;
  case CC_OP_ADCQ:
    compute_all_adcq(state, expr_dst, expr_src1, expr_src2, expr_res);
    break;

  case CC_OP_SUBB:
    compute_all_subb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SUBW:
    compute_all_subw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SUBL:
    compute_all_subl(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SUBQ:
    compute_all_subq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_SBBB:
    compute_all_sbbb(state, expr_dst, expr_src1, expr_src2, expr_res);
    break;
  case CC_OP_SBBW:
    compute_all_sbbw(state, expr_dst, expr_src1, expr_src2, expr_res);
    break;
  case CC_OP_SBBL:
    compute_all_sbbl(state, expr_dst, expr_src1, expr_src2, expr_res);
    break;
  case CC_OP_SBBQ:
    compute_all_sbbq(state, expr_dst, expr_src1, expr_src2, expr_res);
    break;

  case CC_OP_LOGICB:
    compute_all_logicb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_LOGICW:
    compute_all_logicw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_LOGICL:
    compute_all_logicl(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_LOGICQ:
    compute_all_logicq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_INCB:
    compute_all_incb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_INCW:
    compute_all_incw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_INCL:
    compute_all_incl(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_INCQ:
    compute_all_incq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_DECB:
    compute_all_decb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_DECW:
    compute_all_decw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_DECL:
    compute_all_decl(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_DECQ:
    compute_all_decq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_SHLB:
    compute_all_shlb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SHLW:
    compute_all_shlw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SHLL:
    compute_all_shll(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SHLQ:
    compute_all_shlq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_SARB:
    compute_all_sarb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SARW:
    compute_all_sarw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SARL:
    compute_all_sarl(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_SARQ:
    compute_all_sarq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_BMILGB:
    compute_all_bmilgb(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_BMILGW:
    compute_all_bmilgw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_BMILGL:
    compute_all_bmilgl(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_BMILGQ:
    compute_all_bmilgq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_BLSIB:
    compute_all_blsib(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_BLSIW:
    compute_all_blsiw(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_BLSIL:
    compute_all_blsil(state, expr_dst, expr_src1, expr_res);
    break;
  case CC_OP_BLSIQ:
    compute_all_blsiq(state, expr_dst, expr_src1, expr_res);
    break;

  case CC_OP_ADCX: {
    QCEExpr expr_v, expr_tmp1, expr_tmp2;
    qce_expr_init_v64(&expr_v, ~CC_C);
    qce_expr_bvand_i64(&state->solver_z3, expr_src1, &expr_v, &expr_tmp1);
    qce_expr_init_v64(&expr_v, CC_C);
    qce_expr_mul_i64(&state->solver_z3, expr_dst, &expr_v, &expr_tmp2);
    qce_expr_bvor_i64(&state->solver_z3, &expr_tmp1, &expr_tmp2, expr_res);
    break;
  }
  case CC_OP_ADOX: {
    QCEExpr expr_v, expr_tmp1, expr_tmp2;
    qce_expr_init_v64(&expr_v, ~CC_O);
    qce_expr_bvand_i64(&state->solver_z3, expr_src1, &expr_v, &expr_tmp1);
    qce_expr_init_v64(&expr_v, CC_O);
    qce_expr_mul_i64(&state->solver_z3, expr_src2, &expr_v, &expr_tmp2);
    qce_expr_bvor_i64(&state->solver_z3, &expr_tmp1, &expr_tmp2, expr_res);
    break;
  }
  case CC_OP_ADCOX: {
    QCEExpr expr_v, expr_tmp1, expr_tmp2, expr_tmp3;
    qce_expr_init_v64(&expr_v, ~(CC_C | CC_O));
    qce_expr_bvand_i64(&state->solver_z3, expr_src1, &expr_v, &expr_tmp1);
    qce_expr_init_v64(&expr_v, CC_C);
    qce_expr_mul_i64(&state->solver_z3, expr_dst, &expr_v, &expr_tmp2);
    qce_expr_init_v64(&expr_v, CC_O);
    qce_expr_mul_i64(&state->solver_z3, expr_src2, &expr_v, &expr_tmp3);
    qce_expr_bvor_i64(&state->solver_z3, &expr_tmp1, &expr_tmp2, expr_res);
    qce_expr_bvor_i64(&state->solver_z3, expr_res, &expr_tmp3, expr_res);
    break;
  }
  }
}

static inline void qce_sym_inst_call_cc_compute_all(
    CPUArchState *env, QCEState *state, QCEVar *dst, QCEVar *src1,
    QCEVar *src2, QCEVar *opc, QCEVar *res) {
  QCEExpr expr_dst, expr_src1, expr_src2, expr_opc;
  qce_state_get_var(env, state, dst, &expr_dst);
  qce_state_get_var(env, state, src1, &expr_src1);
  qce_state_get_var(env, state, src2, &expr_src2);
  qce_state_get_var(env, state, opc, &expr_opc);
  /* mode checking */
//  qce_expr_assert_mode(&expr_src1, CONCRETE);
//  qce_expr_assert_mode(&expr_src2, CONCRETE);
  qce_expr_assert_mode(&expr_opc, CONCRETE);
  if (expr_src1.mode == QCE_EXPR_SYMBOLIC) {
    qce_fatal("cc_compute_all: src1 symbolic");
  }
  if (expr_src2.mode == QCE_EXPR_SYMBOLIC) {
    qce_fatal("cc_compute_all: src2 symbolic");
  }
  /* type checking */
  qce_expr_assert_type(&expr_dst, I64);
  qce_expr_assert_type(&expr_src1, I64);
  qce_expr_assert_type(&expr_src2, I64);
  qce_expr_assert_type(&expr_opc, I32);

  QCEExpr expr_res;
  qce_helper_cc_compute_all(state, &expr_dst, &expr_src1, &expr_src2,
                            &expr_opc, &expr_res);
  qce_state_put_var(env, state, res, &expr_res);
}

#define DEFINE_CC_COMPUTE_C_HELPERS(suffix, bits)                              \
  static inline void compute_c_add##suffix(                                    \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    QCEPred pred;                                                              \
    qce_expr_ult_i64(&state->solver_z3, expr_dst, expr_src1, &pred);           \
    qce_expr_init_from_pred_i64(&state->solver_z3, expr_res, &pred);           \
  }                                                                            \
                                                                               \
  static inline void compute_c_adc##suffix(                                    \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_src3, QCEExpr *expr_res) {                                 \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src3);    \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_src13;                                                        \
    qce_expr_add_i64(&state->solver_z3, expr_src1, expr_src3, &expr_src13);    \
    qce_expr_ult_i64(&state->solver_z3, expr_dst, &expr_src13, &pred);         \
    qce_expr_init_from_pred_i64(&state->solver_z3, expr_res, &pred);           \
  }                                                                            \
                                                                               \
  static inline void compute_c_sub##suffix(                                    \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src2,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src2, 0, bits, expr_src2);    \
                                                                               \
    QCEPred pred;                                                              \
    qce_expr_add_i64(&state->solver_z3, expr_dst, expr_src2, expr_res);        \
    qce_expr_ult_i64(&state->solver_z3, expr_res, expr_src2, &pred);           \
    qce_expr_init_from_pred_i64(&state->solver_z3, expr_res, &pred);           \
  }                                                                            \
                                                                               \
  static inline void compute_c_sbb##suffix(                                    \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src2,                  \
      QCEExpr *expr_src3, QCEExpr *expr_res) {                                 \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src2, 0, bits, expr_src2);    \
    qce_expr_extract_i64(&state->solver_z3, expr_src3, 0, bits, expr_src3);    \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_src23, expr_src1;                                             \
    qce_expr_add_i64(&state->solver_z3, expr_src2, expr_src3, &expr_src23);    \
    qce_expr_add_i64(&state->solver_z3, expr_dst, &expr_src23, &expr_src1);    \
    qce_expr_extract_i64(&state->solver_z3, &expr_src1, 0, bits, &expr_src1);  \
    qce_expr_ult_i64(&state->solver_z3, &expr_src1, &expr_src23, &pred);       \
    qce_expr_init_from_pred_i64(&state->solver_z3, expr_res, &pred);           \
  }                                                                            \
                                                                               \
  static inline void compute_c_shl##suffix(                                    \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    QCEExpr expr_v, expr_cc_c;                                                 \
    qce_expr_init_v64(&expr_v, bits-1);                                        \
    qce_expr_init_v64(&expr_cc_c, CC_C);                                       \
    qce_expr_shr_i64(&state->solver_z3, expr_src1, &expr_v, expr_res);         \
    qce_expr_bvand_i64(&state->solver_z3, expr_res, &expr_cc_c, expr_res);     \
  }                                                                            \
                                                                               \
  static inline void compute_c_bmilg##suffix(                                  \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0;                                                           \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_eq_i64(&state->solver_z3, expr_src1, &expr_v0, &pred);            \
    qce_expr_init_from_pred_i64(&state->solver_z3, expr_res, &pred);           \
  }                                                                            \
                                                                               \
  static inline void compute_c_blsi##suffix(                                   \
      QCEState *state, QCEExpr *expr_dst, QCEExpr *expr_src1,                  \
      QCEExpr *expr_res) {                                                     \
    qce_expr_extract_i64(&state->solver_z3, expr_dst, 0, bits, expr_dst);      \
    qce_expr_extract_i64(&state->solver_z3, expr_src1, 0, bits, expr_src1);    \
                                                                               \
    QCEPred pred;                                                              \
    QCEExpr expr_v0;                                                           \
    qce_expr_init_v64(&expr_v0, 0);                                            \
    qce_expr_ne_i64(&state->solver_z3, expr_src1, &expr_v0, &pred);            \
    qce_expr_init_from_pred_i64(&state->solver_z3, expr_res, &pred);           \
  }

DEFINE_CC_COMPUTE_C_HELPERS(b, 8)
DEFINE_CC_COMPUTE_C_HELPERS(w, 16)
DEFINE_CC_COMPUTE_C_HELPERS(l, 32)
DEFINE_CC_COMPUTE_C_HELPERS(q, 64)

static inline void qce_sym_inst_call_cc_compute_c(
    CPUArchState *env, QCEState *state, QCEVar *dst, QCEVar *src1,
    QCEVar *src2, QCEVar *opc, QCEVar *res) {
  QCEExpr expr_dst, expr_src1, expr_src2, expr_opc;
  qce_state_get_var(env, state, dst, &expr_dst);
  qce_state_get_var(env, state, src1, &expr_src1);
  qce_state_get_var(env, state, src2, &expr_src2);
  qce_state_get_var(env, state, opc, &expr_opc);
  /* mode checking */
//  qce_expr_assert_mode(&expr_src2, CONCRETE);
  qce_expr_assert_mode(&expr_opc, CONCRETE);
  if (expr_src2.mode == QCE_EXPR_SYMBOLIC) {
    qce_fatal("cc_compute_c: src2 symbolic");
  }
  /* type checking */
  qce_expr_assert_type(&expr_dst, I64);
  qce_expr_assert_type(&expr_src1, I64);
  qce_expr_assert_type(&expr_src2, I64);
  qce_expr_assert_type(&expr_opc, I32);

  QCEExpr expr_res;
  switch (expr_opc.v_i32) {
  default: /* should never happen */
  case CC_OP_LOGICB:
  case CC_OP_LOGICW:
  case CC_OP_LOGICL:
  case CC_OP_LOGICQ:
  case CC_OP_POPCNT: {
    qce_expr_init_v64(&expr_res, 0);
    break;
  }

  case CC_OP_EFLAGS:
  case CC_OP_SARB:
  case CC_OP_SARW:
  case CC_OP_SARL:
  case CC_OP_SARQ:
  case CC_OP_ADOX:{
    QCEExpr expr_v1;
    qce_expr_init_v64(&expr_v1, 1);
    qce_expr_bvand_i64(&state->solver_z3, &expr_src1, &expr_v1, &expr_res);
    break;
  }

  case CC_OP_INCB:
  case CC_OP_INCW:
  case CC_OP_INCL:
  case CC_OP_INCQ:
  case CC_OP_DECB:
  case CC_OP_DECW:
  case CC_OP_DECL:
  case CC_OP_DECQ: {
    qce_state_put_var(env, state, res, &expr_src1);
    return;
  }

  case CC_OP_MULB:
  case CC_OP_MULW:
  case CC_OP_MULL:
  case CC_OP_MULQ: {
    QCEPred pred;
    QCEExpr expr_v0;
    qce_expr_init_v64(&expr_v0, 0);
    qce_expr_ne_i64(&state->solver_z3, &expr_src1, &expr_v0, &pred);
    qce_expr_init_from_pred_i64(&state->solver_z3, &expr_res, &pred);
    break;
  }

  case CC_OP_ADCX:
  case CC_OP_ADCOX: {
    qce_state_put_var(env, state, res, &expr_dst);
    return;
  }

  case CC_OP_ADDB:
    compute_c_addb(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_ADDW:
    compute_c_addw(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_ADDL:
    compute_c_addl(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_ADDQ:
    compute_c_addq(state, &expr_dst, &expr_src1, &expr_res);
    break;

  case CC_OP_ADCB:
    compute_c_adcb(state, &expr_dst, &expr_src1, &expr_src2, &expr_res);
    break;
  case CC_OP_ADCW:
    compute_c_adcw(state, &expr_dst, &expr_src1, &expr_src2, &expr_res);
    break;
  case CC_OP_ADCL:
    compute_c_adcl(state, &expr_dst, &expr_src1, &expr_src2, &expr_res);
    break;
  case CC_OP_ADCQ:
    compute_c_adcq(state, &expr_dst, &expr_src1, &expr_src2, &expr_res);
    break;

  case CC_OP_SUBB:
    compute_c_subb(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_SUBW:
    compute_c_subw(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_SUBL:
    compute_c_subl(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_SUBQ:
    compute_c_subq(state, &expr_dst, &expr_src1, &expr_res);
    break;

  case CC_OP_SBBB:
    compute_c_sbbb(state, &expr_dst, &expr_src1, &expr_src2, &expr_res);
    break;
  case CC_OP_SBBW:
    compute_c_sbbw(state, &expr_dst, &expr_src1, &expr_src2, &expr_res);
    break;
  case CC_OP_SBBL:
    compute_c_sbbl(state, &expr_dst, &expr_src1, &expr_src2, &expr_res);
    break;
  case CC_OP_SBBQ:
    compute_c_sbbq(state, &expr_dst, &expr_src1, &expr_src2, &expr_res);
    break;

  case CC_OP_SHLB:
    compute_c_shlb(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_SHLW:
    compute_c_shlw(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_SHLL:
    compute_c_shll(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_SHLQ:
    compute_c_shlq(state, &expr_dst, &expr_src1, &expr_res);
    break;

  case CC_OP_BMILGB:
    compute_c_bmilgb(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_BMILGW:
    compute_c_bmilgw(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_BMILGL:
    compute_c_bmilgl(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_BMILGQ:
    compute_c_bmilgq(state, &expr_dst, &expr_src1, &expr_res);
    break;

  case CC_OP_BLSIB:
    compute_c_blsib(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_BLSIW:
    compute_c_blsiw(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_BLSIL:
    compute_c_blsil(state, &expr_dst, &expr_src1, &expr_res);
    break;
  case CC_OP_BLSIQ:
    compute_c_blsiq(state, &expr_dst, &expr_src1, &expr_res);
    break;
  }

  qce_state_put_var(env, state, res, &expr_res);
}

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
  qce_expr_assert_mode(&expr_opc, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_dst, I64);
  qce_expr_assert_type(&expr_src, I64);
  qce_expr_assert_type(&expr_opc, I32);

  QCEExpr expr_res;
  if (CC_OP_HAS_EFLAGS(expr_opc.v_i32)) {
    QCEExpr expr_cc_z;
    qce_expr_init_v64(&expr_cc_z, CC_Z);
    qce_expr_bvnot_i64(&state->solver_z3, &expr_src, &expr_res);
    qce_expr_bvand_i64(&state->solver_z3, &expr_res, &expr_cc_z, &expr_res);
  } else {
    MemOp size = cc_op_size(expr_opc.v_i32);
    QCEExpr expr_mask;
    qce_expr_init_v64(&expr_mask, MAKE_64BIT_MASK(0, 8 << size));
    qce_expr_bvand_i64(&state->solver_z3, &expr_dst, &expr_mask, &expr_res);
  }

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
  qce_expr_assert_mode(&expr_cc_dst, CONCRETE);
  qce_expr_assert_mode(&expr_cc_src, CONCRETE);
  qce_expr_assert_mode(&expr_cc_src2, CONCRETE);
  qce_expr_assert_mode(&expr_cc_op, CONCRETE);

  return helper_cc_compute_all(expr_cc_dst.v_i64, expr_cc_src.v_i64,
                               expr_cc_src2.v_i64, expr_cc_op.v_i32);
}

static inline uint32_t qce_cpu_compute_eflags(CPUX86State *env,
                                              QCEState * state) {
  QCEExpr expr_df, expr_eflags;
  qce_state_env_get_i32(state, (intptr_t)&env->df, &expr_df);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);
  qce_expr_assert_mode(&expr_df, CONCRETE);
  qce_expr_assert_mode(&expr_eflags, CONCRETE);

  uint32_t eflags = expr_eflags.v_i64;
  if (tcg_enabled()) {
    eflags |= qce_cpu_cc_compute_all(env, state) | (expr_df.v_i32 & DF_MASK);
  }
  return eflags;
}

static inline void qce_cpu_load_eflags(CPUX86State *env, QCEState * state,
                                       int eflags, int update_mask) {
  QCEExpr expr_cc_src, expr_cc_op, expr_df, expr_eflags;
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);
  qce_expr_assert_mode(&expr_eflags, CONCRETE);

  qce_expr_init_v64(&expr_cc_src,
                    eflags & (CC_O | CC_S | CC_Z | CC_A | CC_P | CC_C));
  qce_expr_init_v32(&expr_cc_op, CC_OP_EFLAGS);
  qce_expr_init_v32(&expr_df, 1 - (2 * ((eflags >> 10) & 1)));
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
  qce_expr_assert_mode(&expr_hflags, CONCRETE);
  qce_expr_assert_mode(&expr_cs_flags, CONCRETE);
  qce_expr_assert_mode(&expr_ss_flags, CONCRETE);
  qce_expr_assert_mode(&expr_cr0, CONCRETE);
  qce_expr_assert_mode(&expr_eflags, CONCRETE);
  qce_expr_assert_mode(&expr_ds_base, CONCRETE);
  qce_expr_assert_mode(&expr_es_base, CONCRETE);
  qce_expr_assert_mode(&expr_ss_base, CONCRETE);

  unsigned int new_hflags;

  qce_expr_init_v32(&expr_sr_selector, selector);
  qce_expr_init_v64(&expr_sr_base, base);
  qce_expr_init_v32(&expr_sr_limit, limit);
  qce_expr_init_v32(&expr_sr_flags, flags);

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
  qce_state_env_get_i64(state, (intptr_t)&env->eip, &expr_eip);
  qce_state_env_get_i64(state, (intptr_t)&env->regs[11], &expr_r11);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);
  qce_state_env_get_i64(state, (intptr_t)&env->fmask, &expr_fmask);
  qce_state_env_get_i64(state, (intptr_t)&env->lstar, &expr_lstar);
  qce_state_env_get_i64(state, (intptr_t)&env->cstar, &expr_cstar);
  qce_expr_assert_mode(&expr_efer, CONCRETE);
  qce_expr_assert_mode(&expr_star, CONCRETE);
  qce_expr_assert_mode(&expr_hflags, CONCRETE);
  qce_expr_assert_mode(&expr_eip, CONCRETE);
  qce_expr_assert_mode(&expr_r11, CONCRETE);
  qce_expr_assert_mode(&expr_eflags, CONCRETE);
  qce_expr_assert_mode(&expr_fmask, CONCRETE);
  qce_expr_assert_mode(&expr_lstar, CONCRETE);
  qce_expr_assert_mode(&expr_cstar, CONCRETE);

  int selector;

  if (!(expr_efer.v_i64 & MSR_EFER_SCE)) {
    qce_fatal("syscall raises an exception");
//    raise_exception_err_ra(env, EXCP06_ILLOP, 0, GETPC());
  }
  selector = ((uint64_t)expr_star.v_i64 >> 32) & 0xffff;
#ifdef TARGET_X86_64
  if (expr_hflags.v_i32 & HF_LMA_MASK) {
    int code64;

    qce_expr_init_v64(&expr_rcx, expr_eip.v_i64 + expr_next_eip.v_i32);
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
    qce_expr_init_v64(&expr_rcx,
                      (uint32_t)(expr_eip.v_i64 + expr_next_eip.v_i32));

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
  qce_state_env_get_i64(state, (intptr_t)&env->regs[R_ECX], &expr_rcx);
  qce_state_env_get_i64(state, (intptr_t)&env->eflags, &expr_eflags);
  qce_expr_assert_mode(&expr_efer, CONCRETE);
  qce_expr_assert_mode(&expr_hflags, CONCRETE);
  qce_expr_assert_mode(&expr_cr0, CONCRETE);
  qce_expr_assert_mode(&expr_star, CONCRETE);
  qce_expr_assert_mode(&expr_r11, CONCRETE);
  qce_expr_assert_mode(&expr_rcx, CONCRETE);
  qce_expr_assert_mode(&expr_eflags, CONCRETE);

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
      qce_expr_init_v64(&expr_eip, expr_rcx.v_i64);
    } else {
      qce_cpu_x86_load_seg_cache(env, state, R_CS, selector | 3,
                                 0, 0xffffffff,
                                 DESC_G_MASK | DESC_B_MASK | DESC_P_MASK |
                                    DESC_S_MASK | (3 << DESC_DPL_SHIFT) |
                                    DESC_CS_MASK | DESC_R_MASK | DESC_A_MASK);
      qce_expr_init_v64(&expr_eip, (uint32_t)expr_rcx.v_i64);
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
    qce_expr_init_v64(&expr_eip, (uint32_t)expr_rcx.v_i64);
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
  qce_expr_assert_mode(&expr_eflags, CONCRETE);

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
#define B(n) ZMM_B(n)
#define W(n) ZMM_W(n)
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
    qce_expr_init_v64(&expr_into_d, expr_from_v.v_i64);
    qce_state_env_put_i64(state, (intptr_t)&ptr_d->Q(i), &expr_into_d);
    qce_expr_init_v64(&expr_into_d, expr_from_s.v_i64);
    qce_state_env_put_i64(state, (intptr_t)&ptr_d->Q(i + 1), &expr_into_d);
  }
}

static inline void qce_sym_inst_call_punpcklbw_xmm(
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

  uint8_t r[PACK_WIDTH * 2];
  int j, i;

  for (j = 0; j < 8 << SHIFT; ) {
    g_assert(PACK_WIDTH % 4 == 0);
    for (i = 0; i < PACK_WIDTH; i+=4) {
      QCEExpr expr_from_v, expr_from_s;
      qce_state_env_get_i32(state, (intptr_t)&ptr_v->B(j + i), &expr_from_v);
      qce_state_env_get_i32(state, (intptr_t)&ptr_s->B(j + i), &expr_from_s);
      qce_expr_assert_mode(&expr_from_v, CONCRETE);
      qce_expr_assert_mode(&expr_from_s, CONCRETE);
      for (uint8_t byte = 0; byte < 4; ++byte) {
        r[2 * (i + byte)] = ((uint8_t *)&expr_from_v.v_i32)[byte];
        r[2 * (i + byte) + 1] = ((uint8_t *)&expr_from_s.v_i32)[byte];
      }
    }
    g_assert(PACK_WIDTH * 2 % 4 == 0);
    for (i = 0; i < PACK_WIDTH * 2; i+=4, j+=4) {
      QCEExpr expr_into_d;
      qce_expr_init_v32(&expr_into_d, 0);
      for (uint8_t byte = 0; byte < 4; ++byte) {
        ((uint8_t *)&expr_into_d.v_i32)[byte] = r[i + byte];
      }
      qce_state_env_put_i32(state, (intptr_t)&ptr_d->B(j), &expr_into_d);
    }
  }
}

static inline void qce_sym_inst_call_punpcklwd_xmm(
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

  uint16_t r[PACK_WIDTH];
  int j, i;

  for (j = 0; j < 4 << SHIFT; ) {
    g_assert(PACK_WIDTH / 2 % 2 == 0);
    for (i = 0; i < PACK_WIDTH / 2; i+=2) {
      QCEExpr expr_from_v, expr_from_s;
      qce_state_env_get_i32(state, (intptr_t)&ptr_v->W(j + i), &expr_from_v);
      qce_state_env_get_i32(state, (intptr_t)&ptr_s->W(j + i), &expr_from_s);
      qce_expr_assert_mode(&expr_from_v, CONCRETE);
      qce_expr_assert_mode(&expr_from_s, CONCRETE);
      for (uint8_t byte = 0; byte < 2; ++byte) {
        r[2 * (i + byte)] = ((uint16_t *)&expr_from_v.v_i32)[byte];
        r[2 * (i + byte) + 1] = ((uint16_t *)&expr_from_s.v_i32)[byte];
      }
    }
    g_assert(PACK_WIDTH % 2 == 0);
    for (i = 0; i < PACK_WIDTH; i+=2, j+=2) {
      QCEExpr expr_into_d;
      qce_expr_init_v32(&expr_into_d, 0);
      for (uint8_t byte = 0; byte < 2; ++byte) {
        ((uint16_t *)&expr_into_d.v_i32)[byte] = r[i + byte];
      }
      qce_state_env_put_i32(state, (intptr_t)&ptr_d->W(j), &expr_into_d);
    }
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
    qce_expr_assert_mode(&expr_from_s0, CONCRETE);
    qce_expr_assert_mode(&expr_from_s1, CONCRETE);
    qce_expr_assert_mode(&expr_from_s2, CONCRETE);
    qce_expr_assert_mode(&expr_from_s3, CONCRETE);

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
    qce_expr_assert_mode(&expr_rax, CONCRETE);                                 \
    qce_expr_assert_mode(&expr_rdx, CONCRETE);                                 \
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

static inline void qce_sym_inst_call_divl_EAX(
    CPUArchState *env, QCEState *state, QCEVar *val) {
  QCEExpr expr_val;
  qce_state_get_var(env, state, val, &expr_val);
  /* mode checking */
  qce_expr_assert_mode(&expr_val, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_val, I64);

  QCEExpr expr_rax, expr_rdx;
  qce_state_env_get_i64(state, (intptr_t)&env->regs[R_EAX], &expr_rax);
  qce_state_env_get_i64(state, (intptr_t)&env->regs[R_EDX], &expr_rdx);
  qce_expr_assert_mode(&expr_rax, CONCRETE);
  qce_expr_assert_mode(&expr_rdx, CONCRETE);

  unsigned int den, r;
  uint64_t num, q;

  num = ((uint32_t)expr_rax.v_i64) |
        ((uint64_t)((uint32_t)expr_rdx.v_i64) << 32);
  den = expr_val.v_i64;
  if (den == 0) {
    qce_fatal("divl raises an exception");
//    raise_exception_ra(env, EXCP00_DIVZ, GETPC());
  }
  q = (num / den);
  r = (num % den);
  if (q > 0xffffffff) {
    qce_fatal("divl raises an exception");
//    raise_exception_ra(env, EXCP00_DIVZ, GETPC());
  }
  expr_rax.v_i64 = (uint32_t)q;
  expr_rdx.v_i64 = (uint32_t)r;

  qce_state_env_put_i64(state, (intptr_t)&env->regs[R_EAX], &expr_rax);
  qce_state_env_put_i64(state, (intptr_t)&env->regs[R_EDX], &expr_rdx);
}

static inline void qce_sym_inst_call_idivl_EAX(
    CPUArchState *env, QCEState *state, QCEVar *val) {
  QCEExpr expr_val;
  qce_state_get_var(env, state, val, &expr_val);
  /* mode checking */
  qce_expr_assert_mode(&expr_val, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_val, I64);

  QCEExpr expr_rax, expr_rdx;
  qce_state_env_get_i64(state, (intptr_t)&env->regs[R_EAX], &expr_rax);
  qce_state_env_get_i64(state, (intptr_t)&env->regs[R_EDX], &expr_rdx);
  qce_expr_assert_mode(&expr_rax, CONCRETE);
  qce_expr_assert_mode(&expr_rdx, CONCRETE);

  int den, r;
  int64_t num, q;

  num = ((uint32_t)expr_rax.v_i64) |
        ((uint64_t)((uint32_t)expr_rdx.v_i64) << 32);
  den = expr_val.v_i64;
  if (den == 0) {
    qce_fatal("idivl raises an exception");
//    raise_exception_ra(env, EXCP00_DIVZ, GETPC());
  }
  q = (num / den);
  r = (num % den);
  if (q != (int32_t)q) {
    qce_fatal("idivl raises an exception");
//    raise_exception_ra(env, EXCP00_DIVZ, GETPC());
  }
  expr_rax.v_i64 = (uint32_t)q;
  expr_rdx.v_i64 = (uint32_t)r;

  qce_state_env_put_i64(state, (intptr_t)&env->regs[R_EAX], &expr_rax);
  qce_state_env_put_i64(state, (intptr_t)&env->regs[R_EDX], &expr_rdx);
}

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
  qce_expr_assert_mode(&expr_df, CONCRETE);
  qce_expr_assert_mode(&expr_eflags, CONCRETE);

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

tcg_target_ulong qce_helper_inb_ret;
static inline void qce_sym_inst_call_inb(
    CPUArchState *env, QCEState *state, QCEVar *port, QCEVar *res) {
  QCEExpr expr_port;
  qce_state_get_var(env, state, port, &expr_port);
  /* mode checking */
  qce_expr_assert_mode(&expr_port, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_port, I32);

  QCEExpr expr_res;
  qce_expr_init_v64(&expr_res,
                    g_qce->session->emulation_ctx.call_inst_helper_ret);
  qce_state_put_var(env, state, res, &expr_res);
}

#define HANDLE_SYM_INST_CALL_in(name)                                          \
  case QCE_INST_CALL_in##name: {                                               \
    static bool retried = false;                                               \
    if (!retried) {                                                            \
      cursor-=1;                                                               \
      retried = true;                                                          \
      goto suspend_emulation;                                                  \
    } else {                                                                   \
      retried = false;                                                         \
    }                                                                          \
    qce_sym_inst_call_in##name(                                                \
        arch, &session->state,                                                 \
        &inst->i_call_in##name.port, &inst->i_call_in##name.res);              \
    break;                                                                     \
  }

static inline void qce_sym_inst_call_outb(
    CPUArchState *env, QCEState *state, QCEVar *port, QCEVar *val) {
  QCEExpr expr_port, expr_val;
  qce_state_get_var(env, state, port, &expr_port);
  qce_state_get_var(env, state, val, &expr_val);
  /* mode checking */
  qce_expr_assert_mode(&expr_port, CONCRETE);
  qce_expr_assert_mode(&expr_val, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_port, I32);
  qce_expr_assert_type(&expr_val, I32);
}

#define HANDLE_SYM_INST_CALL_out(name)                                         \
  case QCE_INST_CALL_out##name: {                                              \
    qce_sym_inst_call_out##name(                                               \
        arch, &session->state,                                                 \
        &inst->i_call_out##name.port, &inst->i_call_out##name.val);            \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_iret_protected(
    CPUArchState *env, QCEState *state, QCEVar *shift, QCEVar *next_eip) {
  QCEExpr expr_shift, expr_next_eip;
  qce_state_get_var(env, state, shift, &expr_shift);
  qce_state_get_var(env, state, next_eip, &expr_next_eip);
  /* mode checking */
  qce_expr_assert_mode(&expr_shift, CONCRETE);
  qce_expr_assert_mode(&expr_next_eip, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_shift, I32);
  qce_expr_assert_type(&expr_next_eip, I32);
}

#define HANDLE_SYM_INST_CALL_iret_protected                                    \
  case QCE_INST_CALL_iret_protected: {                                         \
    qce_sym_inst_call_iret_protected(arch, &session->state,                    \
                                     &inst->i_call_iret_protected.shift,       \
                                     &inst->i_call_iret_protected.next_eip);   \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_rdtsc(CPUArchState *env,
                                           QCEState *state) {
}

#define HANDLE_SYM_INST_CALL_rdtsc                                             \
  case QCE_INST_CALL_rdtsc: {                                                  \
    qce_sym_inst_call_rdtsc(arch, &session->state);                            \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_write_crN(
    CPUArchState *env, QCEState *state, QCEVar *n, QCEVar *val) {
  QCEExpr expr_n, expr_val;
  qce_state_get_var(env, state, n, &expr_n);
  qce_state_get_var(env, state, val, &expr_val);
  /* mode checking */
  qce_expr_assert_mode(&expr_n, CONCRETE);
  qce_expr_assert_mode(&expr_val, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_n, I32);
  qce_expr_assert_type(&expr_val, I64);
}

#define HANDLE_SYM_INST_CALL_write_crN                                         \
  case QCE_INST_CALL_write_crN: {                                              \
    qce_sym_inst_call_write_crN(                                               \
        arch, &session->state,                                                 \
        &inst->i_call_write_crN.n, &inst->i_call_write_crN.val);               \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_fxsave(
    CPUArchState *env, QCEState *state, QCEVar *val) {
  QCEExpr expr_val;
  qce_state_get_var(env, state, val, &expr_val);
  /* mode checking */
  qce_expr_assert_mode(&expr_val, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_val, I64);
}

#define HANDLE_SYM_INST_CALL_fxsave                                            \
  case QCE_INST_CALL_fxsave: {                                                 \
    qce_sym_inst_call_fxsave(arch, &session->state, &inst->i_call_fxsave.val); \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_wrmsr(
    CPUArchState *env, QCEState *state) {
}

#define HANDLE_SYM_INST_CALL_wrmsr                                             \
  case QCE_INST_CALL_wrmsr: {                                                  \
    qce_sym_inst_call_wrmsr(arch, &session->state);                            \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_load_seg(
    CPUArchState *env, QCEState *state, QCEVar *segment, QCEVar *selector) {
  QCEExpr expr_segment, expr_selector;
  qce_state_get_var(env, state, segment, &expr_segment);
  qce_state_get_var(env, state, selector, &expr_selector);
  /* mode checking */
  qce_expr_assert_mode(&expr_segment, CONCRETE);
  qce_expr_assert_mode(&expr_selector, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_segment, I32);
  qce_expr_assert_type(&expr_selector, I32);
}

#define HANDLE_SYM_INST_CALL_load_seg                                          \
  case QCE_INST_CALL_load_seg: {                                               \
    qce_sym_inst_call_load_seg(                                                \
        arch, &session->state,                                                 \
        &inst->i_call_load_seg.segment, &inst->i_call_load_seg.selector);      \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_flush_page(
    CPUArchState *env, QCEState *state, QCEVar *addr) {
  QCEExpr expr_addr;
  qce_state_get_var(env, state, addr, &expr_addr);
  /* mode checking */
  qce_expr_assert_mode(&expr_addr, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_addr, I64);
}

#define HANDLE_SYM_INST_CALL_flush_page                                        \
  case QCE_INST_CALL_flush_page: {                                             \
    qce_sym_inst_call_flush_page(                                              \
        arch, &session->state, &inst->i_call_flush_page.addr);                 \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_fclex(CPUArchState *env,
                                           QCEState *state) {
  QCEExpr expr_fpu;
  qce_state_env_get_i32(state, (intptr_t)&env->fpus, &expr_fpu);
  qce_expr_assert_mode(&expr_fpu, CONCRETE);

  uint16_t fpus = (uint16_t)expr_fpu.v_i32;
  fpus &= 0x7f00;
  expr_fpu.v_i32 &= 0xffff0000;
  expr_fpu.v_i32 |= (uint32_t)fpus;

  qce_state_env_put_i32(state, (intptr_t)&env->fpus, &expr_fpu);
}

#define HANDLE_SYM_INST_CALL_fclex                                             \
  case QCE_INST_CALL_fclex: {                                                  \
    qce_sym_inst_call_fclex(arch, &session->state);                            \
    break;                                                                     \
  }

static inline void qce_sym_inst_call_emms(CPUArchState *env,
                                          QCEState *state) {
  QCEExpr expr_fptags;
  qce_expr_init_v64(&expr_fptags, 0x0101010101010101);
  qce_state_env_put_i64(state, (intptr_t)&env->fptags, &expr_fptags);
}

#define HANDLE_SYM_INST_CALL_emms                                              \
  case QCE_INST_CALL_emms: {                                                   \
    qce_sym_inst_call_emms(arch, &session->state);                             \
    break;                                                                     \
  }

static inline void qce_sym_inst_call_fildl_ST0(
    CPUArchState *env, QCEState *state, QCEVar *val) {
  QCEExpr expr_val;
  qce_state_get_var(env, state, val, &expr_val);
  /* mode checking */
  qce_expr_assert_mode(&expr_val, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_val, I32);
}

#define HANDLE_SYM_INST_CALL_fildl_ST0                                         \
  case QCE_INST_CALL_fildl_ST0: {                                              \
    qce_sym_inst_call_fildl_ST0(arch, &session->state,                         \
                                &inst->i_call_fildl_ST0.val);                  \
    goto suspend_emulation;                                                    \
  }

static inline void qce_sym_inst_call_fxrstor(
    CPUArchState *env, QCEState *state, QCEVar *val) {
  QCEExpr expr_val;
  qce_state_get_var(env, state, val, &expr_val);
  /* mode checking */
  qce_expr_assert_mode(&expr_val, CONCRETE);
  /* type checking */
  qce_expr_assert_type(&expr_val, I64);
}

#define HANDLE_SYM_INST_CALL_fxrstor                                           \
  case QCE_INST_CALL_fxrstor: {                                                \
    qce_sym_inst_call_fxrstor(                                                 \
        arch, &session->state, &inst->i_call_fxrstor.val);                     \
    goto suspend_emulation;                                                    \
  }

#define HANDLE_SYM_INST_CALL_hlt                                               \
  case QCE_INST_CALL_hlt: {                                                    \
    session->emulation_ctx.status = QCE_Emulation_Normal;                      \
    qce_state_reset(&session->state);                                          \
    goto end_of_loop;                                                          \
  }

#endif /* QCE_SYM_CALL_H */
