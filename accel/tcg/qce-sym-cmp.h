#ifndef QCE_SYM_CMP_H
#define QCE_SYM_CMP_H

static inline void __record_symbolic_predicate(QCEState *state, QCEPred *pred,
                                               bool actual, vaddr last_pc) {
  // record symbolic predicate
  char *ast = g_strdup(Z3_ast_to_string(state->solver_z3.ctx, pred->symbolic));
#ifdef QCE_DEBUG_IR
  if (g_qce->trace_file != NULL) {
    fprintf(g_qce->trace_file, "**** predicate %016lx [%s]: %s\n", last_pc,
            actual ? "T" : "F", ast);
  }
#endif
  qce_debug("predicate %016lx [%s]: %s", last_pc, actual ? "T" : "F", ast);
  g_free(ast);

  // register coverage and check whether we need to solve for a new seed here
  QCESession *session = g_qce->session;
  bool should_solve = qce_session_add_cov_item(session, last_pc, actual);
  if (!should_solve) {
    return;
  }

  // solve for a new seed to toggle the branch
  Z3_ast cond = pred->symbolic;
  if (actual) {
    cond = Z3_mk_not(state->solver_z3.ctx, cond);
  }

  uint8_t blob[BLOB_SIZE_MAX];
  size_t size = qce_smt_z3_solve_for(&state->solver_z3, cond, blob);

  // save the seed
  FILE *handle = checked_open("w+", "%s/%ld/seeds/%ld", g_qce->output_dir,
                              session->id, session->seed_count);
  fwrite(blob, 1, size, handle);
  fclose(handle);

  // advance the counter
  session->seed_count++;
}

static inline bool __handle_branch_predicate(QCEState *state, QCEPred *pred,
                                             vaddr last_pc) {
  // if the condition is deterministic, simply return its value
  if (pred->mode == QCE_PRED_CONCRETE) {
    return pred->concrete;
  }

#ifndef QCE_RELEASE
  assert(pred->mode == QCE_PRED_SYMBOLIC);
#endif

  // concretize the predicate
  QCESession *session = g_qce->session;
  bool concretized = qce_smt_z3_concretize_bool(
      &state->solver_z3, session->blob_addr, session->blob_size,
      session->blob_content, pred->symbolic);

  // record it
  __record_symbolic_predicate(state, pred, concretized, last_pc);

  // assert path condition
  qce_state_assert_path_constraint(state, pred->symbolic, concretized);
  return concretized;
}

#define DEFINE_SYM_INST_brcond(bits)                                           \
  static inline bool qce_sym_inst_brcond_i##bits(                              \
      CPUArchState *env, QCEState *state, QCEVar *v1, QCEVar *v2,              \
      tcg_target_ulong cond, vaddr last_pc) {                                  \
    QCEExpr expr_v1, expr_v2;                                                  \
    qce_state_get_var(env, state, v1, &expr_v1);                               \
    qce_state_get_var(env, state, v2, &expr_v2);                               \
                                                                               \
    QCEPred pred;                                                              \
    switch (cond) {                                                            \
      /* equality */                                                           \
    case TCG_COND_EQ: {                                                        \
      qce_expr_eq_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);       \
      break;                                                                   \
    }                                                                          \
    case TCG_COND_NE: {                                                        \
      qce_expr_ne_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);       \
      break;                                                                   \
    }                                                                          \
      /* signed comparison */                                                  \
    case TCG_COND_LT: {                                                        \
      qce_expr_slt_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);      \
      break;                                                                   \
    }                                                                          \
    case TCG_COND_LE: {                                                        \
      qce_expr_sle_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);      \
      break;                                                                   \
    }                                                                          \
    case TCG_COND_GE: {                                                        \
      qce_expr_sge_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);      \
      break;                                                                   \
    }                                                                          \
    case TCG_COND_GT: {                                                        \
      qce_expr_sgt_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);      \
      break;                                                                   \
    }                                                                          \
      /* unsigned comparison */                                                \
    case TCG_COND_LTU: {                                                       \
      qce_expr_ult_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);      \
      break;                                                                   \
    }                                                                          \
    case TCG_COND_LEU: {                                                       \
      qce_expr_ule_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);      \
      break;                                                                   \
    }                                                                          \
    case TCG_COND_GEU: {                                                       \
      qce_expr_uge_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);      \
      break;                                                                   \
    }                                                                          \
    case TCG_COND_GTU: {                                                       \
      qce_expr_ugt_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &pred);      \
      break;                                                                   \
    }                                                                          \
      /* "test" i.e. and then compare vs 0 */                                  \
    case TCG_COND_TSTEQ: {                                                     \
      QCEExpr expr_r;                                                          \
      qce_expr_bvand_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &expr_r);  \
      QCEExpr expr_0;                                                          \
      qce_expr_init_v##bits(&expr_0, 0);                                       \
      qce_expr_eq_i##bits(&state->solver_z3, &expr_r, &expr_0, &pred);         \
      break;                                                                   \
    }                                                                          \
    case TCG_COND_TSTNE: {                                                     \
      QCEExpr expr_r;                                                          \
      qce_expr_bvand_i##bits(&state->solver_z3, &expr_v1, &expr_v2, &expr_r);  \
      QCEExpr expr_0;                                                          \
      qce_expr_init_v##bits(&expr_0, 0);                                       \
      qce_expr_ne_i##bits(&state->solver_z3, &expr_r, &expr_0, &pred);         \
      break;                                                                   \
    }                                                                          \
      /* unconditional jump */                                                 \
    case TCG_COND_NEVER:                                                       \
      return false;                                                            \
    case TCG_COND_ALWAYS:                                                      \
      return true;                                                             \
    /* all other cases are invalid */                                          \
    default:                                                                   \
      qce_fatal("unknown condition: %lx", cond);                               \
    }                                                                          \
                                                                               \
    return __handle_branch_predicate(state, &pred, last_pc);                   \
  }

DEFINE_SYM_INST_brcond(32);
DEFINE_SYM_INST_brcond(64);

#define HANDLE_SYM_INST_brcond(bits)                                           \
  case QCE_INST_BRCOND_I##bits: {                                              \
    bool should_jump = qce_sym_inst_brcond_i##bits(                            \
        arch, &session->state, &inst->i_brcond_i##bits.v1,                     \
        &inst->i_brcond_i##bits.v2, inst->i_brcond_i##bits.cond, last_pc);     \
    if (should_jump) {                                                         \
      cursor = entry->labels[inst->i_brcond_i##bits.label.id];                 \
      continue;                                                                \
    }                                                                          \
    break;                                                                     \
  }

#endif /* QCE_SYM_CMP_H */
