
#if !defined(VM_HEADER_IR_TOIR)
#define VM_HEADER_IR_TOIR

#include "../opcode.h"
#include "ir.h"

struct vm_ir_read_t;
typedef struct vm_ir_read_t vm_ir_read_t;

struct vm_ir_read_t {
  uint8_t *jumps;
  vm_ir_block_t *blocks;
  size_t nops;
  size_t nregs;
  const vm_opcode_t *ops;
};

void vm_ir_read_from(vm_ir_read_t *state, size_t index);
void vm_ir_read(vm_ir_read_t *state, size_t *index);

vm_ir_block_t *vm_ir_parse(size_t nops, const vm_opcode_t *ops);

#endif
