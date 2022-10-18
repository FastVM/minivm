#include "./int3.h"
#include "./value.h"
#include "../tag.h"
vm_opcode_t *vm_run_comp(vm_state_t *state, vm_rblock_t *rblock) {
    if (rblock->block->cache == NULL) {
        rblock->block->cache = vm_cache_new();
    }
    vm_opcode_t *ret = vm_cache_get(rblock->block->cache, rblock);
    if (ret != NULL) {
        return ret;
    }
    vm_block_t *block = rblock->block;
    uint8_t *types = vm_rblock_regs_dup(rblock->regs);
    vm_rblock_t *rnext = vm_rblock_new(rblock->block, rblock->regs);
    rnext->regs = types;
    rnext->block = rblock->block;
    size_t aops = 64;
    vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * aops);
    size_t nops = 0;
    for (size_t ninstr = 0; ninstr < block->len; ninstr++) {
        if (nops + 32 >= aops) {
            aops = (nops + 32) * 2;
            ops = vm_realloc(ops, sizeof(vm_opcode_t) * aops);
        }
        vm_instr_t instr = vm_rblock_type_specialize_instr(types, block->instrs[ninstr]);
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
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
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
                    ops[nops++].f32 = (float) instr.args[0].num;
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
                    ops[nops++].f64 = (double) instr.args[0].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].f32 = (float) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_CONST_REG];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F32_CONST_CONST];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].f32 = (float) instr.args[1].num;
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
                    ops[nops++].f64 = (double) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_CONST_REG];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_ADD_F64_CONST_CONST];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].f64 = (double) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].f32 = (float) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_CONST_REG];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F32_CONST_CONST];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].f32 = (float) instr.args[1].num;
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
                    ops[nops++].f64 = (double) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_CONST_REG];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_SUB_F64_CONST_CONST];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].f64 = (double) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].f32 = (float) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_CONST_REG];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F32_CONST_CONST];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].f32 = (float) instr.args[1].num;
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
                    ops[nops++].f64 = (double) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_CONST_REG];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MUL_F64_CONST_CONST];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].f64 = (double) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].f32 = (float) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_CONST_REG];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F32_CONST_CONST];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].f32 = (float) instr.args[1].num;
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
                    ops[nops++].f64 = (double) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_CONST_REG];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_DIV_F64_CONST_CONST];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].f64 = (double) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].f32 = (float) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_CONST_REG];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F32_CONST_CONST];
                    ops[nops++].f32 = (float) instr.args[0].num;
                    ops[nops++].f32 = (float) instr.args[1].num;
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
                    ops[nops++].f64 = (double) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_CONST_REG];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_MOD_F64_CONST_CONST];
                    ops[nops++].f64 = (double) instr.args[0].num;
                    ops[nops++].f64 = (double) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BOR_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BXOR_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BAND_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHL_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_CONST_REG];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I8_CONST_CONST];
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
                    ops[nops++].i8 = (int8_t) instr.args[1].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_CONST_REG];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I16_CONST_CONST];
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
                    ops[nops++].i16 = (int16_t) instr.args[1].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_CONST_REG];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I32_CONST_CONST];
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
                    ops[nops++].i32 = (int32_t) instr.args[1].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_CONST_REG];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_I64_CONST_CONST];
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
                    ops[nops++].i64 = (int64_t) instr.args[1].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_CONST_REG];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U8_CONST_CONST];
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
                    ops[nops++].u8 = (uint8_t) instr.args[1].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_CONST_REG];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U16_CONST_CONST];
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
                    ops[nops++].u16 = (uint16_t) instr.args[1].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_CONST_REG];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U32_CONST_CONST];
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
                    ops[nops++].u32 = (uint32_t) instr.args[1].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_CONST_REG];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].reg = instr.args[1].reg;
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (instr.args[0].type != VM_ARG_REG && instr.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BSHR_U64_CONST_CONST];
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
                    ops[nops++].u64 = (uint64_t) instr.args[1].num;
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
                uint8_t *args = vm_rblock_regs_empty();
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[1].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_REG];
                ops[nops++].reg = instr.args[0].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[2].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG];
                uint8_t *args = vm_rblock_regs_empty();
                args[1] = types[instr.args[1].reg];
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                ops[nops++].reg = instr.args[1].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
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
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[3].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG];
                uint8_t *args = vm_rblock_regs_empty();
                args[1] = types[instr.args[1].reg];
                args[2] = types[instr.args[2].reg];
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                ops[nops++].reg = instr.args[1].reg;
                ops[nops++].reg = instr.args[2].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
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
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[4].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG];
                uint8_t *args = vm_rblock_regs_empty();
                args[1] = types[instr.args[1].reg];
                args[2] = types[instr.args[2].reg];
                args[3] = types[instr.args[3].reg];
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                ops[nops++].reg = instr.args[1].reg;
                ops[nops++].reg = instr.args[2].reg;
                ops[nops++].reg = instr.args[3].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
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
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[5].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG];
                uint8_t *args = vm_rblock_regs_empty();
                args[1] = types[instr.args[1].reg];
                args[2] = types[instr.args[2].reg];
                args[3] = types[instr.args[3].reg];
                args[4] = types[instr.args[4].reg];
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                ops[nops++].reg = instr.args[1].reg;
                ops[nops++].reg = instr.args[2].reg;
                ops[nops++].reg = instr.args[3].reg;
                ops[nops++].reg = instr.args[4].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
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
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[6].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG];
                uint8_t *args = vm_rblock_regs_empty();
                args[1] = types[instr.args[1].reg];
                args[2] = types[instr.args[2].reg];
                args[3] = types[instr.args[3].reg];
                args[4] = types[instr.args[4].reg];
                args[5] = types[instr.args[5].reg];
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                ops[nops++].reg = instr.args[1].reg;
                ops[nops++].reg = instr.args[2].reg;
                ops[nops++].reg = instr.args[3].reg;
                ops[nops++].reg = instr.args[4].reg;
                ops[nops++].reg = instr.args[5].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
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
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[7].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG];
                uint8_t *args = vm_rblock_regs_empty();
                args[1] = types[instr.args[1].reg];
                args[2] = types[instr.args[2].reg];
                args[3] = types[instr.args[3].reg];
                args[4] = types[instr.args[4].reg];
                args[5] = types[instr.args[5].reg];
                args[6] = types[instr.args[6].reg];
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                ops[nops++].reg = instr.args[1].reg;
                ops[nops++].reg = instr.args[2].reg;
                ops[nops++].reg = instr.args[3].reg;
                ops[nops++].reg = instr.args[4].reg;
                ops[nops++].reg = instr.args[5].reg;
                ops[nops++].reg = instr.args[6].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
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
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[8].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG];
                uint8_t *args = vm_rblock_regs_empty();
                args[1] = types[instr.args[1].reg];
                args[2] = types[instr.args[2].reg];
                args[3] = types[instr.args[3].reg];
                args[4] = types[instr.args[4].reg];
                args[5] = types[instr.args[5].reg];
                args[6] = types[instr.args[6].reg];
                args[7] = types[instr.args[7].reg];
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                ops[nops++].reg = instr.args[1].reg;
                ops[nops++].reg = instr.args[2].reg;
                ops[nops++].reg = instr.args[3].reg;
                ops[nops++].reg = instr.args[4].reg;
                ops[nops++].reg = instr.args[5].reg;
                ops[nops++].reg = instr.args[6].reg;
                ops[nops++].reg = instr.args[7].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
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
                        ops[nops++].reg = VM_NREGS;
                } else {
                        ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.args[9].type == VM_ARG_NONE) {
                ops[nops++].ptr = state->ptrs[VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG_REG];
                uint8_t *args = vm_rblock_regs_empty();
                args[1] = types[instr.args[1].reg];
                args[2] = types[instr.args[2].reg];
                args[3] = types[instr.args[3].reg];
                args[4] = types[instr.args[4].reg];
                args[5] = types[instr.args[5].reg];
                args[6] = types[instr.args[6].reg];
                args[7] = types[instr.args[7].reg];
                args[8] = types[instr.args[8].reg];
                ops[nops++].func = vm_rblock_new(instr.args[0].func, args);
                ops[nops++].reg = instr.args[1].reg;
                ops[nops++].reg = instr.args[2].reg;
                ops[nops++].reg = instr.args[3].reg;
                ops[nops++].reg = instr.args[4].reg;
                ops[nops++].reg = instr.args[5].reg;
                ops[nops++].reg = instr.args[6].reg;
                ops[nops++].reg = instr.args[7].reg;
                ops[nops++].reg = instr.args[8].reg;
                if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
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
                        ops[nops++].reg = VM_NREGS;
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
                    ops[nops++].i8 = (int8_t) instr.args[0].num;
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
                    ops[nops++].i16 = (int16_t) instr.args[0].num;
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
                    ops[nops++].i32 = (int32_t) instr.args[0].num;
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
                    ops[nops++].i64 = (int64_t) instr.args[0].num;
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
                    ops[nops++].u8 = (uint8_t) instr.args[0].num;
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
                    ops[nops++].u16 = (uint16_t) instr.args[0].num;
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
                    ops[nops++].u32 = (uint32_t) instr.args[0].num;
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
                    ops[nops++].u64 = (uint64_t) instr.args[0].num;
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
                    ops[nops++].f32 = (float) instr.args[0].num;
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
                    ops[nops++].f64 = (double) instr.args[0].num;
                    break;
                }
                if (instr.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_OUT_F64_REG];
                    ops[nops++].reg = instr.args[0].reg;
                    break;
                }
            }
        }
        default: goto err;
        }
        if (instr.out.type == VM_ARG_REG) {
            types[instr.out.reg] = instr.tag;
        }
     }
     vm_branch_t branch = vm_rblock_type_specialize_branch(types, block->branch);
     switch (branch.op) {
        case VM_BOP_EXIT: {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_EXIT_BREAK_VOID];
            break;
        }
        case VM_BOP_JUMP: {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_JUMP_FUNC_CONST];
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
            break;
        }
        case VM_BOP_RET: {
            if (branch.tag == VM_TAG_I8) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I8_CONST];
                   ops[nops++].i8 = (int8_t) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I8_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_I16) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I16_CONST];
                   ops[nops++].i16 = (int16_t) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I16_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_I32) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I32_CONST];
                   ops[nops++].i32 = (int32_t) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I32_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_I64) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I64_CONST];
                   ops[nops++].i64 = (int64_t) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_I64_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_U8) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U8_CONST];
                   ops[nops++].u8 = (uint8_t) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U8_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_U16) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U16_CONST];
                   ops[nops++].u16 = (uint16_t) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U16_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_U32) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U32_CONST];
                   ops[nops++].u32 = (uint32_t) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U32_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_U64) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U64_CONST];
                   ops[nops++].u64 = (uint64_t) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_U64_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_F32) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_F32_CONST];
                   ops[nops++].f32 = (float) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_F32_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            if (branch.tag == VM_TAG_F64) {
                if (branch.args[0].type != VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_F64_CONST];
                   ops[nops++].f64 = (double) branch.args[0].num;
                   break;
               }
                if (branch.args[0].type == VM_ARG_REG) {
                   ops[nops++].ptr = state->ptrs[VM_OPCODE_RET_F64_REG];
                   ops[nops++].reg = branch.args[0].reg;
                   break;
               }
            }
            goto err;
        }
        case VM_BOP_BB: {
                if (branch.tag == VM_TAG_I8) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I8_CONST_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I8_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_I16) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I16_CONST_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I16_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_I32) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I32_CONST_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I32_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_I64) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I64_CONST_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_I64_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_U8) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U8_CONST_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U8_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_U16) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U16_CONST_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U16_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_U32) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U32_CONST_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U32_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_U64) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U64_CONST_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_U64_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_F32) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_F32_CONST_FUNC_FUNC];
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_F32_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_F64) {
                    if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_F64_CONST_FUNC_FUNC];
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BB_F64_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                    }
                    break;
                }
             goto err;
        }
        case VM_BOP_BLT: {
            if (branch.tag == VM_TAG_I8) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i8 = (int8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_CONST_REG_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I8_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].i8 = (int8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I16) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i16 = (int16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_CONST_REG_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I16_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].i16 = (int16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i32 = (int32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_CONST_REG_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].i32 = (int32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i64 = (int64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_CONST_REG_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_I64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].i64 = (int64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U8) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u8 = (uint8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_CONST_REG_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U8_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].u8 = (uint8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U16) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u16 = (uint16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_CONST_REG_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U16_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].u16 = (uint16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u32 = (uint32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_CONST_REG_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].u32 = (uint32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u64 = (uint64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_CONST_REG_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_U64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].u64 = (uint64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_F32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].f32 = (float) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_CONST_REG_FUNC_FUNC];
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].f32 = (float) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_F64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].f64 = (double) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_CONST_REG_FUNC_FUNC];
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BLT_F64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].f64 = (double) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
             goto err;
        }
        case VM_BOP_BEQ: {
            if (branch.tag == VM_TAG_I8) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i8 = (int8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_CONST_REG_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I8_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].i8 = (int8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I16) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i16 = (int16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_CONST_REG_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I16_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].i16 = (int16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i32 = (int32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_CONST_REG_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].i32 = (int32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i64 = (int64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_CONST_REG_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_I64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].i64 = (int64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U8) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u8 = (uint8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_CONST_REG_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U8_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].u8 = (uint8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U16) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u16 = (uint16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_CONST_REG_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U16_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].u16 = (uint16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u32 = (uint32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_CONST_REG_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].u32 = (uint32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u64 = (uint64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_CONST_REG_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_U64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].u64 = (uint64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_F32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].f32 = (float) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_CONST_REG_FUNC_FUNC];
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F32_CONST_CONST_FUNC_FUNC];
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].f32 = (float) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_F64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_REG_REG_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_REG_CONST_FUNC_FUNC];
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].f64 = (double) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_CONST_REG_FUNC_FUNC];
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].ptr = state->ptrs[VM_OPCODE_BEQ_F64_CONST_CONST_FUNC_FUNC];
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].f64 = (double) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
             goto err;
        }
        default: goto err;
     }
     vm_cache_set(rblock->block->cache, rnext, ops);
     return ops;
err:;
     fprintf(stderr, "BAD INSTR!\n");
     exit(1);
}