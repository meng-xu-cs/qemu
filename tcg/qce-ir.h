struct QCEInst {
  TCGOpcode opc;
  union {
    // op: discard
    struct {
      TCGArg out;
    } op_discard;
  } inst;
};

static inline void parse(const TCGOp *op, struct QCEInst *inst) {
  switch (op->opc) {
  case INDEX_op_discard:
    return;
  default:
    return;
  }
}