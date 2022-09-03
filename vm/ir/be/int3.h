#if !defined(VM_HEADER_IR_BE_INT3)
#define VM_HEADER_IR_BE_INT3

#include "../ir.h"
#include "../../opcode.h"
#include "../../gc.h"

typedef vm_value_t (*vm_int_func_ptr_t)(void *ptr, vm_gc_t *gc, size_t nargs, vm_value_t *args); 

typedef struct {
    void *data;
    vm_int_func_ptr_t func;
} vm_int_func_t;

void vm_ir_be_int3(size_t nblocks, vm_ir_block_t *blocks, vm_int_func_t *funcs);
void vm_run_arch_int(size_t nops, vm_opcode_t *opcodes, vm_int_func_t *funcs);

#endif
