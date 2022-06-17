
#if !defined(VM_HEADER_IR_TOIR)
#define VM_HEADER_IR_TOIR

#include "./ir.h"
#include "../opcode.h"

void vm_ir_read(size_t nops, const vm_opcode_t *ops, size_t *index, vm_ir_block_t *blocks, uint8_t *jumps);
void vm_test_toir(size_t nops, const vm_opcode_t *ops);

#endif
