static inline void qce_sym_inst_ld_i32(CPUArchState *env, QCEState *state,
                                       QCEVar *addr, tcg_target_ulong offset,
                                       QCEVar *res) {
  QCEExpr expr_addr;
  qce_state_get_var(env, state, addr, &expr_addr);
}
