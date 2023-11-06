
#include "type.h"

#include "ir.h"

vm_rblock_t *vm_rblock_new(vm_block_t *block, vm_tags_t *regs) {
    vm_rblock_t *rblock = vm_malloc(sizeof(vm_rblock_t));
    rblock->block = block;
    rblock->regs = regs;
    rblock->jit = NULL;
    rblock->count = 0;
    rblock->least_faults = SIZE_MAX;
    rblock->base_redo = 256;
    rblock->redo = 0;
    return rblock;
}

void vm_cache_new(vm_cache_t *out) {
    *out = (vm_cache_t){0};
}

void *vm_cache_get(vm_cache_t *cache, vm_rblock_t *rblock) {
    for (ptrdiff_t i = (ptrdiff_t)cache->len - 1; i >= 0; i--) {
        vm_rblock_t *found = cache->keys[i];
        if (rblock->block->isfunc == found->block->isfunc &&
            rblock->block == found->block) {
            for (size_t j = 0; j < rblock->block->nargs; j++) {
                vm_arg_t arg = rblock->block->args[j];
                if (rblock->regs->tags[arg.reg] != found->regs->tags[arg.reg]) {
                    goto next;
                }
            }
            return cache->values[i];
        }
    next:;
    }
    return NULL;
}

void vm_cache_set(vm_cache_t *cache, vm_rblock_t *rblock, void *value) {
    // for (ptrdiff_t i = (ptrdiff_t) cache->len - 1; i >= 0; i--) {
    //     vm_rblock_t *found = cache->keys[i];
    //     if (rblock->block->isfunc == found->block->isfunc &&
    //         rblock->block == found->block) {
    //         for (size_t j = 0; j < rblock->block->nargs; j++) {
    //             vm_arg_t arg = rblock->block->args[j];
    //             if (rblock->regs->tags[arg.reg] != found->regs->tags[arg.reg]) {
    //                 goto next;
    //             }
    //         }
    //         __builtin_trap();
    //         // cache->values[i] = value;
    //         // return;
    //     }
    // next:;
    // }
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
    vm_tags_t *ret = vm_malloc(sizeof(vm_tags_t));
    ret->ntags = ntags;
    ret->tags = vm_malloc(sizeof(vm_tag_t) * ntags);
    for (size_t i = 0; i < ntags; i++) {
        ret->tags[i] = VM_TAG_UNK;
    }
    return ret;
}

vm_tags_t *vm_rblock_regs_dup(vm_tags_t *regs, size_t ntags) {
    vm_tags_t *ret = vm_malloc(sizeof(vm_tags_t));
    ret->ntags = ntags;
    ret->tags = vm_malloc(sizeof(vm_tag_t) * ntags);
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
        if (a->tags[i] != b->tags[i]) {
            return false;
        }
    }
    return true;
}

vm_instr_t vm_rblock_type_specialize_instr(vm_tags_t *types, vm_instr_t instr) {
    if (instr.op == VM_IOP_STD) {
        instr.tag = VM_TAG_TAB;
        return instr;
    }
    if (instr.op == VM_IOP_MOVE) {
        if (instr.args[0].type == VM_ARG_STR) {
            instr.tag = VM_TAG_STR;
            return instr;
        }
        if (instr.args[0].type == VM_ARG_FUNC) {
            instr.tag = VM_TAG_FUN;
            return instr;
        }
    }
    if (instr.op == VM_IOP_NEW) {
        instr.tag = VM_TAG_TAB;
        return instr;
    }
    if (instr.op == VM_IOP_LEN) {
        instr.tag = VM_TAG_F64;
        return instr;
    }
    if (instr.tag == VM_TAG_UNK) {
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                instr.tag = types->tags[instr.args[i].reg];
                return instr;
            }
        }
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_NUM) {
                instr.tag = instr.args[i].num.tag;
                return instr;
            }
        }
        instr.tag = VM_TAG_NIL;
    }
    return instr;
}

vm_branch_t vm_rblock_type_specialize_branch(vm_tags_t *types, vm_branch_t branch) {
    if (branch.op == VM_BOP_GET) {
        return branch;
    } else if (branch.op == VM_BOP_CALL) {
        return branch;
    } else if (branch.tag == VM_TAG_UNK) {
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_REG) {
                branch.tag = types->tags[branch.args[i].reg];
                return branch;
            }
        }
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_NUM) {
                branch.tag = branch.args[i].num.tag;
                return branch;
            }
        }
        branch.tag = VM_TAG_NIL;
    }
    return branch;
}
