
#include "rblock.h"

#include "ir.h"
#include "type.h"

void vm_rblock_reset(vm_rblock_t *rblock) {
    // printf("rblock = %p\n", rblock);
    if (rblock == NULL) {
        return;
    }
    // printf("rblock->jit = %p\n", rblock->jit);
    // printf("rblock->code = %p\n", rblock->code);
    // printf("rblock->del = %p\n", rblock->del);
    if (rblock->del != NULL) {
        rblock->del(rblock);
    }
    vm_free(rblock->regs->tags);
    vm_free(rblock->regs);
    vm_free(rblock);
}

vm_block_t *vm_rblock_version(vm_blocks_t *blocks, vm_rblock_t *rblock) {
    if (rblock->cache != NULL) {
        return rblock->cache;
    }
    void *cache = vm_cache_get(&rblock->block->cache, rblock);
    if (cache != NULL) {
        return cache;
    }
    vm_block_t *ret = vm_malloc(sizeof(vm_block_t));
    vm_cache_set(&rblock->block->cache, rblock, ret);
    rblock->cache = cache;
    vm_tags_t *regs = vm_rblock_regs_dup(rblock->regs);
    *ret = *rblock->block;
    ret->label = -1;
    ret->instrs = vm_malloc(sizeof(vm_instr_t) * rblock->block->len);
    ret->args = vm_malloc(sizeof(vm_arg_t) * ret->nargs);
    ret->mark = false;
    for (size_t i = 0; i < ret->nargs; i++) {
        ret->args[i] = rblock->block->args[i];
        if (ret->args[i].type != VM_ARG_REG) {
            __builtin_trap();
        }
    }
    for (size_t ninstr = 0; ninstr < rblock->block->len; ninstr++) {
        vm_instr_t instr = vm_rblock_type_specialize_instr(regs, rblock->block->instrs[ninstr]);
        size_t nargs = 1;
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            nargs += 1;
        }
        vm_arg_t *args = vm_malloc(sizeof(vm_arg_t) * nargs);
        memcpy(args, instr.args, sizeof(vm_arg_t) * nargs);
        instr.args = args;
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                instr.args[i].reg_tag = regs->tags[instr.args[i].reg];
            }
        }
        ret->instrs[ninstr] = instr;
        if (instr.out.type == VM_ARG_REG) {
            regs->tags[instr.out.reg] = instr.tag;
        }
    }
    vm_branch_t branch = vm_rblock_type_specialize_branch(regs, rblock->block->branch);
    size_t nargs = 1;
    for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
        nargs += 1;
    }
    vm_arg_t *args = vm_malloc(sizeof(vm_arg_t) * nargs);
    memcpy(args, branch.args, sizeof(vm_arg_t) * nargs);
    branch.args = args;
    for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
        if (branch.args[i].type == VM_ARG_REG) {
            branch.args[i].reg_tag = regs->tags[branch.args[i].reg];
        }
    }
    switch (branch.op) {
        case VM_BOP_GET: {
            if (branch.args[1].type == VM_ARG_REG) {
                branch.tag = regs->tags[branch.args[1].reg];
            } else if (branch.args[1].type == VM_ARG_LIT) {
                branch.tag = branch.args[1].lit.tag;
            }
            vm_block_t *from = branch.targets[0];
            for (size_t i = 0; i < from->nargs; i++) {
                vm_arg_t *arg = &from->args[i];
                if (arg->type == VM_ARG_REG) {
                    if (arg->reg != branch.out.reg) {
                        arg->reg_tag = regs->tags[arg->reg];
                    }
                }
            }
            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                regs->tags[branch.out.reg] = i;
                branch.rtargets[i] = vm_rblock_new(from, regs);
            }
            break;
        }
        case VM_BOP_CALL: {
            vm_block_t *from = branch.targets[0];
            if (branch.args[0].type == VM_ARG_FUN) {
                vm_tags_t *regs2 = vm_rblock_regs_empty(branch.args[0].func->nregs);
                for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
                    if (branch.args[i].type == VM_ARG_REG) {
                        regs2->tags[branch.args[i].reg] = branch.args[i].reg_tag;
                    }
                }
                vm_rblock_t *rblock = vm_rblock_new(branch.args[0].func, regs2);
                branch.args[0] = (vm_arg_t){
                    .type = VM_ARG_RFUNC,
                    .rfunc = rblock,
                };
            }
            if (branch.args[0].type == VM_ARG_REG) {
                if (branch.args[0].reg_tag == VM_TAG_FUN) {
                    size_t nargs = 0;
                    for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                        nargs += 1;
                    }
                    branch.call_table = vm_malloc(sizeof(vm_rblock_t *) * blocks->len);
                    for (size_t j = 0; j < blocks->len; j++) {
                        if (!blocks->blocks[j]->isfunc || blocks->blocks[j]->nargs != nargs) {
                            branch.call_table[j] = NULL;
                            continue;
                        }
                        vm_tags_t *regs2 = vm_rblock_regs_empty(blocks->blocks[j]->nregs);
                        for (size_t i = 1; i <= blocks->blocks[j]->nargs; i++) {
                            if (i <= nargs) {
                                regs2->tags[blocks->blocks[j]->args[i - 1].reg] = vm_arg_to_tag(branch.args[i]);
                            } else {
                                regs2->tags[blocks->blocks[j]->args[i - 1].reg] = VM_TAG_NIL;
                            }
                        }
                        branch.call_table[j] = vm_rblock_new(blocks->blocks[j], regs2);
                    }
                }
                if (branch.args[0].reg_tag == VM_TAG_CLOSURE) {
                    size_t nargs = 0;
                    for (size_t i = 1; branch.args[i].type != VM_ARG_NONE; i++) {
                        nargs += 1;
                    }
                    branch.call_table = vm_malloc(sizeof(vm_rblock_t *) * blocks->len);
                    for (size_t j = 0; j < blocks->len; j++) {
                        if (!blocks->blocks[j]->isfunc) {
                            branch.call_table[j] = NULL;
                            continue;
                        }
                        vm_tags_t *regs2 = vm_rblock_regs_empty(blocks->blocks[j]->nregs);
                        if (blocks->blocks[j]->nargs != 0) {
                            regs2->tags[blocks->blocks[j]->args[0].reg] = VM_TAG_CLOSURE;
                            for (size_t i = 1; i < blocks->blocks[j]->nargs; i++) {
                                if (i <= nargs) {
                                    regs2->tags[blocks->blocks[j]->args[i].reg] = vm_arg_to_tag(branch.args[i]);
                                } else {
                                    regs2->tags[blocks->blocks[j]->args[i].reg] = VM_TAG_NIL;
                                }
                            }
                        }
                        branch.call_table[j] = vm_rblock_new(blocks->blocks[j], regs2);
                    }
                }
            }
            for (size_t i = 0; i < from->nargs; i++) {
                vm_arg_t *arg = &from->args[i];
                if (arg->type == VM_ARG_REG) {
                    if (arg->reg != branch.out.reg) {
                        arg->reg_tag = regs->tags[arg->reg];
                    }
                }
            }
            for (size_t i = 1; i < VM_TAG_MAX; i++) {
                regs->tags[branch.out.reg] = i;
                branch.rtargets[i] = vm_rblock_new(from, regs);
            }
            break;
        }
        case VM_BOP_JUMP: {
            branch.targets[0] = vm_rblock_version(blocks, vm_rblock_new(branch.targets[0], regs));
            if (branch.targets[0] == NULL) {
                ret = NULL;
                return ret;
            }
            break;
        }
        case VM_BOP_BB:
        case VM_BOP_BEQ:
        case VM_BOP_BLT:
        case VM_BOP_BLE: {
            branch.targets[0] = vm_rblock_version(blocks, vm_rblock_new(branch.targets[0], regs));
            branch.targets[1] = vm_rblock_version(blocks, vm_rblock_new(branch.targets[1], regs));
            if (branch.targets[0] == NULL) {
                ret = NULL;
                return ret;
            }
            if (branch.targets[1] == NULL) {
                ret = NULL;
                return ret;
            }
            break;
        }
        case VM_BOP_RET: {
            break;
        }
        default: {
            __builtin_trap();
        }
    }
    ret->branch = branch;
    for (size_t i = 0; i < ret->nargs; i++) {
        if (ret->args[i].type == VM_ARG_REG) {
            ret->args[i].reg_tag = regs->tags[ret->args[i].reg];
        }
    }
ret:;
    vm_free(regs->tags);
    vm_free(regs);
    // fprintf(stdout, "\n--- version ---\n");
    // vm_io_format_block(stdout, ret);
    return ret;
}