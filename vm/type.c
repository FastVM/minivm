
#include "./ir.h"
#include "./interp/int3.h"

vm_rblock_t *vm_rblock_new(vm_block_t *block, vm_tags_t *regs) {
    vm_rblock_t *rblock = vm_malloc(sizeof(vm_rblock_t));
    rblock->block = block;
    rblock->regs = regs;
    rblock->start = 0;
    rblock->targets[0] = NULL;
    rblock->targets[1] = NULL;
    rblock->isfunc = rblock->block->isfunc;
    return rblock;
}

void *vm_cache_get(vm_cache_t *cache, vm_rblock_t *rblock) {
    for (size_t i = 0; i < cache->len; i++) {
        vm_rblock_t *found = cache->keys[i];
        if (rblock->start == found->start && rblock->block->isfunc == found->block->isfunc && vm_rblock_regs_match(rblock->regs, found->regs)) {
            return cache->values[i];
        }
    }
    return NULL;
}

void vm_cache_set(vm_cache_t *cache, vm_rblock_t *rblock, void *value) {
    if (cache->len + 1 >= cache->alloc) {
        cache->alloc = (cache->len + 1) * 2;
        cache->keys = vm_realloc(cache->keys, sizeof(vm_rblock_t *) * cache->alloc);
        cache->values = vm_realloc(cache->values, sizeof(void *) * cache->alloc);
    }
    cache->keys[cache->len] = rblock;
    cache->values[cache->len] = value;
    cache->len += 1;
}

vm_tags_t *vm_rblock_regs_empty(size_t ntags) {
    vm_tags_t *ret = vm_malloc(sizeof(vm_tags_t) + sizeof(vm_tag_t) * ntags);
    ret->ntags = ntags;
    for (size_t i = 0; i < ntags; i++) {
        ret->tags[i] = VM_TAG_UNK;
    }
    return ret;
}

vm_tags_t *vm_rblock_regs_dup(vm_tags_t *regs, size_t ntags) {
    vm_tags_t *ret = vm_malloc(sizeof(vm_tags_t) + sizeof(vm_tag_t) * ntags);
    ret->ntags = ntags;
    for (size_t i = 0; i < ret->ntags && i < regs->ntags; i++) {
        ret->tags[i] = regs->tags[i];
    }
    for (size_t i = regs->ntags; i < ret->ntags; i++) {
        ret->tags[i] = VM_TAG_UNK;
    }
    return ret;
}

bool vm_rblock_regs_match(vm_tags_t *a, vm_tags_t *b) {
    if (a->ntags != b->ntags) {
        return false;
    }
    for (size_t i = 0; i < a->ntags && i < b->ntags; i++) {
        if (!vm_tag_eq(a->tags[i], b->tags[i])) {
            return false;
        }
    }
    return true;
}

vm_instr_t vm_rblock_type_specialize_instr(vm_tags_t *types, vm_instr_t instr) {
    if (instr.op == VM_IOP_MOVE) {
        if (instr.args[0].type == VM_ARG_STR) {
            instr.tag = VM_TAG_PTR;
            return instr;
        }
    }
    if (vm_tag_eq(instr.tag, VM_TAG_UNK)) {
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                instr.tag = types->tags[instr.args[i].reg];
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

bool vm_rblock_type_check_instr(vm_tags_t *types, vm_instr_t instr) {
    if (instr.op != VM_IOP_CAST && instr.op != VM_IOP_CALL) {
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                if (!vm_tag_eq(types->tags[instr.args[i].reg], instr.tag)) {
                    vm_print_instr(stdout, instr);
                    printf("\n^ TYPE ERROR (arg r%zu of type #%zu) ^\n", instr.args[i].reg, (size_t)types->tags[instr.args[i].reg]);
                    return false;
                }
            }
        }
    }
    return true;
}

bool vm_rblock_type_check_branch(vm_tags_t *types, vm_branch_t branch) {
    return true;
}

vm_branch_t vm_rblock_type_specialize_branch(vm_tags_t *types, vm_branch_t branch) {
    if (vm_tag_eq(branch.tag, VM_TAG_UNK)) {
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_REG) {
                branch.tag = types->tags[branch.args[i].reg];
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
