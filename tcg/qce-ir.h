typedef enum {
  QCE_VAR_FIXED,
  QCE_VAR_GLOBAL_DIRECT,
  QCE_VAR_GLOBAL_INDIRECT,
} QCEVarKind;

struct QCEVar {
  QCEVarKind kind;
  union {
    // kind: TEMP_FIXED
    struct {
      TCGReg reg;
    } v_fixed;

    // kind: TEMP_GLOBAL
    struct {
      TCGReg base;
      intptr_t offset;
    } v_global_direct;
    struct {
      TCGReg base;
      intptr_t offset1;
      intptr_t offset2;
    } v_global_indirect;
  };
};

static inline void parse_var(const TCGContext *tcg, TCGTemp *t,
                             struct QCEVar *v) {
  qce_debug_assert_ir1(tcg, t->type == t->base_type, t);

  switch (t->kind) {
  case TEMP_FIXED: {
    v->kind = QCE_VAR_FIXED;
    v->v_fixed.reg = t->reg;
    break;
  }
  case TEMP_GLOBAL: {
    if (t->indirect_reg == 0) {
      TCGTemp *base = t->mem_base;
      qce_debug_assert_ir2(tcg, base->kind == TEMP_FIXED, t, base);
      v->kind = QCE_VAR_GLOBAL_DIRECT;
      v->v_global_direct.base = base->reg;
      v->v_global_direct.offset = t->mem_offset;
    } else {
      TCGTemp *offset = t->mem_base;
      TCGTemp *base = offset->mem_base;
      qce_debug_assert_ir3(
          tcg, offset->kind == TEMP_GLOBAL && base->kind == TEMP_FIXED, t,
          offset, base);
      v->kind = QCE_VAR_GLOBAL_INDIRECT;
      v->v_global_indirect.base = base->reg;
      v->v_global_indirect.offset1 = offset->mem_offset;
      v->v_global_indirect.offset2 = t->mem_offset;
    }
    break;
  }
  default:
    g_assert_not_reached();
  }
}

struct QCEInst {
  TCGOpcode opc;
  union {
    // opc: discard
    struct {
      TCGArg out;
    } op_discard;
  } inst;
};

static inline void parse_op(const TCGOp *op, struct QCEInst *inst) {
  switch (op->opc) {
  case INDEX_op_discard:
    return;
  default:
    return;
  }
}
