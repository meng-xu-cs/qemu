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

static inline void qce_expr_init_i32(QCEExpr *expr, int32_t val) {
  expr->mode = QCE_EXPR_CONCRETE;
  expr->type = QCE_EXPR_I32;
  expr->v_i32 = val;
}

static inline void qce_expr_init_i64(QCEExpr *expr, int64_t val) {
  expr->mode = QCE_EXPR_CONCRETE;
  expr->type = QCE_EXPR_I64;
  expr->v_i64 = val;
}
