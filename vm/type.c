
#include "ir.h"

vm_rblock_t *vm_rblock_new(vm_block_t *block, vm_tags_t *regs) {
    vm_rblock_t *rblock = vm_malloc(sizeof(vm_rblock_t));
    rblock->block = block;
    rblock->regs = regs;
    rblock->cache = NULL;
    return rblock;
}

void *vm_cache_get(vm_cache_t *cache, vm_rblock_t *rblock) {
    for (ptrdiff_t i = cache->len - 1; i >= 0; i--) {
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
    for (ptrdiff_t i = cache->len - 1; i >= 0; i--) {
        vm_rblock_t *found = cache->keys[i];
        if (rblock->block->isfunc == found->block->isfunc &&
            rblock->block == found->block) {
            for (size_t j = 0; j < rblock->block->nargs; j++) {
                vm_arg_t arg = rblock->block->args[j];
                if (rblock->regs->tags[arg.reg] != found->regs->tags[arg.reg]) {
                    goto next;
                }
            }
            cache->values[i] = value;
            return;
        }
    next:;
    }
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
        if (a->tags[i] != b->tags[i]) {
            return false;
        }
    }
    return true;
}

vm_instr_t vm_rblock_type_specialize_instr(vm_tags_t *types, vm_instr_t instr) {
    if (instr.op == VM_IOP_STD) {
        instr.tag = VM_TAG_TABLE;
        return instr;
    }
    if (instr.op == VM_IOP_MOVE) {
        if (instr.args[0].type == VM_ARG_STR) {
            instr.tag = VM_TAG_STR;
            return instr;
        }
    }
    if (instr.op == VM_IOP_NEW) {
        instr.tag = VM_TAG_TABLE;
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
                instr.tag = VM_TAG_F64;
#if defined(VM_INTS)
                if (fmod(instr.args[i].num, 1) == 0) {
                    instr.tag = VM_TAG_I64;
                }
#endif
                return instr;
            }
        }
    }
    return instr;
}

bool vm_rblock_type_check_instr(vm_tags_t *types, vm_instr_t instr) {
    if (instr.op == VM_IOP_SET) {
    } else if (instr.op == VM_IOP_LEN) {
    } else {
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                if (types->tags[instr.args[i].reg] != instr.tag) {
                    vm_print_instr(stdout, instr);
                    printf("\n^ TYPE ERROR (arg %%%zu of type #%zu) ^\n",
                           (size_t)instr.args[i].reg,
                           (size_t)types->tags[instr.args[i].reg]);
                    return false;
                }
            }
        }
    }
    return true;
}

bool vm_rblock_type_check_branch(vm_tags_t *types, vm_branch_t branch) {
    (void)types;
    (void)branch;
    return true;
}

vm_branch_t vm_rblock_type_specialize_branch(vm_tags_t *types,
                                             vm_branch_t branch) {
    if (branch.op == VM_BOP_GET) {
        return branch;
    } else if (branch.op == VM_BOP_CALL) {
        if (branch.args[0].type == VM_ARG_FFI) {
            branch.tag = VM_TAG_FFI;
            return branch;
        } else {
            return branch;
        }
    } else if (branch.tag == VM_TAG_UNK) {
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_REG) {
                branch.tag = types->tags[branch.args[i].reg];
                return branch;
            }
        }
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_NUM) {
                branch.tag = VM_TAG_F64;
#if defined(VM_INTS)
                if (fmod(branch.args[i].num, 1) == 0) {
                    branch.tag = VM_TAG_I64;
                }
#endif
                return branch;
            }
        }
    }
    return branch;
}
