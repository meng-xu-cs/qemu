#ifndef QCE_COV_H
#define QCE_COV_H

#define _QCE_COV_BIT_EVAL 48

static inline vaddr cov_bit_eval_set(vaddr cov) {
  return cov | (1ul << _QCE_COV_BIT_EVAL);
}

static inline vaddr cov_bit_eval_clear(vaddr cov) {
  return cov & ~(1ul << _QCE_COV_BIT_EVAL);
}

gint qce_gtree_cov_cmp(gconstpointer a, gconstpointer b);
gint qce_gtree_cov_cmp(gconstpointer a, gconstpointer b) {
  return (intptr_t)a - (intptr_t)b;
}

gboolean qce_gtree_cov_destroy_on_iter(gpointer _key, gpointer value,
                                       gpointer _data);
gboolean qce_gtree_cov_destroy_on_iter(gpointer _key, gpointer value,
                                       gpointer _data) {
  GArray *l2 = value;
  for (uint64_t i = 0; i < l2->len; i++) {
    uint64_t *trace = g_array_index(l2, uint64_t *, i);
    g_free(trace);
  }
  g_array_free(l2, true);
  return false;
}

// load up the coverage from buffer
static inline GArray *__qce_parse_cov_db(const uint64_t *buf, size_t size) {
  const uint64_t *cursor = buf;
#ifndef QCE_RELEASE
  size_t counter = 0;
#endif

  uint64_t num_sizes = *cursor++;
  GArray *db = g_array_sized_new(false, true, sizeof(GTree *), num_sizes + 1);
  /* db[0] is left unchanged (i.e., NULL) as there is no zero-length tree */

  for (uint64_t len = 1; len <= num_sizes; len++) {
    GTree *l1 = g_tree_new(qce_gtree_cov_cmp);

    uint64_t num_hashes = *cursor++;
    for (uint64_t i = 0; i < num_hashes; i++) {
      uint64_t hash = *cursor++;

      uint64_t num_traces = *cursor++;
      GArray *l2 =
          g_array_sized_new(false, false, sizeof(uint64_t *), num_traces);
      for (uint64_t j = 0; j < num_traces; j++) {
        uint64_t *trace = g_malloc0_n(len, sizeof(uint64_t));
        for (uint64_t k = 0; k < len; k++) {
          trace[k] = *cursor++;
        }
        g_array_insert_val(l2, j, trace);
      }
#ifndef QCE_RELEASE
      counter += num_traces;
#endif

      g_tree_insert(l1, (gpointer)hash, l2);
    }

    g_array_insert_val(db, len, l1);
  }

  g_assert((uintptr_t)cursor - (uintptr_t)buf == size);
#ifndef QCE_RELEASE
  qce_debug("traces loaded into coverage database: %lu", counter);
#endif
  return db;
}

// clean-up and free the coverage database
static inline void __qce_free_cov_db(GArray *db) {
  for (uint64_t i = 1; i < db->len; i++) {
    // NOTE: i does not start from 0
    GTree *l1 = g_array_index(db, GTree *, i);
    g_tree_foreach(l1, qce_gtree_cov_destroy_on_iter, NULL);
    g_tree_destroy(l1);
  }
  g_array_free(db, true);
}

static inline GArray *__qce_load_cov_db(FILE *file) {
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // short-circuit if the file is empty
  if (size == 0) {
    qce_debug("empty coverage file found, no traces are loaded");
    return g_array_sized_new(false, true, sizeof(GTree *), 1);
  }

  void *buf = g_malloc(size);
  if (fread(buf, size, 1, file) != 1) {
    qce_fatal("unable to read cov file");
  }

  GArray *db = __qce_parse_cov_db(buf, size);
  g_free(buf);
  return db;
}

// register this coverage,
// - return true if we need to generate a seed that toggles this predicate.
static inline bool qce_session_add_cov_item(QCESession *session, vaddr pc,
                                            bool actual) {
  // derive the cov items
  vaddr cov, cov_flip;
  if (actual) {
    cov = cov_bit_eval_set(pc);
    cov_flip = cov_bit_eval_clear(pc);
  } else {
    cov = cov_bit_eval_clear(pc);
    cov_flip = cov_bit_eval_set(pc);
  }

  // derive the flip-side hash
  XXH64_state_t hasher;
  XXH64_copyState(&hasher, &session->cov_hash);
  XXH64_update(&hasher, &cov_flip, sizeof(vaddr));
  uint64_t hash_flip = XXH64_digest(&hasher);

  // register the path-side coverage
  g_array_append_val(session->coverage, cov);
  XXH64_update(&session->cov_hash, &cov, sizeof(vaddr));

  // check whether we need to cover the flip of this case
  uint64_t len = session->coverage->len;
  if (len >= session->database->len) {
    // condition 1: none of existing trace has gone this far
    return true;
  }

  GTree *l1 = g_array_index(session->database, GTree *, len);
  GArray *l2 = g_tree_lookup(l1, (gpointer)hash_flip);
  if (l2 == NULL) {
    // condition 2: this is a new path hash
    return true;
  }

  // NOTE: this should really be the rare path (and will be slow)
  for (uint64_t i = 0; i < l2->len; i++) {
    uint64_t *trace = g_array_index(l2, uint64_t *, i);
    bool match = true;
    for (uint64_t j = 0; j < len - 1; j++) {
      if (g_array_index(session->coverage, uint64_t, j) != trace[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      // we have found an exact trace
      return false;
    }
  }

  // condition 3: this is a hash collision and we actually found a new trace
  return true;
}

#endif /* QCE_COV_H */
