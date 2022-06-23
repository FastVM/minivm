#if !defined(VM_HEADER_IR_BE_LUA)
#define VM_HEADER_IR_BE_LUA

#include "../ir.h"

struct vm_ir_be_lua_state_t;
typedef struct vm_ir_be_lua_state_t vm_ir_be_lua_state_t;

struct  vm_ir_be_lua_state_t
{
    FILE *file;
    uint8_t *back;
    size_t *blocks;
    size_t nblocks;
};

void vm_ir_be_lua_print(vm_ir_be_lua_state_t *out, vm_ir_arg_t *arg);
void vm_ir_be_lua_print_target(vm_ir_be_lua_state_t *out, vm_ir_block_t *block, size_t target, size_t indent);
void vm_ir_be_lua_print_body(vm_ir_be_lua_state_t *out, vm_ir_block_t *block, size_t indent);
void vm_ir_be_lua_print_block(vm_ir_be_lua_state_t *out, vm_ir_block_t *block);
void vm_ir_be_lua_main(vm_ir_be_lua_state_t *out, size_t nops, vm_ir_block_t *blocks);
void vm_ir_be_lua(FILE *file, size_t nops, vm_ir_block_t *blocks);

#endif