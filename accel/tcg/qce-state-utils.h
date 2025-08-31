#ifndef QCE_STATE_UTILS_H
#define QCE_STATE_UTILS_H

struct UserData {
  CPUArchState *env;
  QCEState *state;
  QCECellHolder *holder;
  unsigned mmu_idx;
  bool validated;
  bool supported;
};

static gboolean qce_state_env_verify(gpointer key, gpointer value,
                                     gpointer user_data) {
  struct UserData *data = (struct UserData *)user_data;
  QCECellHolder *holder = data->holder;
  QCECellMeta cell = *(QCECellMeta *)&value;
  if (cell.mode != QCE_CELL_MODE_NULL) {
    g_assert(cell.type == QCE_CELL_TYPE_I32);
    int32_t v_record =
        (int32_t)((intptr_t)g_tree_lookup(holder->concrete, key));
    int32_t v_actual = *(int32_t *)key;
    /*
     * cc_src and cc_dst may not be synchronized back to CPUState
     * during TB execution, so ignore verifying them.
     */
    if (v_record != v_actual &&
        key != (gpointer)&data->env->cc_src  &&
        key != (gpointer)((intptr_t)&data->env->cc_src + 4) &&
        key != (gpointer)&data->env->cc_dst &&
        key != (gpointer)((intptr_t)&data->env->cc_dst + 4)) {
      if (cell.mode == QCE_CELL_MODE_CONCRETE) {
        data->validated = false;
        qce_debug("mismatched concrete value "
                  "at host address %p (env offset 0x%02x): "
                  "record value = 0x%08x, actual value = 0x%08x",
                  (void*)key, (uint32_t)((intptr_t)key - (intptr_t)data->env),
                  (uint32_t)v_record, (uint32_t)v_actual);
      } else if (cell.mode == QCE_CELL_MODE_SYMBOLIC) {
        data->supported = false;
        qce_debug("mismatched symbolic value "
                  "at host address %p (env offset 0x%02x): "
                  "record value = 0x%08x, actual value = 0x%08x",
                  (void *)key, (uint32_t)((intptr_t)key-(intptr_t)data->env),
                  (uint32_t)v_record, (uint32_t)v_actual);
      }
    }
  }
  return FALSE;
}

static gboolean qce_state_mem_verify(gpointer key, gpointer value,
                                     gpointer user_data) {
  struct UserData *data = (struct UserData *)user_data;
  QCECellHolder *holder = data->holder;
  QCECellMeta cell = *(QCECellMeta *)&value;
  if (cell.mode != QCE_CELL_MODE_NULL) {
    g_assert(cell.type == QCE_CELL_TYPE_I32);
    int32_t v_record =
        (int32_t)((intptr_t)g_tree_lookup(holder->concrete, key));
    int32_t v_actual =
        cpu_ldl_le_mmuidx_ra(data->env, (intptr_t)key, data->mmu_idx, 0);
    if (v_record != v_actual) {
      if (cell.mode == QCE_CELL_MODE_CONCRETE) {
        data->validated = false;
        qce_debug("mismatched concrete value at guest address %p (MMU %u): "
                  "record value = 0x%08x, actual value = 0x%08x",
                  (void*)key, data->mmu_idx,
                  (uint32_t)v_record, (uint32_t)v_actual);
      } else {
        data->supported = false;
        qce_debug("mismatched symbolic value at guest address %p (MMU %u): "
                  "record value = 0x%08x, actual value = 0x%08x",
                  (void*)key, data->mmu_idx, (uint32_t)v_record,
                  (uint32_t)v_actual);
      }
    }
  }
  return FALSE;
}

static gboolean qce_state_mem_verify_by_tid(gpointer key, gpointer value,
                                            gpointer user_data) {
  struct UserData *data = (struct UserData *)user_data;
  data->mmu_idx = *(unsigned *)&key == 0 ? 4 : 2;
  QCECellHolder *holder = (QCECellHolder *)value;
  data->holder = holder;
  g_tree_foreach(holder->meta, qce_state_mem_verify, data);
  return FALSE;
}

static bool qce_state_verify(CPUArchState *env, QCEState *state,
                             TranslationBlock * tb) {
  /*
   * We don't need to verify temp state since temp registers live within a
   * TB and die on any exit, and the values of temp registers will always be
   * propagated to CPU registers or guest memory. As long as env state and
   * mem state pass the verification, we should be fine.
   */
  struct UserData data = {.env = env, .state = state,
                          .validated = true, .supported = true};

  // verify env state
  data.holder = &state->env;
  g_tree_foreach(state->env.meta, qce_state_env_verify, (gpointer)&data);

  // verify mem state
  g_tree_foreach(state->mem, qce_state_mem_verify_by_tid, (gpointer)&data);

  if (!data.validated) {
    qce_fatal("state verification failed before executing TB %p", tb);
  }
  if (!data.supported) {
    return false;
  }
  return true;
}

static gboolean qce_state_reset_concrete(gpointer key, gpointer value,
                                         gpointer user_data) {
  QCECellHolder *holder = (QCECellHolder *)user_data;
  QCECellMeta cell = *(QCECellMeta *)&value;
  if (cell.mode == QCE_CELL_MODE_CONCRETE) {
    QCECellMeta new_cell = {.mode = QCE_CELL_MODE_NULL};
    g_tree_insert(holder->meta, key, *(gpointer *)&new_cell);
  }
  return FALSE;
}

static gboolean qce_state_mem_reset_by_tid(gpointer key, gpointer value,
                                           gpointer user_data) {
  QCECellHolder *holder = (QCECellHolder *)value;
  g_tree_foreach(holder->meta, qce_state_reset_concrete, (gpointer)holder);
  return FALSE;
}

static void qce_state_reset(QCEState *state) {
  g_tree_foreach(state->env.meta, qce_state_reset_concrete,
                 (gpointer)&state->env);
  g_tree_foreach(state->mem, qce_state_mem_reset_by_tid, NULL);
}

static gboolean qce_state_env_record_concrete(gpointer key, gpointer value,
                                              gpointer user_data) {
  QCECellHolder *holder = (QCECellHolder *)user_data;
  QCECellMeta cell = *(QCECellMeta *)&value;
  if (cell.mode == QCE_CELL_MODE_SYMBOLIC) {
    int32_t val = *(int32_t *)key;
    g_tree_insert(holder->concrete, key, (gpointer)((intptr_t)val));
  }
  return FALSE;
}

static gboolean qce_state_mem_record_concrete(gpointer key, gpointer value,
                                              gpointer user_data) {
  struct UserData *data = (struct UserData *)user_data;
  QCECellHolder *holder = data->holder;
  QCECellMeta cell = *(QCECellMeta *)&value;
  if (cell.mode == QCE_CELL_MODE_SYMBOLIC) {
    int32_t val = cpu_ldl_le_mmuidx_ra(data->env, (intptr_t)key,
                                       data->mmu_idx, 0);
    g_tree_insert(holder->concrete, key, (gpointer)((intptr_t)val));
  }
  return FALSE;
}

static gboolean qce_state_mem_record_concrete_by_tid(gpointer key,
                                                     gpointer value,
                                                     gpointer user_data) {
  struct UserData *data = (struct UserData *)user_data;
  data->mmu_idx = *(unsigned *)&key == 0 ? 4 : 2;
  QCECellHolder *holder = (QCECellHolder *)value;
  data->holder = holder;
  g_tree_foreach(holder->meta, qce_state_mem_record_concrete, data);
  return FALSE;
}

static void qce_state_record_concrete(CPUArchState *env, QCEState *state) {
  g_tree_foreach(state->env.meta, qce_state_env_record_concrete,
                 (gpointer)&state->env);
  struct UserData data = {.env = env};
  g_tree_foreach(state->mem, qce_state_mem_record_concrete_by_tid,
                 (gpointer)&data);
}

#endif /* QCE_STATE_UTILS_H */
