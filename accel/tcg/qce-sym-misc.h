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

#define DEFINE_SYM_INST_extract2(bits)                                         \
  static inline void qce_sym_inst_extract2_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *v_b, QCEVar *v_t,            \
      tcg_target_ulong pos, QCEVar *res) {                                     \
    QCEExpr expr_v_b, expr_v_t;                                                \
    qce_state_get_var(env, state, v_b, &expr_v_b);                             \
    qce_state_get_var(env, state, v_t, &expr_v_t);                             \
    qce_debug_assert(0 < pos && pos < bits);                                   \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_extract2_i##bits(&state->solver_z3, &expr_v_b, &expr_v_t,         \
                             pos, &expr_res);                                  \
    qce_state_put_var(env, state, res, &expr_res);                             \
}

DEFINE_SYM_INST_extract2(32)
DEFINE_SYM_INST_extract2(64)

#define HANDLE_SYM_INST_extract2(bits)                                         \
  case QCE_INST_EXTRACT2_I##bits: {                                            \
    qce_sym_inst_extract2_i##bits(                                             \
        arch, &session->state,                                                 \
        &inst->i_extract2_i##bits.v_b, &inst->i_extract2_i##bits.v_t,          \
        inst->i_extract2_i##bits.pos, &inst->i_extract2_i##bits.res);          \
    break;                                                                     \
}

#endif /* QCE_SYM_MISC_H */
