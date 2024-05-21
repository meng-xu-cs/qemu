/*
 * Template Definitions
 */

#ifdef QCE_INST_TEMPLATE_IN_KIND_ENUM
#define X_HEAD(func, args_i, args_o) QCE_INST_CALL_##func,
#define X_ARG_VAR(func, idx, name)
#define X_ARG_OUT(func, idx, name, type)
#define X_ARG_IN(func, idx, name, type)
#define X_ARG_ADDR(func, idx, name)
#define X_ARG_CONST(func, idx, name)
#define X_ARG_LABEL(func, idx, name)
#define X_TAIL(func)
#endif

#ifdef QCE_INST_TEMPLATE_IN_INST_UNION
#define X_HEAD(func, args_i, args_o) struct {
#define X_ARG_VAR(func, idx, name) QCEVar name;
#define X_ARG_OUT(func, idx, name, type) QCEVar name;
#define X_ARG_IN(func, idx, name, type) QCEVar name;
#define X_ARG_ADDR(func, idx, name) QCEVar name;
#define X_ARG_CONST(func, idx, name) tcg_target_ulong name;
#define X_ARG_LABEL(func, idx, name) QCELabel name;
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
#define X_ARG_VAR(func, idx, name)                                             \
  parse_arg_as_var(tcg, op->args[idx],                                         \
                   &(UTIL_DOT_JOIN(inst->i_call_##func, name)));
#define X_ARG_OUT(func, idx, name, type)                                       \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_call_##func, name)),    \
                               TCG_TYPE_##type);
#define X_ARG_IN(func, idx, name, type)                                        \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_call_##func, name)),    \
                               TCG_TYPE_##type);
#define X_ARG_ADDR(func, idx, name)                                            \
  parse_arg_as_var_expect_host_addr(                                           \
      tcg, op->args[idx], &(UTIL_DOT_JOIN(inst->i_call_##func, name)));
#define X_ARG_CONST(func, idx, name)                                           \
  UTIL_DOT_JOIN(inst->i_call_##func, name) = op->args[idx];
#define X_ARG_LABEL(func, idx, name)                                           \
  parse_arg_as_label(tcg, op->args[idx],                                       \
                     &(UTIL_DOT_JOIN(inst->i_call_##func, name)));
#define X_TAIL(func) }
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

X_INST_O1I4(cc_compute_all, res, I64, dst, I64, src1, I64, src2, I64, opc, I32)
X_INST_O1I4(cc_compute_c, res, I64, dst, I64, src1, I64, src2, I64, opc, I32)

X_INST_O0I2(divl_EAX, env, I64, val, I64)
X_INST_O0I2(divq_EAX, env, I64, val, I64)
X_INST_O0I2(idivl_EAX, env, I64, val, I64)
X_INST_O0I2(idivq_EAX, env, I64, val, I64)

X_INST_O2I3(ld_i128, out_t, I64, out_b, I64, env, I64, base, I64, offset, I32)
X_INST_O0I5(st_i128, env, I64, base, I64, in_t, I64, in_b, I64, offset, I32)

/*
 * Template Teardown
 */

#undef X_HEAD
#undef X_ARG_VAR
#undef X_ARG_OUT
#undef X_ARG_IN
#undef X_ARG_ADDR
#undef X_ARG_CONST
#undef X_ARG_LABEL
#undef X_TAIL

#undef X_INST_O0I1
#undef X_INST_O0I2
#undef X_INST_O0I3
#undef X_INST_O0I5
#undef X_INST_O1I1
#undef X_INST_O1I2
#undef X_INST_O1I3
#undef X_INST_O1I4
#undef X_INST_O2I3
