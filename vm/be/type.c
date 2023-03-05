#include <stdint.h>

#include "./int3.h"

vm_state_t *vm_state_init(size_t nregs) {
    vm_state_t *ret = vm_malloc(sizeof(vm_state_t));
    ret->nlocals = nregs;
    ret->locals = vm_malloc(sizeof(vm_value_t) * (ret->nlocals));
    ret->ips = vm_malloc(sizeof(vm_opcode_t *) * (ret->nlocals / VM_NREGS));
    return ret;
}

void vm_state_deinit(vm_state_t *state) {
    vm_free(state->ips);
    vm_free(state->locals);
    vm_free(state);
}

vm_rblock_t *vm_rblock_new(vm_block_t *block, vm_tag_t *regs) {
    vm_rblock_t *rblock = vm_malloc(sizeof(vm_rblock_t));
    rblock->block = block;
    rblock->regs = regs;
    rblock->start = 0;
    return rblock;
}

vm_opcode_t *vm_cache_get(vm_cache_t *cache, vm_rblock_t *rblock) {
    for (size_t i = 0; i < cache->len; i++) {
        if (rblock->start == cache->keys[i]->start && vm_rblock_regs_match(rblock->regs, cache->keys[i]->regs)) {
            return cache->values[i];
        }
    }
    return NULL;
}

void vm_cache_set(vm_cache_t *cache, vm_rblock_t *rblock, vm_opcode_t *value) {
    if (cache->len + 1 >= cache->alloc) {
        cache->alloc = cache->len * 2 + 1;
        cache->keys = vm_realloc(cache->keys, sizeof(vm_rblock_t *) * cache->alloc);
        cache->values = vm_realloc(cache->values, sizeof(vm_opcode_t *) * cache->alloc);
    }
    cache->keys[cache->len] = rblock;
    cache->values[cache->len] = value;
    cache->len += 1;
}

vm_tag_t *vm_rblock_regs_empty(void) {
    vm_tag_t *ret = vm_malloc(sizeof(vm_tag_t) * VM_NREGS);
    for (size_t i = 0; i < VM_NREGS; i++) {
        ret[i] = VM_TAG_UNK;
    }
    return ret;
}

vm_tag_t *vm_rblock_regs_dup(vm_tag_t *regs) {
    vm_tag_t *ret = vm_malloc(sizeof(vm_tag_t) * VM_NREGS);
    for (size_t i = 0; i < VM_NREGS; i++) {
        ret[i] = regs[i];
    }
    return ret;
}

bool vm_rblock_regs_match(vm_tag_t *a, vm_tag_t *b) {
    for (size_t i = 0; i < VM_NREGS; i++) {
        if (!vm_tag_eq(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

vm_instr_t vm_rblock_type_specialize_instr(vm_tag_t *types, vm_instr_t instr) {
    if (instr.op == VM_IOP_MOVE) {
        if (instr.args[0].type == VM_ARG_STR) {
            instr.tag = VM_TAG_PTR;
            return instr;
        }
    }
    if (vm_tag_eq(instr.tag, VM_TAG_UNK)) {
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                instr.tag = types[instr.args[i].reg];
                return instr;
            }
        }
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_NUM) {
                if (fmod(instr.args[i].num, 1) == 0) {
                    instr.tag = VM_TAG_I64;
                } else {
                    instr.tag = VM_TAG_F64;
                }
                return instr;
            }
        }
    }
    return instr;
}

bool vm_rblock_type_check_instr(vm_tag_t *types, vm_instr_t instr) {
    if (instr.op != VM_IOP_CAST) {
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                if (!vm_tag_eq(types[instr.args[i].reg], instr.tag)) {
                    vm_print_instr(stdout, instr);
                    printf("\n^ TYPE ERROR ^\n");
                    return false;
                }
            }
        }
    }
    return true;
}

bool vm_rblock_type_check_branch(vm_tag_t *types, vm_branch_t branch) {
    return true;
}

vm_branch_t vm_rblock_type_specialize_branch(vm_tag_t *types, vm_branch_t branch) {
    if (vm_tag_eq(branch.tag, VM_TAG_UNK)) {
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_REG) {
                branch.tag = types[branch.args[i].reg];
                return branch;
            }
        }
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_NUM) {
                if (fmod(branch.args[i].num, 1) == 0) {
                    branch.tag = VM_TAG_I64;
                } else {
                    branch.tag = VM_TAG_F64;
                }
                return branch;
            }
        }
    }
    return branch;
}
