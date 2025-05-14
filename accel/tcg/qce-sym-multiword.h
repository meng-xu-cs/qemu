#ifndef QCE_SYM_MULTIWORD_H
#define QCE_SYM_MULTIWORD_H

#define DEFINE_SYM_INST_MULTIWORD_OP(name, bits)                               \
  static inline void qce_sym_inst_##name##_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *v1_b, QCEVar *v1_t,          \
      QCEVar *v2_b, QCEVar *v2_t, QCEVar *res_b, QCEVar *res_t) {              \
    QCEExpr expr_v1_b, expr_v1_t, expr_v2_b, expr_v2_t;                        \
    qce_state_get_var(env, state, v1_b, &expr_v1_b);                           \
    qce_state_get_var(env, state, v1_t, &expr_v1_t);                           \
    qce_state_get_var(env, state, v2_b, &expr_v2_b);                           \
    qce_state_get_var(env, state, v2_t, &expr_v2_t);                           \
                                                                               \
    QCEExpr expr_res_b, expr_res_t;                                            \
    qce_expr_##name##_i##bits(&state->solver_z3, &expr_v1_b, &expr_v1_t,       \
                              &expr_v2_b, &expr_v2_t,                          \
                              &expr_res_b, &expr_res_t);                       \
    qce_state_put_var(env, state, res_b, &expr_res_b);                         \
    qce_state_put_var(env, state, res_t, &expr_res_t);                         \
  }

DEFINE_SYM_INST_MULTIWORD_OP(add2, 32)
DEFINE_SYM_INST_MULTIWORD_OP(add2, 64)
DEFINE_SYM_INST_MULTIWORD_OP(sub2, 32)
DEFINE_SYM_INST_MULTIWORD_OP(sub2, 64)

#define HANDLE_SYM_INST_MULTIWORD_OP(key, name, bits)                          \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state,                                                 \
        &inst->i_##name##_i##bits.v1_b, &inst->i_##name##_i##bits.v1_t,        \
        &inst->i_##name##_i##bits.v2_b, &inst->i_##name##_i##bits.v2_t,        \
        &inst->i_##name##_i##bits.res_b, &inst->i_##name##_i##bits.res_t);     \
    break;                                                                     \
  }

#define DEFINE_SYM_INST_MULTIWORD_OP2(name, bits)                              \
  static inline void qce_sym_inst_##name##_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *v1, QCEVar *v2,              \
      QCEVar *res_b, QCEVar *res_t) {                                          \
    QCEExpr expr_v1, expr_v2;                                                  \
    qce_state_get_var(env, state, v1, &expr_v1);                               \
    qce_state_get_var(env, state, v2, &expr_v2);                               \
                                                                               \
    QCEExpr expr_res_b, expr_res_t;                                            \
    qce_expr_##name##_i##bits(&state->solver_z3, &expr_v1, &expr_v2,           \
                              &expr_res_b, &expr_res_t);                       \
    qce_state_put_var(env, state, res_b, &expr_res_b);                         \
    qce_state_put_var(env, state, res_t, &expr_res_t);                         \
}

// DEFINE_SYM_INST_MULTIWORD_OP2(mulu2, 32)
// DEFINE_SYM_INST_MULTIWORD_OP2(mulu2, 64)
DEFINE_SYM_INST_MULTIWORD_OP2(muls2, 32)
DEFINE_SYM_INST_MULTIWORD_OP2(muls2, 64)

#define HANDLE_SYM_INST_MULTIWORD_OP2(key, name, bits)                         \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state,                                                 \
        &inst->i_##name##_i##bits.v1, &inst->i_##name##_i##bits.v2,            \
        &inst->i_##name##_i##bits.res_b, &inst->i_##name##_i##bits.res_t);     \
    break;                                                                     \
}

#endif /* QCE_SYM_MULTIWORD_H */
