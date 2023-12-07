
#include "type.h"

#include "ir.h"

vm_rblock_t *vm_rblock_new(vm_block_t *block, vm_tags_t *regs) {
    vm_rblock_t *rblock = vm_malloc(sizeof(vm_rblock_t));
    *rblock = (vm_rblock_t){
        .block = block,
        .jit = NULL,
    };
    rblock->regs = vm_rblock_regs_dup(regs);
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

vm_tags_t *vm_rblock_regs_dup(vm_tags_t *regs) {
    vm_tags_t *ret = vm_malloc(sizeof(vm_tags_t));
    ret->ntags = regs->ntags;
    ret->tags = vm_malloc(sizeof(vm_tag_t) * ret->ntags);
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
    if (instr.tag != VM_TAG_UNK) {
        __builtin_trap();
    }
    if (instr.op == VM_IOP_STD) {
        instr.tag = VM_TAG_TAB;
        goto ret;
    }
    if (instr.op == VM_IOP_MOVE) {
        if (instr.args[0].type == VM_ARG_FUN) {
            instr.tag = VM_TAG_FUN;
            goto ret;
        }
    }
    if (instr.op == VM_IOP_NEW) {
        instr.tag = VM_TAG_TAB;
        goto ret;
    }
    if (instr.op == VM_IOP_LEN) {
        instr.tag = VM_TAG_I32;
        goto ret;
    }
    if (instr.tag == VM_TAG_UNK) {
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                instr.tag = types->tags[instr.args[i].reg];
                goto ret;
            }
        }
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_LIT) {
                instr.tag = instr.args[i].lit.tag;
                goto ret;
            }
        }
        instr.tag = VM_TAG_NIL;
    }
ret:;
    return instr;
}

vm_branch_t vm_rblock_type_specialize_branch(vm_tags_t *types, vm_branch_t branch) {
    if (branch.tag != VM_TAG_UNK) {
        __builtin_trap();
    }
    if (branch.op == VM_BOP_GET) {
        goto ret;
    } else if (branch.op == VM_BOP_CALL) {
        goto ret;
    } else if (branch.tag == VM_TAG_UNK) {
        for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
            if (branch.args[i].type == VM_ARG_REG) {
                branch.tag = types->tags[branch.args[i].reg];
                goto ret;
            }
        }
        for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
            if (branch.args[i].type == VM_ARG_LIT) {
                branch.tag = branch.args[i].lit.tag;
                goto ret;
            }
        }
        branch.tag = VM_TAG_NIL;
    }
ret:;
    return branch;
}
