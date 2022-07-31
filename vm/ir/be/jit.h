#if !defined(VM_HEADER_IR_BE_JIT)
#define VM_HEADER_IR_BE_JIT

#include "../ir.h"

void vm_ir_be_jit_block_impl(void *vDst, void *vgc, void *vstore, vm_ir_block_t *block);
void vm_ir_be_jit(size_t nops, vm_ir_block_t *blocks);

#define vm_ir_be_jit_block(dst, gc, store, block) (vm_ir_be_jit_block_impl((void*) (dst), (void*) (gc), (void*) (store), block))

#endif