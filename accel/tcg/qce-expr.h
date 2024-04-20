#define qce_expr_assert_type(expr, ty)                                         \
  if (expr->type != QCE_EXPR_##ty) {                                           \
    qce_fatal("[expr] type mismatch: expect " #ty ", actual %d", expr->type);  \
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

/*
 * Initialization
 */

static inline void qce_expr_init_v32(QCEExpr *expr, int32_t val) {
  expr->mode = QCE_EXPR_CONCRETE;
  expr->type = QCE_EXPR_I32;
  expr->v_i32 = val;
}

#ifndef QCE_RELEASE
static inline void qce_expr_init_s32(SolverZ3 *solver, QCEExpr *expr) {
  expr->mode = QCE_EXPR_SYMBOLIC;
  expr->type = QCE_EXPR_I32;
  expr->symbolic = qce_smt_z3_bv32_var(solver);
}
#endif

static inline void qce_expr_init_v64(QCEExpr *expr, int64_t val) {
  expr->mode = QCE_EXPR_CONCRETE;
  expr->type = QCE_EXPR_I64;
  expr->v_i64 = val;
}

#ifndef QCE_RELEASE
static inline void qce_expr_init_s64(SolverZ3 *solver, QCEExpr *expr) {
  expr->mode = QCE_EXPR_SYMBOLIC;
  expr->type = QCE_EXPR_I64;
  expr->symbolic = qce_smt_z3_bv64_var(solver);
}
#endif

/*
 * Arithmetics
 */

static inline void qce_expr_add_i32(SolverZ3 *solver, QCEExpr *lhs,
                                    QCEExpr *rhs, QCEExpr *result) {
  qce_expr_assert_type(lhs, I32);
  qce_expr_assert_type(rhs, I32);

  if (lhs->mode == QCE_EXPR_CONCRETE) {
    if (rhs->mode == QCE_EXPR_CONCRETE) {
      result->mode = QCE_EXPR_CONCRETE;
      result->v_i32 = lhs->v_i32 + rhs->v_i32;
    } else {
      result->mode = QCE_EXPR_SYMBOLIC;
      result->symbolic = qce_smt_z3_bv32_add(
          solver, qce_smt_z3_bv32_value(solver, lhs->v_i32), rhs->symbolic);
    }
  } else {
    if (rhs->mode == QCE_EXPR_CONCRETE) {
      result->mode = QCE_EXPR_SYMBOLIC;
      result->symbolic = qce_smt_z3_bv32_add(
          solver, lhs->symbolic, qce_smt_z3_bv32_value(solver, rhs->v_i32));
    } else {
      Z3_ast res = qce_smt_z3_bv32_add(solver, lhs->symbolic, rhs->symbolic);
      int32_t val = 0;
      if (qce_smt_z3_probe_bv32(solver, res, &val)) {
        result->mode = QCE_EXPR_CONCRETE;
        result->v_i32 = val;
      } else {
        result->mode = QCE_EXPR_SYMBOLIC;
        result->symbolic = res;
      }
    }
  }

  result->type = QCE_EXPR_I32;
}

static inline void qce_expr_add_i64(SolverZ3 *solver, QCEExpr *lhs,
                                    QCEExpr *rhs, QCEExpr *result) {
  qce_expr_assert_type(lhs, I64);
  qce_expr_assert_type(rhs, I64);

  if (lhs->mode == QCE_EXPR_CONCRETE) {
    if (rhs->mode == QCE_EXPR_CONCRETE) {
      result->mode = QCE_EXPR_CONCRETE;
      result->v_i64 = lhs->v_i64 + rhs->v_i64;
    } else {
      result->mode = QCE_EXPR_SYMBOLIC;
      result->symbolic = qce_smt_z3_bv64_add(
          solver, qce_smt_z3_bv64_value(solver, lhs->v_i64), rhs->symbolic);
    }
  } else {
    if (rhs->mode == QCE_EXPR_CONCRETE) {
      result->mode = QCE_EXPR_SYMBOLIC;
      result->symbolic = qce_smt_z3_bv64_add(
          solver, lhs->symbolic, qce_smt_z3_bv64_value(solver, rhs->v_i64));
    } else {
      Z3_ast res = qce_smt_z3_bv64_add(solver, lhs->symbolic, rhs->symbolic);
      int64_t val = 0;
      if (qce_smt_z3_probe_bv64(solver, res, &val)) {
        result->mode = QCE_EXPR_CONCRETE;
        result->v_i64 = val;
      } else {
        result->mode = QCE_EXPR_SYMBOLIC;
        result->symbolic = res;
      }
    }
  }

  result->type = QCE_EXPR_I64;
}

static inline void qce_expr_sub_i32(SolverZ3 *solver, QCEExpr *lhs,
                                    QCEExpr *rhs, QCEExpr *result) {
  qce_expr_assert_type(lhs, I32);
  qce_expr_assert_type(rhs, I32);

  if (lhs->mode == QCE_EXPR_CONCRETE) {
    if (rhs->mode == QCE_EXPR_CONCRETE) {
      result->mode = QCE_EXPR_CONCRETE;
      result->v_i32 = lhs->v_i32 - rhs->v_i32;
    } else {
      result->mode = QCE_EXPR_SYMBOLIC;
      result->symbolic = qce_smt_z3_bv32_sub(
          solver, qce_smt_z3_bv32_value(solver, lhs->v_i32), rhs->symbolic);
    }
  } else {
    if (rhs->mode == QCE_EXPR_CONCRETE) {
      result->mode = QCE_EXPR_SYMBOLIC;
      result->symbolic = qce_smt_z3_bv32_sub(
          solver, lhs->symbolic, qce_smt_z3_bv32_value(solver, rhs->v_i32));
    } else {
      Z3_ast res = qce_smt_z3_bv32_sub(solver, lhs->symbolic, rhs->symbolic);
      int32_t val = 0;
      if (qce_smt_z3_probe_bv32(solver, res, &val)) {
        result->mode = QCE_EXPR_CONCRETE;
        result->v_i32 = val;
      } else {
        result->mode = QCE_EXPR_SYMBOLIC;
        result->symbolic = res;
      }
    }
  }

  result->type = QCE_EXPR_I32;
}

static inline void qce_expr_sub_i64(SolverZ3 *solver, QCEExpr *lhs,
                                    QCEExpr *rhs, QCEExpr *result) {
  qce_expr_assert_type(lhs, I64);
  qce_expr_assert_type(rhs, I64);

  if (lhs->mode == QCE_EXPR_CONCRETE) {
    if (rhs->mode == QCE_EXPR_CONCRETE) {
      result->mode = QCE_EXPR_CONCRETE;
      result->v_i64 = lhs->v_i64 - rhs->v_i64;
    } else {
      result->mode = QCE_EXPR_SYMBOLIC;
      result->symbolic = qce_smt_z3_bv64_sub(
          solver, qce_smt_z3_bv64_value(solver, lhs->v_i64), rhs->symbolic);
    }
  } else {
    if (rhs->mode == QCE_EXPR_CONCRETE) {
      result->mode = QCE_EXPR_SYMBOLIC;
      result->symbolic = qce_smt_z3_bv64_sub(
          solver, lhs->symbolic, qce_smt_z3_bv64_value(solver, rhs->v_i64));
    } else {
      Z3_ast res = qce_smt_z3_bv64_sub(solver, lhs->symbolic, rhs->symbolic);
      int64_t val = 0;
      if (qce_smt_z3_probe_bv64(solver, res, &val)) {
        result->mode = QCE_EXPR_CONCRETE;
        result->v_i64 = val;
      } else {
        result->mode = QCE_EXPR_SYMBOLIC;
        result->symbolic = res;
      }
    }
  }

  result->type = QCE_EXPR_I64;
}

/*
 * Testing
 */

#ifndef QCE_RELEASE

// helper macro
#define QCE_UNIT_TEST_EXPR_PROLOGUE(name)                                      \
  static inline void qce_unit_test_expr_##name(void) {                         \
    qce_debug("[test][expr] " #name);                                          \
    SolverZ3 solver;                                                           \
    qce_smt_z3_init(&solver);

#define QCE_UNIT_TEST_EXPR_EPILOGUE                                            \
  qce_smt_z3_fini(&solver);                                                    \
  }

#define QCE_UNIT_TEST_EXPR_RUN(name) qce_unit_test_expr_##name()

// individual test cases
QCE_UNIT_TEST_EXPR_PROLOGUE(basics)
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(add_concrete_i32) {
  QCEExpr lhs, rhs, res;
  qce_expr_init_v32(&lhs, 1);
  qce_expr_init_v32(&rhs, 2);
  qce_expr_add_i32(&solver, &lhs, &rhs, &res);
  assert(res.type == QCE_EXPR_I32);
  assert(res.mode == QCE_EXPR_CONCRETE);
  assert(res.v_i32 == 3);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(add_symbolic_i32) {
  QCEExpr lhs, rhs, res;
  qce_expr_init_s32(&solver, &lhs);
  qce_expr_init_s32(&solver, &rhs);
  qce_expr_add_i32(&solver, &lhs, &rhs, &res);
  assert(res.type == QCE_EXPR_I32);
  assert(res.mode == QCE_EXPR_SYMBOLIC);
  assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv32_eq(&solver, res.symbolic,
                                                      qce_smt_z3_bv32_add(
                                                          &solver, lhs.symbolic,
                                                          rhs.symbolic))) ==
         SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(add_concrete_i64) {
  QCEExpr lhs, rhs, res;
  qce_expr_init_v64(&lhs, -1);
  qce_expr_init_v64(&rhs, 3);
  qce_expr_add_i64(&solver, &lhs, &rhs, &res);
  assert(res.type == QCE_EXPR_I64);
  assert(res.mode == QCE_EXPR_CONCRETE);
  assert(res.v_i64 == 2);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(add_symbolic_i64) {
  QCEExpr lhs, rhs, res;
  qce_expr_init_s64(&solver, &lhs);
  qce_expr_init_s64(&solver, &rhs);
  qce_expr_add_i64(&solver, &lhs, &rhs, &res);
  assert(res.type == QCE_EXPR_I64);
  assert(res.mode == QCE_EXPR_SYMBOLIC);
  assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv64_eq(&solver, res.symbolic,
                                                      qce_smt_z3_bv64_add(
                                                          &solver, lhs.symbolic,
                                                          rhs.symbolic))) ==
         SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(sub_concrete_i32) {
  QCEExpr lhs, rhs, res;
  qce_expr_init_v32(&lhs, 1);
  qce_expr_init_v32(&rhs, 2);
  qce_expr_sub_i32(&solver, &lhs, &rhs, &res);
  assert(res.type == QCE_EXPR_I32);
  assert(res.mode == QCE_EXPR_CONCRETE);
  assert(res.v_i32 == -1);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(sub_symbolic_i32) {
  QCEExpr lhs, rhs, res;
  qce_expr_init_s32(&solver, &lhs);
  qce_expr_init_s32(&solver, &rhs);
  qce_expr_sub_i32(&solver, &lhs, &rhs, &res);
  assert(res.type == QCE_EXPR_I32);
  assert(res.mode == QCE_EXPR_SYMBOLIC);
  assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv32_eq(&solver, res.symbolic,
                                                      qce_smt_z3_bv32_sub(
                                                          &solver, lhs.symbolic,
                                                          rhs.symbolic))) ==
         SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(sub_concrete_i64) {
  QCEExpr lhs, rhs, res;
  qce_expr_init_v64(&lhs, -1);
  qce_expr_init_v64(&rhs, -3);
  qce_expr_sub_i64(&solver, &lhs, &rhs, &res);
  assert(res.type == QCE_EXPR_I64);
  assert(res.mode == QCE_EXPR_CONCRETE);
  assert(res.v_i64 == 2);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(sub_symbolic_i64) {
  QCEExpr lhs, rhs, res;
  qce_expr_init_s64(&solver, &lhs);
  qce_expr_init_s64(&solver, &rhs);
  qce_expr_sub_i64(&solver, &lhs, &rhs, &res);
  assert(res.type == QCE_EXPR_I64);
  assert(res.mode == QCE_EXPR_SYMBOLIC);
  assert(qce_smt_z3_prove(&solver, qce_smt_z3_bv64_eq(&solver, res.symbolic,
                                                      qce_smt_z3_bv64_sub(
                                                          &solver, lhs.symbolic,
                                                          rhs.symbolic))) ==
         SMT_Z3_PROVE_PROVED);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(special_a_sub_a_i32) {
  QCEExpr a, res;
  qce_expr_init_s32(&solver, &a);
  qce_expr_sub_i32(&solver, &a, &a, &res);
  assert(res.type == QCE_EXPR_I32);
  assert(res.mode == QCE_EXPR_CONCRETE);
  assert(res.v_i32 == 0);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(special_a_sub_a_i64) {
  QCEExpr a, res;
  qce_expr_init_s64(&solver, &a);
  qce_expr_sub_i64(&solver, &a, &a, &res);
  assert(res.type == QCE_EXPR_I64);
  assert(res.mode == QCE_EXPR_CONCRETE);
  assert(res.v_i64 == 0);
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(special_a_add_then_sub_i32) {
  QCEExpr a, c1, c2, r1, r2;
  qce_expr_init_s32(&solver, &a);
  qce_expr_init_v32(&c1, 1);
  qce_expr_init_v32(&c2, 3);
  qce_expr_add_i32(&solver, &a, &c1, &r1);
  qce_expr_sub_i32(&solver, &r1, &c2, &r2);
  assert(r2.type == QCE_EXPR_I32);
  assert(r2.mode == QCE_EXPR_SYMBOLIC);

  QCEExpr v, r;
  qce_expr_init_v32(&v, 2);
  qce_expr_sub_i32(&solver, &a, &v, &r);
  assert(r.type == QCE_EXPR_I32);
  assert(r.mode == QCE_EXPR_SYMBOLIC);

  assert(qce_smt_z3_prove(
      &solver, qce_smt_z3_bv32_eq(&solver, r2.symbolic, r.symbolic)));
}
QCE_UNIT_TEST_EXPR_EPILOGUE

QCE_UNIT_TEST_EXPR_PROLOGUE(special_a_add_then_sub_i64) {
  QCEExpr a, c1, c2, r1, r2;
  qce_expr_init_s64(&solver, &a);
  qce_expr_init_v64(&c1, -1);
  qce_expr_init_v64(&c2, -3);
  qce_expr_add_i64(&solver, &a, &c1, &r1);
  qce_expr_sub_i64(&solver, &r1, &c2, &r2);
  assert(r2.type == QCE_EXPR_I64);
  assert(r2.mode == QCE_EXPR_SYMBOLIC);

  QCEExpr v, r;
  qce_expr_init_v64(&v, 2);
  qce_expr_add_i64(&solver, &a, &v, &r);
  assert(r.type == QCE_EXPR_I64);
  assert(r.mode == QCE_EXPR_SYMBOLIC);

  assert(qce_smt_z3_prove(
      &solver, qce_smt_z3_bv64_eq(&solver, r2.symbolic, r.symbolic)));
}
QCE_UNIT_TEST_EXPR_EPILOGUE

// collector
static inline void qce_unit_test_expr(void) {
  QCE_UNIT_TEST_EXPR_RUN(basics);

  QCE_UNIT_TEST_EXPR_RUN(add_concrete_i32);
  QCE_UNIT_TEST_EXPR_RUN(add_symbolic_i32);
  QCE_UNIT_TEST_EXPR_RUN(add_concrete_i64);
  QCE_UNIT_TEST_EXPR_RUN(add_symbolic_i64);

  QCE_UNIT_TEST_EXPR_RUN(sub_concrete_i32);
  QCE_UNIT_TEST_EXPR_RUN(sub_symbolic_i32);
  QCE_UNIT_TEST_EXPR_RUN(sub_concrete_i64);
  QCE_UNIT_TEST_EXPR_RUN(sub_symbolic_i64);

  QCE_UNIT_TEST_EXPR_RUN(special_a_sub_a_i32);
  QCE_UNIT_TEST_EXPR_RUN(special_a_sub_a_i64);
  QCE_UNIT_TEST_EXPR_RUN(special_a_add_then_sub_i32);
  QCE_UNIT_TEST_EXPR_RUN(special_a_add_then_sub_i64);
}
#endif
