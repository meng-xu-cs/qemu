#ifndef QCE_SYM_MOV_H
#define QCE_SYM_MOV_H

#define DEFINE_SYM_INST_mov(bits)                                              \
  static inline void qce_sym_inst_mov_i##bits(                                 \
      CPUArchState *env, QCEState *state, QCEVar *from, QCEVar *into) {        \
    QCEExpr expr;                                                              \
    qce_state_get_var(env, state, from, &expr);                                \
    qce_state_put_var(env, state, into, &expr);                                \
  }

DEFINE_SYM_INST_mov(32);
DEFINE_SYM_INST_mov(64);

#define HANDLE_SYM_INST_mov(bits)                                              \
  case QCE_INST_MOV_I##bits: {                                                 \
    qce_sym_inst_mov_i##bits(arch, &session->state, &inst->i_mov_i##bits.from, \
                             &inst->i_mov_i##bits.into);                       \
    break;                                                                     \
  }

#define DEFINE_SYM_INST_ext(bits, n, sign)                                     \
  static inline void qce_sym_inst_ext##n##sign##_i##bits(                      \
      CPUArchState *env, QCEState *state, QCEVar *from, QCEVar *into) {        \
    QCEExpr expr;                                                              \
    qce_state_get_var(env, state, from, &expr);                                \
    QCEExpr res;                                                               \
    qce_expr_ld##n##sign##_i##bits(&state->solver_z3, &expr, &res);            \
    qce_state_put_var(env, state, into, &res);                                 \
  }

DEFINE_SYM_INST_ext(32, 8, u);
DEFINE_SYM_INST_ext(32, 8, s);
DEFINE_SYM_INST_ext(32, 16, u);
DEFINE_SYM_INST_ext(32, 16, s);
DEFINE_SYM_INST_ext(64, 8, u);
DEFINE_SYM_INST_ext(64, 8, s);
DEFINE_SYM_INST_ext(64, 16, u);
DEFINE_SYM_INST_ext(64, 16, s);
DEFINE_SYM_INST_ext(64, 32, u);
DEFINE_SYM_INST_ext(64, 32, s);

#define HANDLE_SYM_INST_ext(bits, n, sign, SIGN)                               \
  case QCE_INST_EXT##n##SIGN##_I##bits: {                                      \
    qce_sym_inst_ext##n##sign##_i##bits(arch, &session->state,                 \
                                        &inst->i_mov_i##bits.from,             \
                                        &inst->i_mov_i##bits.into);            \
    break;                                                                     \
  }

#endif /* QCE_SYM_MOV_H */
