#ifndef QEMU_QCE_STATE_H
#define QEMU_QCE_STATE_H

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "only little endian supported by QCE"
#endif

#define QCE_EXPR_BASE_SIZE sizeof(uint32_t)

// dual-mode representation of the machine state
typedef struct {
  // z3 context
  SolverZ3 solver_z3;

  // env (on host)
  QTree *env_symbolic;
  QTree *env_concrete;
  // mem (on guest)
  QTree *mem_symbolic;
  QTree *mem_concrete;
} QCEState;

gint qce_qtree_addr_key_cmp(gconstpointer a, gconstpointer b);
gint qce_qtree_addr_key_cmp(gconstpointer a, gconstpointer b) {
  return (uintptr_t)a - (uintptr_t)b;
}

static inline void qce_state_init(QCEState *state) {
  qce_smt_z3_init(&state->solver_z3);
  state->env_symbolic = q_tree_new(qce_qtree_addr_key_cmp);
  state->env_concrete = q_tree_new(qce_qtree_addr_key_cmp);
  state->mem_symbolic = q_tree_new(qce_qtree_addr_key_cmp);
  state->mem_concrete = q_tree_new(qce_qtree_addr_key_cmp);
}

static inline void qce_state_fini(QCEState *state) {
  q_tree_destroy(state->env_symbolic);
  q_tree_destroy(state->env_concrete);
  q_tree_destroy(state->mem_symbolic);
  q_tree_destroy(state->mem_concrete);
  qce_smt_z3_fini(&state->solver_z3);
}

static inline void
qce_state_env_put_symbolic_i32(QCEState *state, uintptr_t offset, Z3_ast expr) {
  if (offset % QCE_EXPR_BASE_SIZE != 0) {
    qce_fatal("misaligned symbolic mark");
  }
  q_tree_remove(state->env_concrete, (gpointer)offset);
  q_tree_insert(state->env_symbolic, (gpointer)offset, expr);
}

static inline void
qce_state_env_put_symbolic_i64(QCEState *state, uintptr_t offset, Z3_ast expr) {
  if (offset % QCE_EXPR_BASE_SIZE != 0) {
    qce_fatal("misaligned symbolic mark");
  }

  uintptr_t offset_t = offset;
  Z3_ast expr_t = qce_smt_z3_bv64_extract_t(&state->solver_z3, expr);
  q_tree_remove(state->env_concrete, (gpointer)offset_t);
  q_tree_insert(state->env_symbolic, (gpointer)offset_t, expr_t);

  uintptr_t offset_b = offset + QCE_EXPR_BASE_SIZE;
  Z3_ast expr_b = qce_smt_z3_bv64_extract_b(&state->solver_z3, expr);
  q_tree_remove(state->env_concrete, (gpointer)offset_b);
  q_tree_insert(state->env_symbolic, (gpointer)offset_b, expr_b);
}

#endif // QEMU_QCE_STATE_H
