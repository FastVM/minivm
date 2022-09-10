
#include "be/int3.h"
#include "build.h"

vm_ir_block_t *vm_ir_block_opt(vm_int_state_t *state, vm_ir_block_t *block, vm_value_t *locals) {
    // vm_ir_block_t *ret = vm_ir_block_new();
    // uint8_t *used = vm_alloc0(sizeof(uint8_t) * state->framesize);
    // for (size_t i = 0; i < block->len; i++) {
    //     vm_ir_instr_t *instr = block->instrs[i];
    //     vm_ir_instr_t *next = vm_ir_new(vm_ir_instr_t);
    //     vm_ir_block_realloc(ret, next);
    //     next->op = instr->op;
    //     next->out = instr->out;
    //     for (size_t j = 0; instr->args[j].type != VM_IR_ARG_NONE; j++) {
    //         vm_ir_arg_t arg = instr->args[j];
    //         if (arg.type == VM_IR_ARG_REG && vm_typeof(locals[arg.reg]) == VM_TYPE_INT) {
    //             next->args[j] = vm_ir_arg_num(vm_value_to_int(locals[arg.reg]));
    //         } else {
    //             next->args[j] = arg;
    //         }
    //     }
    // }
    // for (size_t i = 0; i < 2; i++) {
    //     ret->branch = block->branch;
    // }
    // vm_free(used);
    return block;
}
