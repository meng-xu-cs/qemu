#ifndef QCE_SYM_GUEST_LD_ST_H
#define QCE_SYM_GUEST_LD_ST_H

typedef enum {
  MMIO_CAN_ACCESS,
  MMIO_CANNOT_ACCESS,
  VALID,
  INVALID,
} QCEAddressFlag;

static inline QCEAddressFlag __check_addr_validity(
    CPUArchState *env, QCEState *state, QCEExpr *addr, unsigned mmu_idx,
    MMUAccessType access_type) {
  // concretize the symbolic address which is not in blob
  if (addr->mode == QCE_EXPR_SYMBOLIC) {
    QCESession *session = g_qce->session;
    uint64_t concretized_addr =
        qce_smt_z3_concretize_bv64(&state->solver_z3, session->blob_addr,
                                   session->blob_size, session->blob_content,
                                   addr->symbolic);
    if (concretized_addr >= session->blob_addr &&
        concretized_addr < session->blob_addr + session->blob_size) {
      return VALID;
    } else {
      addr->mode = QCE_EXPR_CONCRETE;
      addr->v_i64 = concretized_addr;
    }
  }
#ifndef QCE_RELEASE
  assert(addr->mode == QCE_EXPR_CONCRETE);
#endif

  /*
   * If an MMIO address is accessed when can_do_io is not set to true,
   * QEMU will rewind and recompile the current TB. We need to stop
   * and exit the TB.
   */
  void *host;
  int flag = probe_access_flags(env, (vaddr)addr->v_i64, 0, access_type,
                                mmu_idx, true, &host, 0);
  if (flag == TLB_INVALID_MASK) return INVALID;
  if (flag == TLB_MMIO) {
    QCEExpr expr_can_do_io;
    qce_state_env_get_i32(state, (intptr_t)&env_cpu(env)->neg.can_do_io,
                          &expr_can_do_io);
#ifndef QCE_RELEASE
    qce_expr_assert_mode(&expr_can_do_io, CONCRETE);
#endif
    if (!(int16_t)expr_can_do_io.v_i32) return MMIO_CANNOT_ACCESS;
    else return MMIO_CAN_ACCESS;
  }
  return VALID;
}

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
    align = access_bytes;
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
      qce_fatal("unaligned guest memory access is not supported");
    }
  } else {
//    Z3_ast offset = qce_smt_z3_bv64_sub(&state->solver_z3, addr->symbolic,
//                                        state->solver_z3.blob_addr);
//
//    // check offset is within range
//    qce_smt_z3_prove(
//        &state->solver_z3,
//        qce_smt_z3_bv64_uge(&state->solver_z3, offset,
//                            qce_smt_z3_bv64_value(&state->solver_z3, 0)));
//    qce_smt_z3_prove(
//        &state->solver_z3,
//        qce_smt_z3_bv64_ult(
//            &state->solver_z3, offset,
//            qce_smt_z3_bv64_value(&state->solver_z3, BLOB_SIZE_MAX)));
//
//    // check offset is aligned (if needed)
//    if (align != 1) {
//      Z3_ast alignment =
//          qce_smt_z3_bv64_umod(&state->solver_z3, offset,
//                               qce_smt_z3_bv64_value(&state->solver_z3, align));
//      qce_smt_z3_prove(
//          &state->solver_z3,
//          qce_smt_z3_bv64_eq(&state->solver_z3, alignment,
//                             qce_smt_z3_bv64_value(&state->solver_z3, 0)));
//    }
  }

  // expect atomicity is not requested
  if ((mo & MO_ATOM_MASK) != MO_ATOM_NONE) {
    qce_fatal("atomic guest memory access is not supported");
  }
}
#else
#define __check_memop_validity(mo, addr, access_bytes)
#endif

static inline void __prepare_expr_for_ld_memop_i32(QCEState *state, MemOp mo,
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

static inline void __prepare_expr_for_ld_memop_i64(QCEState *state, MemOp mo,
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

static inline void __prepare_expr_for_st_memop_i32(QCEState *state, MemOp mo,
                                  QCEExpr *src, QCEExpr *dst, QCEExpr *res) {
  switch (mo & MO_SIZE) {
  case MO_8: {
    qce_expr_st8_i32(&state->solver_z3, src, dst, res);
    break;
  }
  case MO_16: {
    qce_expr_st16_i32(&state->solver_z3, src, dst, res);
    break;
  }
  case MO_32: {
    memcpy(res, src, sizeof(QCEExpr));
    break;
  }
  case MO_64: {
    qce_fatal("64-bit operation observed on a 32-bit guest memory access");
  }
  default:
    __qce_unreachable__
  }
}

static inline void __prepare_expr_for_st_memop_i64(QCEState *state, MemOp mo,
                                  QCEExpr *src, QCEExpr *dst, QCEExpr *res) {
  switch (mo & MO_SIZE) {
  case MO_8: {
    qce_expr_st8_i64(&state->solver_z3, src, dst, res);
    break;
  }
  case MO_16: {
    qce_expr_st16_i64(&state->solver_z3, src, dst, res);
    break;
  }
  case MO_32: {
    qce_expr_st32_i64(&state->solver_z3, src, dst, res);
    break;
  }
  case MO_64: {
    memcpy(res, src, sizeof(QCEExpr));
    break;
  }
  default:
    __qce_unreachable__
  }
}

#define DEFINE_SYM_INST_qemu_ld(bits)                                          \
  static inline QCEAddressFlag qce_sym_inst_guest_ld_i##bits(                  \
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
    /* check the address */                                                    \
    QCEAddressFlag addr_flag =                                                 \
        __check_addr_validity(env, state, &expr_addr, mmu_idx, 0);             \
    if (addr_flag == MMIO_CANNOT_ACCESS || addr_flag == INVALID) {             \
      return addr_flag;                                                        \
    }                                                                          \
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
    __prepare_expr_for_ld_memop_i##bits(state, mo, &expr_cell, &expr_val);     \
                                                                               \
    /* put back the result */                                                  \
    qce_state_put_var(env, state, res, &expr_val);                             \
                                                                               \
    return addr_flag;                                                          \
  }

DEFINE_SYM_INST_qemu_ld(32);
DEFINE_SYM_INST_qemu_ld(64);

#define DEFINE_SYM_INST_qemu_st(bits)                                          \
  static inline QCEAddressFlag qce_sym_inst_guest_st_i##bits(                  \
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
    /* check the address */                                                    \
    QCEAddressFlag addr_flag =                                                 \
        __check_addr_validity(env, state, &expr_addr, mmu_idx, 1);             \
    if (addr_flag == MMIO_CANNOT_ACCESS || addr_flag == INVALID) {             \
      return addr_flag;                                                        \
    }                                                                          \
                                                                               \
    /* check the access */                                                     \
    __check_memop_validity(state, mo, &expr_addr, bits / 8);                   \
                                                                               \
    /* load the value */                                                       \
    QCEExpr expr_val;                                                          \
    qce_state_get_var(env, state, val, &expr_val);                             \
                                                                               \
    /* load and update the original value first and then store the value */    \
    QCEExpr expr_cell, expr_cell_updated;                                      \
    switch (expr_addr.mode) {                                                  \
    case QCE_EXPR_CONCRETE: {                                                  \
      qce_state_mem_get_i##bits(env, state, expr_addr.v_i64, mmu_idx,          \
                                &expr_cell);                                   \
      __prepare_expr_for_st_memop_i##bits(state, mo, &expr_val, &expr_cell,    \
                                          &expr_cell_updated);                 \
      qce_state_mem_put_i##bits(env, state, expr_addr.v_i64, mmu_idx,          \
                                &expr_cell_updated);                           \
      break;                                                                   \
    }                                                                          \
    case QCE_EXPR_SYMBOLIC: {                                                  \
      qce_fatal("Waiting for a case [st] on symbolic address to support");     \
    }                                                                          \
    }                                                                          \
                                                                               \
    return addr_flag;                                                          \
  }

DEFINE_SYM_INST_qemu_st(32);
DEFINE_SYM_INST_qemu_st(64);

#define HANDLE_SYM_INST_qemu_ld(bits)                                          \
  case QCE_INST_GUEST_LD##bits: {                                              \
    QCEAddressFlag addr_flag = qce_sym_inst_guest_ld_i##bits(                  \
        arch, &session->state, &inst->i_qemu_ld_i##bits.addr,                  \
        inst->i_qemu_ld_i##bits.flag, &inst->i_qemu_ld_i##bits.res);           \
    if (addr_flag == MMIO_CANNOT_ACCESS || addr_flag == INVALID) {             \
      session->emulation_ctx.status = QCE_Emulation_Normal;                    \
      qce_state_reset(&session->state);                                        \
      goto end_of_loop;                                                        \
    }                                                                          \
    break;                                                                     \
  }

#define HANDLE_SYM_INST_qemu_st(bits)                                          \
  case QCE_INST_GUEST_ST##bits: {                                              \
    QCEAddressFlag addr_flag = qce_sym_inst_guest_st_i##bits(                  \
        arch, &session->state, &inst->i_qemu_st_i##bits.val,                   \
        &inst->i_qemu_st_i##bits.addr, inst->i_qemu_st_i##bits.flag);          \
    if (addr_flag == MMIO_CANNOT_ACCESS || addr_flag == INVALID) {             \
      session->emulation_ctx.status = QCE_Emulation_Normal;                    \
      qce_state_reset(&session->state);                                        \
      goto end_of_loop;                                                        \
    } else if (addr_flag == MMIO_CAN_ACCESS) {                                 \
      goto suspend_emulation;                                                  \
    }                                                                          \
    break;                                                                     \
  }

#endif /* QCE_SYM_GUEST_LD_ST_H */
