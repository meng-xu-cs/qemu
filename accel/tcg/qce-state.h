#ifndef QEMU_QCE_STATE_H
#define QEMU_QCE_STATE_H

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "only little endian supported by QCE"
#endif

#define QCE_EXPR_BASE_SIZE sizeof(uint32_t)

typedef enum {
  QCE_EXPR_NONE = 0ul,
  QCE_EXPR_CONCRETE = 1ul,
  QCE_EXPR_SYMBOLIC = 2ul,
  __QCE_EXPR_FORCE_LONG__ = 0xffffffffffffffff,
} QCEExprMode;

// dual-mode holder for expressions
typedef struct {
  GTree *dispatch;
  GTree *symbolic;
  GTree *concrete;
} QCEExprHolder;

gint qce_gtree_addr_key_cmp(gconstpointer a, gconstpointer b);
gint qce_gtree_addr_key_cmp(gconstpointer a, gconstpointer b) {
  return (uintptr_t)a - (uintptr_t)b;
}

static inline void qce_expr_holder_init(QCEExprHolder *holder) {
  holder->dispatch = g_tree_new(qce_gtree_addr_key_cmp);
  holder->symbolic = g_tree_new(qce_gtree_addr_key_cmp);
  holder->concrete = g_tree_new(qce_gtree_addr_key_cmp);
}

static inline void qce_expr_holder_fini(QCEExprHolder *holder) {
  g_tree_destroy(holder->dispatch);
  g_tree_destroy(holder->symbolic);
  g_tree_destroy(holder->concrete);
}

static inline void qce_expr_holder_put_symbolic(QCEExprHolder *holder,
                                                gpointer key, Z3_ast expr) {
  g_tree_insert(holder->dispatch, key, (gpointer)QCE_EXPR_SYMBOLIC);
  g_tree_insert(holder->symbolic, key, expr);
}

static inline QCEExprMode qce_expr_holder_get(QCEExprHolder *holder,
                                              gpointer key) {
  return (QCEExprMode)g_tree_lookup(holder->dispatch, key);
}

// dual-mode representation of the machine state
typedef struct {
  // z3 context
  SolverZ3 solver_z3;

  // expressions
  QCEExprHolder env; // on host
  QCEExprHolder mem; // on guest
  QCEExprHolder tmp; // in IR
} QCEState;

static inline void qce_state_init(QCEState *state) {
  qce_smt_z3_init(&state->solver_z3);
  qce_expr_holder_init(&state->env);
  qce_expr_holder_init(&state->mem);
  qce_expr_holder_init(&state->tmp);
}

static inline void qce_state_fini(QCEState *state) {
  qce_expr_holder_fini(&state->env);
  qce_expr_holder_fini(&state->mem);
  qce_expr_holder_fini(&state->tmp);
  qce_smt_z3_fini(&state->solver_z3);
}

static inline void
qce_state_env_put_symbolic_i32(QCEState *state, uintptr_t offset, Z3_ast expr) {
  if (offset % QCE_EXPR_BASE_SIZE != 0) {
    qce_fatal("misaligned symbolic mark");
  }
  qce_expr_holder_put_symbolic(&state->env, (gpointer)offset, expr);
}

static inline void
qce_state_env_put_symbolic_i64(QCEState *state, uintptr_t offset, Z3_ast expr) {
  if (offset % QCE_EXPR_BASE_SIZE != 0) {
    qce_fatal("misaligned symbolic mark");
  }

  gpointer key_t = (gpointer)offset;
  Z3_ast expr_t = qce_smt_z3_bv64_extract_t(&state->solver_z3, expr);
  qce_expr_holder_put_symbolic(&state->env, key_t, expr_t);

  gpointer key_b = (gpointer)(offset + QCE_EXPR_BASE_SIZE);
  Z3_ast expr_b = qce_smt_z3_bv64_extract_b(&state->solver_z3, expr);
  qce_expr_holder_put_symbolic(&state->env, key_b, expr_b);
}

#endif // QEMU_QCE_STATE_H
