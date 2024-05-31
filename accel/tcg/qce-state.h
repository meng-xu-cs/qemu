#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "only little endian supported by QCE"
#endif

#define QCE_CELL_SIZE sizeof(int32_t)

// if symbolic == NULL (which means mask == 0x00), the expr is null
// if mask == 0x00, the expr is symbolic
// if mask == 0xff, the expr is concrete
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

typedef enum {
  QCE_CELL_NULL,
  QCE_CELL_CONCRETE,
  QCE_CELL_SYMBOLIC,
} QCECellMode;

static inline QCECellMode qce_cell_mode(const QCECell cell) {
  if (cell.symbolic == NULL) {
    return QCE_CELL_NULL;
  }
  const uint8_t v = cell.mask & 0xff;
  if (v == 0xff) {
    return QCE_CELL_CONCRETE;
  }
  assert(v == 0);
  return QCE_CELL_SYMBOLIC;
}

// dual-mode holder for expressions
typedef struct {
  GTree *tree;
} QCECellHolder;

gint qce_gtree_addr_key_cmp(gconstpointer a, gconstpointer b);
gint qce_gtree_addr_key_cmp(gconstpointer a, gconstpointer b) {
  return (intptr_t)a - (intptr_t)b;
}

static inline void qce_cell_holder_init(QCECellHolder *holder) {
  holder->tree = g_tree_new(qce_gtree_addr_key_cmp);
}

static inline void qce_cell_holder_fini(QCECellHolder *holder) {
  g_tree_destroy(holder->tree);
}

static inline void qce_cell_holder_put_concrete(QCECellHolder *holder,
                                                gpointer key, int32_t val) {
  QCECell cell = {.mask = 0xff, .concrete = val};
  g_tree_insert(holder->tree, key, *(gpointer *)&cell);
}

static inline void qce_cell_holder_put_symbolic(QCECellHolder *holder,
                                                gpointer key, Z3_ast expr) {
  QCECell cell = {.symbolic = expr};
  g_tree_insert(holder->tree, key, *(gpointer *)&cell);
}

static inline QCECell qce_cell_holder_get(QCECellHolder *holder, gpointer key) {
  gpointer res = g_tree_lookup(holder->tree, key);
  return *(QCECell *)&res;
}

// dual-mode representation of an expression
typedef struct {
  enum {
    QCE_EXPR_CONCRETE,
    QCE_EXPR_SYMBOLIC,
  } mode;
  enum {
    QCE_EXPR_I32,
    QCE_EXPR_I64,
  } type;
  union {
    int32_t v_i32;
    int64_t v_i64;
    Z3_ast symbolic;
  };
} QCEExpr;

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
qce_state_env_put_concrete_i32(QCEState *state, intptr_t offset, int32_t val) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }
  qce_cell_holder_put_concrete(&state->env, (gpointer)offset, val);
}

static inline void
qce_state_env_put_concrete_i64(QCEState *state, intptr_t offset, int64_t val) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }
  int32_t *cell = (int32_t *)&val;

  gpointer key_t = (gpointer)offset;
  qce_cell_holder_put_concrete(&state->env, key_t, cell[0]);

  gpointer key_b = (gpointer)(offset + QCE_CELL_SIZE);
  qce_cell_holder_put_concrete(&state->env, key_b, cell[1]);
}

static inline void
qce_state_env_put_symbolic_i32(QCEState *state, intptr_t offset, Z3_ast expr) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }
  qce_cell_holder_put_symbolic(&state->env, (gpointer)offset, expr);
}

static inline void
qce_state_env_put_symbolic_i64(QCEState *state, intptr_t offset, Z3_ast expr) {
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

static inline void qce_state_env_get_i32(CPUArchState *env, QCEState *state,
                                         intptr_t offset, QCEExpr *expr) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }

  const QCECell cell = qce_cell_holder_get(&state->env, (gpointer)offset);
  switch (qce_cell_mode(cell)) {
  case QCE_CELL_NULL: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i32 = *(int32_t *)((intptr_t)env + offset);
    break;
  }
  case QCE_CELL_CONCRETE: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i32 = cell.concrete;
    break;
  }
  case QCE_CELL_SYMBOLIC: {
    expr->mode = QCE_EXPR_SYMBOLIC;
    expr->symbolic = cell.symbolic;
    break;
  }
  }
  expr->type = QCE_EXPR_I32;
}

static inline void qce_state_env_get_i64(CPUArchState *env, QCEState *state,
                                         intptr_t offset, QCEExpr *expr) {
  if (offset % QCE_CELL_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }

  gpointer key_t = (gpointer)offset;
  const QCECell cell_t = qce_cell_holder_get(&state->env, key_t);
  const QCECellMode mode_t = qce_cell_mode(cell_t);

  gpointer key_b = (gpointer)(offset + QCE_CELL_SIZE);
  const QCECell cell_b = qce_cell_holder_get(&state->env, key_b);
  const QCECellMode mode_b = qce_cell_mode(cell_b);

  switch (mode_t) {
  case QCE_CELL_NULL: {
    switch (mode_b) {
    case QCE_CELL_NULL: {
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)((intptr_t)env + offset);
      break;
    }
    case QCE_CELL_CONCRETE: {
      int32_t val_t = *(int32_t *)((intptr_t)env + offset);
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = (int64_t)val_t << 32 | cell_b.concrete;
      break;
    }
    case QCE_CELL_SYMBOLIC: {
      int32_t val_t = *(int32_t *)((intptr_t)env + offset);
      Z3_ast expr_t = qce_smt_z3_bv32_value(&state->solver_z3, val_t);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, expr_t, cell_b.symbolic);
      break;
    }
    }
    break;
  }
  case QCE_CELL_CONCRETE: {
    switch (mode_b) {
    case QCE_CELL_NULL: {
      int32_t val_b = *(int32_t *)((intptr_t)env + offset + QCE_CELL_SIZE);
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = (int64_t)cell_t.concrete << 32 | val_b;
      break;
    }
    case QCE_CELL_CONCRETE: {
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = (int64_t)cell_t.concrete << 32 | cell_b.concrete;
      break;
    }
    case QCE_CELL_SYMBOLIC: {
      Z3_ast expr_t = qce_smt_z3_bv32_value(&state->solver_z3, cell_t.concrete);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, expr_t, cell_b.symbolic);
      break;
    }
    }
    break;
  }
  case QCE_CELL_SYMBOLIC: {
    switch (mode_b) {
    case QCE_CELL_NULL: {
      int32_t val_b = *(int32_t *)((intptr_t)env + offset + QCE_CELL_SIZE);
      Z3_ast expr_b = qce_smt_z3_bv32_value(&state->solver_z3, val_b);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, cell_t.symbolic, expr_b);
      break;
    }
    case QCE_CELL_CONCRETE: {
      Z3_ast expr_b = qce_smt_z3_bv32_value(&state->solver_z3, cell_b.concrete);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, cell_t.symbolic, expr_b);
      break;
    }
    case QCE_CELL_SYMBOLIC: {
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic = qce_smt_z3_bv64_concat(&state->solver_z3,
                                              cell_t.symbolic, cell_b.symbolic);
      break;
    }
    }
    break;
  }
  }
  expr->type = QCE_EXPR_I64;
}

static inline void qce_state_tmp_get_i32(QCEState *state, ptrdiff_t index,
                                         QCEExpr *expr) {
  const QCECell cell = qce_cell_holder_get(&state->tmp, (gpointer)index);
  switch (qce_cell_mode(cell)) {
  case QCE_CELL_NULL: {
    qce_fatal("undefined tmp variable: %ld", index);
  }
  case QCE_CELL_CONCRETE: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i32 = cell.concrete;
    break;
  }
  case QCE_CELL_SYMBOLIC: {
    expr->mode = QCE_EXPR_SYMBOLIC;
    expr->symbolic = cell.symbolic;
    break;
  }
  }
  expr->type = QCE_EXPR_I32;
}

static inline void qce_state_get_var(CPUArchState *env, QCEState *state,
                                     QCEVar *var, QCEExpr *expr) {
  switch (var->kind) {
  case QCE_VAR_CONST: {
    expr->mode = QCE_EXPR_CONCRETE;
    switch (var->type) {
    case TCG_TYPE_I32: {
      expr->type = QCE_EXPR_I32;
      expr->v_i32 = var->v_const.val;
      break;
    }
    case TCG_TYPE_I64: {
      expr->type = QCE_EXPR_I64;
      expr->v_i64 = var->v_const.val;
      break;
    }
    default:
      qce_fatal("invalid QCE variable type for const");
    }
    break;
  }
  case QCE_VAR_FIXED: {
#ifdef QCE_DEBUG_IR
    if (var->type != TCG_TYPE_I64) {
      qce_fatal("invalid QCE variable type for fixed");
    }
#endif
    expr->mode = QCE_EXPR_CONCRETE;
    expr->type = QCE_EXPR_I64;
    expr->v_i64 = (int64_t)env;
    break;
  }
  case QCE_VAR_GLOBAL_DIRECT: {
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_env_get_i32(env, state, var->v_global_direct.offset, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_env_get_i64(env, state, var->v_global_direct.offset, expr);
      break;
    }
    default:
      qce_fatal("invalid QCE variable type for direct_global");
    }
    break;
  }
  case QCE_VAR_GLOBAL_INDIRECT: {
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_env_get_i32(env, state,
                            var->v_global_indirect.offset1 +
                                var->v_global_indirect.offset2,
                            expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_env_get_i64(env, state,
                            var->v_global_indirect.offset1 +
                                var->v_global_indirect.offset2,
                            expr);
      break;
    }
    default:
      qce_fatal("invalid QCE variable type for direct_global");
    }
    break;
  }
  case QCE_VAR_TB:
    break;
  case QCE_VAR_EBB:
    break;
  }
}
