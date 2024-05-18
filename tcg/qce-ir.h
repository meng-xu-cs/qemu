struct QCEVar {
  TCGTempKind kind;
  union {
    // kind: TEMP_FIXED
    struct {
      TCGReg reg : 8;
    } v_fixed;

    // kind: TEMP_GLOBAL
    struct {
    } v_global_direct;
    struct {
    } v_global_indirect;
  };
};

static inline void parse_temp(const TCGTemp *t) {}

struct QCEInst {
  TCGOpcode opc;
  union {
    // opc: discard
    struct {
      TCGArg out;
    } op_discard;
  } inst;
};

static inline void parse_op(const TCGOp *op, struct QCEInst *inst) {
  switch (op->opc) {
  case INDEX_op_discard:
    return;
  default:
    return;
  }
}
