#include "int3.h"
#include "value.h"
#include "../tag.h"

vm_run_block_t *vm_run_block_new(vm_block_t *block);
vm_run_comp_t *vm_run_cache_get(vm_run_block_t *rblock);
void vm_run_cache_set(vm_run_block_t *rblock, vm_run_comp_t *comp);
void vm_run_comp_block(vm_state_t *state, vm_run_comp_t *comp, vm_block_t *block, uint8_t *regs);

vm_opcode_t *vm_run_comp(vm_state_t *state, vm_run_block_t *rblock) {
    if (rblock->block->cache == NULL) {
        rblock->block->cache = vm_malloc(sizeof(vm_run_cache_t));
    }
    vm_run_comp_t *check = vm_run_cache_get(rblock);
    if (check != NULL) {
       return check->ops;
    }
    vm_run_comp_t *comp = vm_alloc0(sizeof(vm_run_comp_t));
    comp->aops = 64;
    comp->ops = vm_malloc(sizeof(vm_opcode_t) * comp->aops);
    comp->nops = 0;
    vm_run_comp_block(state, comp, rblock->block, rblock->args);
    vm_run_cache_set(rblock, comp);
    return comp->ops;
}

void vm_run_comp_block(vm_state_t *state, vm_run_comp_t *comp, vm_block_t *block, uint8_t *regs) {
    for (size_t ninstr = 0; ninstr < block->len; ninstr++) {
        if (comp->nops + 32 >= comp->aops) {
            comp->aops = (comp->nops + 32) * 2;
            comp->ops = vm_realloc(comp->ops, sizeof(vm_opcode_t) * comp->aops);
        }
        vm_instr_t instr = block->instrs[ninstr];
        switch (instr.op) {
        case VM_IOP_MOVE: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I8_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I8_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I16_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I16_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I32_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I32_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I64_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_I64_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U8_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U8_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U16_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U16_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U32_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U32_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U64_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_U64_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_F32_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_F32_CONST];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_F64_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOVE_F64_CONST];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I8_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I8_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I16_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I16_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I32_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I32_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I64_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_I64_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U8_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U8_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U16_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U16_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U32_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U32_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U64_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BNOT_U64_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_CONST_REG];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_CONST_CONST];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_CONST_REG];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_CONST_CONST];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_CONST_REG];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_CONST_CONST];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_CONST_REG];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_CONST_CONST];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_CONST_REG];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_CONST_CONST];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_CONST_REG];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_CONST_CONST];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_CONST_REG];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_CONST_CONST];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_CONST_REG];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_CONST_CONST];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_CONST_REG];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_CONST_CONST];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    comp->ops[comp->nops++].f32 = (float) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_CONST_REG];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_CONST_CONST];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    comp->ops[comp->nops++].f64 = (double) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_CONST_REG];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_CONST_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_CONST_REG];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_CONST_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_CONST_REG];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_CONST_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_CONST_REG];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_CONST_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_CONST_REG];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_CONST_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_CONST_REG];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_CONST_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_CONST_REG];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_CONST_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_REG_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_REG_CONST];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_CONST_REG];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].reg = instr.args[1].reg;
                    comp->ops[comp->nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_CONST_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[1].num;
                    comp->ops[comp->nops++].reg = instr.out.reg;
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
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_I8_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_I16) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_I16_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_I32) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_I32_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_I64) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_I64_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_U8) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_U8_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_U16) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_U16_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_U32) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_U32_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_U64) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_U64_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_F32) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_F32_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            if (instr.tag == VM_TAG_F64) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_IN_F64_VOID];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
            }
            break;
        }
        case VM_IOP_CALL: {
            if (instr.args[1].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[1].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[2].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[2].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[3].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[3].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[4].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[4].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[5].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[5].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[6].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                comp->ops[comp->nops++].reg = instr.args[5].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[6].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                comp->ops[comp->nops++].reg = instr.args[5].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[7].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                comp->ops[comp->nops++].reg = instr.args[5].reg;
                comp->ops[comp->nops++].reg = instr.args[6].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[7].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                comp->ops[comp->nops++].reg = instr.args[5].reg;
                comp->ops[comp->nops++].reg = instr.args[6].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[8].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                comp->ops[comp->nops++].reg = instr.args[5].reg;
                comp->ops[comp->nops++].reg = instr.args[6].reg;
                comp->ops[comp->nops++].reg = instr.args[7].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[8].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                comp->ops[comp->nops++].reg = instr.args[5].reg;
                comp->ops[comp->nops++].reg = instr.args[6].reg;
                comp->ops[comp->nops++].reg = instr.args[7].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[9].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].func = vm_run_block_new(instr.args[0].func);
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                comp->ops[comp->nops++].reg = instr.args[5].reg;
                comp->ops[comp->nops++].reg = instr.args[6].reg;
                comp->ops[comp->nops++].reg = instr.args[7].reg;
                comp->ops[comp->nops++].reg = instr.args[8].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[9].type == VM_ARG_NONE) {
                comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG_REG_REG];
                comp->ops[comp->nops++].reg = instr.args[0].reg;
                comp->ops[comp->nops++].reg = instr.args[1].reg;
                comp->ops[comp->nops++].reg = instr.args[2].reg;
                comp->ops[comp->nops++].reg = instr.args[3].reg;
                comp->ops[comp->nops++].reg = instr.args[4].reg;
                comp->ops[comp->nops++].reg = instr.args[5].reg;
                comp->ops[comp->nops++].reg = instr.args[6].reg;
                comp->ops[comp->nops++].reg = instr.args[7].reg;
                comp->ops[comp->nops++].reg = instr.args[8].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        comp->ops[comp->nops++].reg = 256;
                } else {
                        comp->ops[comp->nops++].reg = instr.out.reg;
                }
                break;
            }
            goto err;
        }
        case VM_IOP_OUT: {
            if (instr.tag == VM_TAG_I8) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_I8_CONST];
                    comp->ops[comp->nops++].i8 = (int8_t) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_I8_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_I16_CONST];
                    comp->ops[comp->nops++].i16 = (int16_t) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_I16_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_I32_CONST];
                    comp->ops[comp->nops++].i32 = (int32_t) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_I32_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_I64_CONST];
                    comp->ops[comp->nops++].i64 = (int64_t) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_I64_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_U8_CONST];
                    comp->ops[comp->nops++].u8 = (uint8_t) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_U8_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_U16_CONST];
                    comp->ops[comp->nops++].u16 = (uint16_t) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_U16_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_U32_CONST];
                    comp->ops[comp->nops++].u32 = (uint32_t) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_U32_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_U64_CONST];
                    comp->ops[comp->nops++].u64 = (uint64_t) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_U64_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_F32_CONST];
                    comp->ops[comp->nops++].f32 = (float) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_F32_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (instr.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_F64_CONST];
                    comp->ops[comp->nops++].f64 = (double) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_OUT_F64_REG];
                    comp->ops[comp->nops++].reg = instr.args[0].reg;
                    break;
                }
            }
        }
        default: goto err;
         }
     }
     switch (block->branch.op) {
        case VM_BOP_EXIT: {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_EXIT_BREAK_VOID];
            break;
        }
        case VM_BOP_JUMP: {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_JUMP_FUNC_CONST];
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
            break;
        }
        case VM_BOP_RET: {
            if (block->branch.tag == VM_TAG_I8) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_I8_CONST];
                   comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_I8_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_I16) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_I16_CONST];
                   comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_I16_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_I32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_I32_CONST];
                   comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_I32_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_I64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_I64_CONST];
                   comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_I64_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_U8) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_U8_CONST];
                   comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_U8_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_U16) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_U16_CONST];
                   comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_U16_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_U32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_U32_CONST];
                   comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_U32_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_U64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_U64_CONST];
                   comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_U64_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_F32) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_F32_CONST];
                   comp->ops[comp->nops++].f32 = (float) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_F32_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            if (block->branch.tag == VM_TAG_F64) {
                if (block->branch.args[0].type != VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_F64_CONST];
                   comp->ops[comp->nops++].f64 = (double) block->branch.args[0].num;
                   break;
               }
                if (block->branch.args[0].type == VM_ARG_REG) {
                   comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_RET_F64_REG];
                   comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                   break;
               }
            }
            goto err;
        }
        case VM_BOP_BB: {
                if (block->branch.tag == VM_TAG_I8) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_I8_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_I8_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_I16) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_I16_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_I16_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_I32) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_I32_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_I32_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_I64) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_I64_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_I64_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_U8) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_U8_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_U8_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_U16) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_U16_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_U16_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_U32) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_U32_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_U32_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_U64) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_U64_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_U64_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_F32) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_F32_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_F32_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
                if (block->branch.tag == VM_TAG_F64) {
                    if (block->branch.args[0].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_F64_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[0].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    if (block->branch.args[0].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BB_F64_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                    }
                    break;
                }
             goto err;
        }
        case VM_BOP_BLT: {
            if (block->branch.tag == VM_TAG_I8) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I16) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U8) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U16) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[0].num;
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[0].num;
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            goto err;
        }
        case VM_BOP_BEQ: {
            if (block->branch.tag == VM_TAG_I8) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].i8 = (int8_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I16) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].i16 = (int16_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].i32 = (int32_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_I64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].i64 = (int64_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U8) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].u8 = (uint8_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U16) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].u16 = (uint16_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].u32 = (uint32_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_U64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[0].num;
                    comp->ops[comp->nops++].u64 = (uint64_t) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F32) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[0].num;
                    comp->ops[comp->nops++].f32 = (float) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            if (block->branch.tag == VM_TAG_F64) {
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_REG_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type == VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_REG_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].reg = block->branch.args[0].reg;
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type == VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_CONST_REG_FUNC_FUNC];
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[0].num;
                    comp->ops[comp->nops++].reg = block->branch.args[1].reg;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
                if (block->branch.args[0].type != VM_ARG_REG && block->branch.args[1].type != VM_ARG_REG) {
                    comp->ops[comp->nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_CONST_CONST_FUNC_FUNC];
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[0].num;
                    comp->ops[comp->nops++].f64 = (double) block->branch.args[1].num;
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[0]);
                    comp->ops[comp->nops++].func = vm_run_block_new(block->branch.targets[1]);
                    break;
                }
            }
            goto err;
        }
        default: goto err;
    }
    return;
err:;
    fprintf(stderr, "BAD INSTR!\n");
    __builtin_trap();
}