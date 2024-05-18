typedef enum {
  QCE_VAR_CONST,
  QCE_VAR_FIXED,
  QCE_VAR_GLOBAL_DIRECT,
  QCE_VAR_GLOBAL_INDIRECT,
  QCE_VAR_TB_SINGLE,
  QCE_VAR_TB_DOUBLE,
} QCEVarKind;

struct QCEVar {
  QCEVarKind kind;
  union {
    // kind: TEMP_CONST
    struct {
      TCGType type;
      int64_t val;
    } v_const;

    // kind: TEMP_FIXED
    struct {
      TCGType type;
      TCGReg reg;
    } v_fixed;

    // kind: TEMP_GLOBAL
    struct {
      TCGType type;
      TCGReg base;
      intptr_t offset;
    } v_global_direct;
    struct {
      TCGType type;
      TCGReg base;
      intptr_t offset1;
      intptr_t offset2;
    } v_global_indirect;

    // kind: TEMP_TB
    struct {
      TCGType type;
    } v_tb_single;
    struct {
      TCGType type;
    } v_tb_double;
  };
};

static inline void parse_var(const TCGContext *tcg, TCGTemp *t,
                             struct QCEVar *v) {
#ifndef QCE_SUPPORTS_VEC
  // there should never be a variable in vector type
  switch (t->base_type) {
  case TCG_TYPE_I32:
  case TCG_TYPE_I64:
  case TCG_TYPE_I128:
    break;
  case TCG_TYPE_V64:
  case TCG_TYPE_V128:
  case TCG_TYPE_V256:
    qce_debug_assert_ir1(tcg, false, t);
    break;
  default:
    g_assert_not_reached();
  }
#endif

  switch (t->kind) {
  case TEMP_CONST: {
    // expected when emulating x86_64 guest on x86_64 host
    qce_debug_assert_ir1(tcg, t->type == t->base_type, t);
    qce_debug_assert_ir1(tcg, t->temp_subindex == 0, t);

    v->kind = QCE_VAR_CONST;
    v->v_const.type = t->type;
    v->v_const.val = t->val;
    break;
  }
  case TEMP_FIXED: {
    // expected when emulating x86_64 guest on x86_64 host
    qce_debug_assert_ir1(tcg, t->type == t->base_type, t);
    qce_debug_assert_ir1(tcg, t->temp_subindex == 0, t);

    v->kind = QCE_VAR_FIXED;
    v->v_fixed.type = t->type;
    v->v_fixed.reg = t->reg;
    break;
  }
  case TEMP_GLOBAL: {
    // expected when emulating x86_64 guest on x86_64 host
    qce_debug_assert_ir1(tcg, t->type == t->base_type, t);
    qce_debug_assert_ir1(tcg, t->temp_subindex == 0, t);

    if (t->indirect_reg == 0) {
      TCGTemp *base = t->mem_base;
      qce_debug_assert_ir2(tcg, base->kind == TEMP_FIXED, t, base);

      v->kind = QCE_VAR_GLOBAL_DIRECT;
      v->v_global_direct.type = t->type;
      v->v_global_direct.base = base->reg;
      v->v_global_direct.offset = t->mem_offset;
    } else {
      TCGTemp *offset = t->mem_base;
      TCGTemp *base = offset->mem_base;
      qce_debug_assert_ir3(
          tcg, offset->kind == TEMP_GLOBAL && base->kind == TEMP_FIXED, t,
          offset, base);

      v->kind = QCE_VAR_GLOBAL_INDIRECT;
      v->v_global_indirect.type = t->type;
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
