#ifndef QCE_RELEASE
#define QCE_ENV_ADDR_OFFSET_LOWER_BOUND -0x4000
#define QCE_ENV_ADDR_OFFSET_UPPER_BOUND 0x4000
#endif

static inline void qce_sym_inst_ld_i32(CPUArchState *env, QCEState *state,
                                       QCEVar *addr, tcg_target_ulong offset,
                                       QCEVar *res) {
  // derive the load index
  QCEExpr expr_addr;
  qce_state_get_var(env, state, addr, &expr_addr);

  QCEExpr expr_offset;
  qce_expr_init_v64(&expr_offset, offset);

  QCEExpr load_index;
  qce_expr_add_i64(&state->solver_z3, &expr_addr, &expr_offset, &load_index);

  // load index must be a constant (and within range)
  if (load_index.mode != QCE_EXPR_CONCRETE) {
    qce_fatal("trying to load from a symbolic address on host memory");
  }
#ifndef QCE_RELEASE
  ptrdiff_t diff = load_index.v_i64 - (intptr_t)env;
  if (diff <= QCE_ENV_ADDR_OFFSET_LOWER_BOUND ||
      diff >= QCE_ENV_ADDR_OFFSET_UPPER_BOUND) {
    qce_fatal("host memory offset out of bound: 0x%lx", diff);
  }
#endif

  // actual loading
  QCEExpr load_res;
  qce_state_env_get_i32(state, load_index.v_i64, &load_res);

  // putting the result to the variable
  qce_state_put_var(env, state, res, &load_res);
}

static inline void qce_sym_inst_add_i32(CPUArchState *env, QCEState *state,
                                        QCEVar *v1, QCEVar *v2, QCEVar *res) {
  QCEExpr expr_v1, expr_v2;
  qce_state_get_var(env, state, v1, &expr_v1);
  qce_state_get_var(env, state, v2, &expr_v2);

  QCEExpr expr_res;
  qce_expr_add_i32(&state->solver_z3, &expr_v1, &expr_v2, &expr_res);
  qce_state_put_var(env, state, res, &expr_res);
}