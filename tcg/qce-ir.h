typedef enum {
  QCE_VAR_CONST,
  QCE_VAR_FIXED,
  QCE_VAR_GLOBAL_DIRECT,
  QCE_VAR_GLOBAL_INDIRECT,
  QCE_VAR_TB,
  QCE_VAR_EBB,
} QCEVarKind;

typedef struct {
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
      ptrdiff_t index;
    } v_tb;

    // kind: TEMP_EBB
    struct {
      TCGType type;
      ptrdiff_t index;
    } v_ebb;
  };
} QCEVar;

static inline ptrdiff_t temp_index(const TCGContext *tcg, TCGTemp *t) {
  ptrdiff_t n = t - tcg->temps;
#ifdef QCE_DEBUG_IR
  g_assert(n >= 0 && n < tcg->nb_temps);
#endif
  return n;
}

static inline void parse_var(TCGContext *tcg, TCGTemp *t, QCEVar *v) {
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
  case TEMP_TB: {
#ifdef QCE_DEBUG_IR
    // expected when emulating x86_64 guest on x86_64 host
    if (t->base_type == TCG_TYPE_I128) {
      qce_debug_assert_ir1(tcg, t->type == TCG_TYPE_I64, t);
      TCGTemp *n;
      switch (t->temp_subindex) {
      case 0:
        n = t + 1;
        qce_debug_assert_ir1(tcg, n->temp_subindex == 1, n);
        break;
      case 1:
        n = t - 1;
        qce_debug_assert_ir1(tcg, n->temp_subindex == 0, n);
        break;
      default:
        g_assert_not_reached();
        break;
      }
      qce_debug_assert_ir1(tcg, n->type == TCG_TYPE_I64, n);
      qce_debug_assert_ir1(tcg, n->kind == TEMP_TB, n);
    } else {
      // expected when emulating x86_64 guest on x86_64 host
      qce_debug_assert_ir1(tcg, t->type == t->base_type, t);
      qce_debug_assert_ir1(tcg, t->temp_subindex == 0, t);
    }
#endif

    v->kind = QCE_VAR_TB;
    v->v_tb.type = t->base_type;
    v->v_tb.index = temp_index(tcg, t);
    break;
  }
  case TEMP_EBB: {
#ifdef QCE_DEBUG_IR
    // expected when emulating x86_64 guest on x86_64 host
    if (t->base_type == TCG_TYPE_I128) {
      qce_debug_assert_ir1(tcg, t->type == TCG_TYPE_I64, t);
      TCGTemp *n;
      switch (t->temp_subindex) {
      case 0:
        n = t + 1;
        qce_debug_assert_ir1(tcg, n->temp_subindex == 1, n);
        break;
      case 1:
        n = t - 1;
        qce_debug_assert_ir1(tcg, n->temp_subindex == 0, n);
        break;
      default:
        g_assert_not_reached();
        break;
      }
      qce_debug_assert_ir1(tcg, n->type == TCG_TYPE_I64, n);
      qce_debug_assert_ir1(tcg, n->kind == TEMP_EBB, n);
    } else {
      // expected when emulating x86_64 guest on x86_64 host
      qce_debug_assert_ir1(tcg, t->type == t->base_type, t);
      qce_debug_assert_ir1(tcg, t->temp_subindex == 0, t);
    }
#endif

    v->kind = QCE_VAR_EBB;
    v->v_ebb.type = t->base_type;
    v->v_ebb.index = temp_index(tcg, t);
    break;
  }
  default:
    g_assert_not_reached();
  }
}
static inline void parse_arg_as_var(TCGContext *tcg, TCGArg arg, QCEVar *v) {
  parse_var(tcg, arg_temp(arg), v);
}

typedef struct {
  uint16_t id;
} QCELabel;

static inline void parse_label(TCGContext *tcg, TCGLabel *l, QCELabel *v) {
  qce_debug_assert_label_intact(tcg, l);
  v->id = l->id;
}
static inline void parse_arg_as_label(TCGContext *tcg, TCGArg arg,
                                      QCELabel *v) {
  parse_label(tcg, arg_label(arg), v);
}

typedef enum {
  QCE_INST_DISCARD,
  QCE_INST_SET_LABEL,
} QCEInstKind;

typedef struct {
  QCEInstKind kind;
  union {
    // opc: discard
    struct {
      QCEVar out;
    } i_discard;

    // opc: set_label
    struct {
      QCELabel label;
    } i_set_label;
  };
} QCEInst;

static inline void parse_op(TCGContext *tcg, const TCGOp *op, QCEInst *inst) {
  TCGOpcode c = op->opc;
  const TCGOpDef *def = &tcg_op_defs[c];

#ifndef QCE_SUPPORTS_VEC
  // there should never be an op with vector operands
  qce_debug_assert_op1(tcg, (def->flags & TCG_OPF_VECTOR) == 0, op);
#endif

  if (c == INDEX_op_insn_start) {
    // TODO: special case
    return;
  }

  if (c == INDEX_op_call) {
    // TODO: special case
    return;
  }

  // all other instructions
  qce_debug_assert_op1(
      tcg, op->nargs >= def->nb_oargs + def->nb_iargs + def->nb_cargs, op);

  // TODO: temporary check (to be removed)
  QCEVar v;
  for (int i = 0; i < def->nb_oargs; i++) {
    parse_arg_as_var(tcg, op->args[i], &v);
  }
  for (int i = 0; i < def->nb_iargs; i++) {
    parse_arg_as_var(tcg, op->args[i + def->nb_oargs], &v);
  }

  // parse the instructions
  switch (c) {
  case INDEX_op_discard: {
    inst->kind = QCE_INST_DISCARD;
    parse_arg_as_var(tcg, op->args[0], &inst->i_discard.out);
    break;
  }
  case INDEX_op_set_label: {
    inst->kind = QCE_INST_SET_LABEL;
    parse_arg_as_label(tcg, op->args[0], &inst->i_set_label.label);
    break;
  }
  default: {
    // TODO
    // g_assert_not_reached();
    break;
  }
  }
}
