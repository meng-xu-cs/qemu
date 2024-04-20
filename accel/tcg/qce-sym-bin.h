#ifndef QCE_SYM_BIN_H
#define QCE_SYM_BIN_H

#define DEFINE_SYM_INST_BIN_OP(name, bits)                                     \
  static inline void qce_sym_inst_##name##_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *v1, QCEVar *v2,              \
      QCEVar *res) {                                                           \
    QCEExpr expr_v1, expr_v2;                                                  \
    qce_state_get_var(env, state, v1, &expr_v1);                               \
    qce_state_get_var(env, state, v2, &expr_v2);                               \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_##name##_i##bits(&state->solver_z3, &expr_v1, &expr_v2,           \
                              &expr_res);                                      \
    qce_state_put_var(env, state, res, &expr_res);                             \
  }

DEFINE_SYM_INST_BIN_OP(add, 32)
DEFINE_SYM_INST_BIN_OP(add, 64)
DEFINE_SYM_INST_BIN_OP(sub, 32)
DEFINE_SYM_INST_BIN_OP(sub, 64)

#define HANDLE_SYM_INST_BIN_OP(key, name, bits)                                \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state, &inst->i_##name##_i##bits.v1,                   \
        &inst->i_##name##_i##bits.v2, &inst->i_##name##_i##bits.res);          \
    break;                                                                     \
  }

#define DEFINE_SYM_INST_BIN_OP_BV(name, bits)                                  \
  static inline void qce_sym_inst_bv##name##_i##bits(                          \
      CPUArchState *env, QCEState *state, QCEVar *v1, QCEVar *v2,              \
      QCEVar *res) {                                                           \
    QCEExpr expr_v1, expr_v2;                                                  \
    qce_state_get_var(env, state, v1, &expr_v1);                               \
    qce_state_get_var(env, state, v2, &expr_v2);                               \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_bv##name##_i##bits(&state->solver_z3, &expr_v1, &expr_v2,         \
                                &expr_res);                                    \
    qce_state_put_var(env, state, res, &expr_res);                             \
  }

DEFINE_SYM_INST_BIN_OP_BV(and, 32)
DEFINE_SYM_INST_BIN_OP_BV(and, 64)
DEFINE_SYM_INST_BIN_OP_BV(or, 32)
DEFINE_SYM_INST_BIN_OP_BV(or, 64)
DEFINE_SYM_INST_BIN_OP_BV(xor, 32)
DEFINE_SYM_INST_BIN_OP_BV(xor, 64)

#define HANDLE_SYM_INST_BIN_OP_BV(key, name, bits)                             \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_bv##name##_i##bits(                                           \
        arch, &session->state, &inst->i_##name##_i##bits.v1,                   \
        &inst->i_##name##_i##bits.v2, &inst->i_##name##_i##bits.res);          \
    break;                                                                     \
  }

#endif /* QCE_SYM_BIN_H */
