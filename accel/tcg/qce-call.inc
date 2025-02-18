/*
 * Template Definitions
 */

#ifdef QCE_INST_TEMPLATE_IN_KIND_ENUM
#define X_HEAD(func, args_i, args_o) QCE_INST_CALL_##func,
#define X_ARG_OUT(func, idx, name, type)
#define X_ARG_IN(func, idx, name, type)
#define X_TAIL(func)
#endif

#ifdef QCE_INST_TEMPLATE_IN_INST_UNION
#define X_HEAD(func, args_i, args_o) struct {
#define X_ARG_OUT(func, idx, name, type) QCEVar name;
#define X_ARG_IN(func, idx, name, type) QCEVar name;
#define X_TAIL(func)                                                           \
  }                                                                            \
  i_call_##func;
#endif

#ifdef QCE_INST_TEMPLATE_IN_PARSER
#define X_HEAD(func, args_i, args_o)                                           \
  else if (strcmp(info->name, #func) == 0) {                                   \
    qce_debug_assert_op1(tcg, nb_oargs == args_i, op);                         \
    qce_debug_assert_op1(tcg, nb_iargs == args_o, op);                         \
    inst->kind = QCE_INST_CALL_##func;
#define X_ARG_OUT(func, idx, name, type)                                       \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_call_##func, name)),    \
                               TCG_TYPE_##type);
#define X_ARG_IN(func, idx, name, type)                                        \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_call_##func, name)),    \
                               TCG_TYPE_##type);
#define X_TAIL(func) }
#endif

#ifdef QCE_INST_TEMPLATE_IN_DEBUG_PRINT
#define X_HEAD(func, args_i, args_o)                                           \
  case QCE_INST_CALL_##func: {                                                 \
    fprintf(f, #func " (\n");
#define X_ARG_OUT(func, idx, name, type)                                       \
  fprintf(f, "\t" #name ": ");                                                 \
  qce_debug_print_var(f, &(UTIL_DOT_JOIN(inst->i_call_##func, name)));         \
  fprintf(f, ",\n");
#define X_ARG_IN(func, idx, name, type)                                        \
  fprintf(f, "\t" #name ": ");                                                 \
  qce_debug_print_var(f, &(UTIL_DOT_JOIN(inst->i_call_##func, name)));         \
  fprintf(f, ",\n");
#define X_TAIL(func)                                                           \
  fprintf(f, ")");                                                             \
  break;                                                                       \
  }
#endif

/*
 * Templates Structures
 */

#define X_INST_O0I1(func, i1_name, i1_type)                                    \
  X_HEAD(func, 0, 1)                                                           \
  X_ARG_IN(func, 0, i1_name, i1_type)                                          \
  X_TAIL(func)

#define X_INST_O0I2(func, i1_name, i1_type, i2_name, i2_type)                  \
  X_HEAD(func, 0, 2)                                                           \
  X_ARG_IN(func, 0, i1_name, i1_type)                                          \
  X_ARG_IN(func, 1, i2_name, i2_type)                                          \
  X_TAIL(func)

#define X_INST_O0I3(func, i1_name, i1_type, i2_name, i2_type, i3_name,         \
                    i3_type)                                                   \
  X_HEAD(func, 0, 3)                                                           \
  X_ARG_IN(func, 0, i1_name, i1_type)                                          \
  X_ARG_IN(func, 1, i2_name, i2_type)                                          \
  X_ARG_IN(func, 2, i3_name, i3_type)                                          \
  X_TAIL(func)

#define X_INST_O0I4(func, i1_name, i1_type, i2_name, i2_type, i3_name,         \
                    i3_type, i4_name, i4_type)                                 \
  X_HEAD(func, 0, 4)                                                           \
  X_ARG_IN(func, 0, i1_name, i1_type)                                          \
  X_ARG_IN(func, 1, i2_name, i2_type)                                          \
  X_ARG_IN(func, 2, i3_name, i3_type)                                          \
  X_ARG_IN(func, 3, i4_name, i4_type)                                          \
  X_TAIL(func)

#define X_INST_O0I5(func, i1_name, i1_type, i2_name, i2_type, i3_name,         \
                    i3_type, i4_name, i4_type, i5_name, i5_type)               \
  X_HEAD(func, 0, 5)                                                           \
  X_ARG_IN(func, 0, i1_name, i1_type)                                          \
  X_ARG_IN(func, 1, i2_name, i2_type)                                          \
  X_ARG_IN(func, 2, i3_name, i3_type)                                          \
  X_ARG_IN(func, 3, i4_name, i4_type)                                          \
  X_ARG_IN(func, 4, i5_name, i5_type)                                          \
  X_TAIL(func)

#define X_INST_O1I1(func, o1_name, o1_type, i1_name, i1_type)                  \
  X_HEAD(func, 1, 1)                                                           \
  X_ARG_OUT(func, 0, o1_name, o1_type)                                         \
  X_ARG_IN(func, 1, i1_name, i1_type)                                          \
  X_TAIL(func)

#define X_INST_O1I2(func, o1_name, o1_type, i1_name, i1_type, i2_name,         \
                    i2_type)                                                   \
  X_HEAD(func, 1, 2)                                                           \
  X_ARG_OUT(func, 0, o1_name, o1_type)                                         \
  X_ARG_IN(func, 1, i1_name, i1_type)                                          \
  X_ARG_IN(func, 2, i2_name, i2_type)                                          \
  X_TAIL(func)

#define X_INST_O1I3(func, o1_name, o1_type, i1_name, i1_type, i2_name,         \
                    i2_type, i3_name, i3_type)                                 \
  X_HEAD(func, 1, 3)                                                           \
  X_ARG_OUT(func, 0, o1_name, o1_type)                                         \
  X_ARG_IN(func, 1, i1_name, i1_type)                                          \
  X_ARG_IN(func, 2, i2_name, i2_type)                                          \
  X_ARG_IN(func, 3, i3_name, i3_type)                                          \
  X_TAIL(func)

#define X_INST_O1I4(func, o1_name, o1_type, i1_name, i1_type, i2_name,         \
                    i2_type, i3_name, i3_type, i4_name, i4_type)               \
  X_HEAD(func, 1, 4)                                                           \
  X_ARG_OUT(func, 0, o1_name, o1_type)                                         \
  X_ARG_IN(func, 1, i1_name, i1_type)                                          \
  X_ARG_IN(func, 2, i2_name, i2_type)                                          \
  X_ARG_IN(func, 3, i3_name, i3_type)                                          \
  X_ARG_IN(func, 4, i4_name, i4_type)                                          \
  X_TAIL(func)

#define X_INST_O2I3(func, o1_name, o1_type, o2_name, o2_type, i1_name,         \
                    i1_type, i2_name, i2_type, i3_name, i3_type)               \
  X_HEAD(func, 2, 3)                                                           \
  X_ARG_OUT(func, 0, o1_name, o1_type)                                         \
  X_ARG_OUT(func, 1, o2_name, o2_type)                                         \
  X_ARG_IN(func, 2, i1_name, i1_type)                                          \
  X_ARG_IN(func, 3, i2_name, i2_type)                                          \
  X_ARG_IN(func, 4, i3_name, i3_type)                                          \
  X_TAIL(func)

/*
 * Template Instantiations
 */

X_INST_O1I1(lookup_tb_ptr, res, I64, env, I64)
X_INST_O0I1(rechecking_single_step, env, I64)
X_INST_O0I1(sgx, env, I64)

X_INST_O1I2(inb, res, I64, env, I64, port, I32)
X_INST_O1I2(inw, res, I64, env, I64, port, I32)
X_INST_O1I2(inl, res, I64, env, I64, port, I32)
X_INST_O0I3(outb, env, I64, port, I32, val, I32)
X_INST_O0I3(outw, env, I64, port, I32, val, I32)
X_INST_O0I3(outl, env, I64, port, I32, val, I32)

X_INST_O0I1(cpuid, env, I64)
X_INST_O0I1(rdmsr, env, I64)
X_INST_O0I1(wrmsr, env, I64)
X_INST_O0I1(rdtsc, env, I64)
X_INST_O1I2(read_crN, res, I64, env, I64, n, I32)
X_INST_O0I3(write_crN, env, I64, n, I32, val, I64)
X_INST_O1I1(read_eflags, res, I64, env, I64)
X_INST_O0I3(write_eflags, env, I64, val, I64, mask, I32)
X_INST_O0I3(set_dr, env, I64, reg, I32, val, I64)

X_INST_O0I1(das, env, I64)
X_INST_O0I1(emms, env, I64)
X_INST_O0I1(rsm, env, I64)

X_INST_O1I4(cc_compute_all, res, I64, dst, I64, src1, I64, src2, I64, opc, I32)
X_INST_O1I4(cc_compute_c, res, I64, dst, I64, src1, I64, src2, I64, opc, I32)
X_INST_O1I3(cc_compute_nz, res, I64, dst, I64, src, I64, opc, I32)

X_INST_O0I2(divl_EAX, env, I64, val, I64)
X_INST_O0I2(divq_EAX, env, I64, val, I64)
X_INST_O0I2(idivl_EAX, env, I64, val, I64)
X_INST_O0I2(idivq_EAX, env, I64, val, I64)

X_INST_O2I3(ld_i128, out_t, I64, out_b, I64, env, I64, base, I64, offset, I32)
X_INST_O0I5(st_i128, env, I64, base, I64, in_t, I64, in_b, I64, offset, I32)

X_INST_O0I3(pshufd_xmm, d, I64, s, I64, encodig, I32)
X_INST_O0I4(pslldq_xmm, env, I64, d, I64, s, I64, c, I64)
X_INST_O0I4(psrldq_xmm, env, I64, d, I64, s, I64, c, I64)
X_INST_O0I4(punpckhdq_xmm, env, I64, d, I64, v, I64, s, I64)
X_INST_O0I4(punpckldq_xmm, env, I64, d, I64, v, I64, s, I64)
X_INST_O0I4(punpcklqdq_xmm, env, I64, d, I64, v, I64, s, I64)
X_INST_O0I4(punpcklbw_xmm, env, I64, d, I64, v, I64, s, I64)
X_INST_O0I4(punpcklwd_xmm, env, I64, d, I64, v, I64, s, I64)
X_INST_O0I4(shufpd_xmm, d, I64, v, I64, s, I64, order, I32)

X_INST_O0I1(fninit, env, I64)
X_INST_O1I1(fnstcw, res, I32, env, I64)
X_INST_O1I1(fnstsw, res, I32, env, I64)

X_INST_O0I1(fwait, env, I64)
X_INST_O0I1(fclex, env, I64)

X_INST_O0I2(fxsave, env, I64, val, I64)
X_INST_O0I2(fxrstor, env, I64, val, I64)

X_INST_O0I4(gvec_eq8, d, I64, a, I64, b, I64, desc, I32)
X_INST_O0I4(gvec_lt8, d, I64, a, I64, b, I64, desc, I32)
X_INST_O0I4(gvec_umin8, d, I64, a, I64, b, I64, desc, I32)

X_INST_O0I2(fildl_ST0, env, I64, val, I32)

X_INST_O0I2(lldt, env, I64, selector, I32)
X_INST_O0I2(ltr, env, I64, selector, I32)
X_INST_O0I3(load_seg, env, I64, segment, I32, selector, I32)

X_INST_O0I2(flush_page, env, I64, addr, I64)
X_INST_O0I1(hlt, env, I64)

X_INST_O0I2(iret_real, env, I64, shift, I32)
X_INST_O0I3(iret_protected, env, I64, shift, I32, next_eip, I32)
X_INST_O0I5(lcall_real, env, I64, new_cs, I32, new_eip, I32, shift, I32,
            next_eip, I32)
X_INST_O0I4(ljmp_protected, env, I64, new_cs, I32, new_eip, I64, next_eip, I64)
X_INST_O0I3(lret_protected, env, I64, shift, I32, next_eip, I32)
X_INST_O0I3(raise_interrupt, env, I64, intno, I32, next_eip, I32)
X_INST_O0I1(pause, env, I64)

X_INST_O0I2(syscall, env, I64, next_eip, I32)
X_INST_O0I2(sysret, env, I64, dflag, I32)

// X_INST_O1I2(get_dr, res, I64, env, I64, reg, I32)
// X_INST_O0I2(raise_exception, env, I64, exception_index, I32)

/*
 * Template Teardown
 */

#undef X_HEAD
#undef X_ARG_OUT
#undef X_ARG_IN
#undef X_TAIL

#undef X_INST_O0I1
#undef X_INST_O0I2
#undef X_INST_O0I3
#undef X_INST_O0I4
#undef X_INST_O0I5
#undef X_INST_O1I1
#undef X_INST_O1I2
#undef X_INST_O1I3
#undef X_INST_O1I4
#undef X_INST_O2I3
