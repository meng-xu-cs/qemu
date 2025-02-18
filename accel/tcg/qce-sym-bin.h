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
DEFINE_SYM_INST_BIN_OP(mul, 32)
DEFINE_SYM_INST_BIN_OP(mul, 64)

DEFINE_SYM_INST_BIN_OP(shl, 32)
DEFINE_SYM_INST_BIN_OP(shl, 64)
DEFINE_SYM_INST_BIN_OP(shr, 32)
DEFINE_SYM_INST_BIN_OP(shr, 64)
DEFINE_SYM_INST_BIN_OP(sar, 32)
DEFINE_SYM_INST_BIN_OP(sar, 64)

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

DEFINE_SYM_INST_BIN_OP_BV(andc, 32)
DEFINE_SYM_INST_BIN_OP_BV(andc, 64)
DEFINE_SYM_INST_BIN_OP_BV(orc, 32)
DEFINE_SYM_INST_BIN_OP_BV(orc, 64)
DEFINE_SYM_INST_BIN_OP_BV(nand, 32)
DEFINE_SYM_INST_BIN_OP_BV(nand, 64)
DEFINE_SYM_INST_BIN_OP_BV(nor, 32)
DEFINE_SYM_INST_BIN_OP_BV(nor, 64)
DEFINE_SYM_INST_BIN_OP_BV(eqv, 32)
DEFINE_SYM_INST_BIN_OP_BV(eqv, 64)

#define HANDLE_SYM_INST_BIN_OP_BV(key, name, bits)                             \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_bv##name##_i##bits(                                           \
        arch, &session->state, &inst->i_##name##_i##bits.v1,                   \
        &inst->i_##name##_i##bits.v2, &inst->i_##name##_i##bits.res);          \
    break;                                                                     \
  }

#define DEFINE_SYM_INST_BIN_OP_BIN_RES(name, bits)                             \
  static inline void qce_sym_inst_##name##_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *v1, QCEVar *v2,              \
      QCEVar *res_t, QCEVar *res_b) {                                          \
    QCEExpr expr_v1, expr_v2;                                                  \
    qce_state_get_var(env, state, v1, &expr_v1);                               \
    qce_state_get_var(env, state, v2, &expr_v2);                               \
                                                                               \
    QCEExpr expr_res_t, expr_res_b;                                            \
    qce_expr_##name##_i##bits(&state->solver_z3, &expr_v1, &expr_v2,           \
                              &expr_res_t, &expr_res_b);                       \
    qce_state_put_var(env, state, res_t, &expr_res_t);                         \
    qce_state_put_var(env, state, res_b, &expr_res_b);                         \
  }

// DEFINE_SYM_INST_BIN_OP_BIN_RES(mulu2, 32)
// DEFINE_SYM_INST_BIN_OP_BIN_RES(mulu2, 64)
DEFINE_SYM_INST_BIN_OP_BIN_RES(muls2, 32)
DEFINE_SYM_INST_BIN_OP_BIN_RES(muls2, 64)

#define HANDLE_SYM_INST_BIN_OP_BIN_RES(key, name, bits)                        \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state,                                                 \
        &inst->i_##name##_i##bits.v1, &inst->i_##name##_i##bits.v2,            \
        &inst->i_##name##_i##bits.res_t, &inst->i_##name##_i##bits.res_b);     \
    break;                                                                     \
  }

#define DEFINE_SYM_INST_QUAD_OP(name, bits)                                    \
  static inline void qce_sym_inst_##name##_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *v1_t, QCEVar *v1_b,          \
      QCEVar *v2_t, QCEVar *v2_b, QCEVar *res_t, QCEVar *res_b) {              \
    QCEExpr expr_v1_t, expr_v1_b, expr_v2_t, expr_v2_b;                        \
    qce_state_get_var(env, state, v1_t, &expr_v1_t);                           \
    qce_state_get_var(env, state, v1_b, &expr_v1_b);                           \
    qce_state_get_var(env, state, v2_t, &expr_v2_t);                           \
    qce_state_get_var(env, state, v2_b, &expr_v2_b);                           \
                                                                               \
    QCEExpr expr_res_t, expr_res_b;                                            \
    qce_expr_##name##_i##bits(&state->solver_z3, &expr_v1_t, &expr_v1_b,       \
                              &expr_v2_t, &expr_v2_b,                          \
                              &expr_res_t, &expr_res_b);                       \
    qce_state_put_var(env, state, res_t, &expr_res_t);                         \
    qce_state_put_var(env, state, res_b, &expr_res_b);                         \
  }

DEFINE_SYM_INST_QUAD_OP(add2, 32)
DEFINE_SYM_INST_QUAD_OP(add2, 64)
DEFINE_SYM_INST_QUAD_OP(sub2, 32)
DEFINE_SYM_INST_QUAD_OP(sub2, 64)

#define HANDLE_SYM_INST_QUAD_OP(key, name, bits)                               \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state,                                                 \
        &inst->i_##name##_i##bits.v1_t, &inst->i_##name##_i##bits.v1_b,        \
        &inst->i_##name##_i##bits.v2_t, &inst->i_##name##_i##bits.v2_t,        \
        &inst->i_##name##_i##bits.res_t, &inst->i_##name##_i##bits.res_b);     \
    break;                                                                     \
  }

#endif /* QCE_SYM_BIN_H */
