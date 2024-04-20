typedef enum {
  QCE_VAR_CONST,
  QCE_VAR_FIXED,
  QCE_VAR_GLOBAL_DIRECT,
  QCE_VAR_GLOBAL_INDIRECT,
  QCE_VAR_TB,
  QCE_VAR_EBB,
} QCEVarKind;

#define QCE_VAR_NAME_MAX 8
typedef struct {
  QCEVarKind kind;
  TCGType type;
  union {
    // kind: TEMP_CONST
    struct {
      int64_t val;
    } v_const;

    // kind: TEMP_FIXED
    struct {
      TCGReg reg;
      char name[QCE_VAR_NAME_MAX];
    } v_fixed;

    // kind: TEMP_GLOBAL
    struct {
      TCGReg base;
      intptr_t offset;
      char name[QCE_VAR_NAME_MAX];
    } v_global_direct;
    struct {
      TCGReg base;
      intptr_t offset1;
      intptr_t offset2;
      char name[QCE_VAR_NAME_MAX];
    } v_global_indirect;

    // kind: TEMP_TB
    struct {
      ptrdiff_t index;
    } v_tb;

    // kind: TEMP_EBB
    struct {
      ptrdiff_t index;
    } v_ebb;
  };
} QCEVar;

#ifdef QCE_DEBUG_IR
static inline void qce_debug_print_type(FILE *f, TCGType t) {
  switch (t) {
  case TCG_TYPE_I32:
    fprintf(f, "i32");
    break;
  case TCG_TYPE_I64:
    fprintf(f, "i64");
    break;
  default:
    g_assert_not_reached();
    break;
  }
}

static inline void qce_debug_print_var(FILE *f, const QCEVar *var) {
  fprintf(f, "[");
  qce_debug_print_type(f, var->type);
  fprintf(f, "]");

  switch (var->kind) {
  case QCE_VAR_CONST:
    fprintf(f, "$0x%lx", var->v_const.val);
    break;
  case QCE_VAR_FIXED:
    fprintf(f, "%s(#%u)", var->v_fixed.name, var->v_fixed.reg);
    break;
  case QCE_VAR_GLOBAL_DIRECT:
    fprintf(f, "#%s(%u::0x%lx)", var->v_global_direct.name,
            var->v_global_direct.base, var->v_global_direct.offset);
    break;
  case QCE_VAR_GLOBAL_INDIRECT:
    fprintf(f, "#%s(%u::0x%lx::%lx)", var->v_global_indirect.name,
            var->v_global_indirect.base, var->v_global_indirect.offset1,
            var->v_global_indirect.offset2);
    break;
  case QCE_VAR_TB:
    fprintf(f, "%%v%lu", var->v_tb.index);
    break;
  case QCE_VAR_EBB:
    fprintf(f, "%%t%lu", var->v_ebb.index);
    break;
  default:
    g_assert_not_reached();
    break;
  }
}
#else
#define qce_var_debug_print(f, var)
#endif

static inline ptrdiff_t temp_index(const TCGContext *tcg, TCGTemp *t) {
  ptrdiff_t n = t - tcg->temps;
#ifdef QCE_DEBUG_IR
  g_assert(n >= 0 && n < tcg->nb_temps);
#endif
  return n;
}

static inline void copy_var_name(char *dst, const char *src) {
  strncpy(dst, src, QCE_VAR_NAME_MAX);
  if (dst[0] == '\0' || dst[QCE_VAR_NAME_MAX - 1] != '\0') {
    qce_fatal("Malformed name: %s", src);
  }
}

static inline void parse_var(TCGContext *tcg, TCGTemp *t, QCEVar *v) {
  // there should never be a variable in vector type
  switch (t->base_type) {
  case TCG_TYPE_I32:
  case TCG_TYPE_I64:
  case TCG_TYPE_I128:
    break;
  case TCG_TYPE_V64:
  case TCG_TYPE_V128:
  case TCG_TYPE_V256:
#ifndef QCE_SUPPORTS_VEC
    qce_debug_assert_ir1(tcg, false, t);
#endif
    break;
  default:
    g_assert_not_reached();
  }

  switch (t->kind) {
  case TEMP_CONST: {
    // expected when emulating x86_64 guest on x86_64 host
    qce_debug_assert_ir1(tcg, t->type == t->base_type, t);
    qce_debug_assert_ir1(tcg, t->temp_subindex == 0, t);

    v->kind = QCE_VAR_CONST;
    v->type = t->type;
    v->v_const.val = t->val;
    break;
  }
  case TEMP_FIXED: {
    // expected when emulating x86_64 guest on x86_64 host
    qce_debug_assert_ir1(tcg, t->type == t->base_type, t);
    qce_debug_assert_ir1(tcg, t->temp_subindex == 0, t);

    v->kind = QCE_VAR_FIXED;
    v->type = t->type;
    v->v_fixed.reg = t->reg;
    copy_var_name(v->v_fixed.name, t->name);
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
      v->type = t->type;
      v->v_global_direct.base = base->reg;
      v->v_global_direct.offset = t->mem_offset;
      copy_var_name(v->v_global_direct.name, t->name);
    } else {
      TCGTemp *offset = t->mem_base;
      TCGTemp *base = offset->mem_base;
      qce_debug_assert_ir3(
          tcg, offset->kind == TEMP_GLOBAL && base->kind == TEMP_FIXED, t,
          offset, base);

      v->kind = QCE_VAR_GLOBAL_INDIRECT;
      v->type = t->type;
      v->v_global_indirect.base = base->reg;
      v->v_global_indirect.offset1 = offset->mem_offset;
      v->v_global_indirect.offset2 = t->mem_offset;
      copy_var_name(v->v_global_indirect.name, t->name);
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
    v->type = t->type;
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
    v->type = t->type;
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
#ifdef QCE_DEBUG_IR
static inline void parse_arg_as_var_expect_type(TCGContext *tcg, TCGArg arg,
                                                QCEVar *v, TCGType ty) {
  parse_arg_as_var(tcg, arg, v);
  qce_debug_assert_ir1(tcg, v->type == ty, arg_temp(arg));
}
static inline void parse_arg_as_var_expect_host_addr(TCGContext *tcg,
                                                     TCGArg arg, QCEVar *v) {
  parse_arg_as_var_expect_type(tcg, arg, v, TCG_TYPE_I64);
}
#else
#define parse_arg_as_var_expect_type(tcg, arg, v, ty)                          \
  parse_var((tcg), arg_temp(arg), (v))
#define parse_arg_as_var_expect_host_addr(tcg, arg, v)                         \
  parse_var((tcg), arg_temp(arg), (v))
#endif

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
  QCE_INST_START,
  QCE_INST_EXIT_TB,
  QCE_INST_GOTO_TB,
  QCE_INST_GOTO_PTR,
#define QCE_INST_TEMPLATE_IN_KIND_ENUM
#include "qce-op.inc"
#undef QCE_INST_TEMPLATE_IN_KIND_ENUM
#define QCE_INST_TEMPLATE_IN_KIND_ENUM
#include "qce-call.inc"
#undef QCE_INST_TEMPLATE_IN_KIND_ENUM
  QCE_INST_UNKNOWN,
} QCEInstKind;

typedef struct {
  QCEInstKind kind;
  union {
    // opc: insn_start
    struct {
      vaddr pc;
    } i_start;

    // opc: exit_tb
    struct {
      uintptr_t idx;
    } i_exit_tb;

    // opc: goto_tb
    struct {
      uintptr_t idx;
    } i_goto_tb;

    // opc: goto_ptr
    struct {
      QCEVar ptr;
    } i_goto_ptr;

#define QCE_INST_TEMPLATE_IN_INST_UNION
#include "qce-op.inc"
#undef QCE_INST_TEMPLATE_IN_INST_UNION
#define QCE_INST_TEMPLATE_IN_INST_UNION
#include "qce-call.inc"
#undef QCE_INST_TEMPLATE_IN_INST_UNION
  };
} QCEInst;

#ifdef QCE_DEBUG_IR
static inline void qce_debug_print_inst(FILE *f, const QCEInst *inst) {
  switch (inst->kind) {
  case QCE_INST_START:
    fprintf(f, "---- 0x%lx ----", inst->i_start.pc);
    break;
  case QCE_INST_EXIT_TB:
    fprintf(f, "exit_tb: %ld", inst->i_exit_tb.idx);
    break;
  case QCE_INST_GOTO_TB:
    fprintf(f, "goto_tb: %ld", inst->i_goto_tb.idx);
    break;
  case QCE_INST_GOTO_PTR:
    fprintf(f, "goto_ptr: ");
    qce_debug_print_var(f, &inst->i_goto_ptr.ptr);
    break;
#define QCE_INST_TEMPLATE_IN_DEBUG_PRINT
#include "qce-op.inc"
#undef QCE_INST_TEMPLATE_IN_DEBUG_PRINT
#define QCE_INST_TEMPLATE_IN_DEBUG_PRINT
#include "qce-call.inc"
#undef QCE_INST_TEMPLATE_IN_DEBUG_PRINT
  case QCE_INST_UNKNOWN:
    fprintf(f, "[!!!] unknown instruction");
    break;
  }
  fprintf(f, "\n");
}
#else
#define qce_debug_print_inst(f, inst)
#endif

static inline void parse_op(TCGContext *tcg, TCGOp *op, QCEInst *inst) {
  TCGOpcode c = op->opc;
#ifndef QCE_RELEASE
  const TCGOpDef *def = &tcg_op_defs[c];
#endif

#ifndef QCE_SUPPORTS_VEC
  // there should never be an op with vector operands
  qce_debug_assert_op1(tcg, (def->flags & TCG_OPF_VECTOR) == 0, op);
#endif

  // special case: start marker
  if (c == INDEX_op_insn_start) {
    inst->kind = QCE_INST_START;
    inst->i_start.pc = op->args[0];
    // TODO: there is a second operand (at least on x86_64) of type CCOp
    // where CC stands for "condition code", it is currently ignored
    return;
  }

  // special case: call instruction
  if (c == INDEX_op_call) {
    const TCGHelperInfo *info = tcg_call_info(op);
#ifndef QCE_RELEASE
    void *func = tcg_call_func(op);
#endif
    qce_debug_assert_op1(tcg, func == info->func, op);

#ifndef QCE_RELEASE
    // variable number of arguments
    unsigned nb_oargs = TCGOP_CALLO(op);
    unsigned nb_iargs = TCGOP_CALLI(op);
#endif

    if (unlikely(false)) {
      g_assert_not_reached();
    }

#define QCE_INST_TEMPLATE_IN_PARSER
#include "qce-call.inc"
#undef QCE_INST_TEMPLATE_IN_PARSER

    else {
      qce_error("unhandled call: %s, oargs: %u, iargs: %u, type: %o, flag: %x",
                info->name, nb_oargs, nb_iargs, info->typemask, info->flags);
      inst->kind = QCE_INST_UNKNOWN;
    }

    // short-circuit
    return;
  }

  // all other instructions
  qce_debug_assert_op1(
      tcg, op->nargs >= def->nb_oargs + def->nb_iargs + def->nb_cargs, op);

  switch (c) {
  case INDEX_op_exit_tb: {
    uintptr_t addr = op->args[0];
    if (addr == 0) {
      addr = TB_EXIT_MASK + 1;
    } else {
      addr -= (uintptr_t)tcg_splitwx_to_rx((void *)tcg->gen_tb);
      qce_debug_assert_op1(tcg, addr <= TB_EXIT_MASK, op);
    }
    inst->kind = QCE_INST_EXIT_TB;
    inst->i_exit_tb.idx = addr;
    break;
  }

  case INDEX_op_goto_tb: {
    uintptr_t idx = op->args[0];
    qce_debug_assert_op1(tcg, idx <= TB_EXIT_IDXMAX, op);
    inst->kind = QCE_INST_GOTO_TB;
    inst->i_goto_tb.idx = idx;
    break;
  }

  case INDEX_op_goto_ptr: {
    inst->kind = QCE_INST_GOTO_PTR;
    parse_arg_as_var_expect_host_addr(tcg, op->args[0], &inst->i_goto_ptr.ptr);
    break;
  }

#define QCE_INST_TEMPLATE_IN_PARSER
#include "qce-op.inc"
#undef QCE_INST_TEMPLATE_IN_PARSER

    // unsupported: mul[su]h
  case INDEX_op_mulsh_i32:
  case INDEX_op_mulsh_i64:
  case INDEX_op_muluh_i32:
  case INDEX_op_muluh_i64:
    qce_fatal("[op] mul[su]h opcode not supported");
    break;

    // unsupported: setcond2
  case INDEX_op_setcond2_i32:
    qce_fatal("[op] setcond2 opcode not supported");
    break;

    // unsupported: qemu_st8_a[32|64]
  case INDEX_op_qemu_st8_a32_i32:
  case INDEX_op_qemu_st8_a64_i32:
    qce_fatal("[op] qemu_st8_a[32|64] opcode not supported");
    break;

    // unsupported: qemu_[ld|st]_a32_i*
  case INDEX_op_qemu_ld_a32_i128:
  case INDEX_op_qemu_ld_a32_i32:
  case INDEX_op_qemu_ld_a32_i64:
  case INDEX_op_qemu_st_a32_i32:
  case INDEX_op_qemu_st_a32_i64:
  case INDEX_op_qemu_st_a32_i128:
    qce_fatal("[op] qemu_[ld|st]_a32_i* opcode not supported");
    break;

    // unsupported: qemu_[ld|st]_a64_i128
  case INDEX_op_qemu_ld_a64_i128:
  case INDEX_op_qemu_st_a64_i128:
    qce_fatal("[op] qemu_[ld|st]_a64_i128 opcode not supported");
    break;

    // unsupported: plugin
  case INDEX_op_plugin_cb:
  case INDEX_op_plugin_mem_cb:
    qce_fatal("[op] plugin opcode not supported");
    break;

    // unreachable
  case INDEX_op_last_generic:
    g_assert_not_reached();
    break;
  default: {
    qce_fatal("[op] unhandled opcode: %s", def->name);
    break;
  }
  }
}
