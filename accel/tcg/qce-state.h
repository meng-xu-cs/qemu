#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "only little endian supported by QCE"
#endif

#define QCE_CELL_SIZE sizeof(int32_t)

// if mask == 0x00, the expr is a symbolic expr
// if mask == 0xff, the expr is a concrete value
// invalid in all other cases
typedef union {
  struct __attribute__((__packed__)) {
    uint8_t mask;
    uint8_t __pad[3];
    int32_t concrete;
  };
  Z3_ast symbolic;
} QCECell;
static_assert(sizeof(QCECell) == sizeof(gpointer));

// dual-mode holder for expressions
typedef struct {
  GTree *tree;
} QCECellHolder;

gint qce_gtree_addr_key_cmp(gconstpointer a, gconstpointer b);
gint qce_gtree_addr_key_cmp(gconstpointer a, gconstpointer b) {
  return (uintptr_t)a - (uintptr_t)b;
}

static inline void qce_cell_holder_init(QCECellHolder *holder) {
  holder->tree = g_tree_new(qce_gtree_addr_key_cmp);
}

static inline void qce_cell_holder_fini(QCECellHolder *holder) {
  g_tree_destroy(holder->tree);
}

static inline void qce_cell_holder_put_concrete(QCECellHolder *holder,
                                                gpointer key, int32_t val) {
  QCECell cell{.mask = 0xff, .concrete = val};
  g_tree_insert(holder->tree, key, (gpointer)cell);
}

static inline void qce_cell_holder_put_symbolic(QCECellHolder *holder,
                                                gpointer key, Z3_ast expr) {
  QCECell cell{.symbolic = expr};
  g_tree_insert(holder->tree, key, (gpointer)cell);
}

static inline QCECell qce_cell_holder_get(QCECellHolder *holder, gpointer key) {
  return (QCECell)g_tree_lookup(holder->tree, key);
}

// dual-mode representation of the machine state
typedef struct {
  // z3 context
  SolverZ3 solver_z3;

  // expressions
  QCECellHolder env; // on host
  QCECellHolder mem; // on guest
  QCECellHolder tmp; // in IR
} QCEState;

static inline void qce_state_init(QCEState *state) {
  qce_smt_z3_init(&state->solver_z3);
  qce_cell_holder_init(&state->env);
  qce_cell_holder_init(&state->mem);
  qce_cell_holder_init(&state->tmp);
}

static inline void qce_state_fini(QCEState *state) {
  qce_cell_holder_fini(&state->env);
  qce_cell_holder_fini(&state->mem);
  qce_cell_holder_fini(&state->tmp);
  qce_smt_z3_fini(&state->solver_z3);
}

static inline void
qce_state_env_put_concrete_i32(QCEState *state, uintptr_t offset, int32_t val) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }
  qce_cell_holder_put_concrete(&state->env, (gpointer)offset, val);
}

static inline void
qce_state_env_put_concrete_i64(QCEState *state, uintptr_t offset, int64_t val) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }

  int32_t *cell = (int32_t *)&val;
  gpointer key_t = (gpointer)offset;
  qce_cell_holder_put_concrete(&state->env, key_t, *cell);
}

static inline void
qce_state_env_put_symbolic_i32(QCEState *state, uintptr_t offset, Z3_ast expr) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }
  qce_cell_holder_put_symbolic(&state->env, (gpointer)offset, expr);
}

static inline void
qce_state_env_put_symbolic_i64(QCEState *state, uintptr_t offset, Z3_ast expr) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }

  gpointer key_t = (gpointer)offset;
  Z3_ast expr_t = qce_smt_z3_bv64_extract_t(&state->solver_z3, expr);
  qce_cell_holder_put_symbolic(&state->env, key_t, expr_t);

  gpointer key_b = (gpointer)(offset + QCE_CELL_SIZE);
  Z3_ast expr_b = qce_smt_z3_bv64_extract_b(&state->solver_z3, expr);
  qce_cell_holder_put_symbolic(&state->env, key_b, expr_b);
}

typedef struct {
  enum {
    QCE_EXPR_CONCRETE,
    QCE_EXPR_SYMBOLIC,
  } mode;
  union {
    int64_t concrete;
    Z3_ast symbolic;
  };
} QCEExpr;

static inline void qce_state_get_var(CPUArchState *env, QCEState *state,
                                     QCEVar *var, QCEExpr *expr) {
  // TODO
  switch (var->kind) {
  case QCE_VAR_CONST: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->concrete = var->v_const.val;
    break;
  }
  case QCE_VAR_FIXED: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->concrete = (int64_t)env;
    break;
  }
  case QCE_VAR_GLOBAL_DIRECT: {
    break;
  }
  case QCE_VAR_GLOBAL_INDIRECT:
    break;
  case QCE_VAR_TB:
    break;
  case QCE_VAR_EBB:
    break;
  }
}
