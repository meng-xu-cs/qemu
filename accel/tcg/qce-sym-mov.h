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

#define DEFINE_SYM_INST_setcond(bits)                                          \
  static inline void qce_sym_inst_setcond_i##bits(                             \
      CPUArchState *env, QCEState *state, QCEVar *v1, QCEVar *v2,              \
      tcg_target_ulong cond, QCEVar *res) {                                    \
    QCEExpr expr_c1, expr_c2, expr_v1, expr_v2;                                \
    expr_c1.mode = QCE_EXPR_CONCRETE;                                          \
    expr_c1.type = QCE_EXPR_I##bits;                                           \
    expr_c1.v_i##bits = 1;                                                     \
    expr_c2.mode = QCE_EXPR_CONCRETE;                                          \
    expr_c2.type = QCE_EXPR_I##bits;                                           \
    expr_c2.v_i##bits = 0;                                                     \
    qce_state_get_var(env, state, v1, &expr_v1);                               \
    qce_state_get_var(env, state, v2, &expr_v2);                               \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_COND_MOV##_i##bits(&state->solver_z3, &expr_c1, &expr_c2,         \
                                &expr_v1, &expr_v2, cond, &expr_res);          \
    qce_state_put_var(env, state, res, &expr_res);                             \
}

DEFINE_SYM_INST_setcond(32);
DEFINE_SYM_INST_setcond(64);

#define HANDLE_SYM_INST_setcond(bits)                                          \
  case QCE_INST_SETCOND_I##bits: {                                             \
    qce_sym_inst_setcond_i##bits(                                              \
        arch, &session->state,                                                 \
        &inst->i_setcond_i##bits.v1, &inst->i_setcond_i##bits.v2,              \
        inst->i_setcond_i##bits.cond,&inst->i_setcond_i##bits.res);            \
    break;                                                                     \
}

#define DEFINE_SYM_INST_negsetcond(bits)                                       \
  static inline void qce_sym_inst_negsetcond_i##bits(                          \
      CPUArchState *env, QCEState *state, QCEVar *v1, QCEVar *v2,              \
      tcg_target_ulong cond, QCEVar *res) {                                    \
    QCEExpr expr_c1, expr_c2, expr_v1, expr_v2;                                \
    expr_c1.mode = QCE_EXPR_CONCRETE;                                          \
    expr_c1.type = QCE_EXPR_I##bits;                                           \
    expr_c1.v_i##bits = -1;                                                    \
    expr_c2.mode = QCE_EXPR_CONCRETE;                                          \
    expr_c2.type = QCE_EXPR_I##bits;                                           \
    expr_c2.v_i##bits = 0;                                                     \
    qce_state_get_var(env, state, v1, &expr_v1);                               \
    qce_state_get_var(env, state, v2, &expr_v2);                               \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_COND_MOV##_i##bits(&state->solver_z3, &expr_c1, &expr_c2,         \
                                &expr_v1, &expr_v2, cond, &expr_res);          \
    qce_state_put_var(env, state, res, &expr_res);                             \
}

DEFINE_SYM_INST_negsetcond(32);
DEFINE_SYM_INST_negsetcond(64);

#define HANDLE_SYM_INST_negsetcond(bits)                                       \
  case QCE_INST_NEGSETCOND_I##bits: {                                          \
    qce_sym_inst_negsetcond_i##bits(                                           \
        arch, &session->state,                                                 \
        &inst->i_negsetcond_i##bits.v1, &inst->i_negsetcond_i##bits.v2,        \
        inst->i_negsetcond_i##bits.cond,&inst->i_negsetcond_i##bits.res);      \
    break;                                                                     \
}

#define DEFINE_SYM_INST_movcond(bits)                                          \
  static inline void qce_sym_inst_movcond_i##bits(                             \
      CPUArchState *env, QCEState *state, QCEVar *c1, QCEVar *c2,              \
      QCEVar *v1, QCEVar *v2, tcg_target_ulong cond, QCEVar *res) {            \
    QCEExpr expr_c1, expr_c2, expr_v1, expr_v2;                                \
    qce_state_get_var(env, state, c1, &expr_c1);                               \
    qce_state_get_var(env, state, c2, &expr_c2);                               \
    qce_state_get_var(env, state, v1, &expr_v1);                               \
    qce_state_get_var(env, state, v2, &expr_v2);                               \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_COND_MOV##_i##bits(&state->solver_z3, &expr_c1, &expr_c2,         \
                               &expr_v1, &expr_v2, cond, &expr_res);           \
    qce_state_put_var(env, state, res, &expr_res);                             \
}

DEFINE_SYM_INST_movcond(32);
DEFINE_SYM_INST_movcond(64);

#define HANDLE_SYM_INST_movcond(bits)                                          \
  case QCE_INST_MOVCOND_I##bits: {                                             \
    qce_sym_inst_movcond_i##bits(                                              \
        arch, &session->state,                                                 \
        &inst->i_movcond_i##bits.c1,&inst->i_movcond_i##bits.c2,               \
        &inst->i_movcond_i##bits.v1, &inst->i_movcond_i##bits.v2,              \
        inst->i_movcond_i##bits.cond,&inst->i_movcond_i##bits.res);            \
    break;                                                                     \
}

#endif /* QCE_SYM_MOV_H */
