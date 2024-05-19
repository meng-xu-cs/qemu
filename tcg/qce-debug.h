#ifdef QCE_DEBUG_IR

static inline char *qce_debug_tcg_temp_to_str(const TCGContext *s, TCGTemp *t) {
  char *buf = NULL;
  int idx = temp_idx(t);

  switch (t->kind) {
  case TEMP_FIXED:
  case TEMP_GLOBAL:
    asprintf(&buf, "%s", t->name);
    break;
  case TEMP_TB:
    asprintf(&buf, "loc%d", idx - s->nb_globals);
    break;
  case TEMP_EBB:
    asprintf(&buf, "tmp%d", idx - s->nb_globals);
    break;
  case TEMP_CONST:
    switch (t->type) {
    case TCG_TYPE_I32:
      asprintf(&buf, "$0x%x", (int32_t)t->val);
      break;
    case TCG_TYPE_I64:
      asprintf(&buf, "$0x%lx", t->val);
      break;
    case TCG_TYPE_V64:
    case TCG_TYPE_V128:
    case TCG_TYPE_V256:
      asprintf(&buf, "v%d$0x%lx", 64 << (t->type - TCG_TYPE_V64), t->val);
      break;
    default:
      g_assert_not_reached();
    }
    break;
  }
  return buf;
}

#define qce_debug_assert_ir1(s, expr, t1)                                      \
  do {                                                                         \
    if (!(expr)) {                                                             \
      qce_debug("[ir] context of assertion failure");                          \
      tcg_dump_ops(s, stderr, false);                                          \
      qce_fatal("[ir] expect %s where %s := %s", #expr, #t1,                   \
                qce_debug_tcg_temp_to_str(s, t1));                             \
    }                                                                          \
  } while (0);
#define qce_debug_assert_ir2(s, expr, t1, t2)                                  \
  do {                                                                         \
    if (!(expr)) {                                                             \
      qce_debug("[ir] context of assertion failure");                          \
      tcg_dump_ops(s, stderr, false);                                          \
      qce_fatal("[ir] expect %s where %s := %s and %s := %s", #expr, #t1,      \
                qce_debug_tcg_temp_to_str(s, t1), #t2,                         \
                qce_debug_tcg_temp_to_str(s, t2));                             \
    }                                                                          \
  } while (0);
#define qce_debug_assert_ir3(s, expr, t1, t2, t3)                              \
  do {                                                                         \
    if (!(expr)) {                                                             \
      qce_debug("[ir] context of assertion failure");                          \
      tcg_dump_ops(s, stderr, false);                                          \
      qce_fatal("[ir] expect %s where %s := %s and %s := %s and %s := %s",     \
                #expr, #t1, qce_debug_tcg_temp_to_str(s, t1), #t2,             \
                qce_debug_tcg_temp_to_str(s, t2), #t3,                         \
                qce_debug_tcg_temp_to_str(s, t3));                             \
    }                                                                          \
  } while (0);

#define qce_debug_assert_op1(s, expr, op1)                                     \
  do {                                                                         \
    if (!(expr)) {                                                             \
      const TCGOpDef *def1 = &tcg_op_defs[op1->opc];                           \
      qce_debug("[op] context of assertion failure");                          \
      tcg_dump_ops(s, stderr, false);                                          \
      qce_fatal("[op] expect %s where %s := %s", #expr, #op1, def1->name);     \
    }                                                                          \
  } while (0);

#define qce_debug_assert_label_intact(s, l)                                    \
  do {                                                                         \
    if (!l->present) {                                                         \
      qce_debug("[op] context of assertion failure");                          \
      tcg_dump_ops(s, stderr, false);                                          \
      qce_fatal("[op] label not present: %d", l->id);                          \
    }                                                                          \
    if (!QSIMPLEQ_EMPTY(&l->relocs)) {                                         \
      qce_debug("[op] context of assertion failure");                          \
      tcg_dump_ops(s, stderr, false);                                          \
      qce_fatal("[op] label has relocations: %d", l->id);                      \
    }                                                                          \
  } while (0);

#else
#define qce_debug_assert_ir1(s, expr, t1)
#define qce_debug_assert_ir2(s, expr, t1, t2)
#define qce_debug_assert_ir3(s, expr, t1, t2, t3)
#define qce_debug_assert_op1(s, expr, op1)
#define qce_debug_assert_label_intact(s, l)
#endif

/*
 * Utilities
 */

#define UTIL_DOT_JOIN(a, b) a.b
