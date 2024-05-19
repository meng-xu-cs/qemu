/*
 * Template Definitions
 */

#ifdef QCE_INST_TEMPLATE_IN_KIND_ENUM
#define ARITH_HEAD(qce_name, tcg_name) QCE_INST_##qce_name,
#define ARITH_ARG_OUT(qce_name, tcg_name, idx, name, type)
#define ARITH_ARG_IN(qce_name, tcg_name, idx, name, type)
#define ARITH_TAIL(qce_name, tcg_name)
#endif

#ifdef QCE_INST_TEMPLATE_IN_INST_UNION
#define ARITH_HEAD(qce_name, tcg_name) struct {
#define ARITH_ARG_OUT(qce_name, tcg_name, idx, name, type) QCEVar name;
#define ARITH_ARG_IN(qce_name, tcg_name, idx, name, type) QCEVar name;
#define ARITH_TAIL(qce_name, tcg_name)                                         \
  }                                                                            \
  i_##tcg_name;
#endif

#ifdef QCE_INST_TEMPLATE_IN_PARSER
#define ARITH_HEAD(qce_name, tcg_name)                                         \
  case INDEX_op_##tcg_name: {                                                  \
    inst->kind = QCE_INST_##qce_name;
#define ARITH_ARG_OUT(qce_name, tcg_name, idx, name, type)                     \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)),     \
                               TCG_TYPE_##type);
#define ARITH_ARG_IN(qce_name, tcg_name, idx, name, type)                      \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)),     \
                               TCG_TYPE_##type);
#define ARITH_TAIL(qce_name, tcg_name)                                         \
  break;                                                                       \
  }
#endif

/*
 * Templates Structures
 */

#define ARITH_INST_O1I1(qce_name, tcg_name, o1_name, o1_type, i1_name,         \
                        i1_type)                                               \
  ARITH_HEAD(qce_name, tcg_name)                                               \
  ARITH_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                       \
  ARITH_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                        \
  ARITH_TAIL(qce_name, tcg_name)

#define ARITH_INST_O1I2(qce_name, tcg_name, o1_name, o1_type, i1_name,         \
                        i1_type, i2_name, i2_type)                             \
  ARITH_HEAD(qce_name, tcg_name)                                               \
  ARITH_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                       \
  ARITH_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                        \
  ARITH_ARG_IN(qce_name, tcg_name, 2, i2_name, i2_type)                        \
  ARITH_TAIL(qce_name, tcg_name)

#define ARITH_INST_MULTI_O1I1(qce_name, tcg_name, o1_name, i1_name)            \
  ARITH_INST_O1I1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32)  \
  ARITH_INST_O1I1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64)

#define ARITH_INST_MULTI_O1I2(qce_name, tcg_name, o1_name, i1_name, i2_name)   \
  ARITH_INST_O1I2(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32,  \
                  i2_name, I32)                                                \
  ARITH_INST_O1I2(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,  \
                  i2_name, I64)

/*
 * Template Instantiations
 */

// arithmetics
ARITH_INST_MULTI_O1I1(MOV, mov, into, from)

ARITH_INST_MULTI_O1I1(NEG, neg, res, v)
ARITH_INST_MULTI_O1I2(ADD, add, res, v1, v2)
ARITH_INST_MULTI_O1I2(SUB, sub, res, v1, v2)
ARITH_INST_MULTI_O1I2(MUL, mul, res, v1, v2)
ARITH_INST_MULTI_O1I2(DIV, div, res, v1, v2)
ARITH_INST_MULTI_O1I2(DIVU, divu, res, v1, v2)
ARITH_INST_MULTI_O1I2(REM, rem, res, v1, v2)
ARITH_INST_MULTI_O1I2(REMU, remu, res, v1, v2)

ARITH_INST_MULTI_O1I1(NOT, not, res, v)
ARITH_INST_MULTI_O1I2(AND, and, res, v1, v2)
ARITH_INST_MULTI_O1I2(OR, or, res, v1, v2)
ARITH_INST_MULTI_O1I2(XOR, xor, res, v1, v2)
ARITH_INST_MULTI_O1I2(ANDC, andc, res, v1, v2)
ARITH_INST_MULTI_O1I2(ORC, orc, res, v1, v2)
ARITH_INST_MULTI_O1I2(NAND, nand, res, v1, v2)
ARITH_INST_MULTI_O1I2(NOR, nor, res, v1, v2)
ARITH_INST_MULTI_O1I2(EQV, eqv, res, v1, v2)

ARITH_INST_MULTI_O1I2(SHL, shl, res, v1, v2)
ARITH_INST_MULTI_O1I2(SHR, shr, res, v1, v2)
ARITH_INST_MULTI_O1I2(SAR, sar, res, v1, v2)
ARITH_INST_MULTI_O1I2(ROTL, rotl, res, v1, v2)
ARITH_INST_MULTI_O1I2(ROTR, rotr, res, v1, v2)

/*
 * Template Teardown
 */

#undef ARITH_HEAD
#undef ARITH_ARG_OUT
#undef ARITH_ARG_IN
#undef ARITH_TAIL
#undef ARITH_INST_O1I1
#undef ARITH_INST_O1I2
#undef ARITH_INST_MULTI_O1I1
#undef ARITH_INST_MULTI_O1I2
