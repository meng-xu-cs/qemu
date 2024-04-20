#ifndef QCE_STATE_H
#define QCE_STATE_H

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

/*
 * Cell holder
 */

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
    val->mode = cell.mode;
    val->type = cell.type;
    val->v_i32 = (int32_t)((intptr_t)g_tree_lookup(holder->concrete, key));
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    val->mode = cell.mode;
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
    val->mode = cell.mode;
    val->type = cell.type;
    val->v_i64 = (int64_t)((intptr_t)g_tree_lookup(holder->concrete, key));
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    val->mode = cell.mode;
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

/*
 * Tree of cell holders
 */

gint qce_gtree_mmu_idx_cmp(gconstpointer a, gconstpointer b);
gint qce_gtree_mmu_idx_cmp(gconstpointer a, gconstpointer b) {
  return (intptr_t)a - (intptr_t)b;
}

gboolean qce_gtree_cell_holder_destroy_on_iter(gpointer _key, gpointer value,
                                               gpointer _data);
gboolean qce_gtree_cell_holder_destroy_on_iter(gpointer _key, gpointer value,
                                               gpointer _data) {
  qce_cell_holder_fini(value);
  g_free(value);
  return false;
}

/*
 * Machine state
 */

// dual-mode representation of the machine state
typedef struct {
  // z3 context
  SolverZ3 solver_z3;

  // expressions
  QCECellHolder env; // on host
  QCECellHolder tmp; // in IR
  GTree *mem;        // on guest
} QCEState;

static inline void qce_state_init(QCEState *state) {
  qce_smt_z3_init(&state->solver_z3);
  qce_cell_holder_init(&state->env);
  qce_cell_holder_init(&state->tmp);
  state->mem = g_tree_new(qce_gtree_mmu_idx_cmp);
}

static inline void qce_state_fini(QCEState *state) {
  g_tree_foreach(state->mem, qce_gtree_cell_holder_destroy_on_iter, NULL);
  g_tree_destroy(state->mem);
  qce_cell_holder_fini(&state->tmp);
  qce_cell_holder_fini(&state->env);
  qce_smt_z3_fini(&state->solver_z3);
}

static inline QCECellHolder *qce_state_guest_mem_by_mmu(QCEState *state,
                                                        intptr_t mmu_idx) {
  gpointer key = *(gpointer *)&mmu_idx;
  QCECellHolder *holder = g_tree_lookup(state->mem, key);
  if (holder == NULL) {
    holder = g_malloc0(sizeof(QCECellHolder));
    qce_cell_holder_init(holder);
    g_tree_insert(state->mem, key, holder);
  }
  return holder;
}

/*
 * State: Env
 */

static inline void qce_state_env_put_concrete_i32(QCEState *state,
                                                  intptr_t addr, int32_t val) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for env location");
  }
#endif
  qce_cell_holder_put_concrete_i32(&state->env, (gpointer)addr, val);
}

static inline void qce_state_env_put_symbolic_i32(QCEState *state,
                                                  intptr_t addr, Z3_ast ast) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for env location");
  }
#endif
  qce_cell_holder_put_symbolic_i32(&state->env, (gpointer)addr, ast);
}

static inline void qce_state_env_put_i32(QCEState *state, intptr_t addr,
                                         QCEExpr *expr) {
#ifndef QCE_RELEASE
  assert(expr->type == QCE_EXPR_I32);
#endif
  switch (expr->mode) {
  case QCE_EXPR_CONCRETE: {
    qce_state_env_put_concrete_i32(state, addr, expr->v_i32);
    break;
  }
  case QCE_EXPR_SYMBOLIC: {
    qce_state_env_put_symbolic_i32(state, addr, expr->symbolic);
    break;
  }
  }
}

static inline void qce_state_env_put_concrete_i64(QCEState *state,
                                                  intptr_t addr, int64_t val) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for env location");
  }
#endif
  int32_t *cell = (int32_t *)&val;

  gpointer key_l = (gpointer)addr;
  qce_cell_holder_put_concrete_i32(&state->env, key_l, cell[0]);

  gpointer key_h = (gpointer)(addr + QCE_CONCOLIC_REGISTER_SIZE);
  qce_cell_holder_put_concrete_i32(&state->env, key_h, cell[1]);
}

static inline void qce_state_env_put_symbolic_i64(QCEState *state,
                                                  intptr_t addr, Z3_ast ast) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for env location");
  }
#endif

  gpointer key_l = (gpointer)addr;
  Z3_ast ast_l = qce_smt_z3_bv64_extract_l(&state->solver_z3, ast);
  qce_cell_holder_put_symbolic_i32(&state->env, key_l, ast_l);

  gpointer key_h = (gpointer)(addr + QCE_CONCOLIC_REGISTER_SIZE);
  Z3_ast ast_h = qce_smt_z3_bv64_extract_h(&state->solver_z3, ast);
  qce_cell_holder_put_symbolic_i32(&state->env, key_h, ast_h);
}

static inline void qce_state_env_put_i64(QCEState *state, intptr_t addr,
                                         QCEExpr *expr) {
#ifndef QCE_RELEASE
  assert(expr->type == QCE_EXPR_I64);
#endif
  switch (expr->mode) {
  case QCE_EXPR_CONCRETE: {
    qce_state_env_put_concrete_i64(state, addr, expr->v_i64);
    break;
  }
  case QCE_EXPR_SYMBOLIC: {
    qce_state_env_put_symbolic_i64(state, addr, expr->symbolic);
    break;
  }
  }
}

static inline void qce_state_env_get_i32(QCEState *state, intptr_t addr,
                                         QCEExpr *expr) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for env location");
  }
#endif

  QCECellValue val;
  qce_cell_holder_get_i32(&state->env, (gpointer)addr, &val);

  switch (val.mode) {
  case QCE_CELL_MODE_NULL: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i32 = *(int32_t *)addr;
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

static inline void qce_state_env_get_i64(QCEState *state, intptr_t addr,
                                         QCEExpr *expr) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for env location");
  }
#endif

  gpointer key_l = (gpointer)addr;
  QCECellValue val_l;
  qce_cell_holder_get_i32(&state->env, key_l, &val_l);

  gpointer key_h = (gpointer)(addr + QCE_CONCOLIC_REGISTER_SIZE);
  QCECellValue val_h;
  qce_cell_holder_get_i32(&state->env, key_h, &val_h);

  switch (val_l.mode) {
  case QCE_CELL_MODE_NULL: {
    switch (val_h.mode) {
    case QCE_CELL_MODE_NULL: {
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)addr;
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      int32_t tmp_l = *(int32_t *)addr;
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)(int32_t[]){tmp_l, val_h.v_i32};
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      int32_t tmp_l = *(int32_t *)addr;
      Z3_ast expr_l = qce_smt_z3_bv32_value(&state->solver_z3, tmp_l);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, val_h.symbolic, expr_l);
      break;
    }
    }
    break;
  }
  case QCE_CELL_MODE_CONCRETE: {
    switch (val_h.mode) {
    case QCE_CELL_MODE_NULL: {
      int32_t tmp_h = *(int32_t *)(addr + QCE_CONCOLIC_REGISTER_SIZE);
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)(int32_t[]){val_l.v_i32, tmp_h};
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)(int32_t[]){val_l.v_i32, val_h.v_i32};
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      Z3_ast expr_l = qce_smt_z3_bv32_value(&state->solver_z3, val_l.v_i32);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, val_h.symbolic, expr_l);
      break;
    }
    }
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    switch (val_h.mode) {
    case QCE_CELL_MODE_NULL: {
      int32_t tmp_h = *(int32_t *)(addr + QCE_CONCOLIC_REGISTER_SIZE);
      Z3_ast expr_h = qce_smt_z3_bv32_value(&state->solver_z3, tmp_h);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, expr_h, val_l.symbolic);
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      Z3_ast expr_h = qce_smt_z3_bv32_value(&state->solver_z3, val_h.v_i32);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, expr_h, val_l.symbolic);
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic = qce_smt_z3_bv64_concat(&state->solver_z3, val_h.symbolic,
                                              val_l.symbolic);
      break;
    }
    }
    break;
  }
  }
  expr->type = QCE_EXPR_I64;
}

/*
 * State: Mem
 */

static inline void qce_state_mem_put_concrete_i32(QCEState *state,
                                                  intptr_t addr,
                                                  unsigned mmu_idx,
                                                  int32_t val) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for mem location");
  }
#endif
  QCECellHolder *mem = qce_state_guest_mem_by_mmu(state, mmu_idx);
  qce_cell_holder_put_concrete_i32(mem, (gpointer)addr, val);
}

static inline void qce_state_mem_put_symbolic_i32(QCEState *state,
                                                  intptr_t addr,
                                                  unsigned mmu_idx,
                                                  Z3_ast ast) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for mem location");
  }
#endif
  QCECellHolder *mem = qce_state_guest_mem_by_mmu(state, mmu_idx);
  qce_cell_holder_put_symbolic_i32(mem, (gpointer)addr, ast);
}

static inline void qce_state_mem_put_i32(QCEState *state, intptr_t addr,
                                         unsigned mmu_idx, QCEExpr *expr) {
#ifndef QCE_RELEASE
  assert(expr->type == QCE_EXPR_I32);
#endif
  switch (expr->mode) {
  case QCE_EXPR_CONCRETE: {
    qce_state_mem_put_concrete_i32(state, addr, mmu_idx, expr->v_i32);
    break;
  }
  case QCE_EXPR_SYMBOLIC: {
    qce_state_mem_put_symbolic_i32(state, addr, mmu_idx, expr->symbolic);
    break;
  }
  }
}

static inline void qce_state_mem_put_concrete_i64(QCEState *state,
                                                  intptr_t addr,
                                                  unsigned mmu_idx,
                                                  int64_t val) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for mem location");
  }
#endif
  QCECellHolder *mem = qce_state_guest_mem_by_mmu(state, mmu_idx);
  int32_t *cell = (int32_t *)&val;

  gpointer key_l = (gpointer)addr;
  qce_cell_holder_put_concrete_i32(mem, key_l, cell[0]);

  gpointer key_h = (gpointer)(addr + QCE_CONCOLIC_REGISTER_SIZE);
  qce_cell_holder_put_concrete_i32(mem, key_h, cell[1]);
}

static inline void qce_state_mem_put_symbolic_i64(QCEState *state,
                                                  intptr_t addr,
                                                  unsigned mmu_idx,
                                                  Z3_ast ast) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for mem location");
  }
#endif
  QCECellHolder *mem = qce_state_guest_mem_by_mmu(state, mmu_idx);

  gpointer key_l = (gpointer)addr;
  Z3_ast ast_l = qce_smt_z3_bv64_extract_l(&state->solver_z3, ast);
  qce_cell_holder_put_symbolic_i32(mem, key_l, ast_l);

  gpointer key_h = (gpointer)(addr + QCE_CONCOLIC_REGISTER_SIZE);
  Z3_ast ast_h = qce_smt_z3_bv64_extract_h(&state->solver_z3, ast);
  qce_cell_holder_put_symbolic_i32(mem, key_h, ast_h);
}

static inline void qce_state_mem_put_i64(QCEState *state, intptr_t addr,
                                         unsigned mmu_idx, QCEExpr *expr) {
  qce_debug_assert(expr->type == QCE_EXPR_I64);
  switch (expr->mode) {
  case QCE_EXPR_CONCRETE: {
    qce_state_mem_put_concrete_i64(state, addr, mmu_idx, expr->v_i64);
    break;
  }
  case QCE_EXPR_SYMBOLIC: {
    qce_state_mem_put_symbolic_i64(state, addr, mmu_idx, expr->symbolic);
    break;
  }
  }
}

static inline void qce_state_mem_get_i32(CPUArchState *env, QCEState *state,
                                         intptr_t addr, int mmu_idx,
                                         QCEExpr *expr) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for mem location");
  }
#endif

  QCECellHolder *mem = qce_state_guest_mem_by_mmu(state, mmu_idx);
  QCECellValue val;
  qce_cell_holder_get_i32(mem, (gpointer)addr, &val);

  switch (val.mode) {
  case QCE_CELL_MODE_NULL: {
    expr->mode = QCE_EXPR_CONCRETE;
    expr->v_i32 = cpu_ldl_le_mmuidx_ra(env, addr, mmu_idx, 0);
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

static inline void qce_state_mem_get_i64(CPUArchState *env, QCEState *state,
                                         intptr_t addr, int mmu_idx,
                                         QCEExpr *expr) {
#ifndef QCE_RELEASE
  if (addr % QCE_CONCOLIC_REGISTER_SIZE != 0) {
    qce_fatal("misaligned address for mem location");
  }
#endif
  QCECellHolder *mem = qce_state_guest_mem_by_mmu(state, mmu_idx);

  gpointer key_l = (gpointer)addr;
  QCECellValue val_l;
  qce_cell_holder_get_i32(mem, key_l, &val_l);

  gpointer key_h = (gpointer)(addr + QCE_CONCOLIC_REGISTER_SIZE);
  QCECellValue val_h;
  qce_cell_holder_get_i32(mem, key_h, &val_h);

  switch (val_l.mode) {
  case QCE_CELL_MODE_NULL: {
    switch (val_h.mode) {
    case QCE_CELL_MODE_NULL: {
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = cpu_ldq_le_mmuidx_ra(env, addr, mmu_idx, 0);
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      int32_t tmp_l = cpu_ldl_le_mmuidx_ra(env, addr, mmu_idx, 0);
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)(int32_t[]){tmp_l, val_h.v_i32};
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      int32_t tmp_l = cpu_ldl_le_mmuidx_ra(env, addr, mmu_idx, 0);
      Z3_ast expr_l = qce_smt_z3_bv32_value(&state->solver_z3, tmp_l);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, val_h.symbolic, expr_l);
      break;
    }
    }
    break;
  }
  case QCE_CELL_MODE_CONCRETE: {
    switch (val_h.mode) {
    case QCE_CELL_MODE_NULL: {
      int32_t tmp_h = cpu_ldl_le_mmuidx_ra(
          env, addr + QCE_CONCOLIC_REGISTER_SIZE, mmu_idx, 0);
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)(int32_t[]){val_l.v_i32, tmp_h};
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      expr->mode = QCE_EXPR_CONCRETE;
      expr->v_i64 = *(int64_t *)(int32_t[]){val_l.v_i32, val_h.v_i32};
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      Z3_ast expr_l = qce_smt_z3_bv32_value(&state->solver_z3, val_l.v_i32);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, val_h.symbolic, expr_l);
      break;
    }
    }
    break;
  }
  case QCE_CELL_MODE_SYMBOLIC: {
    switch (val_h.mode) {
    case QCE_CELL_MODE_NULL: {
      int32_t tmp_h = cpu_ldl_le_mmuidx_ra(
          env, addr + QCE_CONCOLIC_REGISTER_SIZE, mmu_idx, 0);
      Z3_ast expr_h = qce_smt_z3_bv32_value(&state->solver_z3, tmp_h);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, expr_h, val_l.symbolic);
      break;
    }
    case QCE_CELL_MODE_CONCRETE: {
      Z3_ast expr_h = qce_smt_z3_bv32_value(&state->solver_z3, val_h.v_i32);
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic =
          qce_smt_z3_bv64_concat(&state->solver_z3, expr_h, val_l.symbolic);
      break;
    }
    case QCE_CELL_MODE_SYMBOLIC: {
      expr->mode = QCE_EXPR_SYMBOLIC;
      expr->symbolic = qce_smt_z3_bv64_concat(&state->solver_z3, val_h.symbolic,
                                              val_l.symbolic);
      break;
    }
    }
    break;
  }
  }
  expr->type = QCE_EXPR_I64;
}

static inline void qce_state_mem_get_symbolic_i32(QCEState *state, Z3_ast addr,
                                                  QCEExpr *result) {
  Z3_ast expr = qce_smt_z3_blob_ld32(&state->solver_z3, addr);

  uint32_t val;
  if (qce_smt_z3_probe_bv32(&state->solver_z3, expr, &val)) {
    result->mode = QCE_EXPR_CONCRETE;
    result->v_i32 = val;
  } else {
    result->mode = QCE_EXPR_SYMBOLIC;
    result->symbolic = expr;
  }
  result->type = QCE_EXPR_I32;
}

static inline void qce_state_mem_get_symbolic_i64(QCEState *state, Z3_ast addr,
                                                  QCEExpr *result) {
  Z3_ast expr = qce_smt_z3_blob_ld64(&state->solver_z3, addr);

  uint64_t val;
  if (qce_smt_z3_probe_bv64(&state->solver_z3, expr, &val)) {
    result->mode = QCE_EXPR_CONCRETE;
    result->v_i64 = val;
  } else {
    result->mode = QCE_EXPR_SYMBOLIC;
    result->symbolic = expr;
  }
  result->type = QCE_EXPR_I64;
}

/*
 * State: Tmp
 */

static inline void
qce_state_tmp_put_concrete_i32(QCEState *state, ptrdiff_t index, int32_t val) {
  qce_cell_holder_put_concrete_i32(&state->tmp, (gpointer)index, val);
}

static inline void qce_state_tmp_put_symbolic_i32(QCEState *state,
                                                  ptrdiff_t index, Z3_ast ast) {
  qce_cell_holder_put_symbolic_i32(&state->tmp, (gpointer)index, ast);
}

static inline void qce_state_tmp_put_i32(QCEState *state, ptrdiff_t index,
                                         QCEExpr *expr) {
#ifndef QCE_RELEASE
  assert(expr->type == QCE_EXPR_I32);
#endif
  switch (expr->mode) {
  case QCE_EXPR_CONCRETE: {
    qce_state_tmp_put_concrete_i32(state, index, expr->v_i32);
    break;
  }
  case QCE_EXPR_SYMBOLIC: {
    qce_state_tmp_put_symbolic_i32(state, index, expr->symbolic);
    break;
  }
  }
}

static inline void
qce_state_tmp_put_concrete_i64(QCEState *state, ptrdiff_t index, int64_t val) {
  qce_cell_holder_put_concrete_i64(&state->tmp, (gpointer)index, val);
}

static inline void qce_state_tmp_put_symbolic_i64(QCEState *state,
                                                  ptrdiff_t index, Z3_ast ast) {
  qce_cell_holder_put_symbolic_i64(&state->tmp, (gpointer)index, ast);
}

static inline void qce_state_tmp_put_i64(QCEState *state, ptrdiff_t index,
                                         QCEExpr *expr) {
#ifndef QCE_RELEASE
  assert(expr->type == QCE_EXPR_I64);
#endif
  switch (expr->mode) {
  case QCE_EXPR_CONCRETE: {
    qce_state_tmp_put_concrete_i64(state, index, expr->v_i64);
    break;
  }
  case QCE_EXPR_SYMBOLIC: {
    qce_state_tmp_put_symbolic_i64(state, index, expr->symbolic);
    break;
  }
  }
}

static inline void qce_state_tmp_get_i32(QCEState *state, ptrdiff_t index,
                                         QCEExpr *expr) {
  QCECellValue val;
  qce_cell_holder_get_i32(&state->tmp, (gpointer)index, &val);

  switch (val.mode) {
  case QCE_CELL_MODE_NULL: {
    qce_fatal("undefined tmp variable: %ld", index);
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

static inline void qce_state_tmp_get_i64(QCEState *state, ptrdiff_t index,
                                         QCEExpr *expr) {
  QCECellValue val;
  qce_cell_holder_get_i64(&state->tmp, (gpointer)index, &val);

  switch (val.mode) {
  case QCE_CELL_MODE_NULL: {
    qce_fatal("undefined tmp variable: %ld", index);
    break;
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

/*
 * State: Overall
 */

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
      qce_fatal("invalid variable type for const");
    }
    break;
  }
  case QCE_VAR_FIXED: {
#ifdef QCE_DEBUG_IR
    if (var->type != TCG_TYPE_I64) {
      qce_fatal("invalid variable type for fixed");
    }
#endif
    expr->mode = QCE_EXPR_CONCRETE;
    expr->type = QCE_EXPR_I64;
    expr->v_i64 = (int64_t)env;
    break;
  }
  case QCE_VAR_GLOBAL_DIRECT: {
    intptr_t addr = (intptr_t)env + var->v_global_direct.offset;
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_env_get_i32(state, addr, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_env_get_i64(state, addr, expr);
      break;
    }
    default:
      qce_fatal("invalid variable type for direct_global");
    }
    break;
  }
  case QCE_VAR_GLOBAL_INDIRECT: {
    intptr_t addr = (intptr_t)env + var->v_global_indirect.offset1 +
                    var->v_global_indirect.offset2;
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_env_get_i32(state, addr, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_env_get_i64(state, addr, expr);
      break;
    }
    default:
      qce_fatal("invalid variable type for indirect_global");
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
      qce_fatal("invalid variable type for temp_tb");
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
      qce_fatal("invalid variable type for temp_ebb");
    }
    break;
  }
  }
}

static inline void qce_state_put_var(CPUArchState *env, QCEState *state,
                                     QCEVar *var, QCEExpr *expr) {

  switch (var->kind) {
  case QCE_VAR_CONST: {
    qce_fatal("cannot assign to a const variable");
  }
  case QCE_VAR_FIXED: {
    qce_fatal("cannot assign to a fixed variable");
  }
  case QCE_VAR_GLOBAL_DIRECT: {
    intptr_t addr = (intptr_t)env + var->v_global_direct.offset;
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_env_put_i32(state, addr, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_env_put_i64(state, addr, expr);
      break;
    }
    default:
      qce_fatal("invalid variable type for direct_global");
    }
    break;
  }
  case QCE_VAR_GLOBAL_INDIRECT: {
    intptr_t addr = (intptr_t)env + var->v_global_indirect.offset1 +
                    var->v_global_indirect.offset2;
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_env_put_i32(state, addr, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_env_put_i64(state, addr, expr);
      break;
    }
    default:
      qce_fatal("invalid variable type for indirect_global");
    }
    break;
  }
  case QCE_VAR_TB: {
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_tmp_put_i32(state, var->v_tb.index, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_tmp_put_i64(state, var->v_tb.index, expr);
      break;
    }
    default:
      qce_fatal("invalid variable type for temp_tb");
    }
    break;
  }
  case QCE_VAR_EBB: {
    switch (var->type) {
    case TCG_TYPE_I32: {
      qce_state_tmp_put_i32(state, var->v_ebb.index, expr);
      break;
    }
    case TCG_TYPE_I64: {
      qce_state_tmp_put_i64(state, var->v_ebb.index, expr);
      break;
    }
    default:
      qce_fatal("invalid variable type for temp_ebb");
    }
    break;
  }
  }
}

static inline void qce_state_assert_path_constraint(QCEState *state,
                                                    Z3_ast predicate,
                                                    bool actual) {
  Z3_ast constraint =
      actual ? predicate : Z3_mk_not(state->solver_z3.ctx, predicate);
  Z3_solver_assert(state->solver_z3.ctx, state->solver_z3.sol, constraint);
}

/*
 * Testing
 */

#ifndef QCE_RELEASE

// helper macro
#define QCE_UNIT_TEST_STATE_PROLOGUE(name)                                     \
  static inline void qce_unit_test_state_##name(CPUArchState *env) {           \
    qce_debug("[test][state] " #name);                                         \
    QCEState state;                                                            \
    qce_state_init(&state);

#define QCE_UNIT_TEST_STATE_EPILOGUE                                           \
  qce_state_fini(&state);                                                      \
  }

#define QCE_UNIT_TEST_STATE_RUN(name) qce_unit_test_state_##name(env)

// individual test cases
QCE_UNIT_TEST_STATE_PROLOGUE(basics)
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_env_concrete_i32) {
  qce_state_env_put_concrete_i32(&state, (intptr_t)env, 42);
  QCEExpr e;
  qce_state_env_get_i32(&state, (intptr_t)env, &e);
  assert(e.mode == QCE_EXPR_CONCRETE);
  assert(e.type == QCE_EXPR_I32);
  assert(e.v_i32 == 42);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_env_symbolic_i32) {
  Z3_ast ast = qce_smt_z3_bv32_value(&state.solver_z3, 42);
  qce_state_env_put_symbolic_i32(&state, (intptr_t)env + 4, ast);
  QCEExpr e;
  qce_state_env_get_i32(&state, (intptr_t)env + 4, &e);
  assert(e.mode == QCE_EXPR_SYMBOLIC);
  assert(e.type == QCE_EXPR_I32);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv32_eq(&state.solver_z3, e.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_env_override_i32) {
  qce_state_env_put_concrete_i32(&state, (intptr_t)env, 0x42);
  QCEExpr e1;
  qce_state_env_get_i32(&state, (intptr_t)env, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I32);
  assert(e1.v_i32 == 0x42);

  Z3_ast ast = qce_smt_z3_bv32_value(&state.solver_z3, 0x43);
  qce_state_env_put_symbolic_i32(&state, (intptr_t)env, ast);
  QCEExpr e2;
  qce_state_env_get_i32(&state, (intptr_t)env, &e2);
  assert(e2.mode == QCE_EXPR_SYMBOLIC);
  assert(e2.type == QCE_EXPR_I32);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv32_eq(&state.solver_z3, e2.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_tmp_concrete_i32) {
  qce_state_tmp_put_concrete_i32(&state, 0, 77);
  QCEExpr e;
  qce_state_tmp_get_i32(&state, 0, &e);
  assert(e.mode == QCE_EXPR_CONCRETE);
  assert(e.type == QCE_EXPR_I32);
  assert(e.v_i32 == 77);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_tmp_symbolic_i32) {
  Z3_ast ast = qce_smt_z3_bv32_value(&state.solver_z3, 77);
  qce_state_tmp_put_symbolic_i32(&state, 10, ast);
  QCEExpr e;
  qce_state_tmp_get_i32(&state, 10, &e);
  assert(e.mode == QCE_EXPR_SYMBOLIC);
  assert(e.type == QCE_EXPR_I32);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv32_eq(&state.solver_z3, e.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_tmp_override_i32) {
  qce_state_tmp_put_concrete_i32(&state, 120, 0x42);
  QCEExpr e1;
  qce_state_tmp_get_i32(&state, 120, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I32);
  assert(e1.v_i32 == 0x42);

  Z3_ast ast = qce_smt_z3_bv32_value(&state.solver_z3, 0x43);
  qce_state_tmp_put_symbolic_i32(&state, 120, ast);
  QCEExpr e2;
  qce_state_tmp_get_i32(&state, 120, &e2);
  assert(e2.mode == QCE_EXPR_SYMBOLIC);
  assert(e2.type == QCE_EXPR_I32);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv32_eq(&state.solver_z3, e2.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_env_concrete_i64) {
  qce_state_env_put_concrete_i64(&state, (intptr_t)env + 16,
                                 0x0123456789ABCDEF);
  QCEExpr e1;
  qce_state_env_get_i64(&state, (intptr_t)env + 16, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I64);
  assert(e1.v_i64 == 0x0123456789ABCDEF);

  // ensure little-endian
  QCEExpr e1_l;
  qce_state_env_get_i32(&state, (intptr_t)env + 16, &e1_l);
  assert(e1_l.mode == QCE_EXPR_CONCRETE);
  assert(e1_l.type == QCE_EXPR_I32);
  assert(e1_l.v_i32 == 0x89ABCDEF);

  QCEExpr e1_h;
  qce_state_env_get_i32(&state, (intptr_t)env + 16 + QCE_CONCOLIC_REGISTER_SIZE,
                        &e1_h);
  assert(e1_h.mode == QCE_EXPR_CONCRETE);
  assert(e1_h.type == QCE_EXPR_I32);
  assert(e1_h.v_i32 == 0x01234567);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_env_symbolic_i64) {
  Z3_ast ast = qce_smt_z3_bv64_value(&state.solver_z3, 0xABCDEF0123456789);
  qce_state_env_put_symbolic_i64(&state, (intptr_t)env + 4, ast);
  QCEExpr e;
  qce_state_env_get_i64(&state, (intptr_t)env + 4, &e);
  assert(e.mode == QCE_EXPR_SYMBOLIC);
  assert(e.type == QCE_EXPR_I64);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv64_eq(&state.solver_z3, e.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);

  // ensure little-endian
  QCEExpr e1_l;
  qce_state_env_get_i32(&state, (intptr_t)env + 4, &e1_l);
  assert(e1_l.mode == QCE_EXPR_SYMBOLIC);
  assert(e1_l.type == QCE_EXPR_I32);
  assert(
      qce_smt_z3_prove(&state.solver_z3,
                       qce_smt_z3_bv32_eq(&state.solver_z3, e1_l.symbolic,
                                          qce_smt_z3_bv32_value(
                                              &state.solver_z3, 0x23456789))) ==
      SMT_Z3_PROVE_PROVED);

  QCEExpr e1_h;
  qce_state_env_get_i32(&state, (intptr_t)env + 4 + QCE_CONCOLIC_REGISTER_SIZE,
                        &e1_h);
  assert(e1_h.mode == QCE_EXPR_SYMBOLIC);
  assert(e1_h.type == QCE_EXPR_I32);
  assert(
      qce_smt_z3_prove(&state.solver_z3,
                       qce_smt_z3_bv32_eq(&state.solver_z3, e1_h.symbolic,
                                          qce_smt_z3_bv32_value(
                                              &state.solver_z3, 0xABCDEF01))) ==
      SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_env_override_i64) {
  qce_state_env_put_concrete_i64(&state, (intptr_t)env + 16,
                                 0x23456789ABCDEF01);
  QCEExpr e1;
  qce_state_env_get_i64(&state, (intptr_t)env + 16, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I64);
  assert(e1.v_i64 == 0x23456789ABCDEF01);

  // partial override 1
  Z3_ast ast = qce_smt_z3_bv32_value(&state.solver_z3, 0x98765432);
  qce_state_env_put_symbolic_i32(&state, (intptr_t)env + 16, ast);
  QCEExpr e2;
  qce_state_env_get_i64(&state, (intptr_t)env + 16, &e2);
  assert(e2.mode == QCE_EXPR_SYMBOLIC);
  assert(e2.type == QCE_EXPR_I64);
  assert(qce_smt_z3_prove(
             &state.solver_z3,
             qce_smt_z3_bv64_eq(
                 &state.solver_z3, e2.symbolic,
                 qce_smt_z3_bv64_concat(
                     &state.solver_z3,
                     qce_smt_z3_bv32_value(&state.solver_z3, 0x23456789),
                     ast))) == SMT_Z3_PROVE_PROVED);

  // partial override 2
  qce_state_env_put_concrete_i32(
      &state, (intptr_t)env + 16 + QCE_CONCOLIC_REGISTER_SIZE, 0x10FEDCBA);
  QCEExpr e3;
  qce_state_env_get_i64(&state, (intptr_t)env + 16, &e3);
  assert(e3.mode == QCE_EXPR_SYMBOLIC);
  assert(e3.type == QCE_EXPR_I64);
  assert(qce_smt_z3_prove(
             &state.solver_z3,
             qce_smt_z3_bv64_eq(
                 &state.solver_z3, e3.symbolic,
                 qce_smt_z3_bv64_concat(
                     &state.solver_z3,
                     qce_smt_z3_bv32_value(&state.solver_z3, 0x10FEDCBA),
                     ast))) == SMT_Z3_PROVE_PROVED);

  // partial override 3
  qce_state_env_put_concrete_i32(&state, (intptr_t)env + 16, 0x45678923);
  QCEExpr e4;
  qce_state_env_get_i64(&state, (intptr_t)env + 16, &e4);
  assert(e4.mode == QCE_EXPR_CONCRETE);
  assert(e4.type == QCE_EXPR_I64);
  assert(e4.v_i64 == 0x10FEDCBA45678923);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_tmp_concrete_i64) {
  qce_state_tmp_put_concrete_i64(&state, 0, 0xFEDCBA9876543210);
  QCEExpr e2;
  qce_state_tmp_get_i64(&state, 0, &e2);
  assert(e2.mode == QCE_EXPR_CONCRETE);
  assert(e2.type == QCE_EXPR_I64);
  assert(e2.v_i64 == 0xFEDCBA9876543210);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_tmp_symbolic_i64) {
  Z3_ast ast = qce_smt_z3_bv64_value(&state.solver_z3, 0xABCDEF0123456789);
  qce_state_tmp_put_symbolic_i64(&state, 6, ast);
  QCEExpr e;
  qce_state_tmp_get_i64(&state, 6, &e);
  assert(e.mode == QCE_EXPR_SYMBOLIC);
  assert(e.type == QCE_EXPR_I64);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv64_eq(&state.solver_z3, e.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_tmp_override_i64) {
  qce_state_tmp_put_concrete_i64(&state, 76, 1);
  QCEExpr e1;
  qce_state_tmp_get_i64(&state, 76, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I64);
  assert(e1.v_i64 == 1);

  Z3_ast ast = qce_smt_z3_bv64_value(&state.solver_z3, 2);
  qce_state_tmp_put_symbolic_i64(&state, 76, ast);
  QCEExpr e2;
  qce_state_tmp_get_i64(&state, 76, &e2);
  assert(e2.mode == QCE_EXPR_SYMBOLIC);
  assert(e2.type == QCE_EXPR_I64);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv64_eq(&state.solver_z3, e2.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_mem_concrete_i32) {
  unsigned mmu_idx = 0;
  intptr_t vaddr = 0x0;

  qce_state_mem_put_concrete_i32(&state, vaddr, mmu_idx, 42);
  QCEExpr e;
  qce_state_mem_get_i32(env, &state, vaddr, mmu_idx, &e);
  assert(e.mode == QCE_EXPR_CONCRETE);
  assert(e.type == QCE_EXPR_I32);
  assert(e.v_i32 == 42);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_mem_symbolic_i32) {
  unsigned mmu_idx = 1;
  intptr_t vaddr = 0x4;

  Z3_ast ast = qce_smt_z3_bv32_value(&state.solver_z3, 42);
  qce_state_mem_put_symbolic_i32(&state, vaddr, mmu_idx, ast);
  QCEExpr e;
  qce_state_mem_get_i32(env, &state, vaddr, mmu_idx, &e);
  assert(e.mode == QCE_EXPR_SYMBOLIC);
  assert(e.type == QCE_EXPR_I32);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv32_eq(&state.solver_z3, e.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_mem_override_i32) {
  unsigned mmu_idx = 2;
  intptr_t vaddr = 0x8;

  qce_state_mem_put_concrete_i32(&state, vaddr, mmu_idx, 0x42);
  QCEExpr e1;
  qce_state_mem_get_i32(env, &state, vaddr, mmu_idx, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I32);
  assert(e1.v_i32 == 0x42);

  Z3_ast ast = qce_smt_z3_bv32_value(&state.solver_z3, 0x43);
  qce_state_mem_put_symbolic_i32(&state, vaddr, mmu_idx, ast);
  QCEExpr e2;
  qce_state_mem_get_i32(env, &state, vaddr, mmu_idx, &e2);
  assert(e2.mode == QCE_EXPR_SYMBOLIC);
  assert(e2.type == QCE_EXPR_I32);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv32_eq(&state.solver_z3, e2.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_mem_concrete_i64) {
  unsigned mmu_idx = 3;
  intptr_t vaddr = 0x1000;

  qce_state_mem_put_concrete_i64(&state, vaddr, mmu_idx, 0x0123456789ABCDEF);
  QCEExpr e1;
  qce_state_mem_get_i64(env, &state, vaddr, mmu_idx, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I64);
  assert(e1.v_i64 == 0x0123456789ABCDEF);

  // ensure little-endian
  QCEExpr e1_l;
  qce_state_mem_get_i32(env, &state, vaddr, mmu_idx, &e1_l);
  assert(e1_l.mode == QCE_EXPR_CONCRETE);
  assert(e1_l.type == QCE_EXPR_I32);
  assert(e1_l.v_i32 == 0x89ABCDEF);

  QCEExpr e1_h;
  qce_state_mem_get_i32(env, &state, vaddr + QCE_CONCOLIC_REGISTER_SIZE,
                        mmu_idx, &e1_h);
  assert(e1_h.mode == QCE_EXPR_CONCRETE);
  assert(e1_h.type == QCE_EXPR_I32);
  assert(e1_h.v_i32 == 0x01234567);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_mem_symbolic_i64) {
  unsigned mmu_idx = 4;
  intptr_t vaddr = 0x2000;

  Z3_ast ast = qce_smt_z3_bv64_value(&state.solver_z3, 0xABCDEF0123456789);
  qce_state_mem_put_symbolic_i64(&state, vaddr, mmu_idx, ast);
  QCEExpr e;
  qce_state_mem_get_i64(env, &state, vaddr, mmu_idx, &e);
  assert(e.mode == QCE_EXPR_SYMBOLIC);
  assert(e.type == QCE_EXPR_I64);
  assert(qce_smt_z3_prove(&state.solver_z3,
                          qce_smt_z3_bv64_eq(&state.solver_z3, e.symbolic,
                                             ast)) == SMT_Z3_PROVE_PROVED);

  // ensure little-endian
  QCEExpr e1_l;
  qce_state_mem_get_i32(env, &state, vaddr, mmu_idx, &e1_l);
  assert(e1_l.mode == QCE_EXPR_SYMBOLIC);
  assert(e1_l.type == QCE_EXPR_I32);
  assert(
      qce_smt_z3_prove(&state.solver_z3,
                       qce_smt_z3_bv32_eq(&state.solver_z3, e1_l.symbolic,
                                          qce_smt_z3_bv32_value(
                                              &state.solver_z3, 0x23456789))) ==
      SMT_Z3_PROVE_PROVED);

  QCEExpr e1_h;
  qce_state_mem_get_i32(env, &state, vaddr + QCE_CONCOLIC_REGISTER_SIZE,
                        mmu_idx, &e1_h);
  assert(e1_h.mode == QCE_EXPR_SYMBOLIC);
  assert(e1_h.type == QCE_EXPR_I32);
  assert(
      qce_smt_z3_prove(&state.solver_z3,
                       qce_smt_z3_bv32_eq(&state.solver_z3, e1_h.symbolic,
                                          qce_smt_z3_bv32_value(
                                              &state.solver_z3, 0xABCDEF01))) ==
      SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(put_then_get_mem_override_i64) {
  unsigned mmu_idx = 5;
  intptr_t vaddr = 0x4000;

  qce_state_mem_put_concrete_i64(&state, vaddr, mmu_idx, 0x23456789ABCDEF01);
  QCEExpr e1;
  qce_state_mem_get_i64(env, &state, vaddr, mmu_idx, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I64);
  assert(e1.v_i64 == 0x23456789ABCDEF01);

  // partial override 1
  Z3_ast ast = qce_smt_z3_bv32_value(&state.solver_z3, 0x98765432);
  qce_state_mem_put_symbolic_i32(&state, vaddr, mmu_idx, ast);
  QCEExpr e2;
  qce_state_mem_get_i64(env, &state, vaddr, mmu_idx, &e2);
  assert(e2.mode == QCE_EXPR_SYMBOLIC);
  assert(e2.type == QCE_EXPR_I64);
  assert(qce_smt_z3_prove(
             &state.solver_z3,
             qce_smt_z3_bv64_eq(
                 &state.solver_z3, e2.symbolic,
                 qce_smt_z3_bv64_concat(
                     &state.solver_z3,
                     qce_smt_z3_bv32_value(&state.solver_z3, 0x23456789),
                     ast))) == SMT_Z3_PROVE_PROVED);

  // partial override 2
  qce_state_mem_put_concrete_i32(&state, vaddr + QCE_CONCOLIC_REGISTER_SIZE,
                                 mmu_idx, 0x10FEDCBA);
  QCEExpr e3;
  qce_state_mem_get_i64(env, &state, vaddr, mmu_idx, &e3);
  assert(e3.mode == QCE_EXPR_SYMBOLIC);
  assert(e3.type == QCE_EXPR_I64);
  assert(qce_smt_z3_prove(
             &state.solver_z3,
             qce_smt_z3_bv64_eq(
                 &state.solver_z3, e3.symbolic,
                 qce_smt_z3_bv64_concat(
                     &state.solver_z3,
                     qce_smt_z3_bv32_value(&state.solver_z3, 0x10FEDCBA),
                     ast))) == SMT_Z3_PROVE_PROVED);

  // partial override 3
  qce_state_mem_put_concrete_i32(&state, vaddr, mmu_idx, 0x45678923);
  QCEExpr e4;
  qce_state_mem_get_i64(env, &state, vaddr, mmu_idx, &e4);
  assert(e4.mode == QCE_EXPR_CONCRETE);
  assert(e4.type == QCE_EXPR_I64);
  assert(e4.v_i64 == 0x10FEDCBA45678923);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(retrieve_mem_different_mmus) {
  unsigned mmu1_idx = 1;
  unsigned mmu2_idx = 2;
  intptr_t vaddr = 0x4000;

  qce_state_mem_put_concrete_i64(&state, vaddr, mmu1_idx, 0x0123456789ABCDEF);
  qce_state_mem_put_concrete_i64(&state, vaddr, mmu2_idx, 0xFEDCBA9876543210);

  QCEExpr e1;
  qce_state_mem_get_i64(env, &state, vaddr, mmu1_idx, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I64);
  assert(e1.v_i64 == 0x0123456789ABCDEF);

  QCEExpr e2;
  qce_state_mem_get_i64(env, &state, vaddr, mmu2_idx, &e2);
  assert(e2.mode == QCE_EXPR_CONCRETE);
  assert(e2.type == QCE_EXPR_I64);
  assert(e2.v_i64 == 0xFEDCBA9876543210);
}
QCE_UNIT_TEST_STATE_EPILOGUE

QCE_UNIT_TEST_STATE_PROLOGUE(retrieve_mem_concrete_special) {
  /* TODO: this test case is highly specialized and depends on many factors
   *       (e.g., guest agent, test case, etc) outside of the QCE code. */
  unsigned mmu_idx = cpu_mmu_index(env_cpu(env), false);
  intptr_t vaddr = env->regs[R_EDI];

  QCEExpr e1;
  qce_state_mem_get_i32(env, &state, vaddr, mmu_idx, &e1);
  assert(e1.mode == QCE_EXPR_CONCRETE);
  assert(e1.type == QCE_EXPR_I32);
  assert((char)e1.v_i32 == 'X');

  QCEExpr e2;
  qce_state_mem_get_i64(env, &state, vaddr, mmu_idx, &e2);
  assert(e2.mode == QCE_EXPR_CONCRETE);
  assert(e2.type == QCE_EXPR_I64);
  assert((char)e2.v_i64 == 'X');
}
QCE_UNIT_TEST_STATE_EPILOGUE

// collector
static inline void qce_unit_test_state(CPUArchState *env) {
  QCE_UNIT_TEST_STATE_RUN(basics);

  // put_then_get series
  QCE_UNIT_TEST_STATE_RUN(put_then_get_env_concrete_i32);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_env_symbolic_i32);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_env_override_i32);

  QCE_UNIT_TEST_STATE_RUN(put_then_get_tmp_concrete_i32);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_tmp_symbolic_i32);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_tmp_override_i32);

  QCE_UNIT_TEST_STATE_RUN(put_then_get_env_concrete_i64);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_env_symbolic_i64);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_env_override_i64);

  QCE_UNIT_TEST_STATE_RUN(put_then_get_tmp_concrete_i64);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_tmp_symbolic_i64);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_tmp_override_i64);

  QCE_UNIT_TEST_STATE_RUN(put_then_get_mem_concrete_i32);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_mem_symbolic_i32);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_mem_override_i32);

  QCE_UNIT_TEST_STATE_RUN(put_then_get_mem_concrete_i64);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_mem_symbolic_i64);
  QCE_UNIT_TEST_STATE_RUN(put_then_get_mem_override_i64);

  QCE_UNIT_TEST_STATE_RUN(retrieve_mem_different_mmus);
  QCE_UNIT_TEST_STATE_RUN(retrieve_mem_concrete_special);
}
#endif

#endif /* QCE_STATE_H */