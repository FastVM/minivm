#include "int3.h"
#include "value.h"
#include "../tag.h"
        
vm_state_t *vm_state_init(size_t nregs) {
    vm_state_t *ret = vm_malloc(sizeof(vm_state_t));
    ret->nlocals = nregs;
    ret->locals = vm_malloc(sizeof(vm_value_t) * (ret->nlocals));
    ret->ips = vm_malloc(sizeof(void *) * (ret->nlocals / VM_NREGS));
    return ret;
}

void vm_state_deinit(vm_state_t *state) {
    vm_free(state->ips);
    vm_free(state->locals);
    vm_free(state);
}

    
void *vm_run_comp(vm_state_t *state, vm_rblock_t *rblock) {
    vm_opcode_t *ret = vm_cache_get(&rblock->block->cache, rblock);
    if (ret != NULL) {
        return ret;
    }
    vm_block_t *block = rblock->block;
    vm_tags_t *types = vm_rblock_regs_dup(rblock->regs, VM_NREGS);
    vm_rblock_t *rnext = vm_rblock_new(rblock->block, rblock->regs);
    rnext->start = rblock->start;
    size_t aops = 64;
    vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * aops);
    size_t nops = 0;
    for (size_t ninstr = rnext->start; ninstr < block->len; ninstr++) {
        if (nops + 16 + VM_TAG_MAX >= aops) {
            aops = (nops + 16) * 2;
            ops = vm_realloc(ops, sizeof(vm_opcode_t) * aops);
        }
        vm_instr_t instr = vm_rblock_type_specialize_instr(types, block->instrs[ninstr]);
        if (!vm_rblock_type_check_instr(types, instr)) goto fail_return;
        switch (instr.op) {
        case VM_IOP_CAST: {
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I8_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I16_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I32_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_I64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_I64_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U8 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U8_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U16 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U16_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U32_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_U64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_U64_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F32 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F32_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_I8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_I16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_I32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_I64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_I64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U8) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_U8);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U16) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_U16);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_U32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_U64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_U64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F32) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_F32);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
            if (instr.tag == VM_TAG_F64 && vm_instr_get_arg_type(instr, 0) == VM_TAG_F64) {
                ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CAST_F64_F64);
                ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                ops[nops++].reg = instr.out.reg;
            }
        break;
        }
        case VM_IOP_MOVE: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_NIL) {
                break;
            }
            if (vm_instr_get_arg_type(instr, 0) == VM_ARG_STR) {
                if (VM_TAG_I32 == VM_TAG_PTR) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U32_CONST);
                    ops[nops++].u32 = (uint32_t) (size_t) vm_instr_get_arg_str(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                } else {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U64_CONST);
                    ops[nops++].u64 = (uint64_t) (size_t) vm_instr_get_arg_str(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                }
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_I8_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_I8_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_I16_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_I16_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_I32_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_I32_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_I64_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_I64_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U8_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U8_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U16_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U16_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U32_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U32_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U64_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_U64_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_F32_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_F32_CONST);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_F64_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOVE_F64_CONST);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_BNOT: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_I8_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_I8_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_I16_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_I16_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_I32_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_I32_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_I64_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_I64_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_U8_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_U8_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_U16_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_U16_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_U32_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_U32_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_U64_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BNOT_U64_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_ADD: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_F32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_F32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_F32_CONST_REG);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_F32_CONST_CONST);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_F64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_F64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_F64_CONST_REG);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_ADD_F64_CONST_CONST);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_SUB: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_F32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_F32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_F32_CONST_REG);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_F32_CONST_CONST);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_F64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_F64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_F64_CONST_REG);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_SUB_F64_CONST_CONST);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_MUL: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_F32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_F32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_F32_CONST_REG);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_F32_CONST_CONST);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_F64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_F64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_F64_CONST_REG);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MUL_F64_CONST_CONST);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_DIV: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_F32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_F32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_F32_CONST_REG);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_F32_CONST_CONST);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_F64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_F64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_F64_CONST_REG);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_DIV_F64_CONST_CONST);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_MOD: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_F32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_F32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_F32_CONST_REG);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_F32_CONST_CONST);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_F64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_F64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_F64_CONST_REG);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_MOD_F64_CONST_CONST);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_BOR: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BOR_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_BXOR: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BXOR_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_BAND: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BAND_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_BSHL: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHL_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_BSHR: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I8_CONST_REG);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I8_CONST_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I16_CONST_REG);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I16_CONST_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I32_CONST_REG);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I32_CONST_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I64_CONST_REG);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_I64_CONST_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U8_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U8_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U8_CONST_REG);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U8_CONST_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U16_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U16_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U16_CONST_REG);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U16_CONST_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U32_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U32_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U32_CONST_REG);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U32_CONST_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U64_REG_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U64_REG_CONST);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U64_CONST_REG);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG && vm_instr_get_arg_type(instr, 1) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BSHR_U64_CONST_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 1);
                    ops[nops++].reg = instr.out.reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_IN: {
            if (instr.out.type == VM_ARG_NONE) {
                break;
            }
            if (instr.tag == VM_TAG_I8) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_I8_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_I16) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_I16_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_I32) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_I32_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_I64) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_I64_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_U8) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_U8_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_U16) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_U16_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_U32) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_U32_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_U64) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_U64_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_F32) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_F32_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            if (instr.tag == VM_TAG_F64) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_IN_F64_VOID);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
            }
            break;
        }
        case VM_IOP_CALL: {
            if (vm_instr_get_arg_type(instr, 1) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            if (vm_instr_get_arg_type(instr, 2) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            if (vm_instr_get_arg_type(instr, 3) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            if (vm_instr_get_arg_type(instr, 4) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            if (vm_instr_get_arg_type(instr, 5) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            if (vm_instr_get_arg_type(instr, 6) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR_REG_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    args->tags[5] = types->tags[vm_instr_get_arg_reg(instr, 5)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 5);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC_REG_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    args->tags[5] = types->tags[vm_instr_get_arg_reg(instr, 5)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 5);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            if (vm_instr_get_arg_type(instr, 7) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR_REG_REG_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    args->tags[5] = types->tags[vm_instr_get_arg_reg(instr, 5)];
                    args->tags[6] = types->tags[vm_instr_get_arg_reg(instr, 6)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 5);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 6);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC_REG_REG_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    args->tags[5] = types->tags[vm_instr_get_arg_reg(instr, 5)];
                    args->tags[6] = types->tags[vm_instr_get_arg_reg(instr, 6)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 5);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 6);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            if (vm_instr_get_arg_type(instr, 8) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR_REG_REG_REG_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    args->tags[5] = types->tags[vm_instr_get_arg_reg(instr, 5)];
                    args->tags[6] = types->tags[vm_instr_get_arg_reg(instr, 6)];
                    args->tags[7] = types->tags[vm_instr_get_arg_reg(instr, 7)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 5);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 6);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 7);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC_REG_REG_REG_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    args->tags[5] = types->tags[vm_instr_get_arg_reg(instr, 5)];
                    args->tags[6] = types->tags[vm_instr_get_arg_reg(instr, 6)];
                    args->tags[7] = types->tags[vm_instr_get_arg_reg(instr, 7)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 5);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 6);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 7);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            if (vm_instr_get_arg_type(instr, 9) == VM_ARG_NONE) {
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_FUNC) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_PTR_REG_REG_REG_REG_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    args->tags[5] = types->tags[vm_instr_get_arg_reg(instr, 5)];
                    args->tags[6] = types->tags[vm_instr_get_arg_reg(instr, 6)];
                    args->tags[7] = types->tags[vm_instr_get_arg_reg(instr, 7)];
                    args->tags[8] = types->tags[vm_instr_get_arg_reg(instr, 8)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 5);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 6);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 7);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 8);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
                else if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_CALL_FUNC_FUNC_REG_REG_REG_REG_REG_REG_REG_REG);
                    vm_tags_t *args = vm_rblock_regs_empty(VM_NREGS);
                    args->tags[1] = types->tags[vm_instr_get_arg_reg(instr, 1)];
                    args->tags[2] = types->tags[vm_instr_get_arg_reg(instr, 2)];
                    args->tags[3] = types->tags[vm_instr_get_arg_reg(instr, 3)];
                    args->tags[4] = types->tags[vm_instr_get_arg_reg(instr, 4)];
                    args->tags[5] = types->tags[vm_instr_get_arg_reg(instr, 5)];
                    args->tags[6] = types->tags[vm_instr_get_arg_reg(instr, 6)];
                    args->tags[7] = types->tags[vm_instr_get_arg_reg(instr, 7)];
                    args->tags[8] = types->tags[vm_instr_get_arg_reg(instr, 8)];
                    vm_rblock_t *rblock = vm_rblock_new(vm_instr_get_arg_func(instr, 0), args);
                    vm_opcode_t *opcodes = vm_run_comp(state, rblock);
                    if (opcodes == NULL) goto fail_return;
                    ops[nops++].ptr = opcodes;
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 1);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 2);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 3);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 4);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 5);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 6);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 7);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 8);
                    if (instr.out.type == VM_ARG_NONE) {
                        ops[nops++].reg = VM_NREGS;
                    } else {
                        ops[nops++].reg = instr.out.reg;
                    }
                    for (vm_tag_t type = 0; type < VM_TAG_MAX; type++) {
                        vm_tags_t *types_tag = vm_rblock_regs_dup(types, VM_NREGS);
                        if (instr.out.type == VM_ARG_REG) {
                            types_tag->tags[instr.out.reg] = type;
                        }
                        vm_rblock_t *rest_block = vm_rblock_new(rnext->block, types_tag);
                        rest_block->start = ninstr+1;
                        ops[nops++].size = ((size_t) rest_block) + 1;
                    }
                    goto early_return;
                }
            }
            __builtin_trap();
        }
        case VM_IOP_OUT: {
            if (instr.tag == VM_TAG_I8) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_I8_CONST);
                    ops[nops++].i8 = (int8_t) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_I8_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_I16) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_I16_CONST);
                    ops[nops++].i16 = (int16_t) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_I16_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_I32) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_I32_CONST);
                    ops[nops++].i32 = (int32_t) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_I32_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_I64) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_I64_CONST);
                    ops[nops++].i64 = (int64_t) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_I64_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_U8) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_U8_CONST);
                    ops[nops++].u8 = (uint8_t) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_U8_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_U16) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_U16_CONST);
                    ops[nops++].u16 = (uint16_t) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_U16_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_U32) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_U32_CONST);
                    ops[nops++].u32 = (uint32_t) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_U32_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_U64) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_U64_CONST);
                    ops[nops++].u64 = (uint64_t) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_U64_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_F32) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_F32_CONST);
                    ops[nops++].f32 = (float) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_F32_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
            if (instr.tag == VM_TAG_F64) {
                if (vm_instr_get_arg_type(instr, 0) != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_F64_CONST);
                    ops[nops++].f64 = (double) vm_instr_get_arg_num(instr, 0);
                    break;
                }
                if (vm_instr_get_arg_type(instr, 0) == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_OUT_F64_REG);
                    ops[nops++].reg = vm_instr_get_arg_reg(instr, 0);
                    break;
                }
            }
             break;
        }
        default: __builtin_trap();
        }
        if (instr.out.type == VM_ARG_REG) {
            types->tags[instr.out.reg] = instr.tag;
        }
    }
    vm_branch_t branch = vm_rblock_type_specialize_branch(types, block->branch);
    if (!vm_rblock_type_check_branch(types, branch)) goto fail_return;
    switch (branch.op) {
        case VM_BOP_EXIT: {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_EXIT_BREAK_VOID);
            break;
        }
        case VM_BOP_JUMP: {
            ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_JUMP_PTR_CONST);
            ops[nops++].ptr = vm_run_comp(state, vm_rblock_new(branch.targets[0], types));
            break;
        }
        case VM_BOP_BTYPE: {
            ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_JUMP_PTR_CONST);
            if (branch.tag == types->tags[branch.args[0].reg]) {
                ops[nops++].ptr = vm_run_comp(state, vm_rblock_new(branch.targets[0], types));
            } else {
                ops[nops++].ptr = vm_run_comp(state, vm_rblock_new(branch.targets[1], types));
            }
            break;
        }
        case VM_BOP_RET: {
            if (branch.tag == VM_TAG_I8) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_I8_CONST);
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_I8_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_I16) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_I16_CONST);
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_I16_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_I32) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_I32_CONST);
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_I32_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_I64) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_I64_CONST);
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_I64_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_U8) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_U8_CONST);
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_U8_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_U16) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_U16_CONST);
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_U16_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_U32) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_U32_CONST);
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_U32_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_U64) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_U64_CONST);
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_U64_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_F32) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_F32_CONST);
                    ops[nops++].f32 = (float) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_F32_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            if (branch.tag == VM_TAG_F64) {
                if (branch.args[0].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_F64_CONST);
                    ops[nops++].f64 = (double) branch.args[0].num;
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_RET_F64_REG);
                    ops[nops++].reg = branch.args[0].reg;
                    break;
                }
            }
            __builtin_trap();
        }
        case VM_BOP_BB: {
                if (branch.tag == VM_TAG_I8) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_I8_CONST_FUNC_FUNC);
                        ops[nops++].i8 = (int8_t) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_I8_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_I16) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_I16_CONST_FUNC_FUNC);
                        ops[nops++].i16 = (int16_t) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_I16_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_I32) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_I32_CONST_FUNC_FUNC);
                        ops[nops++].i32 = (int32_t) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_I32_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_I64) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_I64_CONST_FUNC_FUNC);
                        ops[nops++].i64 = (int64_t) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_I64_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_U8) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_U8_CONST_FUNC_FUNC);
                        ops[nops++].u8 = (uint8_t) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_U8_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_U16) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_U16_CONST_FUNC_FUNC);
                        ops[nops++].u16 = (uint16_t) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_U16_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_U32) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_U32_CONST_FUNC_FUNC);
                        ops[nops++].u32 = (uint32_t) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_U32_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_U64) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_U64_CONST_FUNC_FUNC);
                        ops[nops++].u64 = (uint64_t) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_U64_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_F32) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_F32_CONST_FUNC_FUNC);
                        ops[nops++].f32 = (float) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_F32_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
                if (branch.tag == VM_TAG_F64) {
                    if (branch.args[0].type != VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_F64_CONST_FUNC_FUNC);
                        ops[nops++].f64 = (double) branch.args[0].num;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    if (branch.args[0].type == VM_ARG_REG) {
                        ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BB_F64_REG_FUNC_FUNC);
                        ops[nops++].reg = branch.args[0].reg;
                        ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                        ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                        break;
                    }
                    break;
                }
             __builtin_trap();
        }
        case VM_BOP_BLT: {
            if (branch.tag == VM_TAG_I8) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I8_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I8_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i8 = (int8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I8_CONST_REG_FUNC_FUNC);
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I8_CONST_CONST_FUNC_FUNC);
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].i8 = (int8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I16) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I16_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I16_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i16 = (int16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I16_CONST_REG_FUNC_FUNC);
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I16_CONST_CONST_FUNC_FUNC);
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].i16 = (int16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I32_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I32_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i32 = (int32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I32_CONST_REG_FUNC_FUNC);
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I32_CONST_CONST_FUNC_FUNC);
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].i32 = (int32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I64_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I64_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i64 = (int64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I64_CONST_REG_FUNC_FUNC);
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_I64_CONST_CONST_FUNC_FUNC);
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].i64 = (int64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U8) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U8_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U8_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u8 = (uint8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U8_CONST_REG_FUNC_FUNC);
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U8_CONST_CONST_FUNC_FUNC);
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].u8 = (uint8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U16) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U16_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U16_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u16 = (uint16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U16_CONST_REG_FUNC_FUNC);
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U16_CONST_CONST_FUNC_FUNC);
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].u16 = (uint16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U32_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U32_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u32 = (uint32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U32_CONST_REG_FUNC_FUNC);
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U32_CONST_CONST_FUNC_FUNC);
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].u32 = (uint32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U64_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U64_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u64 = (uint64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U64_CONST_REG_FUNC_FUNC);
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_U64_CONST_CONST_FUNC_FUNC);
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].u64 = (uint64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_F32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_F32_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_F32_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].f32 = (float) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_F32_CONST_REG_FUNC_FUNC);
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_F32_CONST_CONST_FUNC_FUNC);
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].f32 = (float) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_F64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_F64_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_F64_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].f64 = (double) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_F64_CONST_REG_FUNC_FUNC);
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BLT_F64_CONST_CONST_FUNC_FUNC);
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].f64 = (double) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
             __builtin_trap();
        }
        case VM_BOP_BEQ: {
            if (branch.tag == VM_TAG_I8) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I8_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I8_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i8 = (int8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I8_CONST_REG_FUNC_FUNC);
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I8_CONST_CONST_FUNC_FUNC);
                    ops[nops++].i8 = (int8_t) branch.args[0].num;
                    ops[nops++].i8 = (int8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I16) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I16_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I16_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i16 = (int16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I16_CONST_REG_FUNC_FUNC);
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I16_CONST_CONST_FUNC_FUNC);
                    ops[nops++].i16 = (int16_t) branch.args[0].num;
                    ops[nops++].i16 = (int16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I32_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I32_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i32 = (int32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I32_CONST_REG_FUNC_FUNC);
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I32_CONST_CONST_FUNC_FUNC);
                    ops[nops++].i32 = (int32_t) branch.args[0].num;
                    ops[nops++].i32 = (int32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_I64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I64_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I64_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].i64 = (int64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I64_CONST_REG_FUNC_FUNC);
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_I64_CONST_CONST_FUNC_FUNC);
                    ops[nops++].i64 = (int64_t) branch.args[0].num;
                    ops[nops++].i64 = (int64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U8) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U8_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U8_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u8 = (uint8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U8_CONST_REG_FUNC_FUNC);
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U8_CONST_CONST_FUNC_FUNC);
                    ops[nops++].u8 = (uint8_t) branch.args[0].num;
                    ops[nops++].u8 = (uint8_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U16) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U16_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U16_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u16 = (uint16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U16_CONST_REG_FUNC_FUNC);
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U16_CONST_CONST_FUNC_FUNC);
                    ops[nops++].u16 = (uint16_t) branch.args[0].num;
                    ops[nops++].u16 = (uint16_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U32_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U32_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u32 = (uint32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U32_CONST_REG_FUNC_FUNC);
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U32_CONST_CONST_FUNC_FUNC);
                    ops[nops++].u32 = (uint32_t) branch.args[0].num;
                    ops[nops++].u32 = (uint32_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_U64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U64_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U64_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].u64 = (uint64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U64_CONST_REG_FUNC_FUNC);
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_U64_CONST_CONST_FUNC_FUNC);
                    ops[nops++].u64 = (uint64_t) branch.args[0].num;
                    ops[nops++].u64 = (uint64_t) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_F32) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_F32_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_F32_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].f32 = (float) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_F32_CONST_REG_FUNC_FUNC);
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_F32_CONST_CONST_FUNC_FUNC);
                    ops[nops++].f32 = (float) branch.args[0].num;
                    ops[nops++].f32 = (float) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
            if (branch.tag == VM_TAG_F64) {
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_F64_REG_REG_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type == VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_F64_REG_CONST_FUNC_FUNC);
                    ops[nops++].reg = branch.args[0].reg;
                    ops[nops++].f64 = (double) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_F64_CONST_REG_FUNC_FUNC);
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].reg = branch.args[1].reg;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
                if (branch.args[0].type != VM_ARG_REG && branch.args[1].type != VM_ARG_REG) {
                    ops[nops++].VM_OPCODE_PTR = VM_STATE_LOAD_PTR(state, VM_OPCODE_BEQ_F64_CONST_CONST_FUNC_FUNC);
                    ops[nops++].f64 = (double) branch.args[0].num;
                    ops[nops++].f64 = (double) branch.args[1].num;
                    ops[nops++].func = vm_rblock_new(branch.targets[0], types);
                    ops[nops++].func = vm_rblock_new(branch.targets[1], types);
                    break;
                }
            }
             __builtin_trap();
        }
        default: __builtin_trap();
    }
early_return:;
    vm_cache_set(&rblock->block->cache, rnext, ops);
    return ops;
fail_return:;
    vm_free(ops);
    return NULL;
}