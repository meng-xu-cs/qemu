#ifndef QCE_SYM_LD_ST_H
#define QCE_SYM_LD_ST_H

#ifndef QCE_RELEASE
#define QCE_ENV_ADDR_OFFSET_LOWER_BOUND -0x4000
#define QCE_ENV_ADDR_OFFSET_UPPER_BOUND 0x4000
#endif

static inline intptr_t __qce_sym_derive_memory_index(CPUArchState *env,
                                                     QCEState *state,
                                                     QCEVar *addr,
                                                     tcg_target_ulong offset) {
  QCEExpr expr_addr;
  qce_state_get_var(env, state, addr, &expr_addr);
  qce_debug_assert(expr_addr.type == QCE_EXPR_I64);

  QCEExpr expr_offset;
  qce_expr_init_v64(&expr_offset, offset);

  QCEExpr expr_index;
  qce_expr_add_i64(&state->solver_z3, &expr_addr, &expr_offset, &expr_index);

  // index must be a constant (and within range)
  if (expr_index.mode != QCE_EXPR_CONCRETE) {
    qce_fatal("unexpected symbolic address on host memory");
  }
#ifndef QCE_RELEASE
  ptrdiff_t diff = expr_index.v_i64 - (intptr_t)env;
  if (diff <= QCE_ENV_ADDR_OFFSET_LOWER_BOUND ||
      diff >= QCE_ENV_ADDR_OFFSET_UPPER_BOUND) {
    qce_fatal("host memory offset out of bound: 0x%lx", diff);
  }
#endif

  // done
  return expr_index.v_i64;
}

#define DEFINE_SYM_INST_ld_full(bits)                                          \
  static inline void qce_sym_inst_ld_i##bits(                                  \
      CPUArchState *env, QCEState *state, QCEVar *addr,                        \
      tcg_target_ulong offset, QCEVar *res) {                                  \
    /* load */                                                                 \
    intptr_t index = __qce_sym_derive_memory_index(env, state, addr, offset);  \
    QCEExpr expr_cell;                                                         \
    qce_state_env_get_i##bits(state, index, &expr_cell);                       \
    /* assign */                                                               \
    qce_state_put_var(env, state, res, &expr_cell);                            \
  }

#define DEFINE_SYM_INST_st_full(bits)                                          \
  static inline void qce_sym_inst_st_i##bits(                                  \
      CPUArchState *env, QCEState *state, QCEVar *addr,                        \
      tcg_target_ulong offset, QCEVar *val) {                                  \
    /* retrieve */                                                             \
    QCEExpr expr_val;                                                          \
    qce_state_get_var(env, state, val, &expr_val);                             \
    /* store */                                                                \
    intptr_t index = __qce_sym_derive_memory_index(env, state, addr, offset);  \
    qce_state_env_put_i##bits(state, index, &expr_val);                        \
  }

#define DEFINE_SYM_INST_ld_part(bits, n, sign)                                 \
  static inline void qce_sym_inst_ld##n##sign##_i##bits(                       \
      CPUArchState *env, QCEState *state, QCEVar *addr,                        \
      tcg_target_ulong offset, QCEVar *res) {                                  \
    /* load */                                                                 \
    intptr_t index = __qce_sym_derive_memory_index(env, state, addr, offset);  \
    QCEExpr expr_cell;                                                         \
    qce_state_env_get_i##bits(state, index, &expr_cell);                       \
    /* op */                                                                   \
    QCEExpr expr_val;                                                          \
    qce_expr_ld##n##sign##_i##bits(&state->solver_z3, &expr_cell, &expr_val);  \
    /* assign */                                                               \
    qce_state_put_var(env, state, res, &expr_val);                             \
  }

#define DEFINE_SYM_INST_st_part(bits, n)                                       \
  static inline void qce_sym_inst_st##n##_i##bits(                             \
      CPUArchState *env, QCEState *state, QCEVar *addr,                        \
      tcg_target_ulong offset, QCEVar *val) {                                  \
    /* retrieve */                                                             \
    QCEExpr expr_val;                                                          \
    qce_state_get_var(env, state, val, &expr_val);                             \
    /* op */                                                                   \
    intptr_t index = __qce_sym_derive_memory_index(env, state, addr, offset);  \
    QCEExpr expr_cell;                                                         \
    qce_state_env_get_i##bits(state, index, &expr_cell);                       \
    QCEExpr expr_cell_updated;                                                 \
    qce_expr_st##n##_i##bits(&state->solver_z3, &expr_val, &expr_cell,         \
                             &expr_cell_updated);                              \
    /* store */                                                                \
    qce_state_env_put_i##bits(state, index, &expr_cell_updated);               \
  }

DEFINE_SYM_INST_ld_part(32, 8, u);
DEFINE_SYM_INST_ld_part(32, 8, s);
DEFINE_SYM_INST_ld_part(32, 16, u);
DEFINE_SYM_INST_ld_part(32, 16, s);
DEFINE_SYM_INST_ld_full(32);

DEFINE_SYM_INST_ld_part(64, 8, u);
DEFINE_SYM_INST_ld_part(64, 8, s);
DEFINE_SYM_INST_ld_part(64, 16, u);
DEFINE_SYM_INST_ld_part(64, 16, s);
DEFINE_SYM_INST_ld_part(64, 32, u);
DEFINE_SYM_INST_ld_part(64, 32, s);
DEFINE_SYM_INST_ld_full(64);

DEFINE_SYM_INST_st_part(32, 8);
DEFINE_SYM_INST_st_part(32, 16);
DEFINE_SYM_INST_st_full(32);

DEFINE_SYM_INST_st_part(64, 8);
DEFINE_SYM_INST_st_part(64, 16);
DEFINE_SYM_INST_st_part(64, 32);
DEFINE_SYM_INST_st_full(64);

#define HANDLE_SYM_INST_ld(key, name, bits)                                    \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state, &inst->i_##name##_i##bits.addr,                 \
        inst->i_##name##_i##bits.offset, &inst->i_##name##_i##bits.res);       \
    break;                                                                     \
  }

#define HANDLE_SYM_INST_st(key, name, bits)                                    \
  case QCE_INST_##key##_I##bits: {                                             \
    qce_sym_inst_##name##_i##bits(                                             \
        arch, &session->state, &inst->i_##name##_i##bits.addr,                 \
        inst->i_##name##_i##bits.offset, &inst->i_##name##_i##bits.val);       \
    break;                                                                     \
  }

#endif /* QCE_SYM_LD_ST_H */
