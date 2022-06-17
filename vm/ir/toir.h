
#if !defined(VM_HEADER_IR_TOIR)
#define VM_HEADER_IR_TOIR

#include "./ir.h"
#include "../opcode.h"

struct vm_ir_read_t;
typedef struct vm_ir_read_t vm_ir_read_t;

struct vm_ir_read_t
{
    uint8_t *jumps;
    vm_ir_block_t *blocks;
    size_t nops;
    const vm_opcode_t *ops;
};

void vm_ir_read_from(vm_ir_read_t *state, size_t index);
void vm_ir_read(vm_ir_read_t *state, size_t *index);
void vm_test_toir(size_t nops, const vm_opcode_t *ops);

#endif
