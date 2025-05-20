#ifndef QCE_SYM_CALL_H
#define QCE_SYM_CALL_H

#include "exec/helper-proto.h"

#define DEFINE_SYM_INST_CALL_cc_compute(name)                                  \
  static inline void qce_sym_inst_call_cc_compute_##name(                      \
        CPUArchState *env, QCEState *state, QCEVar *dst, QCEVar *src1,         \
        QCEVar *src2, QCEVar *opc, QCEVar *res) {                              \
    QCEExpr expr_dst, expr_src1, expr_src2, expr_opc;                          \
    qce_state_get_var(env, state, dst, &expr_dst);                             \
    qce_state_get_var(env, state, src1, &expr_src1);                           \
    qce_state_get_var(env, state, src2, &expr_src2);                           \
    qce_state_get_var(env, state, opc, &expr_opc);                             \
    qce_debug_assert(expr_dst.type == QCE_EXPR_I64);                           \
    qce_debug_assert(expr_src1.type == QCE_EXPR_I64);                          \
    qce_debug_assert(expr_src2.type == QCE_EXPR_I64);                          \
    qce_debug_assert(expr_opc.type == QCE_EXPR_I32);                           \
    qce_debug_assert(expr_dst.mode == QCE_EXPR_CONCRETE);                      \
    qce_debug_assert(expr_src1.mode == QCE_EXPR_CONCRETE);                     \
    qce_debug_assert(expr_src2.mode == QCE_EXPR_CONCRETE);                     \
    qce_debug_assert(expr_opc.mode == QCE_EXPR_CONCRETE);                      \
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
  qce_debug_assert(expr_dst.type == QCE_EXPR_I64);
  qce_debug_assert(expr_src.type == QCE_EXPR_I64);
  qce_debug_assert(expr_opc.type == QCE_EXPR_I32);
  qce_debug_assert(expr_dst.mode == QCE_EXPR_CONCRETE);
  qce_debug_assert(expr_src.mode == QCE_EXPR_CONCRETE);
  qce_debug_assert(expr_opc.mode == QCE_EXPR_CONCRETE);

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

#endif /* QCE_SYM_CALL_H */
