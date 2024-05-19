/*
 * Template Definitions
 */

#ifdef QCE_INST_TEMPLATE_IN_KIND_ENUM
#define X_HEAD(qce_name, tcg_name) QCE_INST_##qce_name,
#define X_ARG_OUT(qce_name, tcg_name, idx, name, type)
#define X_ARG_IN(qce_name, tcg_name, idx, name, type)
#define X_ARG_CONST(qce_name, tcg_name, idx, name)
#define X_TAIL(qce_name, tcg_name)
#endif

#ifdef QCE_INST_TEMPLATE_IN_INST_UNION
#define X_HEAD(qce_name, tcg_name) struct {
#define X_ARG_OUT(qce_name, tcg_name, idx, name, type) QCEVar name;
#define X_ARG_IN(qce_name, tcg_name, idx, name, type) QCEVar name;
#define X_ARG_CONST(qce_name, tcg_name, idx, name) tcg_target_ulong name;
#define X_TAIL(qce_name, tcg_name)                                             \
  }                                                                            \
  i_##tcg_name;
#endif

#ifdef QCE_INST_TEMPLATE_IN_PARSER
#define X_HEAD(qce_name, tcg_name)                                             \
  case INDEX_op_##tcg_name: {                                                  \
    inst->kind = QCE_INST_##qce_name;
#define X_ARG_OUT(qce_name, tcg_name, idx, name, type)                         \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)),     \
                               TCG_TYPE_##type);
#define X_ARG_IN(qce_name, tcg_name, idx, name, type)                          \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)),     \
                               TCG_TYPE_##type);
#define X_ARG_CONST(qce_name, tcg_name, idx, name)                             \
  UTIL_DOT_JOIN(inst->i_##tcg_name, name) = op->args[idx];
#define X_TAIL(qce_name, tcg_name)                                             \
  break;                                                                       \
  }
#endif

/*
 * Templates Structures
 */

#define X_INST_O1I1(qce_name, tcg_name, o1_name, o1_type, i1_name, i1_type)    \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                            \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O1I1C1(qce_name, tcg_name, o1_name, o1_type, i1_name, i1_type,  \
                      c1_name)                                                 \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                            \
  X_ARG_CONST(qce_name, tcg_name, 2, c1_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O1I2(qce_name, tcg_name, o1_name, o1_type, i1_name, i1_type,    \
                    i2_name, i2_type)                                          \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 2, i2_name, i2_type)                            \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O2I2(qce_name, tcg_name, o1_name, o1_type, o2_name, o2_type,    \
                    i1_name, i1_type, i2_name, i2_type)                        \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_OUT(qce_name, tcg_name, 1, o2_name, o2_type)                           \
  X_ARG_IN(qce_name, tcg_name, 2, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 3, i2_name, i2_type)                            \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O2I3(qce_name, tcg_name, o1_name, o1_type, o2_name, o2_type,    \
                    i1_name, i1_type, i2_name, i2_type, i3_name, i3_type)      \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_OUT(qce_name, tcg_name, 1, o2_name, o2_type)                           \
  X_ARG_IN(qce_name, tcg_name, 2, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 3, i2_name, i2_type)                            \
  X_ARG_IN(qce_name, tcg_name, 4, i3_name, i3_type)                            \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O2I4(qce_name, tcg_name, o1_name, o1_type, o2_name, o2_type,    \
                    i1_name, i1_type, i2_name, i2_type, i3_name, i3_type,      \
                    i4_name, i4_type)                                          \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_OUT(qce_name, tcg_name, 1, o2_name, o2_type)                           \
  X_ARG_IN(qce_name, tcg_name, 2, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 3, i2_name, i2_type)                            \
  X_ARG_IN(qce_name, tcg_name, 4, i3_name, i3_type)                            \
  X_ARG_IN(qce_name, tcg_name, 5, i4_name, i4_type)                            \
  X_TAIL(qce_name, tcg_name)

#define X_INST_MULTI_O1I1(qce_name, tcg_name, o1_name, i1_name)                \
  X_INST_O1I1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32)      \
  X_INST_O1I1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64)

#define X_INST_MULTI_O1I2(qce_name, tcg_name, o1_name, i1_name, i2_name)       \
  X_INST_O1I2(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32,      \
              i2_name, I32)                                                    \
  X_INST_O1I2(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,      \
              i2_name, I64)

#define X_INST_MULTI_O2I2(qce_name, tcg_name, o1_name, o2_name, i1_name,       \
                          i2_name)                                             \
  X_INST_O2I2(qce_name##_I32, tcg_name##_i32, o1_name, I32, o2_name, I32,      \
              i1_name, I32, i2_name, I32)                                      \
  X_INST_O2I2(qce_name##_I64, tcg_name##_i64, o1_name, I64, o2_name, I64,      \
              i1_name, I64, i2_name, I64)

#define X_INST_MULTI_O2I3(qce_name, tcg_name, o1_name, o2_name, i1_name,       \
                          i2_name, i3_name)                                    \
  X_INST_O2I3(qce_name##_I32, tcg_name##_i32, o1_name, I32, o2_name, I32,      \
              i1_name, I32, i2_name, I32, i3_name, I32)                        \
  X_INST_O2I3(qce_name##_I64, tcg_name##_i64, o1_name, I64, o2_name, I64,      \
              i1_name, I64, i2_name, I64, i3_name, I64)

#define X_INST_MULTI_O2I4(qce_name, tcg_name, o1_name, o2_name, i1_name,       \
                          i2_name, i3_name, i4_name)                           \
  X_INST_O2I4(qce_name##_I32, tcg_name##_i32, o1_name, I32, o2_name, I32,      \
              i1_name, I32, i2_name, I32, i3_name, I32, i4_name, I32)          \
  X_INST_O2I4(qce_name##_I64, tcg_name##_i64, o1_name, I64, o2_name, I64,      \
              i1_name, I64, i2_name, I64, i3_name, I64, i4_name, I64)

#define MEMOP_HOST_MULTI_O1I1C1(qce_name, tcg_name, o1_name, i1_name, c1_name) \
  X_INST_O1I1C1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I64,    \
                c1_name)                                                       \
  X_INST_O1I1C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,    \
                c1_name)

/*
 * Template Instantiations
 */

X_INST_MULTI_O1I1(MOV, mov, into, from)

X_INST_MULTI_O1I1(NEG, neg, res, v)
X_INST_MULTI_O1I2(ADD, add, res, v1, v2)
X_INST_MULTI_O1I2(SUB, sub, res, v1, v2)
X_INST_MULTI_O1I2(MUL, mul, res, v1, v2)
X_INST_MULTI_O1I2(DIV, div, res, v1, v2)
X_INST_MULTI_O1I2(DIVU, divu, res, v1, v2)
X_INST_MULTI_O1I2(REM, rem, res, v1, v2)
X_INST_MULTI_O1I2(REMU, remu, res, v1, v2)

X_INST_MULTI_O2I4(ADD2, add2, res_t, res_b, v1_t, v1_b, v2_t, v2_b)
X_INST_MULTI_O2I4(SUB2, sub2, res_t, res_b, v1_t, v1_b, v2_t, v2_b)
X_INST_MULTI_O2I2(MULU2, mulu2, res_t, res_b, v1, v2)
X_INST_MULTI_O2I2(MULS2, muls2, res_t, res_b, v1, v2)
X_INST_MULTI_O2I3(DIV2, div2, quotient, remainer, v_t, v_b, divisor)
X_INST_MULTI_O2I3(DIVU2, divu2, quotient, remainer, v_t, v_b, divisor)

X_INST_MULTI_O1I1(NOT, not, res, v)
X_INST_MULTI_O1I2(AND, and, res, v1, v2)
X_INST_MULTI_O1I2(OR, or, res, v1, v2)
X_INST_MULTI_O1I2(XOR, xor, res, v1, v2)
X_INST_MULTI_O1I2(ANDC, andc, res, v1, v2)
X_INST_MULTI_O1I2(ORC, orc, res, v1, v2)
X_INST_MULTI_O1I2(NAND, nand, res, v1, v2)
X_INST_MULTI_O1I2(NOR, nor, res, v1, v2)
X_INST_MULTI_O1I2(EQV, eqv, res, v1, v2)

X_INST_MULTI_O1I2(SHL, shl, res, v1, v2)
X_INST_MULTI_O1I2(SHR, shr, res, v1, v2)
X_INST_MULTI_O1I2(SAR, sar, res, v1, v2)
X_INST_MULTI_O1I2(ROTL, rotl, res, v1, v2)
X_INST_MULTI_O1I2(ROTR, rotr, res, v1, v2)

X_INST_MULTI_O1I2(CLZ, clz, res, v1, v2)
X_INST_MULTI_O1I2(CTZ, ctz, res, v1, v2)
X_INST_MULTI_O1I1(CTPOP, ctpop, res, v)

MEMOP_HOST_MULTI_O1I1C1(LD, ld, res, addr, offset)
MEMOP_HOST_MULTI_O1I1C1(LD8U, ld8u, res, addr, offset)
MEMOP_HOST_MULTI_O1I1C1(LD8S, ld8s, res, addr, offset)
MEMOP_HOST_MULTI_O1I1C1(LD16U, ld16u, res, addr, offset)
MEMOP_HOST_MULTI_O1I1C1(LD16S, ld16s, res, addr, offset)

/*
 * Template Teardown
 */

#undef X_HEAD
#undef X_ARG_OUT
#undef X_ARG_IN
#undef X_ARG_CONST
#undef X_TAIL

#undef X_INST_O1I1
#undef X_INST_O1I1C1
#undef X_INST_O1I2
#undef X_INST_O2I2
#undef X_INST_O2I3
#undef X_INST_O2I4

#undef X_INST_MULTI_O1I1
#undef X_INST_MULTI_O1I2
#undef X_INST_MULTI_O2I2
#undef X_INST_MULTI_O2I3
#undef X_INST_MULTI_O2I4

#undef MEMOP_HOST_MULTI_O1I1C1
