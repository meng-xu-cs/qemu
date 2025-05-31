#ifndef QCE_SYM_MISC_H
#define QCE_SYM_MISC_H

#define DEFINE_SYM_INST_bswap(n, bits)                                         \
  static inline void qce_sym_inst_bswap##n##_i##bits(                          \
      CPUArchState *env, QCEState *state, QCEVar *v, tcg_target_ulong flag,    \
      QCEVar *res) {                                                           \
    QCEExpr expr_v;                                                            \
    qce_state_get_var(env, state, v, &expr_v);                                 \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_bswap##n##_i##bits(&state->solver_z3, &expr_v, flag, &expr_res);  \
    qce_state_put_var(env, state, res, &expr_res);                             \
}

DEFINE_SYM_INST_bswap(16, 32)
DEFINE_SYM_INST_bswap(32, 32)
DEFINE_SYM_INST_bswap(16, 64)
DEFINE_SYM_INST_bswap(32, 64)
DEFINE_SYM_INST_bswap(64, 64)

#define HANDLE_SYM_INST_bswap(n, bits)                                         \
  case QCE_INST_BSWAP##n##_I##bits: {                                          \
    qce_sym_inst_bswap##n##_i##bits(                                           \
        arch, &session->state, &inst->i_bswap##n##_i##bits.v,                  \
        inst->i_bswap##n##_i##bits.flag, &inst->i_bswap##n##_i##bits.res);     \
    break;                                                                     \
}

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

#define DEFINE_SYM_INST_extract(name, bits)                                    \
  static inline void qce_sym_inst_##name##_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *from, tcg_target_ulong pos,  \
      tcg_target_ulong len, QCEVar *res) {                                     \
    QCEExpr expr_from;                                                         \
    qce_state_get_var(env, state, from, &expr_from);                           \
                                                                               \
    QCEExpr expr_res;                                                          \
    qce_expr_##name##_i##bits(&state->solver_z3, &expr_from, pos, len,         \
                              &expr_res);                                      \
    qce_state_put_var(env, state, res, &expr_res);                             \
}

DEFINE_SYM_INST_extract(extract, 32)
DEFINE_SYM_INST_extract(extract, 64)
DEFINE_SYM_INST_extract(sextract, 32)
DEFINE_SYM_INST_extract(sextract, 64)

#define HANDLE_SYM_INST_extract(key, name, bits)                               \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state,                                                 \
        &inst->i_##name##_i##bits.from, inst->i_##name##_i##bits.pos,          \
        inst->i_##name##_i##bits.len, &inst->i_##name##_i##bits.res);          \
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

#define DEFINE_SYM_INST_extr_i64_i32(side)                                     \
  static inline void qce_sym_inst_extr##side##_i64_i32(                        \
      CPUArchState *env, QCEState *state, QCEVar *into, QCEVar *from) {        \
    QCEExpr expr_from;                                                         \
    qce_state_get_var(env, state, from, &expr_from);                           \
                                                                               \
    QCEExpr expr_into;                                                         \
    qce_expr_extr##side##_i64_i32(&state->solver_z3,                           \
                                   &expr_from, &expr_into);                    \
    qce_state_put_var(env, state, into, &expr_into);                           \
}

DEFINE_SYM_INST_extr_i64_i32(l)
DEFINE_SYM_INST_extr_i64_i32(h)

#define HANDLE_SYM_INST_extr_i64_i32(SIDE, side)                               \
  case QCE_INST_TRUNC##SIDE: {                                                 \
    qce_sym_inst_extr##side##_i64_i32(arch, &session->state,                   \
                                      &inst->i_extr##side##_i64_i32.into,      \
                                      &inst->i_extr##side##_i64_i32.from);     \
    break;                                                                     \
}

#define DEFINE_SYM_INST_ext_i32_i64(sign)                                      \
  static inline void qce_sym_inst_ext##sign##_i32_i64(                         \
      CPUArchState *env, QCEState *state, QCEVar *into, QCEVar *from) {        \
    QCEExpr expr;                                                              \
    qce_state_get_var(env, state, from, &expr);                                \
    QCEExpr res;                                                               \
    qce_expr_ext##sign##_i32_i64(&state->solver_z3, &expr, &res);              \
    qce_state_put_var(env, state, into, &res);                                 \
  }

DEFINE_SYM_INST_ext_i32_i64( )
DEFINE_SYM_INST_ext_i32_i64(u)

#define HANDLE_SYM_INST_ext_i32_i64(sign, SIGN)                                \
  case QCE_INST_EXT##SIGN: {                                                   \
    qce_sym_inst_ext##sign##_i32_i64(arch, &session->state,                    \
                                        &inst->i_ext##sign##_i32_i64.into,     \
                                        &inst->i_ext##sign##_i32_i64.from);    \
    break;                                                                     \
  }

#endif /* QCE_SYM_MISC_H */
