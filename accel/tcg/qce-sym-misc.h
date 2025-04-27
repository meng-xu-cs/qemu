#ifndef QCE_SYM_MISC_H
#define QCE_SYM_MISC_H

#define DEFINE_SYM_INST_deposit(bits)                                          \
  static inline void qce_sym_inst_deposit_i##bits(                             \
      CPUArchState *env, QCEState *state, QCEVar *into, QCEVar *from,          \
      tcg_target_ulong pos, tcg_target_ulong len, QCEVar *res) {               \
    QCEExpr expr_into, expr_from;                                              \
    qce_state_get_var(env, state, into, &expr_into);                           \
    qce_state_get_var(env, state, from, &expr_from);                           \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_deposit_i##bits(&state->solver_z3, &expr_into, &expr_from,        \
                             pos, len, &expr_res);                             \
    qce_state_put_var(env, state, res, &expr_res);                             \
}

DEFINE_SYM_INST_deposit(32)
DEFINE_SYM_INST_deposit(64)

#define HANDLE_SYM_INST_deposit(bits)                                          \
  case QCE_INST_DEPOSIT_I##bits: {                                             \
    qce_sym_inst_deposit_i##bits(                                              \
        arch, &session->state, &inst->i_deposit_i##bits.into,                  \
        &inst->i_deposit_i##bits.from, inst->i_deposit_i##bits.pos,            \
        inst->i_deposit_i##bits.len, &inst->i_deposit_i##bits.res);            \
    break;                                                                     \
}

#endif /* QCE_SYM_MISC_H */
