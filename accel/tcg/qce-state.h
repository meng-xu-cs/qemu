#ifndef QEMU_QCE_STATE_H
#define QEMU_QCE_STATE_H

// dual-mode representation of an expression
typedef struct {
  enum {
    QCE_EXPR_CONCRETE,
    QCE_EXPR_SYMBOLIC,
  } mode;

  union {
    uintptr_t concrete;
    Z3_ast symbolic;
  } expr;
} QCEExpr;

// QCE machine state
typedef struct {
  // z3 context
  SolverZ3 solver_z3;

  // env (on host)
  QTree *env;
  // mem (on guest)
  QTree *mem;
} QCEState;

gint qce_qtree_addr_key_cmp(gconstpointer a, gconstpointer b);
gint qce_qtree_addr_key_cmp(gconstpointer a, gconstpointer b) {
  return (uintptr_t)a - (uintptr_t)b;
}

static inline void qce_init_state(QCEState *state) {
  qce_init_z3(&state->solver_z3);
  state->env = q_tree_new(qce_qtree_addr_key_cmp);
  state->mem = q_tree_new(qce_qtree_addr_key_cmp);
}

static inline void qce_fini_state(QCEState *state) {
  q_tree_destroy(state->env);
  q_tree_destroy(state->mem);
  qce_fini_z3(&state->solver_z3);
}

#endif // QEMU_QCE_STATE_H
