#if !defined(VM_HEADER_IR_BE_LUA)
#define VM_HEADER_IR_BE_LUA

#include "../ir.h"

void vm_ir_be_lua_print(FILE *out, vm_ir_arg_t *arg);
void vm_ir_be_lua_print_target(FILE *out, vm_ir_block_t *block, size_t target, size_t indent);
void vm_ir_be_lua_print_body(FILE *out, vm_ir_block_t *block, size_t indent);
void vm_ir_be_lua_print_block(FILE *out, vm_ir_block_t *block);
void vm_ir_be_lua(FILE *file, size_t nops, vm_ir_block_t *blocks);

#endif