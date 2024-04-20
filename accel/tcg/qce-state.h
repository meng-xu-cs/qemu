#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "only little endian supported by QCE"
#endif

#define QCE_CONCOLIC_REGISTER_SIZE sizeof(int32_t)
static_assert(sizeof(intptr_t) == 2 * QCE_CONCOLIC_REGISTER_SIZE);

typedef enum {
  QCE_CELL_MODE_NULL = 0, // must be 0 here
  QCE_CELL_MODE_CONCRETE = 1,
  QCE_CELL_MODE_SYMBOLIC = 2,
} QCECellMode;

typedef enum {
  QCE_CELL_TYPE_VOID = 0, // should not be used
  QCE_CELL_TYPE_I32 = 1,
  QCE_CELL_TYPE_I64 = 2,
} QCECellType;

typedef struct __attribute__((__packed__)) {
  QCECellMode mode;
  QCECellType type;
} QCECellMeta;
static_assert(sizeof(QCECellMeta) == sizeof(gpointer));

// dual-mode representation of a cell
typedef struct {
  QCECellMode mode;
  QCECellType type;
  union {
    int32_t v_i32;
    int64_t v_i64;
    Z3_ast symbolic;
  };
} QCECellValue;

// dual-mode holder for expression units
// (i.e., a unit maybe only part of an expression)
typedef struct {
  GTree *meta;
  GTree *concrete;
  GTree *symbolic;
} QCECellHolder;

gint qce_gtree_addr_key_cmp(gconstpointer a, gconstpointer b);
gint qce_gtree_addr_key_cmp(gconstpointer a, gconstpointer b) {
  return (intptr_t)a - (intptr_t)b;
}

static inline void qce_cell_holder_init(QCECellHolder *holder) {
  holder->meta = g_tree_new(qce_gtree_addr_key_cmp);
  holder->concrete = g_tree_new(qce_gtree_addr_key_cmp);
  holder->symbolic = g_tree_new(qce_gtree_addr_key_cmp);
}

static inline void qce_cell_holder_fini(QCECellHolder *holder) {
  g_tree_destroy(holder->meta);
  g_tree_destroy(holder->concrete);
  g_tree_destroy(holder->symbolic);
}

static inline void qce_cell_holder_put_concrete_i32(QCECellHolder *holder,
                                                    gpointer key, int32_t val) {
  QCECellMeta cell = {.mode = QCE_CELL_MODE_CONCRETE,
                      .type = QCE_CELL_TYPE_I32};
  g_tree_insert(holder->meta, key, *(gpointer *)&cell);
  g_tree_insert(holder->concrete, key, (gpointer)((intptr_t)val));
}

static inline void qce_cell_holder_put_concrete_i64(QCECellHolder *holder,
                                                    gpointer key, int64_t val) {
  QCECellMeta cell = {.mode = QCE_CELL_MODE_CONCRETE,
                      .type = QCE_CELL_TYPE_I64};
  g_tree_insert(holder->meta, key, *(gpointer *)&cell);
  g_tree_insert(holder->concrete, key, (gpointer)((intptr_t)val));
}

static inline void qce_cell_holder_put_symbolic_i32(QCECellHolder *holder,
                                                    gpointer key, Z3_ast ast) {
  QCECellMeta cell = {.mode = QCE_CELL_MODE_SYMBOLIC,
                      .type = QCE_CELL_TYPE_I32};
  g_tree_insert(holder->meta, key, *(gpointer *)&cell);
  g_tree_insert(holder->symbolic, key, ast);
}

static inline void qce_cell_holder_put_symbolic_i64(QCECellHolder *holder,
                                                    gpointer key, Z3_ast ast) {
  QCECellMeta cell = {.mode = QCE_CELL_MODE_SYMBOLIC,
                      .type = QCE_CELL_TYPE_I64};
  g_tree_insert(holder->meta, key, *(gpointer *)&cell);
  g_tree_insert(holder->symbolic, key, ast);
}

static inline void qce_cell_holder_get_i32(QCECellHolder *holder, gpointer key,
                                           QCECellValue *val) {
  gpointer gptr = g_tree_lookup(holder->meta, key);
  QCECellMeta cell = *(QCECellMeta *)&gptr;

  switch (cell.mode) {
  case QCE_CELL_MODE_NULL: {
    val->mode = QCE_CELL_MODE_NULL;
    val->type = QCE_CELL_TYPE_I32;
    break;
  }
  case QCE_CELL_MODE_CONCRETE: {
    val->type = cell.type;
    val->v_i32 = (int32_t)((intptr_t)g_tree_lookup(holder->concrete, key));
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    val->type = cell.type;
    val->symbolic = g_tree_lookup(holder->symbolic, key);
    break;
  }
  }

#ifdef QCE_DEBUG_IR
  if (val->type != QCE_CELL_TYPE_I32) {
    qce_fatal("cell type mismatch: expect I32, found %d", cell.type);
  }
#endif
}

static inline void qce_cell_holder_get_i64(QCECellHolder *holder, gpointer key,
                                           QCECellValue *val) {
  gpointer gptr = g_tree_lookup(holder->meta, key);
  QCECellMeta cell = *(QCECellMeta *)&gptr;

  switch (cell.mode) {
  case QCE_CELL_MODE_NULL: {
    val->mode = QCE_CELL_MODE_NULL;
    val->type = QCE_CELL_TYPE_I64;
    break;
  }
  case QCE_CELL_MODE_CONCRETE: {
    val->type = cell.type;
    val->v_i64 = (int64_t)((intptr_t)g_tree_lookup(holder->concrete, key));
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    val->type = cell.type;
    val->symbolic = g_tree_lookup(holder->symbolic, key);
    break;
  }
  }

#ifdef QCE_DEBUG_IR
  if (val->type != QCE_CELL_TYPE_I64) {
    qce_fatal("cell type mismatch: expect I64, found %d", cell.type);
  }
#endif
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
  if (offset % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }
  qce_cell_holder_put_concrete_i32(&state->env, (gpointer)offset, val);
}

static inline void
qce_state_env_put_concrete_i64(QCEState *state, intptr_t offset, int64_t val) {
  if (offset % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }
  int32_t *cell = (int32_t *)&val;

  gpointer key_t = (gpointer)offset;
  qce_cell_holder_put_concrete_i32(&state->env, key_t, cell[0]);

  gpointer key_b = (gpointer)(offset + QCE_CONCOLIC_REGISTER_SIZE);
  qce_cell_holder_put_concrete_i32(&state->env, key_b, cell[1]);
}

static inline void qce_state_env_put_symbolic_i32(QCEState *state,
                                                  intptr_t offset, Z3_ast ast) {
  if (offset % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }
  qce_cell_holder_put_symbolic_i32(&state->env, (gpointer)offset, ast);
}

static inline void qce_state_env_put_symbolic_i64(QCEState *state,
                                                  intptr_t offset, Z3_ast ast) {
  if (offset % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }

  gpointer key_t = (gpointer)offset;
  Z3_ast ast_t = qce_smt_z3_bv64_extract_t(&state->solver_z3, ast);
  qce_cell_holder_put_symbolic_i32(&state->env, key_t, ast_t);

  gpointer key_b = (gpointer)(offset + QCE_CONCOLIC_REGISTER_SIZE);
  Z3_ast ast_b = qce_smt_z3_bv64_extract_b(&state->solver_z3, ast);
  qce_cell_holder_put_symbolic_i32(&state->env, key_b, ast_b);
}

static inline void qce_state_env_get_i32(CPUArchState *env, QCEState *state,
                                         intptr_t offset, QCEExpr *expr) {
  if (offset % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }

  QCECellValue val;
  qce_cell_holder_get_i32(&state->env, (gpointer)offset, &val);

  switch (val.mode) {
  case QCE_CELL_MODE_NULL: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i32 = *(int32_t *)((intptr_t)env + offset);
    break;
  }
  case QCE_CELL_MODE_CONCRETE: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i32 = val.v_i32;
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    expr->mode = QCE_EXPR_SYMBOLIC;
    expr->symbolic = val.symbolic;
    break;
  }
  }
  expr->type = QCE_EXPR_I32;
}

static inline void qce_state_env_get_i64(CPUArchState *env, QCEState *state,
                                         intptr_t offset, QCEExpr *expr) {
  if (offset % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned offset for env location");
  }

  gpointer key_t = (gpointer)offset;
  QCECellValue val_t;
  qce_cell_holder_get_i32(&state->env, key_t, &val_t);

  gpointer key_b = (gpointer)(offset + QCE_CONCOLIC_REGISTER_SIZE);
  QCECellValue val_b;
  qce_cell_holder_get_i32(&state->env, key_b, &val_b);

  switch (val_t.mode) {
  case QCE_CELL_MODE_NULL: {
    switch (val_b.mode) {
    case QCE_CELL_MODE_NULL: {
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)((intptr_t)env + offset);
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      int32_t tmp_t = *(int32_t *)((intptr_t)env + offset);
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = (int64_t)tmp_t << 32 | val_b.v_i32;
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      int32_t tmp_t = *(int32_t *)((intptr_t)env + offset);
      Z3_ast expr_t = qce_smt_z3_bv32_value(&state->solver_z3, tmp_t);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, expr_t, val_b.symbolic);
      break;
    }
    }
    break;
  }
  case QCE_CELL_MODE_CONCRETE: {
    switch (val_b.mode) {
    case QCE_CELL_MODE_NULL: {
      int32_t tmp_b =
          *(int32_t *)((intptr_t)env + offset + QCE_CONCOLIC_REGISTER_SIZE);
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = (int64_t)val_t.v_i32 << 32 | tmp_b;
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = (int64_t)val_t.v_i32 << 32 | val_b.v_i32;
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      Z3_ast expr_t = qce_smt_z3_bv32_value(&state->solver_z3, val_t.v_i32);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, expr_t, val_b.symbolic);
      break;
    }
    }
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    switch (val_b.mode) {
    case QCE_CELL_MODE_NULL: {
      int32_t tmp_b =
          *(int32_t *)((intptr_t)env + offset + QCE_CONCOLIC_REGISTER_SIZE);
      Z3_ast expr_b = qce_smt_z3_bv32_value(&state->solver_z3, tmp_b);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, val_t.symbolic, expr_b);
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      Z3_ast expr_b = qce_smt_z3_bv32_value(&state->solver_z3, val_b.v_i32);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, val_t.symbolic, expr_b);
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic = qce_smt_z3_bv64_concat(&state->solver_z3, val_t.symbolic,
                                              val_b.symbolic);
      break;
    }
    }
    break;
  }
  }
  expr->type = QCE_EXPR_I64;
}

static inline void
qce_state_tmp_put_concrete_i32(QCEState *state, ptrdiff_t index, int32_t val) {
  qce_cell_holder_put_concrete_i32(&state->tmp, (gpointer)index, val);
}

static inline void
qce_state_tmp_put_concrete_i64(QCEState *state, ptrdiff_t index, int64_t val) {
  qce_cell_holder_put_concrete_i64(&state->tmp, (gpointer)index, val);
}

static inline void qce_state_tmp_put_symbolic_i32(QCEState *state,
                                                  ptrdiff_t index, Z3_ast ast) {
  qce_cell_holder_put_symbolic_i32(&state->tmp, (gpointer)index, ast);
}

static inline void qce_state_tmp_put_symbolic_i64(QCEState *state,
                                                  ptrdiff_t index, Z3_ast ast) {
  qce_cell_holder_put_symbolic_i64(&state->tmp, (gpointer)index, ast);
}

static inline void qce_state_tmp_get_i32(QCEState *state, ptrdiff_t index,
                                         QCEExpr *expr) {
  QCECellValue val;
  qce_cell_holder_get_i32(&state->tmp, (gpointer)index, &val);

  switch (val.mode) {
  case QCE_CELL_MODE_NULL: {
    qce_fatal("undefined tmp variable: %ld", index);
  }
  case QCE_CELL_MODE_CONCRETE: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i32 = val.v_i32;
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    expr->mode = QCE_EXPR_SYMBOLIC;
    expr->symbolic = val.symbolic;
    break;
  }
  }
  expr->type = QCE_EXPR_I32;
}

static inline void qce_state_tmp_get_i64(QCEState *state, ptrdiff_t index,
                                         QCEExpr *expr) {
  QCECellValue val;
  qce_cell_holder_get_i64(&state->tmp, (gpointer)index, &val);

  switch (val.mode) {
  case QCE_CELL_MODE_NULL: {
    qce_fatal("undefined tmp variable: %ld", index);
  }
  case QCE_CELL_MODE_CONCRETE: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i64 = val.v_i64;
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    expr->mode = QCE_EXPR_SYMBOLIC;
    expr->symbolic = val.symbolic;
    break;
  }
  }
  expr->type = QCE_EXPR_I64;
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
  case QCE_VAR_TB: {
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_tmp_get_i32(state, var->v_tb.index, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_tmp_get_i64(state, var->v_tb.index, expr);
      break;
    }
    default:
      qce_fatal("invalid QCE variable type for temp_tb");
    }
    break;
  }
  case QCE_VAR_EBB: {
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_tmp_get_i32(state, var->v_ebb.index, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_tmp_get_i64(state, var->v_ebb.index, expr);
      break;
    }
    default:
      qce_fatal("invalid QCE variable type for temp_ebb");
    }
    break;
  }
  }
}
