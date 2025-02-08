#ifndef QCE_SYM_GUEST_LD_ST_H
#define QCE_SYM_GUEST_LD_ST_H

#ifndef QCE_RELEASE
static inline void __check_memop_validity(QCEState *state, MemOp mo,
                                          QCEExpr *addr,
                                          uint64_t access_bytes) {
  // expect a little-endian guest
  g_assert((mo & MO_BSWAP) == MO_LE);

  // derive alignment value
  uint64_t align = 0;
  switch (mo & MO_AMASK) {
  case MO_UNALN: {
    align = 1;
    break;
  }
  case MO_ALIGN: {
    align = memop_size(mo);
    // qce_debug("align=%lu", align);
    // align = access_bytes;
    break;
  }
  case MO_ALIGN_4: {
    align = 4;
    break;
  }
  case MO_ALIGN_8: {
    align = 8;
    break;
  }
  default:
    qce_fatal("unexpected alignment for guest memory access");
  }

  // check on alignment
  if (addr->mode == QCE_EXPR_CONCRETE) {
    if (addr->v_i64 % align != 0) {
      qce_debug("addr=%p, align=%lu", (void *)addr->v_i64, align);
      qce_fatal("unaligned guest memory access is not supported");
    }
  } else {
    Z3_ast offset = qce_smt_z3_bv64_sub(&state->solver_z3, addr->symbolic,
                                        state->solver_z3.blob_addr);

    // check offset is within range
    qce_smt_z3_prove(
        &state->solver_z3,
        qce_smt_z3_bv64_uge(&state->solver_z3, offset,
                            qce_smt_z3_bv64_value(&state->solver_z3, 0)));
    qce_smt_z3_prove(
        &state->solver_z3,
        qce_smt_z3_bv64_ult(
            &state->solver_z3, offset,
            qce_smt_z3_bv64_value(&state->solver_z3, BLOB_SIZE_MAX)));

    // check offset is aligned (if needed)
    if (align != 1) {
      Z3_ast alignment =
          qce_smt_z3_bv64_umod(&state->solver_z3, offset,
                               qce_smt_z3_bv64_value(&state->solver_z3, align));
      qce_smt_z3_prove(
          &state->solver_z3,
          qce_smt_z3_bv64_eq(&state->solver_z3, alignment,
                             qce_smt_z3_bv64_value(&state->solver_z3, 0)));
    }
  }

  // expect atomicity is not requested
  if ((mo & MO_ATOM_MASK) != MO_ATOM_NONE) {
    qce_fatal("atomic guest memory access is not supported");
  }
}
#else
#define __check_memop_validity(mo, addr, access_bytes)
#endif

static inline void __prepare_expr_for_memop_i32(QCEState *state, MemOp mo,
                                                QCEExpr *val, QCEExpr *res) {
  if (mo & MO_SIGN) {
    switch (mo & MO_SIZE) {
    case MO_8: {
      qce_expr_ld8s_i32(&state->solver_z3, val, res);
      break;
    }
    case MO_16: {
      qce_expr_ld16s_i32(&state->solver_z3, val, res);
      break;
    }
    case MO_32: {
      memcpy(res, val, sizeof(QCEExpr));
      break;
    }
    case MO_64: {
      qce_fatal("64-bit operation observed on a 32-bit guest memory access");
    }
    default:
      __qce_unreachable__
    }
  } else {
    switch (mo & MO_SIZE) {
    case MO_8: {
      qce_expr_ld8u_i32(&state->solver_z3, val, res);
      break;
    }
    case MO_16: {
      qce_expr_ld16u_i32(&state->solver_z3, val, res);
      break;
    }
    case MO_32: {
      memcpy(res, val, sizeof(QCEExpr));
      break;
    }
    case MO_64: {
      qce_fatal("64-bit operation observed on a 32-bit guest memory access");
    }
    default:
      __qce_unreachable__
    }
  }
}

static inline void __prepare_expr_for_memop_i64(QCEState *state, MemOp mo,
                                                QCEExpr *val, QCEExpr *res) {
  if (mo & MO_SIGN) {
    switch (mo & MO_SIZE) {
    case MO_8: {
      qce_expr_ld8s_i64(&state->solver_z3, val, res);
      break;
    }
    case MO_16: {
      qce_expr_ld16s_i64(&state->solver_z3, val, res);
      break;
    }
    case MO_32: {
      qce_expr_ld32s_i64(&state->solver_z3, val, res);
      break;
    }
    case MO_64: {
      memcpy(res, val, sizeof(QCEExpr));
      break;
    }
    default:
      __qce_unreachable__
    }
  } else {
    switch (mo & MO_SIZE) {
    case MO_8: {
      qce_expr_ld8u_i64(&state->solver_z3, val, res);
      break;
    }
    case MO_16: {
      qce_expr_ld16u_i64(&state->solver_z3, val, res);
      break;
    }
    case MO_32: {
      qce_expr_ld32u_i64(&state->solver_z3, val, res);
      break;
    }
    case MO_64: {
      memcpy(res, val, sizeof(QCEExpr));
      break;
    }
    default:
      __qce_unreachable__
    }
  }
}

#define DEFINE_SYM_INST_qemu_ld(bits)                                          \
  static inline void qce_sym_inst_guest_ld_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *addr, MemOpIdx flag,         \
      QCEVar *res) {                                                           \
    /* check the flags */                                                      \
    MemOp mo = get_memop(flag);                                                \
    unsigned mmu_idx = get_mmuidx(flag);                                       \
                                                                               \
    /* drive the address */                                                    \
    QCEExpr expr_addr;                                                         \
    qce_state_get_var(env, state, addr, &expr_addr);                           \
    qce_debug_assert(expr_addr.type == QCE_EXPR_I64);                          \
                                                                               \
    /* check the access */                                                     \
    __check_memop_validity(state, mo, &expr_addr, bits / 8);                   \
                                                                               \
    /* load the value */                                                       \
    QCEExpr expr_cell;                                                         \
    switch (expr_addr.mode) {                                                  \
    case QCE_EXPR_CONCRETE: {                                                  \
      qce_state_mem_get_i##bits(env, state, expr_addr.v_i64, mmu_idx,          \
                                &expr_cell);                                   \
      break;                                                                   \
    }                                                                          \
    case QCE_EXPR_SYMBOLIC: {                                                  \
      qce_state_mem_get_symbolic_i##bits(state, expr_addr.symbolic,            \
                                         &expr_cell);                          \
      break;                                                                   \
    }                                                                          \
    }                                                                          \
                                                                               \
    /* handle the flags */                                                     \
    QCEExpr expr_val;                                                          \
    __prepare_expr_for_memop_i##bits(state, mo, &expr_cell, &expr_val);        \
                                                                               \
    /* put back the result */                                                  \
    qce_state_put_var(env, state, res, &expr_val);                             \
  }

DEFINE_SYM_INST_qemu_ld(32);
DEFINE_SYM_INST_qemu_ld(64);

#define DEFINE_SYM_INST_qemu_st(bits)                                          \
  static inline void qce_sym_inst_guest_st_i##bits(                            \
      CPUArchState *env, QCEState *state, QCEVar *val, QCEVar *addr,           \
      MemOpIdx flag) {                                                         \
    /* check the flags */                                                      \
    MemOp mo = get_memop(flag);                                                \
    unsigned mmu_idx = get_mmuidx(flag);                                       \
                                                                               \
    /* drive the address */                                                    \
    QCEExpr expr_addr;                                                         \
    qce_state_get_var(env, state, addr, &expr_addr);                           \
    qce_debug_assert(expr_addr.type == QCE_EXPR_I64);                          \
                                                                               \
    /* check the access */                                                     \
    __check_memop_validity(state, mo, &expr_addr, bits / 8);                   \
                                                                               \
    /* load the value */                                                       \
    QCEExpr expr_val;                                                          \
    qce_state_get_var(env, state, val, &expr_val);                             \
                                                                               \
    /* handle the flags */                                                     \
    QCEExpr expr_cell;                                                         \
    __prepare_expr_for_memop_i##bits(state, mo, &expr_val, &expr_cell);        \
                                                                               \
    /* store the value */                                                      \
    switch (expr_addr.mode) {                                                  \
    case QCE_EXPR_CONCRETE: {                                                  \
      qce_state_mem_put_i##bits(state, expr_addr.v_i64, mmu_idx, &expr_cell);  \
      break;                                                                   \
    }                                                                          \
    case QCE_EXPR_SYMBOLIC: {                                                  \
      qce_fatal("Waiting for a case [st] on symbolic address to support");     \
    }                                                                          \
    }                                                                          \
  }

DEFINE_SYM_INST_qemu_st(32);
DEFINE_SYM_INST_qemu_st(64);

#define HANDLE_SYM_INST_qemu_ld(bits)                                          \
  case QCE_INST_GUEST_LD##bits: {                                              \
    qce_sym_inst_guest_ld_i##bits(                                             \
        arch, &session->state, &inst->i_qemu_ld_a64_i##bits.addr,              \
        inst->i_qemu_ld_a64_i##bits.flag, &inst->i_qemu_ld_a64_i##bits.res);   \
    break;                                                                     \
  }

#define HANDLE_SYM_INST_qemu_st(bits)                                          \
  case QCE_INST_GUEST_ST##bits: {                                              \
    qce_sym_inst_guest_st_i##bits(                                             \
        arch, &session->state, &inst->i_qemu_st_a64_i##bits.val,               \
        &inst->i_qemu_st_a64_i##bits.addr, inst->i_qemu_st_a64_i##bits.flag);  \
    break;                                                                     \
  }

#endif /* QCE_SYM_GUEST_LD_ST_H */
