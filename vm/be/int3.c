#include "int3.h"

#include "../tag.h"
#include "value.h"

vm_state_t *vm_state_init(size_t nregs) {
    vm_state_t *ret = vm_malloc(sizeof(vm_state_t));
    ret->framesize = 256;
    ret->nlocals = nregs;
    ret->locals = vm_malloc(sizeof(vm_value_t) * (ret->nlocals));
    ret->ips = vm_malloc(sizeof(vm_opcode_t *) * (ret->nlocals / ret->framesize));
    return ret;
}

void vm_state_deinit(vm_state_t *state) {
    vm_free(state->ips);
    vm_free(state->locals);
    vm_free(state);
}

vm_opcode_t *vm_run_comp(vm_state_t *state, vm_block_t *block) {
    if (block->cache) {
        return block->cache;
    }
    size_t aops = 64;
    vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * aops);
    size_t nops = 0;
    for (size_t ninstr = 0; ninstr < block->len; ninstr++) {
        if (nops + 32 >= aops) {
            aops = (nops + 32) * 2;
            ops = vm_realloc(ops, sizeof(vm_opcode_t) * aops);
        }
        vm_instr_t instr = block->instrs[ninstr];
        switch (instr.op) {
            case VM_IOP_MOVE: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I8_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I8_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I16_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I16_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I32_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I32_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I64_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I64_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U8_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U8_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U16_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U16_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U32_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U32_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U64_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U64_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F32) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_F32_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_F32_CONST];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F64) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_F64_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOVE_F64_CONST];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_BNOT: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I8_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I8_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I16_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I16_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I32_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I32_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I64_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I64_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U8_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U8_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U16_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U16_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U32_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U32_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U64_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U64_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_ADD: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_CONST_REG];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_CONST_CONST];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_CONST_REG];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_CONST_CONST];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_SUB: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_CONST_REG];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_CONST_CONST];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_CONST_REG];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_CONST_CONST];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_MUL: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_CONST_REG];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_CONST_CONST];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_CONST_REG];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_CONST_CONST];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_DIV: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_CONST_REG];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_CONST_CONST];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_CONST_REG];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_CONST_CONST];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_MOD: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_CONST_REG];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_CONST_CONST];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        ops[nops++].f32 = (float)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_CONST_REG];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_CONST_CONST];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        ops[nops++].f64 = (double)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_BOR: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_BXOR: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_BAND: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_BSHL: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_BSHR: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_CONST_REG];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_CONST_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        ops[nops++].i8 = (int8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_CONST_REG];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_CONST_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        ops[nops++].i16 = (int16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_CONST_REG];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_CONST_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        ops[nops++].i32 = (int32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_CONST_REG];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_CONST_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        ops[nops++].i64 = (int64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_CONST_REG];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_CONST_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        ops[nops++].u8 = (uint8_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_CONST_REG];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_CONST_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        ops[nops++].u16 = (uint16_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_CONST_REG];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_CONST_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        ops[nops++].u32 = (uint32_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_REG_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_REG_CONST];
                        ops[nops++].reg = instr.args[0].reg;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_CONST_REG];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].reg = instr.args[1].reg;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                    if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_CONST_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        ops[nops++].u64 = (uint64_t)instr.args[1].num;
                        ops[nops++].reg = instr.out.reg;
                        break;
                    }
                }
                goto err;
            }
            case VM_IOP_IN: {
                if (instr.out.type == VM_ARG_NONE) {
                    break;
                }
                if (instr.tag == VM_TAG_I8) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_I8_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_I16) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_I16_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_I32) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_I32_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_I64) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_I64_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_U8) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_U8_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_U16) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_U16_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_U32) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_U32_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_U64) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_U64_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_F32) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_F32_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                if (instr.tag == VM_TAG_F64) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_IN_F64_VOID];
                    ops[nops++].reg = instr.args[0].reg;
                }
                break;
            }
            case VM_IOP_CALL: {
                if (instr.args[1].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST];
                    ops[nops++].func = instr.args[0].func;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[1].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[2].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG];
                    ops[nops++].func = instr.args[0].func;
                    ops[nops++].reg = instr.args[1].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[2].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    ops[nops++].reg = instr.args[1].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[3].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG];
                    ops[nops++].func = instr.args[0].func;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[3].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[4].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG];
                    ops[nops++].func = instr.args[0].func;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[4].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[5].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG];
                    ops[nops++].func = instr.args[0].func;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[5].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[6].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG];
                    ops[nops++].func = instr.args[0].func;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    ops[nops++].reg = instr.args[5].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[6].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    ops[nops++].reg = instr.args[5].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[7].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG];
                    ops[nops++].func = instr.args[0].func;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    ops[nops++].reg = instr.args[5].reg;
                    ops[nops++].reg = instr.args[6].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[7].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    ops[nops++].reg = instr.args[5].reg;
                    ops[nops++].reg = instr.args[6].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[8].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG];
                    ops[nops++].func = instr.args[0].func;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    ops[nops++].reg = instr.args[5].reg;
                    ops[nops++].reg = instr.args[6].reg;
                    ops[nops++].reg = instr.args[7].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[8].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    ops[nops++].reg = instr.args[5].reg;
                    ops[nops++].reg = instr.args[6].reg;
                    ops[nops++].reg = instr.args[7].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[9].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG_REG];
                    ops[nops++].func = instr.args[0].func;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    ops[nops++].reg = instr.args[5].reg;
                    ops[nops++].reg = instr.args[6].reg;
                    ops[nops++].reg = instr.args[7].reg;
                    ops[nops++].reg = instr.args[8].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                if (instr.args[9].type == VM_ARG_NONE) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG_REG_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.args[2].reg;
                    ops[nops++].reg = instr.args[3].reg;
                    ops[nops++].reg = instr.args[4].reg;
                    ops[nops++].reg = instr.args[5].reg;
                    ops[nops++].reg = instr.args[6].reg;
                    ops[nops++].reg = instr.args[7].reg;
                    ops[nops++].reg = instr.args[8].reg;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = 256;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    break;
                }
                goto err;
            }
            case VM_IOP_OUT: {
                if (instr.tag == VM_TAG_I8) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_I8_CONST];
                        ops[nops++].i8 = (int8_t)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_I8_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I16) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_I16_CONST];
                        ops[nops++].i16 = (int16_t)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_I16_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I32) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_I32_CONST];
                        ops[nops++].i32 = (int32_t)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_I32_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_I64) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_I64_CONST];
                        ops[nops++].i64 = (int64_t)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_I64_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U8) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_U8_CONST];
                        ops[nops++].u8 = (uint8_t)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_U8_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U16) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_U16_CONST];
                        ops[nops++].u16 = (uint16_t)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_U16_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U32) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_U32_CONST];
                        ops[nops++].u32 = (uint32_t)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_U32_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_U64) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_U64_CONST];
                        ops[nops++].u64 = (uint64_t)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_U64_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F32) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_F32_CONST];
                        ops[nops++].f32 = (float)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_F32_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
                if (instr.tag == VM_TAG_F64) {
                    if (instr.args[0].type != VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_F64_CONST];
                        ops[nops++].f64 = (double)instr.args[0].num;
                        break;
                    }
                    if (instr.args[0].type == VM_ARG_REG) {
                        ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_F64_REG];
                        ops[nops++].reg = instr.args[0].reg;
                        break;
                    }
                }
            }
            default:
                goto err;
        }
    }
    switch (block->branch.op) {
        case VM_BOP_EXIT: {
            ops[nops++].ptr = state->ptrs[VM_OPCODE_EXIT_BREAK_VOID];
            break;
        }
        case VM_BOP_JUMP: {
            ops[nops++].ptr = state->ptrs[VM_OPCODE_JUMP_FUNC_CONST];
            ops[nops++].func = block->branch.targets[0];
            break;
        }
        case VM_BOP_RET: {
            if (block->branch.tag == VM_TAG_I8) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I8_CONST];
                    ops[nops++].i8 = (int8_t)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I8_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I16) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I16_CONST];
                    ops[nops++].i16 = (int16_t)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I16_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I32_CONST];
                    ops[nops++].i32 = (int32_t)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I32_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I64_CONST];
                    ops[nops++].i64 = (int64_t)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I64_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U8) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U8_CONST];
                    ops[nops++].u8 = (uint8_t)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U8_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U16) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U16_CONST];
                    ops[nops++].u16 = (uint16_t)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U16_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U32_CONST];
                    ops[nops++].u32 = (uint32_t)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U32_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U64_CONST];
                    ops[nops++].u64 = (uint64_t)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U64_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_F32_CONST];
                    ops[nops++].f32 = (float)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_F32_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_F64_CONST];
                    ops[nops++].f64 = (double)block->branch.args[0].num;
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_F64_REG];
                    ops[nops++].reg = block->branch.args[0].reg;
                    break;
                }
            }
            goto err;
        }
        case VM_BOP_BB: {
            if (block->branch.tag == VM_TAG_I8) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I8_CONST_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I8_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_I16) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I16_CONST_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I16_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_I32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I32_CONST_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I32_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_I64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I64_CONST_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I64_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_U8) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U8_CONST_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U8_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_U16) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U16_CONST_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U16_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_U32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U32_CONST_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U32_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_U64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U64_CONST_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U64_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_F32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_F32_CONST_FUNC_FUNC];
                    ops[nops++].f32 = (float)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_F32_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            if (block->branch.tag == VM_TAG_F64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_F64_CONST_FUNC_FUNC];
                    ops[nops++].f64 = (double)block->branch.args[0].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_F64_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                break;
            }
            goto err;
        }
        case VM_BOP_BLT: {
            if (block->branch.tag == VM_TAG_I8) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].i8 = (int8_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_CONST_REG_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t)block->branch.args[0].num;
                    ops[nops++].i8 = (int8_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I16) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].i16 = (int16_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_CONST_REG_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t)block->branch.args[0].num;
                    ops[nops++].i16 = (int16_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].i32 = (int32_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_CONST_REG_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t)block->branch.args[0].num;
                    ops[nops++].i32 = (int32_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].i64 = (int64_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_CONST_REG_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t)block->branch.args[0].num;
                    ops[nops++].i64 = (int64_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U8) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].u8 = (uint8_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_CONST_REG_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t)block->branch.args[0].num;
                    ops[nops++].u8 = (uint8_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U16) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].u16 = (uint16_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_CONST_REG_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t)block->branch.args[0].num;
                    ops[nops++].u16 = (uint16_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].u32 = (uint32_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_CONST_REG_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t)block->branch.args[0].num;
                    ops[nops++].u32 = (uint32_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].u64 = (uint64_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_CONST_REG_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t)block->branch.args[0].num;
                    ops[nops++].u64 = (uint64_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].f32 = (float)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_CONST_REG_FUNC_FUNC];
                    ops[nops++].f32 = (float)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].f32 = (float)block->branch.args[0].num;
                    ops[nops++].f32 = (float)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].f64 = (double)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_CONST_REG_FUNC_FUNC];
                    ops[nops++].f64 = (double)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].f64 = (double)block->branch.args[0].num;
                    ops[nops++].f64 = (double)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            goto err;
        }
        case VM_BOP_BEQ: {
            if (block->branch.tag == VM_TAG_I8) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].i8 = (int8_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_CONST_REG_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t)block->branch.args[0].num;
                    ops[nops++].i8 = (int8_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I16) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].i16 = (int16_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_CONST_REG_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t)block->branch.args[0].num;
                    ops[nops++].i16 = (int16_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].i32 = (int32_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_CONST_REG_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t)block->branch.args[0].num;
                    ops[nops++].i32 = (int32_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].i64 = (int64_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_CONST_REG_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t)block->branch.args[0].num;
                    ops[nops++].i64 = (int64_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U8) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].u8 = (uint8_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_CONST_REG_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t)block->branch.args[0].num;
                    ops[nops++].u8 = (uint8_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U16) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].u16 = (uint16_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_CONST_REG_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t)block->branch.args[0].num;
                    ops[nops++].u16 = (uint16_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].u32 = (uint32_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_CONST_REG_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t)block->branch.args[0].num;
                    ops[nops++].u32 = (uint32_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].u64 = (uint64_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_CONST_REG_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t)block->branch.args[0].num;
                    ops[nops++].u64 = (uint64_t)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].f32 = (float)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_CONST_REG_FUNC_FUNC];
                    ops[nops++].f32 = (float)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].f32 = (float)block->branch.args[0].num;
                    ops[nops++].f32 = (float)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = block->branch.args[0].reg;
                    ops[nops++].f64 = (double)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_CONST_REG_FUNC_FUNC];
                    ops[nops++].f64 = (double)block->branch.args[0].num;
                    ops[nops++].reg = block->branch.args[1].reg;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].f64 = (double)block->branch.args[0].num;
                    ops[nops++].f64 = (double)block->branch.args[1].num;
                    ops[nops++].func = block->branch.targets[0];
                    ops[nops++].func = block->branch.targets[1];
                    break;
                }
            }
            goto err;
        }
        default:
            goto err;
    }
    block->cache = ops;
    return ops;
err:;
    fprintf(stderr, "BAD INSTR!\n");
    __builtin_trap();
}
void vm_run(vm_state_t *state, vm_block_t *block) {
    void *ptrs[] = {
        [VM_OPCODE_ADD_I8_REG_REG] = &&do_add_i8_reg_reg,
        [VM_OPCODE_ADD_I8_REG_CONST] = &&do_add_i8_reg_const,
        [VM_OPCODE_ADD_I8_CONST_REG] = &&do_add_i8_const_reg,
        [VM_OPCODE_ADD_I8_CONST_CONST] = &&do_add_i8_const_const,
        [VM_OPCODE_SUB_I8_REG_REG] = &&do_sub_i8_reg_reg,
        [VM_OPCODE_SUB_I8_REG_CONST] = &&do_sub_i8_reg_const,
        [VM_OPCODE_SUB_I8_CONST_REG] = &&do_sub_i8_const_reg,
        [VM_OPCODE_SUB_I8_CONST_CONST] = &&do_sub_i8_const_const,
        [VM_OPCODE_MUL_I8_REG_REG] = &&do_mul_i8_reg_reg,
        [VM_OPCODE_MUL_I8_REG_CONST] = &&do_mul_i8_reg_const,
        [VM_OPCODE_MUL_I8_CONST_REG] = &&do_mul_i8_const_reg,
        [VM_OPCODE_MUL_I8_CONST_CONST] = &&do_mul_i8_const_const,
        [VM_OPCODE_DIV_I8_REG_REG] = &&do_div_i8_reg_reg,
        [VM_OPCODE_DIV_I8_REG_CONST] = &&do_div_i8_reg_const,
        [VM_OPCODE_DIV_I8_CONST_REG] = &&do_div_i8_const_reg,
        [VM_OPCODE_DIV_I8_CONST_CONST] = &&do_div_i8_const_const,
        [VM_OPCODE_MOD_I8_REG_REG] = &&do_mod_i8_reg_reg,
        [VM_OPCODE_MOD_I8_REG_CONST] = &&do_mod_i8_reg_const,
        [VM_OPCODE_MOD_I8_CONST_REG] = &&do_mod_i8_const_reg,
        [VM_OPCODE_MOD_I8_CONST_CONST] = &&do_mod_i8_const_const,
        [VM_OPCODE_BB_I8_REG_FUNC_FUNC] = &&do_bb_i8_reg_func_func,
        [VM_OPCODE_BB_I8_CONST_FUNC_FUNC] = &&do_bb_i8_const_func_func,
        [VM_OPCODE_BB_I8_REG_PTR_PTR] = &&do_bb_i8_reg_ptr_ptr,
        [VM_OPCODE_BB_I8_CONST_PTR_PTR] = &&do_bb_i8_const_ptr_ptr,
        [VM_OPCODE_BEQ_I8_REG_REG_FUNC_FUNC] = &&do_beq_i8_reg_reg_func_func,
        [VM_OPCODE_BEQ_I8_REG_REG_PTR_PTR] = &&do_beq_i8_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_I8_REG_CONST_FUNC_FUNC] = &&do_beq_i8_reg_const_func_func,
        [VM_OPCODE_BEQ_I8_REG_CONST_PTR_PTR] = &&do_beq_i8_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_I8_CONST_REG_FUNC_FUNC] = &&do_beq_i8_const_reg_func_func,
        [VM_OPCODE_BEQ_I8_CONST_REG_PTR_PTR] = &&do_beq_i8_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_I8_CONST_CONST_FUNC_FUNC] = &&do_beq_i8_const_const_func_func,
        [VM_OPCODE_BEQ_I8_CONST_CONST_PTR_PTR] = &&do_beq_i8_const_const_ptr_ptr,
        [VM_OPCODE_BLT_I8_REG_REG_FUNC_FUNC] = &&do_blt_i8_reg_reg_func_func,
        [VM_OPCODE_BLT_I8_REG_REG_PTR_PTR] = &&do_blt_i8_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_I8_REG_CONST_FUNC_FUNC] = &&do_blt_i8_reg_const_func_func,
        [VM_OPCODE_BLT_I8_REG_CONST_PTR_PTR] = &&do_blt_i8_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_I8_CONST_REG_FUNC_FUNC] = &&do_blt_i8_const_reg_func_func,
        [VM_OPCODE_BLT_I8_CONST_REG_PTR_PTR] = &&do_blt_i8_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_I8_CONST_CONST_FUNC_FUNC] = &&do_blt_i8_const_const_func_func,
        [VM_OPCODE_BLT_I8_CONST_CONST_PTR_PTR] = &&do_blt_i8_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_I8_REG] = &&do_move_i8_reg,
        [VM_OPCODE_MOVE_I8_CONST] = &&do_move_i8_const,
        [VM_OPCODE_OUT_I8_REG] = &&do_out_i8_reg,
        [VM_OPCODE_OUT_I8_CONST] = &&do_out_i8_const,
        [VM_OPCODE_IN_I8_VOID] = &&do_in_i8_void,
        [VM_OPCODE_RET_I8_REG] = &&do_ret_i8_reg,
        [VM_OPCODE_RET_I8_CONST] = &&do_ret_i8_const,
        [VM_OPCODE_BNOT_I8_REG] = &&do_bnot_i8_reg,
        [VM_OPCODE_BNOT_I8_CONST] = &&do_bnot_i8_const,
        [VM_OPCODE_BOR_I8_REG_REG] = &&do_bor_i8_reg_reg,
        [VM_OPCODE_BOR_I8_REG_CONST] = &&do_bor_i8_reg_const,
        [VM_OPCODE_BOR_I8_CONST_REG] = &&do_bor_i8_const_reg,
        [VM_OPCODE_BOR_I8_CONST_CONST] = &&do_bor_i8_const_const,
        [VM_OPCODE_BXOR_I8_REG_REG] = &&do_bxor_i8_reg_reg,
        [VM_OPCODE_BXOR_I8_REG_CONST] = &&do_bxor_i8_reg_const,
        [VM_OPCODE_BXOR_I8_CONST_REG] = &&do_bxor_i8_const_reg,
        [VM_OPCODE_BXOR_I8_CONST_CONST] = &&do_bxor_i8_const_const,
        [VM_OPCODE_BAND_I8_REG_REG] = &&do_band_i8_reg_reg,
        [VM_OPCODE_BAND_I8_REG_CONST] = &&do_band_i8_reg_const,
        [VM_OPCODE_BAND_I8_CONST_REG] = &&do_band_i8_const_reg,
        [VM_OPCODE_BAND_I8_CONST_CONST] = &&do_band_i8_const_const,
        [VM_OPCODE_BSHL_I8_REG_REG] = &&do_bshl_i8_reg_reg,
        [VM_OPCODE_BSHL_I8_REG_CONST] = &&do_bshl_i8_reg_const,
        [VM_OPCODE_BSHL_I8_CONST_REG] = &&do_bshl_i8_const_reg,
        [VM_OPCODE_BSHL_I8_CONST_CONST] = &&do_bshl_i8_const_const,
        [VM_OPCODE_BSHR_I8_REG_REG] = &&do_bshr_i8_reg_reg,
        [VM_OPCODE_BSHR_I8_REG_CONST] = &&do_bshr_i8_reg_const,
        [VM_OPCODE_BSHR_I8_CONST_REG] = &&do_bshr_i8_const_reg,
        [VM_OPCODE_BSHR_I8_CONST_CONST] = &&do_bshr_i8_const_const,
        [VM_OPCODE_ADD_I16_REG_REG] = &&do_add_i16_reg_reg,
        [VM_OPCODE_ADD_I16_REG_CONST] = &&do_add_i16_reg_const,
        [VM_OPCODE_ADD_I16_CONST_REG] = &&do_add_i16_const_reg,
        [VM_OPCODE_ADD_I16_CONST_CONST] = &&do_add_i16_const_const,
        [VM_OPCODE_SUB_I16_REG_REG] = &&do_sub_i16_reg_reg,
        [VM_OPCODE_SUB_I16_REG_CONST] = &&do_sub_i16_reg_const,
        [VM_OPCODE_SUB_I16_CONST_REG] = &&do_sub_i16_const_reg,
        [VM_OPCODE_SUB_I16_CONST_CONST] = &&do_sub_i16_const_const,
        [VM_OPCODE_MUL_I16_REG_REG] = &&do_mul_i16_reg_reg,
        [VM_OPCODE_MUL_I16_REG_CONST] = &&do_mul_i16_reg_const,
        [VM_OPCODE_MUL_I16_CONST_REG] = &&do_mul_i16_const_reg,
        [VM_OPCODE_MUL_I16_CONST_CONST] = &&do_mul_i16_const_const,
        [VM_OPCODE_DIV_I16_REG_REG] = &&do_div_i16_reg_reg,
        [VM_OPCODE_DIV_I16_REG_CONST] = &&do_div_i16_reg_const,
        [VM_OPCODE_DIV_I16_CONST_REG] = &&do_div_i16_const_reg,
        [VM_OPCODE_DIV_I16_CONST_CONST] = &&do_div_i16_const_const,
        [VM_OPCODE_MOD_I16_REG_REG] = &&do_mod_i16_reg_reg,
        [VM_OPCODE_MOD_I16_REG_CONST] = &&do_mod_i16_reg_const,
        [VM_OPCODE_MOD_I16_CONST_REG] = &&do_mod_i16_const_reg,
        [VM_OPCODE_MOD_I16_CONST_CONST] = &&do_mod_i16_const_const,
        [VM_OPCODE_BB_I16_REG_FUNC_FUNC] = &&do_bb_i16_reg_func_func,
        [VM_OPCODE_BB_I16_CONST_FUNC_FUNC] = &&do_bb_i16_const_func_func,
        [VM_OPCODE_BB_I16_REG_PTR_PTR] = &&do_bb_i16_reg_ptr_ptr,
        [VM_OPCODE_BB_I16_CONST_PTR_PTR] = &&do_bb_i16_const_ptr_ptr,
        [VM_OPCODE_BEQ_I16_REG_REG_FUNC_FUNC] = &&do_beq_i16_reg_reg_func_func,
        [VM_OPCODE_BEQ_I16_REG_REG_PTR_PTR] = &&do_beq_i16_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_I16_REG_CONST_FUNC_FUNC] = &&do_beq_i16_reg_const_func_func,
        [VM_OPCODE_BEQ_I16_REG_CONST_PTR_PTR] = &&do_beq_i16_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_I16_CONST_REG_FUNC_FUNC] = &&do_beq_i16_const_reg_func_func,
        [VM_OPCODE_BEQ_I16_CONST_REG_PTR_PTR] = &&do_beq_i16_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_I16_CONST_CONST_FUNC_FUNC] = &&do_beq_i16_const_const_func_func,
        [VM_OPCODE_BEQ_I16_CONST_CONST_PTR_PTR] = &&do_beq_i16_const_const_ptr_ptr,
        [VM_OPCODE_BLT_I16_REG_REG_FUNC_FUNC] = &&do_blt_i16_reg_reg_func_func,
        [VM_OPCODE_BLT_I16_REG_REG_PTR_PTR] = &&do_blt_i16_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_I16_REG_CONST_FUNC_FUNC] = &&do_blt_i16_reg_const_func_func,
        [VM_OPCODE_BLT_I16_REG_CONST_PTR_PTR] = &&do_blt_i16_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_I16_CONST_REG_FUNC_FUNC] = &&do_blt_i16_const_reg_func_func,
        [VM_OPCODE_BLT_I16_CONST_REG_PTR_PTR] = &&do_blt_i16_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_I16_CONST_CONST_FUNC_FUNC] = &&do_blt_i16_const_const_func_func,
        [VM_OPCODE_BLT_I16_CONST_CONST_PTR_PTR] = &&do_blt_i16_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_I16_REG] = &&do_move_i16_reg,
        [VM_OPCODE_MOVE_I16_CONST] = &&do_move_i16_const,
        [VM_OPCODE_OUT_I16_REG] = &&do_out_i16_reg,
        [VM_OPCODE_OUT_I16_CONST] = &&do_out_i16_const,
        [VM_OPCODE_IN_I16_VOID] = &&do_in_i16_void,
        [VM_OPCODE_RET_I16_REG] = &&do_ret_i16_reg,
        [VM_OPCODE_RET_I16_CONST] = &&do_ret_i16_const,
        [VM_OPCODE_BNOT_I16_REG] = &&do_bnot_i16_reg,
        [VM_OPCODE_BNOT_I16_CONST] = &&do_bnot_i16_const,
        [VM_OPCODE_BOR_I16_REG_REG] = &&do_bor_i16_reg_reg,
        [VM_OPCODE_BOR_I16_REG_CONST] = &&do_bor_i16_reg_const,
        [VM_OPCODE_BOR_I16_CONST_REG] = &&do_bor_i16_const_reg,
        [VM_OPCODE_BOR_I16_CONST_CONST] = &&do_bor_i16_const_const,
        [VM_OPCODE_BXOR_I16_REG_REG] = &&do_bxor_i16_reg_reg,
        [VM_OPCODE_BXOR_I16_REG_CONST] = &&do_bxor_i16_reg_const,
        [VM_OPCODE_BXOR_I16_CONST_REG] = &&do_bxor_i16_const_reg,
        [VM_OPCODE_BXOR_I16_CONST_CONST] = &&do_bxor_i16_const_const,
        [VM_OPCODE_BAND_I16_REG_REG] = &&do_band_i16_reg_reg,
        [VM_OPCODE_BAND_I16_REG_CONST] = &&do_band_i16_reg_const,
        [VM_OPCODE_BAND_I16_CONST_REG] = &&do_band_i16_const_reg,
        [VM_OPCODE_BAND_I16_CONST_CONST] = &&do_band_i16_const_const,
        [VM_OPCODE_BSHL_I16_REG_REG] = &&do_bshl_i16_reg_reg,
        [VM_OPCODE_BSHL_I16_REG_CONST] = &&do_bshl_i16_reg_const,
        [VM_OPCODE_BSHL_I16_CONST_REG] = &&do_bshl_i16_const_reg,
        [VM_OPCODE_BSHL_I16_CONST_CONST] = &&do_bshl_i16_const_const,
        [VM_OPCODE_BSHR_I16_REG_REG] = &&do_bshr_i16_reg_reg,
        [VM_OPCODE_BSHR_I16_REG_CONST] = &&do_bshr_i16_reg_const,
        [VM_OPCODE_BSHR_I16_CONST_REG] = &&do_bshr_i16_const_reg,
        [VM_OPCODE_BSHR_I16_CONST_CONST] = &&do_bshr_i16_const_const,
        [VM_OPCODE_ADD_I32_REG_REG] = &&do_add_i32_reg_reg,
        [VM_OPCODE_ADD_I32_REG_CONST] = &&do_add_i32_reg_const,
        [VM_OPCODE_ADD_I32_CONST_REG] = &&do_add_i32_const_reg,
        [VM_OPCODE_ADD_I32_CONST_CONST] = &&do_add_i32_const_const,
        [VM_OPCODE_SUB_I32_REG_REG] = &&do_sub_i32_reg_reg,
        [VM_OPCODE_SUB_I32_REG_CONST] = &&do_sub_i32_reg_const,
        [VM_OPCODE_SUB_I32_CONST_REG] = &&do_sub_i32_const_reg,
        [VM_OPCODE_SUB_I32_CONST_CONST] = &&do_sub_i32_const_const,
        [VM_OPCODE_MUL_I32_REG_REG] = &&do_mul_i32_reg_reg,
        [VM_OPCODE_MUL_I32_REG_CONST] = &&do_mul_i32_reg_const,
        [VM_OPCODE_MUL_I32_CONST_REG] = &&do_mul_i32_const_reg,
        [VM_OPCODE_MUL_I32_CONST_CONST] = &&do_mul_i32_const_const,
        [VM_OPCODE_DIV_I32_REG_REG] = &&do_div_i32_reg_reg,
        [VM_OPCODE_DIV_I32_REG_CONST] = &&do_div_i32_reg_const,
        [VM_OPCODE_DIV_I32_CONST_REG] = &&do_div_i32_const_reg,
        [VM_OPCODE_DIV_I32_CONST_CONST] = &&do_div_i32_const_const,
        [VM_OPCODE_MOD_I32_REG_REG] = &&do_mod_i32_reg_reg,
        [VM_OPCODE_MOD_I32_REG_CONST] = &&do_mod_i32_reg_const,
        [VM_OPCODE_MOD_I32_CONST_REG] = &&do_mod_i32_const_reg,
        [VM_OPCODE_MOD_I32_CONST_CONST] = &&do_mod_i32_const_const,
        [VM_OPCODE_BB_I32_REG_FUNC_FUNC] = &&do_bb_i32_reg_func_func,
        [VM_OPCODE_BB_I32_CONST_FUNC_FUNC] = &&do_bb_i32_const_func_func,
        [VM_OPCODE_BB_I32_REG_PTR_PTR] = &&do_bb_i32_reg_ptr_ptr,
        [VM_OPCODE_BB_I32_CONST_PTR_PTR] = &&do_bb_i32_const_ptr_ptr,
        [VM_OPCODE_BEQ_I32_REG_REG_FUNC_FUNC] = &&do_beq_i32_reg_reg_func_func,
        [VM_OPCODE_BEQ_I32_REG_REG_PTR_PTR] = &&do_beq_i32_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_I32_REG_CONST_FUNC_FUNC] = &&do_beq_i32_reg_const_func_func,
        [VM_OPCODE_BEQ_I32_REG_CONST_PTR_PTR] = &&do_beq_i32_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_I32_CONST_REG_FUNC_FUNC] = &&do_beq_i32_const_reg_func_func,
        [VM_OPCODE_BEQ_I32_CONST_REG_PTR_PTR] = &&do_beq_i32_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_I32_CONST_CONST_FUNC_FUNC] = &&do_beq_i32_const_const_func_func,
        [VM_OPCODE_BEQ_I32_CONST_CONST_PTR_PTR] = &&do_beq_i32_const_const_ptr_ptr,
        [VM_OPCODE_BLT_I32_REG_REG_FUNC_FUNC] = &&do_blt_i32_reg_reg_func_func,
        [VM_OPCODE_BLT_I32_REG_REG_PTR_PTR] = &&do_blt_i32_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_I32_REG_CONST_FUNC_FUNC] = &&do_blt_i32_reg_const_func_func,
        [VM_OPCODE_BLT_I32_REG_CONST_PTR_PTR] = &&do_blt_i32_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_I32_CONST_REG_FUNC_FUNC] = &&do_blt_i32_const_reg_func_func,
        [VM_OPCODE_BLT_I32_CONST_REG_PTR_PTR] = &&do_blt_i32_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_I32_CONST_CONST_FUNC_FUNC] = &&do_blt_i32_const_const_func_func,
        [VM_OPCODE_BLT_I32_CONST_CONST_PTR_PTR] = &&do_blt_i32_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_I32_REG] = &&do_move_i32_reg,
        [VM_OPCODE_MOVE_I32_CONST] = &&do_move_i32_const,
        [VM_OPCODE_OUT_I32_REG] = &&do_out_i32_reg,
        [VM_OPCODE_OUT_I32_CONST] = &&do_out_i32_const,
        [VM_OPCODE_IN_I32_VOID] = &&do_in_i32_void,
        [VM_OPCODE_RET_I32_REG] = &&do_ret_i32_reg,
        [VM_OPCODE_RET_I32_CONST] = &&do_ret_i32_const,
        [VM_OPCODE_BNOT_I32_REG] = &&do_bnot_i32_reg,
        [VM_OPCODE_BNOT_I32_CONST] = &&do_bnot_i32_const,
        [VM_OPCODE_BOR_I32_REG_REG] = &&do_bor_i32_reg_reg,
        [VM_OPCODE_BOR_I32_REG_CONST] = &&do_bor_i32_reg_const,
        [VM_OPCODE_BOR_I32_CONST_REG] = &&do_bor_i32_const_reg,
        [VM_OPCODE_BOR_I32_CONST_CONST] = &&do_bor_i32_const_const,
        [VM_OPCODE_BXOR_I32_REG_REG] = &&do_bxor_i32_reg_reg,
        [VM_OPCODE_BXOR_I32_REG_CONST] = &&do_bxor_i32_reg_const,
        [VM_OPCODE_BXOR_I32_CONST_REG] = &&do_bxor_i32_const_reg,
        [VM_OPCODE_BXOR_I32_CONST_CONST] = &&do_bxor_i32_const_const,
        [VM_OPCODE_BAND_I32_REG_REG] = &&do_band_i32_reg_reg,
        [VM_OPCODE_BAND_I32_REG_CONST] = &&do_band_i32_reg_const,
        [VM_OPCODE_BAND_I32_CONST_REG] = &&do_band_i32_const_reg,
        [VM_OPCODE_BAND_I32_CONST_CONST] = &&do_band_i32_const_const,
        [VM_OPCODE_BSHL_I32_REG_REG] = &&do_bshl_i32_reg_reg,
        [VM_OPCODE_BSHL_I32_REG_CONST] = &&do_bshl_i32_reg_const,
        [VM_OPCODE_BSHL_I32_CONST_REG] = &&do_bshl_i32_const_reg,
        [VM_OPCODE_BSHL_I32_CONST_CONST] = &&do_bshl_i32_const_const,
        [VM_OPCODE_BSHR_I32_REG_REG] = &&do_bshr_i32_reg_reg,
        [VM_OPCODE_BSHR_I32_REG_CONST] = &&do_bshr_i32_reg_const,
        [VM_OPCODE_BSHR_I32_CONST_REG] = &&do_bshr_i32_const_reg,
        [VM_OPCODE_BSHR_I32_CONST_CONST] = &&do_bshr_i32_const_const,
        [VM_OPCODE_ADD_I64_REG_REG] = &&do_add_i64_reg_reg,
        [VM_OPCODE_ADD_I64_REG_CONST] = &&do_add_i64_reg_const,
        [VM_OPCODE_ADD_I64_CONST_REG] = &&do_add_i64_const_reg,
        [VM_OPCODE_ADD_I64_CONST_CONST] = &&do_add_i64_const_const,
        [VM_OPCODE_SUB_I64_REG_REG] = &&do_sub_i64_reg_reg,
        [VM_OPCODE_SUB_I64_REG_CONST] = &&do_sub_i64_reg_const,
        [VM_OPCODE_SUB_I64_CONST_REG] = &&do_sub_i64_const_reg,
        [VM_OPCODE_SUB_I64_CONST_CONST] = &&do_sub_i64_const_const,
        [VM_OPCODE_MUL_I64_REG_REG] = &&do_mul_i64_reg_reg,
        [VM_OPCODE_MUL_I64_REG_CONST] = &&do_mul_i64_reg_const,
        [VM_OPCODE_MUL_I64_CONST_REG] = &&do_mul_i64_const_reg,
        [VM_OPCODE_MUL_I64_CONST_CONST] = &&do_mul_i64_const_const,
        [VM_OPCODE_DIV_I64_REG_REG] = &&do_div_i64_reg_reg,
        [VM_OPCODE_DIV_I64_REG_CONST] = &&do_div_i64_reg_const,
        [VM_OPCODE_DIV_I64_CONST_REG] = &&do_div_i64_const_reg,
        [VM_OPCODE_DIV_I64_CONST_CONST] = &&do_div_i64_const_const,
        [VM_OPCODE_MOD_I64_REG_REG] = &&do_mod_i64_reg_reg,
        [VM_OPCODE_MOD_I64_REG_CONST] = &&do_mod_i64_reg_const,
        [VM_OPCODE_MOD_I64_CONST_REG] = &&do_mod_i64_const_reg,
        [VM_OPCODE_MOD_I64_CONST_CONST] = &&do_mod_i64_const_const,
        [VM_OPCODE_BB_I64_REG_FUNC_FUNC] = &&do_bb_i64_reg_func_func,
        [VM_OPCODE_BB_I64_CONST_FUNC_FUNC] = &&do_bb_i64_const_func_func,
        [VM_OPCODE_BB_I64_REG_PTR_PTR] = &&do_bb_i64_reg_ptr_ptr,
        [VM_OPCODE_BB_I64_CONST_PTR_PTR] = &&do_bb_i64_const_ptr_ptr,
        [VM_OPCODE_BEQ_I64_REG_REG_FUNC_FUNC] = &&do_beq_i64_reg_reg_func_func,
        [VM_OPCODE_BEQ_I64_REG_REG_PTR_PTR] = &&do_beq_i64_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_I64_REG_CONST_FUNC_FUNC] = &&do_beq_i64_reg_const_func_func,
        [VM_OPCODE_BEQ_I64_REG_CONST_PTR_PTR] = &&do_beq_i64_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_I64_CONST_REG_FUNC_FUNC] = &&do_beq_i64_const_reg_func_func,
        [VM_OPCODE_BEQ_I64_CONST_REG_PTR_PTR] = &&do_beq_i64_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_I64_CONST_CONST_FUNC_FUNC] = &&do_beq_i64_const_const_func_func,
        [VM_OPCODE_BEQ_I64_CONST_CONST_PTR_PTR] = &&do_beq_i64_const_const_ptr_ptr,
        [VM_OPCODE_BLT_I64_REG_REG_FUNC_FUNC] = &&do_blt_i64_reg_reg_func_func,
        [VM_OPCODE_BLT_I64_REG_REG_PTR_PTR] = &&do_blt_i64_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_I64_REG_CONST_FUNC_FUNC] = &&do_blt_i64_reg_const_func_func,
        [VM_OPCODE_BLT_I64_REG_CONST_PTR_PTR] = &&do_blt_i64_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_I64_CONST_REG_FUNC_FUNC] = &&do_blt_i64_const_reg_func_func,
        [VM_OPCODE_BLT_I64_CONST_REG_PTR_PTR] = &&do_blt_i64_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_I64_CONST_CONST_FUNC_FUNC] = &&do_blt_i64_const_const_func_func,
        [VM_OPCODE_BLT_I64_CONST_CONST_PTR_PTR] = &&do_blt_i64_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_I64_REG] = &&do_move_i64_reg,
        [VM_OPCODE_MOVE_I64_CONST] = &&do_move_i64_const,
        [VM_OPCODE_OUT_I64_REG] = &&do_out_i64_reg,
        [VM_OPCODE_OUT_I64_CONST] = &&do_out_i64_const,
        [VM_OPCODE_IN_I64_VOID] = &&do_in_i64_void,
        [VM_OPCODE_RET_I64_REG] = &&do_ret_i64_reg,
        [VM_OPCODE_RET_I64_CONST] = &&do_ret_i64_const,
        [VM_OPCODE_BNOT_I64_REG] = &&do_bnot_i64_reg,
        [VM_OPCODE_BNOT_I64_CONST] = &&do_bnot_i64_const,
        [VM_OPCODE_BOR_I64_REG_REG] = &&do_bor_i64_reg_reg,
        [VM_OPCODE_BOR_I64_REG_CONST] = &&do_bor_i64_reg_const,
        [VM_OPCODE_BOR_I64_CONST_REG] = &&do_bor_i64_const_reg,
        [VM_OPCODE_BOR_I64_CONST_CONST] = &&do_bor_i64_const_const,
        [VM_OPCODE_BXOR_I64_REG_REG] = &&do_bxor_i64_reg_reg,
        [VM_OPCODE_BXOR_I64_REG_CONST] = &&do_bxor_i64_reg_const,
        [VM_OPCODE_BXOR_I64_CONST_REG] = &&do_bxor_i64_const_reg,
        [VM_OPCODE_BXOR_I64_CONST_CONST] = &&do_bxor_i64_const_const,
        [VM_OPCODE_BAND_I64_REG_REG] = &&do_band_i64_reg_reg,
        [VM_OPCODE_BAND_I64_REG_CONST] = &&do_band_i64_reg_const,
        [VM_OPCODE_BAND_I64_CONST_REG] = &&do_band_i64_const_reg,
        [VM_OPCODE_BAND_I64_CONST_CONST] = &&do_band_i64_const_const,
        [VM_OPCODE_BSHL_I64_REG_REG] = &&do_bshl_i64_reg_reg,
        [VM_OPCODE_BSHL_I64_REG_CONST] = &&do_bshl_i64_reg_const,
        [VM_OPCODE_BSHL_I64_CONST_REG] = &&do_bshl_i64_const_reg,
        [VM_OPCODE_BSHL_I64_CONST_CONST] = &&do_bshl_i64_const_const,
        [VM_OPCODE_BSHR_I64_REG_REG] = &&do_bshr_i64_reg_reg,
        [VM_OPCODE_BSHR_I64_REG_CONST] = &&do_bshr_i64_reg_const,
        [VM_OPCODE_BSHR_I64_CONST_REG] = &&do_bshr_i64_const_reg,
        [VM_OPCODE_BSHR_I64_CONST_CONST] = &&do_bshr_i64_const_const,
        [VM_OPCODE_ADD_U8_REG_REG] = &&do_add_u8_reg_reg,
        [VM_OPCODE_ADD_U8_REG_CONST] = &&do_add_u8_reg_const,
        [VM_OPCODE_ADD_U8_CONST_REG] = &&do_add_u8_const_reg,
        [VM_OPCODE_ADD_U8_CONST_CONST] = &&do_add_u8_const_const,
        [VM_OPCODE_SUB_U8_REG_REG] = &&do_sub_u8_reg_reg,
        [VM_OPCODE_SUB_U8_REG_CONST] = &&do_sub_u8_reg_const,
        [VM_OPCODE_SUB_U8_CONST_REG] = &&do_sub_u8_const_reg,
        [VM_OPCODE_SUB_U8_CONST_CONST] = &&do_sub_u8_const_const,
        [VM_OPCODE_MUL_U8_REG_REG] = &&do_mul_u8_reg_reg,
        [VM_OPCODE_MUL_U8_REG_CONST] = &&do_mul_u8_reg_const,
        [VM_OPCODE_MUL_U8_CONST_REG] = &&do_mul_u8_const_reg,
        [VM_OPCODE_MUL_U8_CONST_CONST] = &&do_mul_u8_const_const,
        [VM_OPCODE_DIV_U8_REG_REG] = &&do_div_u8_reg_reg,
        [VM_OPCODE_DIV_U8_REG_CONST] = &&do_div_u8_reg_const,
        [VM_OPCODE_DIV_U8_CONST_REG] = &&do_div_u8_const_reg,
        [VM_OPCODE_DIV_U8_CONST_CONST] = &&do_div_u8_const_const,
        [VM_OPCODE_MOD_U8_REG_REG] = &&do_mod_u8_reg_reg,
        [VM_OPCODE_MOD_U8_REG_CONST] = &&do_mod_u8_reg_const,
        [VM_OPCODE_MOD_U8_CONST_REG] = &&do_mod_u8_const_reg,
        [VM_OPCODE_MOD_U8_CONST_CONST] = &&do_mod_u8_const_const,
        [VM_OPCODE_BB_U8_REG_FUNC_FUNC] = &&do_bb_u8_reg_func_func,
        [VM_OPCODE_BB_U8_CONST_FUNC_FUNC] = &&do_bb_u8_const_func_func,
        [VM_OPCODE_BB_U8_REG_PTR_PTR] = &&do_bb_u8_reg_ptr_ptr,
        [VM_OPCODE_BB_U8_CONST_PTR_PTR] = &&do_bb_u8_const_ptr_ptr,
        [VM_OPCODE_BEQ_U8_REG_REG_FUNC_FUNC] = &&do_beq_u8_reg_reg_func_func,
        [VM_OPCODE_BEQ_U8_REG_REG_PTR_PTR] = &&do_beq_u8_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_U8_REG_CONST_FUNC_FUNC] = &&do_beq_u8_reg_const_func_func,
        [VM_OPCODE_BEQ_U8_REG_CONST_PTR_PTR] = &&do_beq_u8_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_U8_CONST_REG_FUNC_FUNC] = &&do_beq_u8_const_reg_func_func,
        [VM_OPCODE_BEQ_U8_CONST_REG_PTR_PTR] = &&do_beq_u8_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_U8_CONST_CONST_FUNC_FUNC] = &&do_beq_u8_const_const_func_func,
        [VM_OPCODE_BEQ_U8_CONST_CONST_PTR_PTR] = &&do_beq_u8_const_const_ptr_ptr,
        [VM_OPCODE_BLT_U8_REG_REG_FUNC_FUNC] = &&do_blt_u8_reg_reg_func_func,
        [VM_OPCODE_BLT_U8_REG_REG_PTR_PTR] = &&do_blt_u8_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_U8_REG_CONST_FUNC_FUNC] = &&do_blt_u8_reg_const_func_func,
        [VM_OPCODE_BLT_U8_REG_CONST_PTR_PTR] = &&do_blt_u8_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_U8_CONST_REG_FUNC_FUNC] = &&do_blt_u8_const_reg_func_func,
        [VM_OPCODE_BLT_U8_CONST_REG_PTR_PTR] = &&do_blt_u8_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_U8_CONST_CONST_FUNC_FUNC] = &&do_blt_u8_const_const_func_func,
        [VM_OPCODE_BLT_U8_CONST_CONST_PTR_PTR] = &&do_blt_u8_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_U8_REG] = &&do_move_u8_reg,
        [VM_OPCODE_MOVE_U8_CONST] = &&do_move_u8_const,
        [VM_OPCODE_OUT_U8_REG] = &&do_out_u8_reg,
        [VM_OPCODE_OUT_U8_CONST] = &&do_out_u8_const,
        [VM_OPCODE_IN_U8_VOID] = &&do_in_u8_void,
        [VM_OPCODE_RET_U8_REG] = &&do_ret_u8_reg,
        [VM_OPCODE_RET_U8_CONST] = &&do_ret_u8_const,
        [VM_OPCODE_BNOT_U8_REG] = &&do_bnot_u8_reg,
        [VM_OPCODE_BNOT_U8_CONST] = &&do_bnot_u8_const,
        [VM_OPCODE_BOR_U8_REG_REG] = &&do_bor_u8_reg_reg,
        [VM_OPCODE_BOR_U8_REG_CONST] = &&do_bor_u8_reg_const,
        [VM_OPCODE_BOR_U8_CONST_REG] = &&do_bor_u8_const_reg,
        [VM_OPCODE_BOR_U8_CONST_CONST] = &&do_bor_u8_const_const,
        [VM_OPCODE_BXOR_U8_REG_REG] = &&do_bxor_u8_reg_reg,
        [VM_OPCODE_BXOR_U8_REG_CONST] = &&do_bxor_u8_reg_const,
        [VM_OPCODE_BXOR_U8_CONST_REG] = &&do_bxor_u8_const_reg,
        [VM_OPCODE_BXOR_U8_CONST_CONST] = &&do_bxor_u8_const_const,
        [VM_OPCODE_BAND_U8_REG_REG] = &&do_band_u8_reg_reg,
        [VM_OPCODE_BAND_U8_REG_CONST] = &&do_band_u8_reg_const,
        [VM_OPCODE_BAND_U8_CONST_REG] = &&do_band_u8_const_reg,
        [VM_OPCODE_BAND_U8_CONST_CONST] = &&do_band_u8_const_const,
        [VM_OPCODE_BSHL_U8_REG_REG] = &&do_bshl_u8_reg_reg,
        [VM_OPCODE_BSHL_U8_REG_CONST] = &&do_bshl_u8_reg_const,
        [VM_OPCODE_BSHL_U8_CONST_REG] = &&do_bshl_u8_const_reg,
        [VM_OPCODE_BSHL_U8_CONST_CONST] = &&do_bshl_u8_const_const,
        [VM_OPCODE_BSHR_U8_REG_REG] = &&do_bshr_u8_reg_reg,
        [VM_OPCODE_BSHR_U8_REG_CONST] = &&do_bshr_u8_reg_const,
        [VM_OPCODE_BSHR_U8_CONST_REG] = &&do_bshr_u8_const_reg,
        [VM_OPCODE_BSHR_U8_CONST_CONST] = &&do_bshr_u8_const_const,
        [VM_OPCODE_ADD_U16_REG_REG] = &&do_add_u16_reg_reg,
        [VM_OPCODE_ADD_U16_REG_CONST] = &&do_add_u16_reg_const,
        [VM_OPCODE_ADD_U16_CONST_REG] = &&do_add_u16_const_reg,
        [VM_OPCODE_ADD_U16_CONST_CONST] = &&do_add_u16_const_const,
        [VM_OPCODE_SUB_U16_REG_REG] = &&do_sub_u16_reg_reg,
        [VM_OPCODE_SUB_U16_REG_CONST] = &&do_sub_u16_reg_const,
        [VM_OPCODE_SUB_U16_CONST_REG] = &&do_sub_u16_const_reg,
        [VM_OPCODE_SUB_U16_CONST_CONST] = &&do_sub_u16_const_const,
        [VM_OPCODE_MUL_U16_REG_REG] = &&do_mul_u16_reg_reg,
        [VM_OPCODE_MUL_U16_REG_CONST] = &&do_mul_u16_reg_const,
        [VM_OPCODE_MUL_U16_CONST_REG] = &&do_mul_u16_const_reg,
        [VM_OPCODE_MUL_U16_CONST_CONST] = &&do_mul_u16_const_const,
        [VM_OPCODE_DIV_U16_REG_REG] = &&do_div_u16_reg_reg,
        [VM_OPCODE_DIV_U16_REG_CONST] = &&do_div_u16_reg_const,
        [VM_OPCODE_DIV_U16_CONST_REG] = &&do_div_u16_const_reg,
        [VM_OPCODE_DIV_U16_CONST_CONST] = &&do_div_u16_const_const,
        [VM_OPCODE_MOD_U16_REG_REG] = &&do_mod_u16_reg_reg,
        [VM_OPCODE_MOD_U16_REG_CONST] = &&do_mod_u16_reg_const,
        [VM_OPCODE_MOD_U16_CONST_REG] = &&do_mod_u16_const_reg,
        [VM_OPCODE_MOD_U16_CONST_CONST] = &&do_mod_u16_const_const,
        [VM_OPCODE_BB_U16_REG_FUNC_FUNC] = &&do_bb_u16_reg_func_func,
        [VM_OPCODE_BB_U16_CONST_FUNC_FUNC] = &&do_bb_u16_const_func_func,
        [VM_OPCODE_BB_U16_REG_PTR_PTR] = &&do_bb_u16_reg_ptr_ptr,
        [VM_OPCODE_BB_U16_CONST_PTR_PTR] = &&do_bb_u16_const_ptr_ptr,
        [VM_OPCODE_BEQ_U16_REG_REG_FUNC_FUNC] = &&do_beq_u16_reg_reg_func_func,
        [VM_OPCODE_BEQ_U16_REG_REG_PTR_PTR] = &&do_beq_u16_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_U16_REG_CONST_FUNC_FUNC] = &&do_beq_u16_reg_const_func_func,
        [VM_OPCODE_BEQ_U16_REG_CONST_PTR_PTR] = &&do_beq_u16_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_U16_CONST_REG_FUNC_FUNC] = &&do_beq_u16_const_reg_func_func,
        [VM_OPCODE_BEQ_U16_CONST_REG_PTR_PTR] = &&do_beq_u16_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_U16_CONST_CONST_FUNC_FUNC] = &&do_beq_u16_const_const_func_func,
        [VM_OPCODE_BEQ_U16_CONST_CONST_PTR_PTR] = &&do_beq_u16_const_const_ptr_ptr,
        [VM_OPCODE_BLT_U16_REG_REG_FUNC_FUNC] = &&do_blt_u16_reg_reg_func_func,
        [VM_OPCODE_BLT_U16_REG_REG_PTR_PTR] = &&do_blt_u16_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_U16_REG_CONST_FUNC_FUNC] = &&do_blt_u16_reg_const_func_func,
        [VM_OPCODE_BLT_U16_REG_CONST_PTR_PTR] = &&do_blt_u16_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_U16_CONST_REG_FUNC_FUNC] = &&do_blt_u16_const_reg_func_func,
        [VM_OPCODE_BLT_U16_CONST_REG_PTR_PTR] = &&do_blt_u16_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_U16_CONST_CONST_FUNC_FUNC] = &&do_blt_u16_const_const_func_func,
        [VM_OPCODE_BLT_U16_CONST_CONST_PTR_PTR] = &&do_blt_u16_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_U16_REG] = &&do_move_u16_reg,
        [VM_OPCODE_MOVE_U16_CONST] = &&do_move_u16_const,
        [VM_OPCODE_OUT_U16_REG] = &&do_out_u16_reg,
        [VM_OPCODE_OUT_U16_CONST] = &&do_out_u16_const,
        [VM_OPCODE_IN_U16_VOID] = &&do_in_u16_void,
        [VM_OPCODE_RET_U16_REG] = &&do_ret_u16_reg,
        [VM_OPCODE_RET_U16_CONST] = &&do_ret_u16_const,
        [VM_OPCODE_BNOT_U16_REG] = &&do_bnot_u16_reg,
        [VM_OPCODE_BNOT_U16_CONST] = &&do_bnot_u16_const,
        [VM_OPCODE_BOR_U16_REG_REG] = &&do_bor_u16_reg_reg,
        [VM_OPCODE_BOR_U16_REG_CONST] = &&do_bor_u16_reg_const,
        [VM_OPCODE_BOR_U16_CONST_REG] = &&do_bor_u16_const_reg,
        [VM_OPCODE_BOR_U16_CONST_CONST] = &&do_bor_u16_const_const,
        [VM_OPCODE_BXOR_U16_REG_REG] = &&do_bxor_u16_reg_reg,
        [VM_OPCODE_BXOR_U16_REG_CONST] = &&do_bxor_u16_reg_const,
        [VM_OPCODE_BXOR_U16_CONST_REG] = &&do_bxor_u16_const_reg,
        [VM_OPCODE_BXOR_U16_CONST_CONST] = &&do_bxor_u16_const_const,
        [VM_OPCODE_BAND_U16_REG_REG] = &&do_band_u16_reg_reg,
        [VM_OPCODE_BAND_U16_REG_CONST] = &&do_band_u16_reg_const,
        [VM_OPCODE_BAND_U16_CONST_REG] = &&do_band_u16_const_reg,
        [VM_OPCODE_BAND_U16_CONST_CONST] = &&do_band_u16_const_const,
        [VM_OPCODE_BSHL_U16_REG_REG] = &&do_bshl_u16_reg_reg,
        [VM_OPCODE_BSHL_U16_REG_CONST] = &&do_bshl_u16_reg_const,
        [VM_OPCODE_BSHL_U16_CONST_REG] = &&do_bshl_u16_const_reg,
        [VM_OPCODE_BSHL_U16_CONST_CONST] = &&do_bshl_u16_const_const,
        [VM_OPCODE_BSHR_U16_REG_REG] = &&do_bshr_u16_reg_reg,
        [VM_OPCODE_BSHR_U16_REG_CONST] = &&do_bshr_u16_reg_const,
        [VM_OPCODE_BSHR_U16_CONST_REG] = &&do_bshr_u16_const_reg,
        [VM_OPCODE_BSHR_U16_CONST_CONST] = &&do_bshr_u16_const_const,
        [VM_OPCODE_ADD_U32_REG_REG] = &&do_add_u32_reg_reg,
        [VM_OPCODE_ADD_U32_REG_CONST] = &&do_add_u32_reg_const,
        [VM_OPCODE_ADD_U32_CONST_REG] = &&do_add_u32_const_reg,
        [VM_OPCODE_ADD_U32_CONST_CONST] = &&do_add_u32_const_const,
        [VM_OPCODE_SUB_U32_REG_REG] = &&do_sub_u32_reg_reg,
        [VM_OPCODE_SUB_U32_REG_CONST] = &&do_sub_u32_reg_const,
        [VM_OPCODE_SUB_U32_CONST_REG] = &&do_sub_u32_const_reg,
        [VM_OPCODE_SUB_U32_CONST_CONST] = &&do_sub_u32_const_const,
        [VM_OPCODE_MUL_U32_REG_REG] = &&do_mul_u32_reg_reg,
        [VM_OPCODE_MUL_U32_REG_CONST] = &&do_mul_u32_reg_const,
        [VM_OPCODE_MUL_U32_CONST_REG] = &&do_mul_u32_const_reg,
        [VM_OPCODE_MUL_U32_CONST_CONST] = &&do_mul_u32_const_const,
        [VM_OPCODE_DIV_U32_REG_REG] = &&do_div_u32_reg_reg,
        [VM_OPCODE_DIV_U32_REG_CONST] = &&do_div_u32_reg_const,
        [VM_OPCODE_DIV_U32_CONST_REG] = &&do_div_u32_const_reg,
        [VM_OPCODE_DIV_U32_CONST_CONST] = &&do_div_u32_const_const,
        [VM_OPCODE_MOD_U32_REG_REG] = &&do_mod_u32_reg_reg,
        [VM_OPCODE_MOD_U32_REG_CONST] = &&do_mod_u32_reg_const,
        [VM_OPCODE_MOD_U32_CONST_REG] = &&do_mod_u32_const_reg,
        [VM_OPCODE_MOD_U32_CONST_CONST] = &&do_mod_u32_const_const,
        [VM_OPCODE_BB_U32_REG_FUNC_FUNC] = &&do_bb_u32_reg_func_func,
        [VM_OPCODE_BB_U32_CONST_FUNC_FUNC] = &&do_bb_u32_const_func_func,
        [VM_OPCODE_BB_U32_REG_PTR_PTR] = &&do_bb_u32_reg_ptr_ptr,
        [VM_OPCODE_BB_U32_CONST_PTR_PTR] = &&do_bb_u32_const_ptr_ptr,
        [VM_OPCODE_BEQ_U32_REG_REG_FUNC_FUNC] = &&do_beq_u32_reg_reg_func_func,
        [VM_OPCODE_BEQ_U32_REG_REG_PTR_PTR] = &&do_beq_u32_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_U32_REG_CONST_FUNC_FUNC] = &&do_beq_u32_reg_const_func_func,
        [VM_OPCODE_BEQ_U32_REG_CONST_PTR_PTR] = &&do_beq_u32_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_U32_CONST_REG_FUNC_FUNC] = &&do_beq_u32_const_reg_func_func,
        [VM_OPCODE_BEQ_U32_CONST_REG_PTR_PTR] = &&do_beq_u32_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_U32_CONST_CONST_FUNC_FUNC] = &&do_beq_u32_const_const_func_func,
        [VM_OPCODE_BEQ_U32_CONST_CONST_PTR_PTR] = &&do_beq_u32_const_const_ptr_ptr,
        [VM_OPCODE_BLT_U32_REG_REG_FUNC_FUNC] = &&do_blt_u32_reg_reg_func_func,
        [VM_OPCODE_BLT_U32_REG_REG_PTR_PTR] = &&do_blt_u32_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_U32_REG_CONST_FUNC_FUNC] = &&do_blt_u32_reg_const_func_func,
        [VM_OPCODE_BLT_U32_REG_CONST_PTR_PTR] = &&do_blt_u32_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_U32_CONST_REG_FUNC_FUNC] = &&do_blt_u32_const_reg_func_func,
        [VM_OPCODE_BLT_U32_CONST_REG_PTR_PTR] = &&do_blt_u32_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_U32_CONST_CONST_FUNC_FUNC] = &&do_blt_u32_const_const_func_func,
        [VM_OPCODE_BLT_U32_CONST_CONST_PTR_PTR] = &&do_blt_u32_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_U32_REG] = &&do_move_u32_reg,
        [VM_OPCODE_MOVE_U32_CONST] = &&do_move_u32_const,
        [VM_OPCODE_OUT_U32_REG] = &&do_out_u32_reg,
        [VM_OPCODE_OUT_U32_CONST] = &&do_out_u32_const,
        [VM_OPCODE_IN_U32_VOID] = &&do_in_u32_void,
        [VM_OPCODE_RET_U32_REG] = &&do_ret_u32_reg,
        [VM_OPCODE_RET_U32_CONST] = &&do_ret_u32_const,
        [VM_OPCODE_BNOT_U32_REG] = &&do_bnot_u32_reg,
        [VM_OPCODE_BNOT_U32_CONST] = &&do_bnot_u32_const,
        [VM_OPCODE_BOR_U32_REG_REG] = &&do_bor_u32_reg_reg,
        [VM_OPCODE_BOR_U32_REG_CONST] = &&do_bor_u32_reg_const,
        [VM_OPCODE_BOR_U32_CONST_REG] = &&do_bor_u32_const_reg,
        [VM_OPCODE_BOR_U32_CONST_CONST] = &&do_bor_u32_const_const,
        [VM_OPCODE_BXOR_U32_REG_REG] = &&do_bxor_u32_reg_reg,
        [VM_OPCODE_BXOR_U32_REG_CONST] = &&do_bxor_u32_reg_const,
        [VM_OPCODE_BXOR_U32_CONST_REG] = &&do_bxor_u32_const_reg,
        [VM_OPCODE_BXOR_U32_CONST_CONST] = &&do_bxor_u32_const_const,
        [VM_OPCODE_BAND_U32_REG_REG] = &&do_band_u32_reg_reg,
        [VM_OPCODE_BAND_U32_REG_CONST] = &&do_band_u32_reg_const,
        [VM_OPCODE_BAND_U32_CONST_REG] = &&do_band_u32_const_reg,
        [VM_OPCODE_BAND_U32_CONST_CONST] = &&do_band_u32_const_const,
        [VM_OPCODE_BSHL_U32_REG_REG] = &&do_bshl_u32_reg_reg,
        [VM_OPCODE_BSHL_U32_REG_CONST] = &&do_bshl_u32_reg_const,
        [VM_OPCODE_BSHL_U32_CONST_REG] = &&do_bshl_u32_const_reg,
        [VM_OPCODE_BSHL_U32_CONST_CONST] = &&do_bshl_u32_const_const,
        [VM_OPCODE_BSHR_U32_REG_REG] = &&do_bshr_u32_reg_reg,
        [VM_OPCODE_BSHR_U32_REG_CONST] = &&do_bshr_u32_reg_const,
        [VM_OPCODE_BSHR_U32_CONST_REG] = &&do_bshr_u32_const_reg,
        [VM_OPCODE_BSHR_U32_CONST_CONST] = &&do_bshr_u32_const_const,
        [VM_OPCODE_ADD_U64_REG_REG] = &&do_add_u64_reg_reg,
        [VM_OPCODE_ADD_U64_REG_CONST] = &&do_add_u64_reg_const,
        [VM_OPCODE_ADD_U64_CONST_REG] = &&do_add_u64_const_reg,
        [VM_OPCODE_ADD_U64_CONST_CONST] = &&do_add_u64_const_const,
        [VM_OPCODE_SUB_U64_REG_REG] = &&do_sub_u64_reg_reg,
        [VM_OPCODE_SUB_U64_REG_CONST] = &&do_sub_u64_reg_const,
        [VM_OPCODE_SUB_U64_CONST_REG] = &&do_sub_u64_const_reg,
        [VM_OPCODE_SUB_U64_CONST_CONST] = &&do_sub_u64_const_const,
        [VM_OPCODE_MUL_U64_REG_REG] = &&do_mul_u64_reg_reg,
        [VM_OPCODE_MUL_U64_REG_CONST] = &&do_mul_u64_reg_const,
        [VM_OPCODE_MUL_U64_CONST_REG] = &&do_mul_u64_const_reg,
        [VM_OPCODE_MUL_U64_CONST_CONST] = &&do_mul_u64_const_const,
        [VM_OPCODE_DIV_U64_REG_REG] = &&do_div_u64_reg_reg,
        [VM_OPCODE_DIV_U64_REG_CONST] = &&do_div_u64_reg_const,
        [VM_OPCODE_DIV_U64_CONST_REG] = &&do_div_u64_const_reg,
        [VM_OPCODE_DIV_U64_CONST_CONST] = &&do_div_u64_const_const,
        [VM_OPCODE_MOD_U64_REG_REG] = &&do_mod_u64_reg_reg,
        [VM_OPCODE_MOD_U64_REG_CONST] = &&do_mod_u64_reg_const,
        [VM_OPCODE_MOD_U64_CONST_REG] = &&do_mod_u64_const_reg,
        [VM_OPCODE_MOD_U64_CONST_CONST] = &&do_mod_u64_const_const,
        [VM_OPCODE_BB_U64_REG_FUNC_FUNC] = &&do_bb_u64_reg_func_func,
        [VM_OPCODE_BB_U64_CONST_FUNC_FUNC] = &&do_bb_u64_const_func_func,
        [VM_OPCODE_BB_U64_REG_PTR_PTR] = &&do_bb_u64_reg_ptr_ptr,
        [VM_OPCODE_BB_U64_CONST_PTR_PTR] = &&do_bb_u64_const_ptr_ptr,
        [VM_OPCODE_BEQ_U64_REG_REG_FUNC_FUNC] = &&do_beq_u64_reg_reg_func_func,
        [VM_OPCODE_BEQ_U64_REG_REG_PTR_PTR] = &&do_beq_u64_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_U64_REG_CONST_FUNC_FUNC] = &&do_beq_u64_reg_const_func_func,
        [VM_OPCODE_BEQ_U64_REG_CONST_PTR_PTR] = &&do_beq_u64_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_U64_CONST_REG_FUNC_FUNC] = &&do_beq_u64_const_reg_func_func,
        [VM_OPCODE_BEQ_U64_CONST_REG_PTR_PTR] = &&do_beq_u64_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_U64_CONST_CONST_FUNC_FUNC] = &&do_beq_u64_const_const_func_func,
        [VM_OPCODE_BEQ_U64_CONST_CONST_PTR_PTR] = &&do_beq_u64_const_const_ptr_ptr,
        [VM_OPCODE_BLT_U64_REG_REG_FUNC_FUNC] = &&do_blt_u64_reg_reg_func_func,
        [VM_OPCODE_BLT_U64_REG_REG_PTR_PTR] = &&do_blt_u64_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_U64_REG_CONST_FUNC_FUNC] = &&do_blt_u64_reg_const_func_func,
        [VM_OPCODE_BLT_U64_REG_CONST_PTR_PTR] = &&do_blt_u64_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_U64_CONST_REG_FUNC_FUNC] = &&do_blt_u64_const_reg_func_func,
        [VM_OPCODE_BLT_U64_CONST_REG_PTR_PTR] = &&do_blt_u64_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_U64_CONST_CONST_FUNC_FUNC] = &&do_blt_u64_const_const_func_func,
        [VM_OPCODE_BLT_U64_CONST_CONST_PTR_PTR] = &&do_blt_u64_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_U64_REG] = &&do_move_u64_reg,
        [VM_OPCODE_MOVE_U64_CONST] = &&do_move_u64_const,
        [VM_OPCODE_OUT_U64_REG] = &&do_out_u64_reg,
        [VM_OPCODE_OUT_U64_CONST] = &&do_out_u64_const,
        [VM_OPCODE_IN_U64_VOID] = &&do_in_u64_void,
        [VM_OPCODE_RET_U64_REG] = &&do_ret_u64_reg,
        [VM_OPCODE_RET_U64_CONST] = &&do_ret_u64_const,
        [VM_OPCODE_BNOT_U64_REG] = &&do_bnot_u64_reg,
        [VM_OPCODE_BNOT_U64_CONST] = &&do_bnot_u64_const,
        [VM_OPCODE_BOR_U64_REG_REG] = &&do_bor_u64_reg_reg,
        [VM_OPCODE_BOR_U64_REG_CONST] = &&do_bor_u64_reg_const,
        [VM_OPCODE_BOR_U64_CONST_REG] = &&do_bor_u64_const_reg,
        [VM_OPCODE_BOR_U64_CONST_CONST] = &&do_bor_u64_const_const,
        [VM_OPCODE_BXOR_U64_REG_REG] = &&do_bxor_u64_reg_reg,
        [VM_OPCODE_BXOR_U64_REG_CONST] = &&do_bxor_u64_reg_const,
        [VM_OPCODE_BXOR_U64_CONST_REG] = &&do_bxor_u64_const_reg,
        [VM_OPCODE_BXOR_U64_CONST_CONST] = &&do_bxor_u64_const_const,
        [VM_OPCODE_BAND_U64_REG_REG] = &&do_band_u64_reg_reg,
        [VM_OPCODE_BAND_U64_REG_CONST] = &&do_band_u64_reg_const,
        [VM_OPCODE_BAND_U64_CONST_REG] = &&do_band_u64_const_reg,
        [VM_OPCODE_BAND_U64_CONST_CONST] = &&do_band_u64_const_const,
        [VM_OPCODE_BSHL_U64_REG_REG] = &&do_bshl_u64_reg_reg,
        [VM_OPCODE_BSHL_U64_REG_CONST] = &&do_bshl_u64_reg_const,
        [VM_OPCODE_BSHL_U64_CONST_REG] = &&do_bshl_u64_const_reg,
        [VM_OPCODE_BSHL_U64_CONST_CONST] = &&do_bshl_u64_const_const,
        [VM_OPCODE_BSHR_U64_REG_REG] = &&do_bshr_u64_reg_reg,
        [VM_OPCODE_BSHR_U64_REG_CONST] = &&do_bshr_u64_reg_const,
        [VM_OPCODE_BSHR_U64_CONST_REG] = &&do_bshr_u64_const_reg,
        [VM_OPCODE_BSHR_U64_CONST_CONST] = &&do_bshr_u64_const_const,
        [VM_OPCODE_ADD_F32_REG_REG] = &&do_add_f32_reg_reg,
        [VM_OPCODE_ADD_F32_REG_CONST] = &&do_add_f32_reg_const,
        [VM_OPCODE_ADD_F32_CONST_REG] = &&do_add_f32_const_reg,
        [VM_OPCODE_ADD_F32_CONST_CONST] = &&do_add_f32_const_const,
        [VM_OPCODE_SUB_F32_REG_REG] = &&do_sub_f32_reg_reg,
        [VM_OPCODE_SUB_F32_REG_CONST] = &&do_sub_f32_reg_const,
        [VM_OPCODE_SUB_F32_CONST_REG] = &&do_sub_f32_const_reg,
        [VM_OPCODE_SUB_F32_CONST_CONST] = &&do_sub_f32_const_const,
        [VM_OPCODE_MUL_F32_REG_REG] = &&do_mul_f32_reg_reg,
        [VM_OPCODE_MUL_F32_REG_CONST] = &&do_mul_f32_reg_const,
        [VM_OPCODE_MUL_F32_CONST_REG] = &&do_mul_f32_const_reg,
        [VM_OPCODE_MUL_F32_CONST_CONST] = &&do_mul_f32_const_const,
        [VM_OPCODE_DIV_F32_REG_REG] = &&do_div_f32_reg_reg,
        [VM_OPCODE_DIV_F32_REG_CONST] = &&do_div_f32_reg_const,
        [VM_OPCODE_DIV_F32_CONST_REG] = &&do_div_f32_const_reg,
        [VM_OPCODE_DIV_F32_CONST_CONST] = &&do_div_f32_const_const,
        [VM_OPCODE_MOD_F32_REG_REG] = &&do_mod_f32_reg_reg,
        [VM_OPCODE_MOD_F32_REG_CONST] = &&do_mod_f32_reg_const,
        [VM_OPCODE_MOD_F32_CONST_REG] = &&do_mod_f32_const_reg,
        [VM_OPCODE_MOD_F32_CONST_CONST] = &&do_mod_f32_const_const,
        [VM_OPCODE_BB_F32_REG_FUNC_FUNC] = &&do_bb_f32_reg_func_func,
        [VM_OPCODE_BB_F32_CONST_FUNC_FUNC] = &&do_bb_f32_const_func_func,
        [VM_OPCODE_BB_F32_REG_PTR_PTR] = &&do_bb_f32_reg_ptr_ptr,
        [VM_OPCODE_BB_F32_CONST_PTR_PTR] = &&do_bb_f32_const_ptr_ptr,
        [VM_OPCODE_BEQ_F32_REG_REG_FUNC_FUNC] = &&do_beq_f32_reg_reg_func_func,
        [VM_OPCODE_BEQ_F32_REG_REG_PTR_PTR] = &&do_beq_f32_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_F32_REG_CONST_FUNC_FUNC] = &&do_beq_f32_reg_const_func_func,
        [VM_OPCODE_BEQ_F32_REG_CONST_PTR_PTR] = &&do_beq_f32_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_F32_CONST_REG_FUNC_FUNC] = &&do_beq_f32_const_reg_func_func,
        [VM_OPCODE_BEQ_F32_CONST_REG_PTR_PTR] = &&do_beq_f32_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_F32_CONST_CONST_FUNC_FUNC] = &&do_beq_f32_const_const_func_func,
        [VM_OPCODE_BEQ_F32_CONST_CONST_PTR_PTR] = &&do_beq_f32_const_const_ptr_ptr,
        [VM_OPCODE_BLT_F32_REG_REG_FUNC_FUNC] = &&do_blt_f32_reg_reg_func_func,
        [VM_OPCODE_BLT_F32_REG_REG_PTR_PTR] = &&do_blt_f32_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_F32_REG_CONST_FUNC_FUNC] = &&do_blt_f32_reg_const_func_func,
        [VM_OPCODE_BLT_F32_REG_CONST_PTR_PTR] = &&do_blt_f32_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_F32_CONST_REG_FUNC_FUNC] = &&do_blt_f32_const_reg_func_func,
        [VM_OPCODE_BLT_F32_CONST_REG_PTR_PTR] = &&do_blt_f32_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_F32_CONST_CONST_FUNC_FUNC] = &&do_blt_f32_const_const_func_func,
        [VM_OPCODE_BLT_F32_CONST_CONST_PTR_PTR] = &&do_blt_f32_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_F32_REG] = &&do_move_f32_reg,
        [VM_OPCODE_MOVE_F32_CONST] = &&do_move_f32_const,
        [VM_OPCODE_OUT_F32_REG] = &&do_out_f32_reg,
        [VM_OPCODE_OUT_F32_CONST] = &&do_out_f32_const,
        [VM_OPCODE_IN_F32_VOID] = &&do_in_f32_void,
        [VM_OPCODE_RET_F32_REG] = &&do_ret_f32_reg,
        [VM_OPCODE_RET_F32_CONST] = &&do_ret_f32_const,
        [VM_OPCODE_ADD_F64_REG_REG] = &&do_add_f64_reg_reg,
        [VM_OPCODE_ADD_F64_REG_CONST] = &&do_add_f64_reg_const,
        [VM_OPCODE_ADD_F64_CONST_REG] = &&do_add_f64_const_reg,
        [VM_OPCODE_ADD_F64_CONST_CONST] = &&do_add_f64_const_const,
        [VM_OPCODE_SUB_F64_REG_REG] = &&do_sub_f64_reg_reg,
        [VM_OPCODE_SUB_F64_REG_CONST] = &&do_sub_f64_reg_const,
        [VM_OPCODE_SUB_F64_CONST_REG] = &&do_sub_f64_const_reg,
        [VM_OPCODE_SUB_F64_CONST_CONST] = &&do_sub_f64_const_const,
        [VM_OPCODE_MUL_F64_REG_REG] = &&do_mul_f64_reg_reg,
        [VM_OPCODE_MUL_F64_REG_CONST] = &&do_mul_f64_reg_const,
        [VM_OPCODE_MUL_F64_CONST_REG] = &&do_mul_f64_const_reg,
        [VM_OPCODE_MUL_F64_CONST_CONST] = &&do_mul_f64_const_const,
        [VM_OPCODE_DIV_F64_REG_REG] = &&do_div_f64_reg_reg,
        [VM_OPCODE_DIV_F64_REG_CONST] = &&do_div_f64_reg_const,
        [VM_OPCODE_DIV_F64_CONST_REG] = &&do_div_f64_const_reg,
        [VM_OPCODE_DIV_F64_CONST_CONST] = &&do_div_f64_const_const,
        [VM_OPCODE_MOD_F64_REG_REG] = &&do_mod_f64_reg_reg,
        [VM_OPCODE_MOD_F64_REG_CONST] = &&do_mod_f64_reg_const,
        [VM_OPCODE_MOD_F64_CONST_REG] = &&do_mod_f64_const_reg,
        [VM_OPCODE_MOD_F64_CONST_CONST] = &&do_mod_f64_const_const,
        [VM_OPCODE_BB_F64_REG_FUNC_FUNC] = &&do_bb_f64_reg_func_func,
        [VM_OPCODE_BB_F64_CONST_FUNC_FUNC] = &&do_bb_f64_const_func_func,
        [VM_OPCODE_BB_F64_REG_PTR_PTR] = &&do_bb_f64_reg_ptr_ptr,
        [VM_OPCODE_BB_F64_CONST_PTR_PTR] = &&do_bb_f64_const_ptr_ptr,
        [VM_OPCODE_BEQ_F64_REG_REG_FUNC_FUNC] = &&do_beq_f64_reg_reg_func_func,
        [VM_OPCODE_BEQ_F64_REG_REG_PTR_PTR] = &&do_beq_f64_reg_reg_ptr_ptr,
        [VM_OPCODE_BEQ_F64_REG_CONST_FUNC_FUNC] = &&do_beq_f64_reg_const_func_func,
        [VM_OPCODE_BEQ_F64_REG_CONST_PTR_PTR] = &&do_beq_f64_reg_const_ptr_ptr,
        [VM_OPCODE_BEQ_F64_CONST_REG_FUNC_FUNC] = &&do_beq_f64_const_reg_func_func,
        [VM_OPCODE_BEQ_F64_CONST_REG_PTR_PTR] = &&do_beq_f64_const_reg_ptr_ptr,
        [VM_OPCODE_BEQ_F64_CONST_CONST_FUNC_FUNC] = &&do_beq_f64_const_const_func_func,
        [VM_OPCODE_BEQ_F64_CONST_CONST_PTR_PTR] = &&do_beq_f64_const_const_ptr_ptr,
        [VM_OPCODE_BLT_F64_REG_REG_FUNC_FUNC] = &&do_blt_f64_reg_reg_func_func,
        [VM_OPCODE_BLT_F64_REG_REG_PTR_PTR] = &&do_blt_f64_reg_reg_ptr_ptr,
        [VM_OPCODE_BLT_F64_REG_CONST_FUNC_FUNC] = &&do_blt_f64_reg_const_func_func,
        [VM_OPCODE_BLT_F64_REG_CONST_PTR_PTR] = &&do_blt_f64_reg_const_ptr_ptr,
        [VM_OPCODE_BLT_F64_CONST_REG_FUNC_FUNC] = &&do_blt_f64_const_reg_func_func,
        [VM_OPCODE_BLT_F64_CONST_REG_PTR_PTR] = &&do_blt_f64_const_reg_ptr_ptr,
        [VM_OPCODE_BLT_F64_CONST_CONST_FUNC_FUNC] = &&do_blt_f64_const_const_func_func,
        [VM_OPCODE_BLT_F64_CONST_CONST_PTR_PTR] = &&do_blt_f64_const_const_ptr_ptr,
        [VM_OPCODE_MOVE_F64_REG] = &&do_move_f64_reg,
        [VM_OPCODE_MOVE_F64_CONST] = &&do_move_f64_const,
        [VM_OPCODE_OUT_F64_REG] = &&do_out_f64_reg,
        [VM_OPCODE_OUT_F64_CONST] = &&do_out_f64_const,
        [VM_OPCODE_IN_F64_VOID] = &&do_in_f64_void,
        [VM_OPCODE_RET_F64_REG] = &&do_ret_f64_reg,
        [VM_OPCODE_RET_F64_CONST] = &&do_ret_f64_const,
        [VM_OPCODE_EXIT_BREAK_VOID] = &&do_exit_break_void,
        [VM_OPCODE_JUMP_FUNC_CONST] = &&do_jump_func_const,
        [VM_OPCODE_CALL_FUNC_CONST] = &&do_call_func_const,
        [VM_OPCODE_CALL_FUNC_REG] = &&do_call_func_reg,
        [VM_OPCODE_CALL_FUNC_CONST_REG] = &&do_call_func_const_reg,
        [VM_OPCODE_CALL_FUNC_REG_REG] = &&do_call_func_reg_reg,
        [VM_OPCODE_CALL_FUNC_CONST_REG_REG] = &&do_call_func_const_reg_reg,
        [VM_OPCODE_CALL_FUNC_REG_REG_REG] = &&do_call_func_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG] = &&do_call_func_const_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_REG_REG_REG_REG] = &&do_call_func_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG] = &&do_call_func_const_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG] = &&do_call_func_reg_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG] = &&do_call_func_const_reg_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG] = &&do_call_func_reg_reg_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG] = &&do_call_func_const_reg_reg_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG] = &&do_call_func_reg_reg_reg_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG] = &&do_call_func_const_reg_reg_reg_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG_REG] = &&do_call_func_reg_reg_reg_reg_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG_REG] = &&do_call_func_const_reg_reg_reg_reg_reg_reg_reg_reg,
        [VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG_REG_REG] = &&do_call_func_reg_reg_reg_reg_reg_reg_reg_reg_reg};
    state->ptrs = ptrs;
    vm_opcode_t *ip = vm_run_comp(state, block);
    vm_value_t *locals = state->locals;
    vm_opcode_t **ips = state->ips;
    goto *(ip++)->ptr;
do_add_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 % a1;
    goto *(ip++)->ptr;
}
do_bb_i8_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_i8_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_i8_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_i8_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_i8_reg_ptr_ptr : {
    int8_t a0 = locals[ip[0].reg].i8;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_i8_const_ptr_ptr : {
    int8_t a0 = ip[0].i8;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i8_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i8_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i8_reg_reg_ptr_ptr : {
    int8_t a0 = locals[ip[0].reg].i8;
    int8_t a1 = ip[1].i8;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i8_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i8_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i8_reg_const_ptr_ptr : {
    int8_t a0 = locals[ip[0].reg].i8;
    int8_t a1 = ip[1].i8;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i8_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i8_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i8_const_reg_ptr_ptr : {
    int8_t a0 = ip[0].i8;
    int8_t a1 = ip[1].i8;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i8_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i8_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i8_const_const_ptr_ptr : {
    int8_t a0 = ip[0].i8;
    int8_t a1 = ip[1].i8;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i8_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i8_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i8_reg_reg_ptr_ptr : {
    int8_t a0 = locals[ip[0].reg].i8;
    int8_t a1 = ip[1].i8;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i8_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i8_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i8_reg_const_ptr_ptr : {
    int8_t a0 = locals[ip[0].reg].i8;
    int8_t a1 = ip[1].i8;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i8_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i8_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i8_const_reg_ptr_ptr : {
    int8_t a0 = ip[0].i8;
    int8_t a1 = ip[1].i8;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i8_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i8_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i8_const_const_ptr_ptr : {
    int8_t a0 = ip[0].i8;
    int8_t a1 = ip[1].i8;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_i8_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0;
    goto *(ip++)->ptr;
}
do_move_i8_const : {
    int8_t a0 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0;
    goto *(ip++)->ptr;
}
do_out_i8_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_i8_const : {
    int8_t a0 = (ip++)->i8;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_i8_void : {
    locals[(ip++)->reg].i8 = (int8_t)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_i8_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].i8 = (int8_t)a0;
    goto *(ip++)->ptr;
}
do_ret_i8_const : {
    int8_t a0 = (ip++)->i8;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].i8 = (int8_t)a0;
    goto *(ip++)->ptr;
}
do_bnot_i8_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = ~a0;
    goto *(ip++)->ptr;
}
do_bnot_i8_const : {
    int8_t a0 = (ip++)->i8;
    locals[(ip++)->reg].i8 = ~a0;
    goto *(ip++)->ptr;
}
do_bor_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bxor_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_band_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 & a1;
    goto *(ip++)->ptr;
}
do_bshl_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshr_i8_reg_reg : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i8_reg_const : {
    int8_t a0 = locals[(ip++)->reg].i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i8_const_reg : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = locals[(ip++)->reg].i8;
    locals[(ip++)->reg].i8 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i8_const_const : {
    int8_t a0 = (ip++)->i8;
    int8_t a1 = (ip++)->i8;
    locals[(ip++)->reg].i8 = a0 << a1;
    goto *(ip++)->ptr;
}
do_add_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 % a1;
    goto *(ip++)->ptr;
}
do_bb_i16_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_i16_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_i16_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_i16_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_i16_reg_ptr_ptr : {
    int16_t a0 = locals[ip[0].reg].i16;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_i16_const_ptr_ptr : {
    int16_t a0 = ip[0].i16;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i16_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i16_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i16_reg_reg_ptr_ptr : {
    int16_t a0 = locals[ip[0].reg].i16;
    int16_t a1 = ip[1].i16;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i16_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i16_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i16_reg_const_ptr_ptr : {
    int16_t a0 = locals[ip[0].reg].i16;
    int16_t a1 = ip[1].i16;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i16_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i16_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i16_const_reg_ptr_ptr : {
    int16_t a0 = ip[0].i16;
    int16_t a1 = ip[1].i16;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i16_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i16_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i16_const_const_ptr_ptr : {
    int16_t a0 = ip[0].i16;
    int16_t a1 = ip[1].i16;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i16_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i16_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i16_reg_reg_ptr_ptr : {
    int16_t a0 = locals[ip[0].reg].i16;
    int16_t a1 = ip[1].i16;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i16_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i16_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i16_reg_const_ptr_ptr : {
    int16_t a0 = locals[ip[0].reg].i16;
    int16_t a1 = ip[1].i16;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i16_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i16_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i16_const_reg_ptr_ptr : {
    int16_t a0 = ip[0].i16;
    int16_t a1 = ip[1].i16;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i16_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i16_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i16_const_const_ptr_ptr : {
    int16_t a0 = ip[0].i16;
    int16_t a1 = ip[1].i16;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_i16_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0;
    goto *(ip++)->ptr;
}
do_move_i16_const : {
    int16_t a0 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0;
    goto *(ip++)->ptr;
}
do_out_i16_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_i16_const : {
    int16_t a0 = (ip++)->i16;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_i16_void : {
    locals[(ip++)->reg].i16 = (int16_t)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_i16_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].i16 = (int16_t)a0;
    goto *(ip++)->ptr;
}
do_ret_i16_const : {
    int16_t a0 = (ip++)->i16;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].i16 = (int16_t)a0;
    goto *(ip++)->ptr;
}
do_bnot_i16_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = ~a0;
    goto *(ip++)->ptr;
}
do_bnot_i16_const : {
    int16_t a0 = (ip++)->i16;
    locals[(ip++)->reg].i16 = ~a0;
    goto *(ip++)->ptr;
}
do_bor_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bxor_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_band_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 & a1;
    goto *(ip++)->ptr;
}
do_bshl_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshr_i16_reg_reg : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i16_reg_const : {
    int16_t a0 = locals[(ip++)->reg].i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i16_const_reg : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = locals[(ip++)->reg].i16;
    locals[(ip++)->reg].i16 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i16_const_const : {
    int16_t a0 = (ip++)->i16;
    int16_t a1 = (ip++)->i16;
    locals[(ip++)->reg].i16 = a0 << a1;
    goto *(ip++)->ptr;
}
do_add_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 % a1;
    goto *(ip++)->ptr;
}
do_bb_i32_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_i32_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_i32_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_i32_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_i32_reg_ptr_ptr : {
    int32_t a0 = locals[ip[0].reg].i32;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_i32_const_ptr_ptr : {
    int32_t a0 = ip[0].i32;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i32_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i32_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i32_reg_reg_ptr_ptr : {
    int32_t a0 = locals[ip[0].reg].i32;
    int32_t a1 = ip[1].i32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i32_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i32_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i32_reg_const_ptr_ptr : {
    int32_t a0 = locals[ip[0].reg].i32;
    int32_t a1 = ip[1].i32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i32_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i32_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i32_const_reg_ptr_ptr : {
    int32_t a0 = ip[0].i32;
    int32_t a1 = ip[1].i32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i32_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i32_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i32_const_const_ptr_ptr : {
    int32_t a0 = ip[0].i32;
    int32_t a1 = ip[1].i32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i32_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i32_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i32_reg_reg_ptr_ptr : {
    int32_t a0 = locals[ip[0].reg].i32;
    int32_t a1 = ip[1].i32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i32_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i32_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i32_reg_const_ptr_ptr : {
    int32_t a0 = locals[ip[0].reg].i32;
    int32_t a1 = ip[1].i32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i32_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i32_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i32_const_reg_ptr_ptr : {
    int32_t a0 = ip[0].i32;
    int32_t a1 = ip[1].i32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i32_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i32_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i32_const_const_ptr_ptr : {
    int32_t a0 = ip[0].i32;
    int32_t a1 = ip[1].i32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_i32_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0;
    goto *(ip++)->ptr;
}
do_move_i32_const : {
    int32_t a0 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0;
    goto *(ip++)->ptr;
}
do_out_i32_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_i32_const : {
    int32_t a0 = (ip++)->i32;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_i32_void : {
    locals[(ip++)->reg].i32 = (int32_t)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_i32_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].i32 = (int32_t)a0;
    goto *(ip++)->ptr;
}
do_ret_i32_const : {
    int32_t a0 = (ip++)->i32;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].i32 = (int32_t)a0;
    goto *(ip++)->ptr;
}
do_bnot_i32_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = ~a0;
    goto *(ip++)->ptr;
}
do_bnot_i32_const : {
    int32_t a0 = (ip++)->i32;
    locals[(ip++)->reg].i32 = ~a0;
    goto *(ip++)->ptr;
}
do_bor_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bxor_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_band_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 & a1;
    goto *(ip++)->ptr;
}
do_bshl_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshr_i32_reg_reg : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i32_reg_const : {
    int32_t a0 = locals[(ip++)->reg].i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i32_const_reg : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = locals[(ip++)->reg].i32;
    locals[(ip++)->reg].i32 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i32_const_const : {
    int32_t a0 = (ip++)->i32;
    int32_t a1 = (ip++)->i32;
    locals[(ip++)->reg].i32 = a0 << a1;
    goto *(ip++)->ptr;
}
do_add_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 % a1;
    goto *(ip++)->ptr;
}
do_bb_i64_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_i64_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_i64_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_i64_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_i64_reg_ptr_ptr : {
    int64_t a0 = locals[ip[0].reg].i64;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_i64_const_ptr_ptr : {
    int64_t a0 = ip[0].i64;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i64_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i64_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i64_reg_reg_ptr_ptr : {
    int64_t a0 = locals[ip[0].reg].i64;
    int64_t a1 = ip[1].i64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i64_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i64_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i64_reg_const_ptr_ptr : {
    int64_t a0 = locals[ip[0].reg].i64;
    int64_t a1 = ip[1].i64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i64_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i64_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i64_const_reg_ptr_ptr : {
    int64_t a0 = ip[0].i64;
    int64_t a1 = ip[1].i64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_i64_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_i64_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_i64_const_const_ptr_ptr : {
    int64_t a0 = ip[0].i64;
    int64_t a1 = ip[1].i64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i64_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i64_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i64_reg_reg_ptr_ptr : {
    int64_t a0 = locals[ip[0].reg].i64;
    int64_t a1 = ip[1].i64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i64_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i64_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i64_reg_const_ptr_ptr : {
    int64_t a0 = locals[ip[0].reg].i64;
    int64_t a1 = ip[1].i64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i64_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i64_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i64_const_reg_ptr_ptr : {
    int64_t a0 = ip[0].i64;
    int64_t a1 = ip[1].i64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_i64_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_i64_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_i64_const_const_ptr_ptr : {
    int64_t a0 = ip[0].i64;
    int64_t a1 = ip[1].i64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_i64_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0;
    goto *(ip++)->ptr;
}
do_move_i64_const : {
    int64_t a0 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0;
    goto *(ip++)->ptr;
}
do_out_i64_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_i64_const : {
    int64_t a0 = (ip++)->i64;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_i64_void : {
    locals[(ip++)->reg].i64 = (int64_t)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_i64_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].i64 = (int64_t)a0;
    goto *(ip++)->ptr;
}
do_ret_i64_const : {
    int64_t a0 = (ip++)->i64;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].i64 = (int64_t)a0;
    goto *(ip++)->ptr;
}
do_bnot_i64_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = ~a0;
    goto *(ip++)->ptr;
}
do_bnot_i64_const : {
    int64_t a0 = (ip++)->i64;
    locals[(ip++)->reg].i64 = ~a0;
    goto *(ip++)->ptr;
}
do_bor_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bxor_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_band_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 & a1;
    goto *(ip++)->ptr;
}
do_bshl_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshr_i64_reg_reg : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i64_reg_const : {
    int64_t a0 = locals[(ip++)->reg].i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i64_const_reg : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = locals[(ip++)->reg].i64;
    locals[(ip++)->reg].i64 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_i64_const_const : {
    int64_t a0 = (ip++)->i64;
    int64_t a1 = (ip++)->i64;
    locals[(ip++)->reg].i64 = a0 << a1;
    goto *(ip++)->ptr;
}
do_add_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 % a1;
    goto *(ip++)->ptr;
}
do_bb_u8_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_u8_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_u8_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_u8_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_u8_reg_ptr_ptr : {
    uint8_t a0 = locals[ip[0].reg].u8;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_u8_const_ptr_ptr : {
    uint8_t a0 = ip[0].u8;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u8_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u8_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u8_reg_reg_ptr_ptr : {
    uint8_t a0 = locals[ip[0].reg].u8;
    uint8_t a1 = ip[1].u8;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u8_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u8_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u8_reg_const_ptr_ptr : {
    uint8_t a0 = locals[ip[0].reg].u8;
    uint8_t a1 = ip[1].u8;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u8_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u8_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u8_const_reg_ptr_ptr : {
    uint8_t a0 = ip[0].u8;
    uint8_t a1 = ip[1].u8;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u8_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u8_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u8_const_const_ptr_ptr : {
    uint8_t a0 = ip[0].u8;
    uint8_t a1 = ip[1].u8;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u8_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u8_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u8_reg_reg_ptr_ptr : {
    uint8_t a0 = locals[ip[0].reg].u8;
    uint8_t a1 = ip[1].u8;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u8_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u8_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u8_reg_const_ptr_ptr : {
    uint8_t a0 = locals[ip[0].reg].u8;
    uint8_t a1 = ip[1].u8;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u8_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u8_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u8_const_reg_ptr_ptr : {
    uint8_t a0 = ip[0].u8;
    uint8_t a1 = ip[1].u8;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u8_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u8_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u8_const_const_ptr_ptr : {
    uint8_t a0 = ip[0].u8;
    uint8_t a1 = ip[1].u8;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_u8_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0;
    goto *(ip++)->ptr;
}
do_move_u8_const : {
    uint8_t a0 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0;
    goto *(ip++)->ptr;
}
do_out_u8_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_u8_const : {
    uint8_t a0 = (ip++)->u8;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_u8_void : {
    locals[(ip++)->reg].u8 = (uint8_t)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_u8_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].u8 = (uint8_t)a0;
    goto *(ip++)->ptr;
}
do_ret_u8_const : {
    uint8_t a0 = (ip++)->u8;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].u8 = (uint8_t)a0;
    goto *(ip++)->ptr;
}
do_bnot_u8_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = ~a0;
    goto *(ip++)->ptr;
}
do_bnot_u8_const : {
    uint8_t a0 = (ip++)->u8;
    locals[(ip++)->reg].u8 = ~a0;
    goto *(ip++)->ptr;
}
do_bor_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bxor_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_band_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 & a1;
    goto *(ip++)->ptr;
}
do_bshl_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshr_u8_reg_reg : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u8_reg_const : {
    uint8_t a0 = locals[(ip++)->reg].u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u8_const_reg : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = locals[(ip++)->reg].u8;
    locals[(ip++)->reg].u8 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u8_const_const : {
    uint8_t a0 = (ip++)->u8;
    uint8_t a1 = (ip++)->u8;
    locals[(ip++)->reg].u8 = a0 << a1;
    goto *(ip++)->ptr;
}
do_add_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 % a1;
    goto *(ip++)->ptr;
}
do_bb_u16_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_u16_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_u16_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_u16_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_u16_reg_ptr_ptr : {
    uint16_t a0 = locals[ip[0].reg].u16;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_u16_const_ptr_ptr : {
    uint16_t a0 = ip[0].u16;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u16_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u16_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u16_reg_reg_ptr_ptr : {
    uint16_t a0 = locals[ip[0].reg].u16;
    uint16_t a1 = ip[1].u16;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u16_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u16_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u16_reg_const_ptr_ptr : {
    uint16_t a0 = locals[ip[0].reg].u16;
    uint16_t a1 = ip[1].u16;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u16_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u16_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u16_const_reg_ptr_ptr : {
    uint16_t a0 = ip[0].u16;
    uint16_t a1 = ip[1].u16;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u16_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u16_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u16_const_const_ptr_ptr : {
    uint16_t a0 = ip[0].u16;
    uint16_t a1 = ip[1].u16;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u16_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u16_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u16_reg_reg_ptr_ptr : {
    uint16_t a0 = locals[ip[0].reg].u16;
    uint16_t a1 = ip[1].u16;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u16_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u16_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u16_reg_const_ptr_ptr : {
    uint16_t a0 = locals[ip[0].reg].u16;
    uint16_t a1 = ip[1].u16;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u16_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u16_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u16_const_reg_ptr_ptr : {
    uint16_t a0 = ip[0].u16;
    uint16_t a1 = ip[1].u16;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u16_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u16_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u16_const_const_ptr_ptr : {
    uint16_t a0 = ip[0].u16;
    uint16_t a1 = ip[1].u16;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_u16_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0;
    goto *(ip++)->ptr;
}
do_move_u16_const : {
    uint16_t a0 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0;
    goto *(ip++)->ptr;
}
do_out_u16_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_u16_const : {
    uint16_t a0 = (ip++)->u16;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_u16_void : {
    locals[(ip++)->reg].u16 = (uint16_t)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_u16_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].u16 = (uint16_t)a0;
    goto *(ip++)->ptr;
}
do_ret_u16_const : {
    uint16_t a0 = (ip++)->u16;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].u16 = (uint16_t)a0;
    goto *(ip++)->ptr;
}
do_bnot_u16_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = ~a0;
    goto *(ip++)->ptr;
}
do_bnot_u16_const : {
    uint16_t a0 = (ip++)->u16;
    locals[(ip++)->reg].u16 = ~a0;
    goto *(ip++)->ptr;
}
do_bor_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bxor_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_band_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 & a1;
    goto *(ip++)->ptr;
}
do_bshl_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshr_u16_reg_reg : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u16_reg_const : {
    uint16_t a0 = locals[(ip++)->reg].u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u16_const_reg : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = locals[(ip++)->reg].u16;
    locals[(ip++)->reg].u16 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u16_const_const : {
    uint16_t a0 = (ip++)->u16;
    uint16_t a1 = (ip++)->u16;
    locals[(ip++)->reg].u16 = a0 << a1;
    goto *(ip++)->ptr;
}
do_add_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 % a1;
    goto *(ip++)->ptr;
}
do_bb_u32_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_u32_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_u32_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_u32_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_u32_reg_ptr_ptr : {
    uint32_t a0 = locals[ip[0].reg].u32;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_u32_const_ptr_ptr : {
    uint32_t a0 = ip[0].u32;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u32_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u32_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u32_reg_reg_ptr_ptr : {
    uint32_t a0 = locals[ip[0].reg].u32;
    uint32_t a1 = ip[1].u32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u32_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u32_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u32_reg_const_ptr_ptr : {
    uint32_t a0 = locals[ip[0].reg].u32;
    uint32_t a1 = ip[1].u32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u32_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u32_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u32_const_reg_ptr_ptr : {
    uint32_t a0 = ip[0].u32;
    uint32_t a1 = ip[1].u32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u32_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u32_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u32_const_const_ptr_ptr : {
    uint32_t a0 = ip[0].u32;
    uint32_t a1 = ip[1].u32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u32_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u32_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u32_reg_reg_ptr_ptr : {
    uint32_t a0 = locals[ip[0].reg].u32;
    uint32_t a1 = ip[1].u32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u32_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u32_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u32_reg_const_ptr_ptr : {
    uint32_t a0 = locals[ip[0].reg].u32;
    uint32_t a1 = ip[1].u32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u32_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u32_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u32_const_reg_ptr_ptr : {
    uint32_t a0 = ip[0].u32;
    uint32_t a1 = ip[1].u32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u32_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u32_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u32_const_const_ptr_ptr : {
    uint32_t a0 = ip[0].u32;
    uint32_t a1 = ip[1].u32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_u32_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0;
    goto *(ip++)->ptr;
}
do_move_u32_const : {
    uint32_t a0 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0;
    goto *(ip++)->ptr;
}
do_out_u32_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_u32_const : {
    uint32_t a0 = (ip++)->u32;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_u32_void : {
    locals[(ip++)->reg].u32 = (uint32_t)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_u32_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].u32 = (uint32_t)a0;
    goto *(ip++)->ptr;
}
do_ret_u32_const : {
    uint32_t a0 = (ip++)->u32;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].u32 = (uint32_t)a0;
    goto *(ip++)->ptr;
}
do_bnot_u32_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = ~a0;
    goto *(ip++)->ptr;
}
do_bnot_u32_const : {
    uint32_t a0 = (ip++)->u32;
    locals[(ip++)->reg].u32 = ~a0;
    goto *(ip++)->ptr;
}
do_bor_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bxor_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_band_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 & a1;
    goto *(ip++)->ptr;
}
do_bshl_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshr_u32_reg_reg : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u32_reg_const : {
    uint32_t a0 = locals[(ip++)->reg].u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u32_const_reg : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = locals[(ip++)->reg].u32;
    locals[(ip++)->reg].u32 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u32_const_const : {
    uint32_t a0 = (ip++)->u32;
    uint32_t a1 = (ip++)->u32;
    locals[(ip++)->reg].u32 = a0 << a1;
    goto *(ip++)->ptr;
}
do_add_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 % a1;
    goto *(ip++)->ptr;
}
do_mod_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 % a1;
    goto *(ip++)->ptr;
}
do_bb_u64_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_u64_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_u64_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_u64_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_u64_reg_ptr_ptr : {
    uint64_t a0 = locals[ip[0].reg].u64;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_u64_const_ptr_ptr : {
    uint64_t a0 = ip[0].u64;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u64_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u64_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u64_reg_reg_ptr_ptr : {
    uint64_t a0 = locals[ip[0].reg].u64;
    uint64_t a1 = ip[1].u64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u64_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u64_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u64_reg_const_ptr_ptr : {
    uint64_t a0 = locals[ip[0].reg].u64;
    uint64_t a1 = ip[1].u64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u64_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u64_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u64_const_reg_ptr_ptr : {
    uint64_t a0 = ip[0].u64;
    uint64_t a1 = ip[1].u64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_u64_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_u64_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_u64_const_const_ptr_ptr : {
    uint64_t a0 = ip[0].u64;
    uint64_t a1 = ip[1].u64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u64_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u64_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u64_reg_reg_ptr_ptr : {
    uint64_t a0 = locals[ip[0].reg].u64;
    uint64_t a1 = ip[1].u64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u64_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u64_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u64_reg_const_ptr_ptr : {
    uint64_t a0 = locals[ip[0].reg].u64;
    uint64_t a1 = ip[1].u64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u64_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u64_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u64_const_reg_ptr_ptr : {
    uint64_t a0 = ip[0].u64;
    uint64_t a1 = ip[1].u64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_u64_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_u64_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_u64_const_const_ptr_ptr : {
    uint64_t a0 = ip[0].u64;
    uint64_t a1 = ip[1].u64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_u64_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0;
    goto *(ip++)->ptr;
}
do_move_u64_const : {
    uint64_t a0 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0;
    goto *(ip++)->ptr;
}
do_out_u64_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_u64_const : {
    uint64_t a0 = (ip++)->u64;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_u64_void : {
    locals[(ip++)->reg].u64 = (uint64_t)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_u64_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].u64 = (uint64_t)a0;
    goto *(ip++)->ptr;
}
do_ret_u64_const : {
    uint64_t a0 = (ip++)->u64;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].u64 = (uint64_t)a0;
    goto *(ip++)->ptr;
}
do_bnot_u64_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = ~a0;
    goto *(ip++)->ptr;
}
do_bnot_u64_const : {
    uint64_t a0 = (ip++)->u64;
    locals[(ip++)->reg].u64 = ~a0;
    goto *(ip++)->ptr;
}
do_bor_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bor_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 | a1;
    goto *(ip++)->ptr;
}
do_bxor_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_bxor_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 ^ a1;
    goto *(ip++)->ptr;
}
do_band_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 & a1;
    goto *(ip++)->ptr;
}
do_band_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 & a1;
    goto *(ip++)->ptr;
}
do_bshl_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshl_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 >> a1;
    goto *(ip++)->ptr;
}
do_bshr_u64_reg_reg : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u64_reg_const : {
    uint64_t a0 = locals[(ip++)->reg].u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u64_const_reg : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = locals[(ip++)->reg].u64;
    locals[(ip++)->reg].u64 = a0 << a1;
    goto *(ip++)->ptr;
}
do_bshr_u64_const_const : {
    uint64_t a0 = (ip++)->u64;
    uint64_t a1 = (ip++)->u64;
    locals[(ip++)->reg].u64 = a0 << a1;
    goto *(ip++)->ptr;
}
do_add_f32_reg_reg : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_f32_reg_const : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_f32_const_reg : {
    float a0 = (ip++)->f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_f32_const_const : {
    float a0 = (ip++)->f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_f32_reg_reg : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_f32_reg_const : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_f32_const_reg : {
    float a0 = (ip++)->f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_f32_const_const : {
    float a0 = (ip++)->f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_f32_reg_reg : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_f32_reg_const : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_f32_const_reg : {
    float a0 = (ip++)->f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_f32_const_const : {
    float a0 = (ip++)->f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_f32_reg_reg : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_f32_reg_const : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_f32_const_reg : {
    float a0 = (ip++)->f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_f32_const_const : {
    float a0 = (ip++)->f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_f32_reg_reg : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = fmodf(a0, a1);
    goto *(ip++)->ptr;
}
do_mod_f32_reg_const : {
    float a0 = locals[(ip++)->reg].f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = fmodf(a0, a1);
    goto *(ip++)->ptr;
}
do_mod_f32_const_reg : {
    float a0 = (ip++)->f32;
    float a1 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = fmodf(a0, a1);
    goto *(ip++)->ptr;
}
do_mod_f32_const_const : {
    float a0 = (ip++)->f32;
    float a1 = (ip++)->f32;
    locals[(ip++)->reg].f32 = fmodf(a0, a1);
    goto *(ip++)->ptr;
}
do_bb_f32_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_f32_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_f32_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_f32_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_f32_reg_ptr_ptr : {
    float a0 = locals[ip[0].reg].f32;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_f32_const_ptr_ptr : {
    float a0 = ip[0].f32;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_f32_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_f32_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_f32_reg_reg_ptr_ptr : {
    float a0 = locals[ip[0].reg].f32;
    float a1 = ip[1].f32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_f32_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_f32_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_f32_reg_const_ptr_ptr : {
    float a0 = locals[ip[0].reg].f32;
    float a1 = ip[1].f32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_f32_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_f32_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_f32_const_reg_ptr_ptr : {
    float a0 = ip[0].f32;
    float a1 = ip[1].f32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_f32_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_f32_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_f32_const_const_ptr_ptr : {
    float a0 = ip[0].f32;
    float a1 = ip[1].f32;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_f32_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_f32_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_f32_reg_reg_ptr_ptr : {
    float a0 = locals[ip[0].reg].f32;
    float a1 = ip[1].f32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_f32_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_f32_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_f32_reg_const_ptr_ptr : {
    float a0 = locals[ip[0].reg].f32;
    float a1 = ip[1].f32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_f32_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_f32_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_f32_const_reg_ptr_ptr : {
    float a0 = ip[0].f32;
    float a1 = ip[1].f32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_f32_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_f32_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_f32_const_const_ptr_ptr : {
    float a0 = ip[0].f32;
    float a1 = ip[1].f32;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_f32_reg : {
    float a0 = locals[(ip++)->reg].f32;
    locals[(ip++)->reg].f32 = a0;
    goto *(ip++)->ptr;
}
do_move_f32_const : {
    float a0 = (ip++)->f32;
    locals[(ip++)->reg].f32 = a0;
    goto *(ip++)->ptr;
}
do_out_f32_reg : {
    float a0 = locals[(ip++)->reg].f32;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_f32_const : {
    float a0 = (ip++)->f32;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_f32_void : {
    locals[(ip++)->reg].f32 = (float)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_f32_reg : {
    float a0 = locals[(ip++)->reg].f32;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].f32 = (float)a0;
    goto *(ip++)->ptr;
}
do_ret_f32_const : {
    float a0 = (ip++)->f32;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].f32 = (float)a0;
    goto *(ip++)->ptr;
}
do_add_f64_reg_reg : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_f64_reg_const : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_f64_const_reg : {
    double a0 = (ip++)->f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_add_f64_const_const : {
    double a0 = (ip++)->f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0 + a1;
    goto *(ip++)->ptr;
}
do_sub_f64_reg_reg : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_f64_reg_const : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_f64_const_reg : {
    double a0 = (ip++)->f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_sub_f64_const_const : {
    double a0 = (ip++)->f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0 - a1;
    goto *(ip++)->ptr;
}
do_mul_f64_reg_reg : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_f64_reg_const : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_f64_const_reg : {
    double a0 = (ip++)->f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_mul_f64_const_const : {
    double a0 = (ip++)->f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0 * a1;
    goto *(ip++)->ptr;
}
do_div_f64_reg_reg : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_f64_reg_const : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_f64_const_reg : {
    double a0 = (ip++)->f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_div_f64_const_const : {
    double a0 = (ip++)->f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0 / a1;
    goto *(ip++)->ptr;
}
do_mod_f64_reg_reg : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = fmod(a0, a1);
    goto *(ip++)->ptr;
}
do_mod_f64_reg_const : {
    double a0 = locals[(ip++)->reg].f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = fmod(a0, a1);
    goto *(ip++)->ptr;
}
do_mod_f64_const_reg : {
    double a0 = (ip++)->f64;
    double a1 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = fmod(a0, a1);
    goto *(ip++)->ptr;
}
do_mod_f64_const_const : {
    double a0 = (ip++)->f64;
    double a1 = (ip++)->f64;
    locals[(ip++)->reg].f64 = fmod(a0, a1);
    goto *(ip++)->ptr;
}
do_bb_f64_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_f64_reg_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_f64_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_bb_f64_const_ptr_ptr;
    ip[1].ptr = vm_run_comp(state, ip[1].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_bb_f64_reg_ptr_ptr : {
    double a0 = locals[ip[0].reg].f64;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_bb_f64_const_ptr_ptr : {
    double a0 = ip[0].f64;
    if (a0) {
        ip = ip[1].ptr;
    } else {
        ip = ip[2].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_f64_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_f64_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_f64_reg_reg_ptr_ptr : {
    double a0 = locals[ip[0].reg].f64;
    double a1 = ip[1].f64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_f64_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_f64_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_f64_reg_const_ptr_ptr : {
    double a0 = locals[ip[0].reg].f64;
    double a1 = ip[1].f64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_f64_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_f64_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_f64_const_reg_ptr_ptr : {
    double a0 = ip[0].f64;
    double a1 = ip[1].f64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_beq_f64_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_beq_f64_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_beq_f64_const_const_ptr_ptr : {
    double a0 = ip[0].f64;
    double a1 = ip[1].f64;
    if (a0 == a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_f64_reg_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_f64_reg_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_f64_reg_reg_ptr_ptr : {
    double a0 = locals[ip[0].reg].f64;
    double a1 = ip[1].f64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_f64_reg_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_f64_reg_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_f64_reg_const_ptr_ptr : {
    double a0 = locals[ip[0].reg].f64;
    double a1 = ip[1].f64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_f64_const_reg_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_f64_const_reg_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_f64_const_reg_ptr_ptr : {
    double a0 = ip[0].f64;
    double a1 = ip[1].f64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_blt_f64_const_const_func_func : {
    vm_opcode_t *head = ip - 1;
    head->ptr = &&do_blt_f64_const_const_ptr_ptr;
    ip[2].ptr = vm_run_comp(state, ip[2].func);
    ip[3].ptr = vm_run_comp(state, ip[3].func);
    ip = head;
    goto *(ip++)->ptr;
}
do_blt_f64_const_const_ptr_ptr : {
    double a0 = ip[0].f64;
    double a1 = ip[1].f64;
    if (a0 < a1) {
        ip = ip[2].ptr;
    } else {
        ip = ip[3].ptr;
    }
    goto *(ip++)->ptr;
}
do_move_f64_reg : {
    double a0 = locals[(ip++)->reg].f64;
    locals[(ip++)->reg].f64 = a0;
    goto *(ip++)->ptr;
}
do_move_f64_const : {
    double a0 = (ip++)->f64;
    locals[(ip++)->reg].f64 = a0;
    goto *(ip++)->ptr;
}
do_out_f64_reg : {
    double a0 = locals[(ip++)->reg].f64;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_out_f64_const : {
    double a0 = (ip++)->f64;
    putchar((int)a0);
    goto *(ip++)->ptr;
}
do_in_f64_void : {
    locals[(ip++)->reg].f64 = (double)fgetc(stdin);
    goto *(ip++)->ptr;
}
do_ret_f64_reg : {
    double a0 = locals[(ip++)->reg].f64;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].f64 = (double)a0;
    goto *(ip++)->ptr;
}
do_ret_f64_const : {
    double a0 = (ip++)->f64;
    locals -= 256;
    ip = *(--ips);
    locals[(ip++)->reg].f64 = (double)a0;
    goto *(ip++)->ptr;
}
do_exit_break_void : {
    return;
    goto *(ip++)->ptr;
}
do_jump_func_const : {
    vm_block_t *t0 = (ip++)->func;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const : {
    vm_block_t *t0 = (ip++)->func;
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const_reg : {
    vm_block_t *t0 = (ip++)->func;
    locals[257] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals[257] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const_reg_reg : {
    vm_block_t *t0 = (ip++)->func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg_reg_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const_reg_reg_reg : {
    vm_block_t *t0 = (ip++)->func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg_reg_reg_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const_reg_reg_reg_reg : {
    vm_block_t *t0 = (ip++)->func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = (ip++)->func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals[261] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals[261] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const_reg_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = (ip++)->func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals[261] = locals[(ip++)->reg];
    locals[262] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg_reg_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals[261] = locals[(ip++)->reg];
    locals[262] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const_reg_reg_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = (ip++)->func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals[261] = locals[(ip++)->reg];
    locals[262] = locals[(ip++)->reg];
    locals[263] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg_reg_reg_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals[261] = locals[(ip++)->reg];
    locals[262] = locals[(ip++)->reg];
    locals[263] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_const_reg_reg_reg_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = (ip++)->func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals[261] = locals[(ip++)->reg];
    locals[262] = locals[(ip++)->reg];
    locals[263] = locals[(ip++)->reg];
    locals[264] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
do_call_func_reg_reg_reg_reg_reg_reg_reg_reg_reg : {
    vm_block_t *t0 = locals[(ip++)->reg].func;
    locals[257] = locals[(ip++)->reg];
    locals[258] = locals[(ip++)->reg];
    locals[259] = locals[(ip++)->reg];
    locals[260] = locals[(ip++)->reg];
    locals[261] = locals[(ip++)->reg];
    locals[262] = locals[(ip++)->reg];
    locals[263] = locals[(ip++)->reg];
    locals[264] = locals[(ip++)->reg];
    locals += 256;
    *(ips++) = ip;
    ip = vm_run_comp(state, t0);
    goto *(ip++)->ptr;
}
}
