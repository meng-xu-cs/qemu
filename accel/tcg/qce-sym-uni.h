#ifndef QCE_SYM_UNI_H
#define QCE_SYM_UNI_H

#define DEFINE_SYM_INST_UNI_OP(name, bits)                                     \
  static inline void qce_sym_inst_##name##_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *v, QCEVar *res) {            \
    QCEExpr expr_v;                                                            \
    qce_state_get_var(env, state, v, &expr_v);                                 \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_##name##_i##bits(&state->solver_z3, &expr_v, &expr_res);          \
    qce_state_put_var(env, state, res, &expr_res);                             \
}

DEFINE_SYM_INST_UNI_OP(neg, 32)
DEFINE_SYM_INST_UNI_OP(neg, 64)

#define HANDLE_SYM_INST_UNI_OP(key, name, bits)                                \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state,                                                 \
        &inst->i_##name##_i##bits.v, &inst->i_##name##_i##bits.res);           \
    break;                                                                     \
}

#endif /* QCE_SYM_UNI_H */
