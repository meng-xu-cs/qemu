#ifndef QCE_SYM_CALL_H
#define QCE_SYM_CALL_H

#include "exec/helper-proto.h"

static inline void qce_sym_inst_call_cc_compute_all(
    CPUArchState *env, QCEState *state, QCEVar *dst, QCEVar *src1,
    QCEVar *src2, QCEVar *opc, QCEVar *res) {
  QCEExpr expr_dst, expr_src1, expr_src2, expr_opc;
  qce_state_get_var(env, state, dst, &expr_dst);
  qce_state_get_var(env, state, src1, &expr_src1);
  qce_state_get_var(env, state, src2, &expr_src2);
  qce_state_get_var(env, state, opc, &expr_opc);

  QCEExpr expr_res;
  expr_res.type = QCE_EXPR_I64;
  expr_res.mode = QCE_EXPR_CONCRETE;
  expr_res.v_i64 = helper_cc_compute_all(expr_dst.v_i64, expr_src1.v_i64,
                                         expr_src2.v_i64, expr_opc.v_i32);
  qce_state_put_var(env, state, res, &expr_res);
}

#define HANDLE_SYM_INST_CALL_cc_compute_all                                    \
  case QCE_INST_CALL_cc_compute_all: {                                         \
    qce_sym_inst_call_cc_compute_all(                                          \
        arch, &session->state, &inst->i_call_cc_compute_all.dst,               \
        &inst->i_call_cc_compute_all.src1, &inst->i_call_cc_compute_all.src2,  \
        &inst->i_call_cc_compute_all.opc, &inst->i_call_cc_compute_all.res);   \
    break;                                                                     \
}

#endif /* QCE_SYM_CALL_H */
