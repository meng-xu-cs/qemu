/*
 * Template Definitions
 */

#ifdef QCE_INST_TEMPLATE_IN_KIND_ENUM
#define X_HEAD(qce_name, tcg_name) QCE_INST_##qce_name,
#define X_ARG_VAR(qce_name, tcg_name, idx, name)
#define X_ARG_OUT(qce_name, tcg_name, idx, name, type)
#define X_ARG_IN(qce_name, tcg_name, idx, name, type)
#define X_ARG_ADDR(qce_name, tcg_name, idx, name)
#define X_ARG_CONST(qce_name, tcg_name, idx, name)
#define X_ARG_LABEL(qce_name, tcg_name, idx, name)
#define X_TAIL(qce_name, tcg_name)
#endif

#ifdef QCE_INST_TEMPLATE_IN_INST_UNION
#define X_HEAD(qce_name, tcg_name) struct {
#define X_ARG_VAR(qce_name, tcg_name, idx, name) QCEVar name;
#define X_ARG_OUT(qce_name, tcg_name, idx, name, type) QCEVar name;
#define X_ARG_IN(qce_name, tcg_name, idx, name, type) QCEVar name;
#define X_ARG_ADDR(qce_name, tcg_name, idx, name) QCEVar name;
#define X_ARG_CONST(qce_name, tcg_name, idx, name) tcg_target_ulong name;
#define X_ARG_LABEL(qce_name, tcg_name, idx, name) QCELabel name;
#define X_TAIL(qce_name, tcg_name)                                             \
  }                                                                            \
  i_##tcg_name;
#endif

#ifdef QCE_INST_TEMPLATE_IN_PARSER
#define X_HEAD(qce_name, tcg_name)                                             \
  case INDEX_op_##tcg_name: {                                                  \
    inst->kind = QCE_INST_##qce_name;
#define X_ARG_VAR(qce_name, tcg_name, idx, name)                               \
  parse_arg_as_var(tcg, op->args[idx],                                         \
                   &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)));
#define X_ARG_OUT(qce_name, tcg_name, idx, name, type)                         \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)),     \
                               TCG_TYPE_##type);
#define X_ARG_IN(qce_name, tcg_name, idx, name, type)                          \
  parse_arg_as_var_expect_type(tcg, op->args[idx],                             \
                               &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)),     \
                               TCG_TYPE_##type);
#define X_ARG_ADDR(qce_name, tcg_name, idx, name)                              \
  parse_arg_as_var_expect_host_addr(                                           \
      tcg, op->args[idx], &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)));
#define X_ARG_CONST(qce_name, tcg_name, idx, name)                             \
  UTIL_DOT_JOIN(inst->i_##tcg_name, name) = op->args[idx];
#define X_ARG_LABEL(qce_name, tcg_name, idx, name)                             \
  parse_arg_as_label(tcg, op->args[idx],                                       \
                     &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)));
#define X_TAIL(qce_name, tcg_name)                                             \
  break;                                                                       \
  }
#endif

#ifdef QCE_INST_TEMPLATE_IN_DEBUG_PRINT
#define X_HEAD(qce_name, tcg_name)                                             \
  case QCE_INST_##qce_name: {                                                  \
    fprintf(f, #tcg_name " {\n");
#define X_ARG_VAR(qce_name, tcg_name, idx, name)                               \
  fprintf(f, "\t" #name ": ");                                                 \
  qce_debug_print_var(f, &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)));          \
  fprintf(f, ",\n");
#define X_ARG_OUT(qce_name, tcg_name, idx, name, type)                         \
  fprintf(f, "\t" #name ": ");                                                 \
  qce_debug_print_var(f, &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)));          \
  fprintf(f, ",\n");
#define X_ARG_IN(qce_name, tcg_name, idx, name, type)                          \
  fprintf(f, "\t" #name ": ");                                                 \
  qce_debug_print_var(f, &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)));          \
  fprintf(f, ",\n");
#define X_ARG_ADDR(qce_name, tcg_name, idx, name)                              \
  fprintf(f, "\t" #name ": ");                                                 \
  qce_debug_print_var(f, &(UTIL_DOT_JOIN(inst->i_##tcg_name, name)));          \
  fprintf(f, ",\n");
#define X_ARG_CONST(qce_name, tcg_name, idx, name)                             \
  fprintf(f, "\t" #name ": 0x%lx,\n", UTIL_DOT_JOIN(inst->i_##tcg_name, name));
#define X_ARG_LABEL(qce_name, tcg_name, idx, name)                             \
  fprintf(f, "\t" #name ": L%u,\n", UTIL_DOT_JOIN(inst->i_##tcg_name, name).id);
#define X_TAIL(qce_name, tcg_name)                                             \
  fprintf(f, "}");                                                             \
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

#define X_INST_O1I1C2(qce_name, tcg_name, o1_name, o1_type, i1_name, i1_type,  \
                      c1_name, c2_name)                                        \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                            \
  X_ARG_CONST(qce_name, tcg_name, 2, c1_name)                                  \
  X_ARG_CONST(qce_name, tcg_name, 3, c2_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O1A1C1(qce_name, tcg_name, o1_name, o1_type, a1_name, c1_name)  \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_ADDR(qce_name, tcg_name, 1, a1_name)                                   \
  X_ARG_CONST(qce_name, tcg_name, 2, c1_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_I1A1C1(qce_name, tcg_name, i1_name, i1_type, a1_name, c1_name)  \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_IN(qce_name, tcg_name, 0, i1_name, i1_type)                            \
  X_ARG_ADDR(qce_name, tcg_name, 1, a1_name)                                   \
  X_ARG_CONST(qce_name, tcg_name, 2, c1_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O1I2(qce_name, tcg_name, o1_name, o1_type, i1_name, i1_type,    \
                    i2_name, i2_type)                                          \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 2, i2_name, i2_type)                            \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O1I2C1(qce_name, tcg_name, o1_name, o1_type, i1_name, i1_type,  \
                      i2_name, i2_type, c1_name)                               \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 2, i2_name, i2_type)                            \
  X_ARG_CONST(qce_name, tcg_name, 3, c1_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O1I2C2(qce_name, tcg_name, o1_name, o1_type, i1_name, i1_type,  \
                      i2_name, i2_type, c1_name, c2_name)                      \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 2, i2_name, i2_type)                            \
  X_ARG_CONST(qce_name, tcg_name, 3, c1_name)                                  \
  X_ARG_CONST(qce_name, tcg_name, 4, c2_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_O1I4C1(qce_name, tcg_name, o1_name, o1_type, i1_name, i1_type,  \
                      i2_name, i2_type, i3_name, i3_type, i4_name, i4_type,    \
                      c1_name)                                                 \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_OUT(qce_name, tcg_name, 0, o1_name, o1_type)                           \
  X_ARG_IN(qce_name, tcg_name, 1, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 2, i2_name, i2_type)                            \
  X_ARG_IN(qce_name, tcg_name, 3, i3_name, i3_type)                            \
  X_ARG_IN(qce_name, tcg_name, 4, i4_name, i4_type)                            \
  X_ARG_CONST(qce_name, tcg_name, 5, c1_name)                                  \
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

#define X_INST_V1(qce_name, tcg_name, v1_name)                                 \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_VAR(qce_name, tcg_name, 0, v1_name)                                    \
  X_TAIL(qce_name, tcg_name)

#define X_INST_C1(qce_name, tcg_name, c1_name)                                 \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_CONST(qce_name, tcg_name, 0, c1_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_L1(qce_name, tcg_name, l1_name)                                 \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_LABEL(qce_name, tcg_name, 0, l1_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_I2C1L1(qce_name, tcg_name, i1_name, i1_type, i2_name, i2_type,  \
                      c1_name, l1_name)                                        \
  X_HEAD(qce_name, tcg_name)                                                   \
  X_ARG_IN(qce_name, tcg_name, 0, i1_name, i1_type)                            \
  X_ARG_IN(qce_name, tcg_name, 1, i2_name, i2_type)                            \
  X_ARG_CONST(qce_name, tcg_name, 2, c1_name)                                  \
  X_ARG_LABEL(qce_name, tcg_name, 3, l1_name)                                  \
  X_TAIL(qce_name, tcg_name)

#define X_INST_MULTI_O1I1(qce_name, tcg_name, o1_name, i1_name)                \
  X_INST_O1I1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32)      \
  X_INST_O1I1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64)

#define X_INST_SINGLE_O1I1(qce_name, tcg_name, o1_name, i1_name)               \
  X_INST_O1I1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64)

#define X_INST_MULTI_O1I1C1(qce_name, tcg_name, o1_name, i1_name, c1_name)     \
  X_INST_O1I1C1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32,    \
                c1_name)                                                       \
  X_INST_O1I1C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,    \
                c1_name)

#define X_INST_SINGLE_O1I1C1(qce_name, tcg_name, o1_name, i1_name, c1_name)    \
  X_INST_O1I1C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,    \
                c1_name)

#define X_INST_MULTI_O1I1C2(qce_name, tcg_name, o1_name, i1_name, c1_name,     \
                            c2_name)                                           \
  X_INST_O1I1C2(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32,    \
                c1_name, c2_name)                                              \
  X_INST_O1I1C2(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,    \
                c1_name, c2_name)

#define X_INST_MULTI_O1I2(qce_name, tcg_name, o1_name, i1_name, i2_name)       \
  X_INST_O1I2(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32,      \
              i2_name, I32)                                                    \
  X_INST_O1I2(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,      \
              i2_name, I64)

#define X_INST_MULTI_O1I2C1(qce_name, tcg_name, o1_name, i1_name, i2_name,     \
                            c1_name)                                           \
  X_INST_O1I2C1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32,    \
                i2_name, I32, c1_name)                                         \
  X_INST_O1I2C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,    \
                i2_name, I64, c1_name)

#define X_INST_MULTI_O1I2C2(qce_name, tcg_name, o1_name, i1_name, i2_name,     \
                            c1_name, c2_name)                                  \
  X_INST_O1I2C2(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32,    \
                i2_name, I32, c1_name, c2_name)                                \
  X_INST_O1I2C2(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,    \
                i2_name, I64, c1_name, c2_name)

#define X_INST_MULTI_O1I4C1(qce_name, tcg_name, o1_name, i1_name, i2_name,     \
                            i3_name, i4_name, c1_name)                         \
  X_INST_O1I4C1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name, I32,    \
                i2_name, I32, i3_name, I32, i4_name, I32, c1_name)             \
  X_INST_O1I4C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, I64,    \
                i2_name, I64, i3_name, I64, i4_name, I64, c1_name)

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

#define X_INST_MULTI_I2C1L1(qce_name, tcg_name, i1_name, i2_name, c1_name,     \
                            l1_name)                                           \
  X_INST_I2C1L1(qce_name##_I32, tcg_name##_i32, i1_name, I32, i2_name, I32,    \
                c1_name, l1_name)                                              \
  X_INST_I2C1L1(qce_name##_I64, tcg_name##_i64, i1_name, I64, i2_name, I64,    \
                c1_name, l1_name)

#define MEMOP_MULTI_O1A1C1(qce_name, tcg_name, o1_name, i1_name, c1_name)      \
  X_INST_O1A1C1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name,         \
                c1_name)                                                       \
  X_INST_O1A1C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, c1_name)

#define MEMOP_SINGLE_O1A1C1(qce_name, tcg_name, o1_name, i1_name, c1_name)     \
  X_INST_O1A1C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, c1_name)

#define MEMOP_MULTI_I1A1C1(qce_name, tcg_name, o1_name, i1_name, c1_name)      \
  X_INST_I1A1C1(qce_name##_I32, tcg_name##_i32, o1_name, I32, i1_name,         \
                c1_name)                                                       \
  X_INST_I1A1C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, c1_name)

#define MEMOP_SINGLE_I1A1C1(qce_name, tcg_name, o1_name, i1_name, c1_name)     \
  X_INST_I1A1C1(qce_name##_I64, tcg_name##_i64, o1_name, I64, i1_name, c1_name)

/*
 * Template Instantiations
 */

X_INST_MULTI_O1I1(MOV, mov, into, from)

X_INST_MULTI_O1I2C1(SETCOND, setcond, res, v1, v2, cond)
X_INST_MULTI_O1I2C1(NEGSETCOND, negsetcond, res, v1, v2, cond)
X_INST_MULTI_O1I4C1(MOVCOND, movcond, res, c1, c2, v1, v2, cond)

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

X_INST_MULTI_O1I1(EXT8S, ext8s, into, from)
X_INST_MULTI_O1I1(EXT8U, ext8u, into, from)
X_INST_MULTI_O1I1(EXT16S, ext16s, into, from)
X_INST_MULTI_O1I1(EXT16U, ext16u, into, from)
X_INST_SINGLE_O1I1(EXT32S, ext32s, into, from)
X_INST_SINGLE_O1I1(EXT32U, ext32u, into, from)

X_INST_O1I1(EXTS, ext_i32_i64, into, I64, from, I32)
X_INST_O1I1(EXTU, extu_i32_i64, into, I64, from, I32)
X_INST_O1I1(TRUNCL, extrl_i64_i32, into, I32, from, I64)
X_INST_O1I1(TRUNCH, extrh_i64_i32, into, I32, from, I64)

X_INST_MULTI_O1I1C1(BSWAP16, bswap16, res, v, flag)
X_INST_MULTI_O1I1C1(BSWAP32, bswap32, res, v, flag)
X_INST_SINGLE_O1I1C1(BSWAP64, bswap64, res, v, flag)

X_INST_MULTI_O1I2C2(DEPOSIT, deposit, res, bits, into, pos, len)
X_INST_MULTI_O1I1C2(EXTRACT, extract, res, from, pos, len)
X_INST_MULTI_O1I1C2(SEXTRACT, sextract, res, from, pos, len)
X_INST_MULTI_O1I2C1(EXTRACT2, extract2, res, v_b, v_t, pos)

X_INST_MULTI_O1I2(CLZ, clz, res, v1, v2)
X_INST_MULTI_O1I2(CTZ, ctz, res, v1, v2)
X_INST_MULTI_O1I1(CTPOP, ctpop, res, v)

MEMOP_MULTI_O1A1C1(LD, ld, res, addr, offset)
MEMOP_MULTI_O1A1C1(LD8U, ld8u, res, addr, offset)
MEMOP_MULTI_O1A1C1(LD8S, ld8s, res, addr, offset)
MEMOP_MULTI_O1A1C1(LD16U, ld16u, res, addr, offset)
MEMOP_MULTI_O1A1C1(LD16S, ld16s, res, addr, offset)
MEMOP_SINGLE_O1A1C1(LD32U, ld32u, res, addr, offset)
MEMOP_SINGLE_O1A1C1(LD32S, ld32s, res, addr, offset)

MEMOP_MULTI_I1A1C1(ST, st, val, addr, offset)
MEMOP_MULTI_I1A1C1(ST8, st8, val, addr, offset)
MEMOP_MULTI_I1A1C1(ST16, st16, val, addr, offset)
MEMOP_SINGLE_I1A1C1(ST32, st32, val, addr, offset)

X_INST_O1A1C1(GUEST_LD32, qemu_ld_a64_i32, res, I32, addr, flag)
X_INST_O1A1C1(GUEST_LD64, qemu_ld_a64_i64, res, I64, addr, flag)
X_INST_I1A1C1(GUEST_ST32, qemu_st_a64_i32, val, I32, addr, flag)
X_INST_I1A1C1(GUEST_ST64, qemu_st_a64_i64, val, I64, addr, flag)

X_INST_V1(DISCARD, discard, var)
X_INST_C1(MB, mb, flag)

X_INST_L1(SET_LABEL, set_label, label)
X_INST_L1(BR, br, label)
X_INST_MULTI_I2C1L1(BRCOND, brcond, v1, v2, cond, label)

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

#undef X_INST_O1I1
#undef X_INST_O1I2
#undef X_INST_O2I2
#undef X_INST_O2I3
#undef X_INST_O2I4
#undef X_INST_MULTI_O1I1
#undef X_INST_SINGLE_O1I1
#undef X_INST_MULTI_O1I2
#undef X_INST_MULTI_O2I2
#undef X_INST_MULTI_O2I3
#undef X_INST_MULTI_O2I4

#undef X_INST_O1I1C1
#undef X_INST_O1I1C2
#undef X_INST_O1I2C1
#undef X_INST_O1I2C2
#undef X_INST_O1I4C1
#undef X_INST_MULTI_O1I1C1
#undef X_INST_SINGLE_O1I1C1
#undef X_INST_MULTI_O1I1C2
#undef X_INST_MULTI_O1I2C1
#undef X_INST_MULTI_O1I2C2
#undef X_INST_MULTI_O1I4C1

#undef X_INST_O1A1C1
#undef X_INST_I1A1C1
#undef MEMOP_MULTI_O1A1C1
#undef MEMOP_MULTI_I1A1C1
#undef MEMOP_SINGLE_O1A1C1
#undef MEMOP_SINGLE_I1A1C1

#undef X_INST_V1
#undef X_INST_C1
#undef X_INST_L1
#undef X_INST_I2C1L1
#undef X_INST_MULTI_I2C1L1
