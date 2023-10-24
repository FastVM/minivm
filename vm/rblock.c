
#include "rblock.h"
#include "type.h"
#include "check.h"

vm_block_t *vm_rblock_version(vm_rblock_t *rblock) {
    void *cache = vm_cache_get(&rblock->block->cache, rblock);
    if (cache != NULL) {
        return cache;
    }
    vm_block_t *ret = vm_malloc(sizeof(vm_block_t));
    vm_cache_set(&rblock->block->cache, rblock, ret);
    vm_tags_t *regs = vm_rblock_regs_dup(rblock->regs, 256);
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
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                instr.args[i].reg_tag = regs->tags[instr.args[i].reg];
            }
        }
        if (instr.op == VM_IOP_SET) {
            if (instr.args[1].type == VM_ARG_REG) {
                instr.args[3] = (vm_arg_t){
                    .type = VM_ARG_TAG,
                    .tag = instr.args[1].reg_tag,
                };
            } else if (instr.args[1].type == VM_ARG_NUM) {
                instr.args[3] = (vm_arg_t){
                    .type = VM_ARG_TAG,
                    .tag = instr.args[1].num.tag,
                };
            }
            if (instr.args[2].type == VM_ARG_REG) {
                instr.args[4] = (vm_arg_t){
                    .type = VM_ARG_TAG,
                    .tag = instr.args[2].reg_tag,
                };
            } else if (instr.args[2].type == VM_ARG_NUM) {
                instr.args[4] = (vm_arg_t){
                    .type = VM_ARG_TAG,
                    .tag = instr.args[1].num.tag,
                };
            }
        }
        ret->instrs[ninstr] = instr;
        if (instr.out.type == VM_ARG_REG) {
            regs->tags[instr.out.reg] = instr.tag;
        }
    }
    vm_branch_t branch = vm_rblock_type_specialize_branch(regs, rblock->block->branch);
    for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
        if (branch.args[i].type == VM_ARG_REG) {
            branch.args[i].reg_tag = regs->tags[branch.args[i].reg];
        }
    }
    switch (branch.op) {
        case VM_BOP_GET: {
            if (branch.args[1].type == VM_ARG_REG) {
                branch.tag = regs->tags[branch.args[1].reg];
            } else if (branch.args[1].type == VM_ARG_NUM) {
                branch.tag = branch.args[1].num.tag;
            }
            if (branch.args[1].type == VM_ARG_REG) {
                branch.args[3] = (vm_arg_t){
                    .type = VM_ARG_TAG,
                    .tag = branch.args[1].reg_tag,
                };
            } else if (branch.args[1].type == VM_ARG_NUM) {
                branch.args[3] = (vm_arg_t){
                    .type = VM_ARG_TAG,
                    .tag = branch.args[1].num.tag,
                };
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
                branch.rtargets[i] = vm_rblock_new(from, vm_rblock_regs_dup(regs, 256));
            }
            break;
        }
        case VM_BOP_CALL: {
            vm_tags_t *regs2 = vm_rblock_regs_empty(256);
            for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
                if (branch.args[i].type == VM_ARG_REG) {
                    regs2->tags[i] = branch.args[i].reg_tag;
                }
            }
            if (branch.args[0].type == VM_ARG_FUNC) {
                vm_rblock_t *rblock = vm_rblock_new(branch.args[0].func, regs2);
                branch.args[0] = (vm_arg_t){
                    .type = VM_ARG_RFUNC,
                    .rfunc = rblock,
                };
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
                branch.rtargets[i] = vm_rblock_new(from, vm_rblock_regs_dup(regs, 256));
            }
            break;
        }
        case VM_BOP_JUMP: {
            branch.targets[0] = vm_rblock_version(vm_rblock_new(branch.targets[0], vm_rblock_regs_dup(regs, 256)));
            if (branch.targets[0] == NULL) {
                return NULL;
            }
            break;
        }
        case VM_BOP_BB:
        case VM_BOP_BEQ:
        case VM_BOP_BLT: {
            branch.targets[0] = vm_rblock_version(vm_rblock_new(branch.targets[0], vm_rblock_regs_dup(regs, 256)));
            branch.targets[1] = vm_rblock_version(vm_rblock_new(branch.targets[1], vm_rblock_regs_dup(regs, 256)));
            if (branch.targets[0] == NULL) {
                return NULL;
            }
            if (branch.targets[1] == NULL) {
                return NULL;
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
    if (!vm_check_block(ret)) {
        return NULL;
    }
    return ret;
}