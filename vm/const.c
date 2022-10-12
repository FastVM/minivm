
#include "const.h"

enum {
    VM_OPT_CONST_REG_NOT_SET,
    VM_OPT_CONST_REG_NOT_NEEDED,
    VM_OPT_CONST_REG_NEEDED,
    VM_OPT_CONST_REG_HAS_VALUE,
};

void vm_opt_const(size_t nops, vm_block_t *blocks) {
    bool redo = true;
    while (redo) {
        redo = false;
        for (size_t i = 0; i < nops; i++) {
            vm_block_t *block = &blocks[i];
            if (block->id < 0) {
                continue;
            }
            uint8_t *named = vm_alloc0(sizeof(uint8_t) * block->nregs);
            double *regs = vm_alloc0(sizeof(double) * block->nregs);
            for (size_t j = 0; j < block->len; j++) {
                vm_instr_t *instr = &block->instrs[j];
                for (size_t k = 0; instr->args[k].type != VM_ARG_NONE; k++) {
                    vm_arg_t arg = instr->args[k];
                    if (arg.type == VM_ARG_REG) {
                        if (named[arg.reg] == VM_OPT_CONST_REG_HAS_VALUE) {
                            arg.type = VM_ARG_NUM;
                            arg.num = regs[arg.reg];
                        } else if (named[arg.reg] == VM_OPT_CONST_REG_NOT_NEEDED) {
                            named[arg.reg] = VM_OPT_CONST_REG_NEEDED;
                        }
                    }
                    if (instr->op != VM_IOP_CALL) {
                        instr->args[k] = arg;
                    }
                }
                if (instr->out.type != VM_ARG_NONE) {
                    vm_arg_t out = instr->out;
                    named[out.reg] = VM_OPT_CONST_REG_NOT_NEEDED;
                    if (instr->op == VM_IOP_MOVE) {
                        vm_arg_t arg0 = instr->args[0];
                        if (arg0.type == VM_ARG_NUM) {
                            named[out.reg] = VM_OPT_CONST_REG_HAS_VALUE;
                            regs[out.reg] = arg0.num;
                        }
                    }
                    if (instr->op == VM_IOP_ADD) {
                        vm_arg_t arg0 = instr->args[0];
                        vm_arg_t arg1 = instr->args[1];
                        if (arg0.type == VM_ARG_NUM && arg1.type == VM_ARG_NUM) {
                            if (arg0.num == 0) {
                                instr->op = VM_IOP_MOVE;
                                instr->args[0] = arg1;
                            } else if (arg1.num == 0) {
                                instr->op = VM_IOP_MOVE;
                                instr->args[0] = arg0;
                            } else {
                                named[out.reg] = VM_OPT_CONST_REG_HAS_VALUE;
                                regs[out.reg] = arg0.num + arg1.num;
                                instr->op = VM_IOP_MOVE;
                                instr->args[0].num = arg0.num + arg1.num;
                            }
                            redo = true;
                        }
                    }
                    if (instr->op == VM_IOP_SUB) {
                        vm_arg_t arg0 = instr->args[0];
                        vm_arg_t arg1 = instr->args[1];
                        if (arg0.type == VM_ARG_NUM && arg1.type == VM_ARG_NUM) {
                            if (arg1.num == 0) {
                                instr->op = VM_IOP_MOVE;
                                instr->args[0] = arg0;
                            } else {
                                named[out.reg] = VM_OPT_CONST_REG_HAS_VALUE;
                                regs[out.reg] = arg0.num - arg1.num;
                                instr->op = VM_IOP_MOVE;
                                instr->args[0].num = arg0.num - arg1.num;
                            }
                            redo = true;
                        }
                    }
                    if (instr->op == VM_IOP_MUL) {
                        vm_arg_t arg0 = instr->args[0];
                        vm_arg_t arg1 = instr->args[1];
                        if (arg0.type == VM_ARG_NUM && arg1.type == VM_ARG_NUM) {
                            if (arg0.num == 1) {
                                instr->op = VM_IOP_MOVE;
                                instr->args[0] = arg1;
                            } else if (arg1.num == 1) {
                                instr->op = VM_IOP_MOVE;
                                instr->args[0] = arg0;
                            } else {
                                named[out.reg] = VM_OPT_CONST_REG_HAS_VALUE;
                                regs[out.reg] = arg0.num * arg1.num;
                                instr->op = VM_IOP_MOVE;
                                instr->args[0].num = arg0.num * arg1.num;
                            }
                            redo = true;
                        }
                    }
                }
            }
            for (size_t j = 0; j < 2; j++) {
                vm_arg_t arg = block->branch.args[j];
                if (arg.type == VM_ARG_REG &&
                    named[arg.reg] == VM_OPT_CONST_REG_HAS_VALUE) {
                    arg.type = VM_ARG_NUM;
                    arg.num = regs[arg.reg];
                }
                block->branch.args[j] = arg;
            }
            vm_free(named);
            vm_free(regs);
        }
    }
}

void vm_opt_dead(size_t nops, vm_block_t *blocks) {
    for (size_t i = 0; i < nops; i++) {
        vm_block_t *block = &blocks[i];
        if (block->id < 0) {
            continue;
        }
        uint8_t *ptrs = vm_alloc0(sizeof(uint8_t) * block->nregs);
        for (size_t t = 0; t < 2; t++) {
            if (block->branch.targets[t] == NULL) {
                break;
            }
            for (size_t j = 0; j < block->branch.targets[t]->nargs; j++) {
                ptrs[block->branch.targets[t]->args[j]] = 1;
            }
        }
        for (size_t r = 0; r < 2; r++) {
            if (block->branch.args[r].type == VM_ARG_REG) {
                ptrs[block->branch.args[r].reg] = 1;
            }
        }
        for (ptrdiff_t j = (ptrdiff_t)block->len - 1; j >= 0; j--) {
            vm_instr_t *instr = &block->instrs[j];
            uint8_t outp = 1;
            if (instr->out.type == VM_ARG_REG) {
                outp = ptrs[instr->out.reg];
                if (outp == 0 && instr->op != VM_IOP_CALL) {
                    // block->instrs[j] = vm_new(vm_instr_t, .op = VM_IOP_NOP);
                    block->instrs[j].op = VM_IOP_NOP;
                }
                ptrs[instr->out.reg] = 0;
            } else if (instr->op != VM_IOP_CALL && instr->op != VM_IOP_SET &&
                       instr->op != VM_IOP_OUT) {
                instr->op = VM_IOP_NOP;
            }
            if (instr->op == VM_IOP_NOP) {
                continue;
            }
            for (size_t k = 0; instr->args[k].type != VM_ARG_NONE; k++) {
                if (instr->args[k].type != VM_ARG_REG) {
                    continue;
                }
                ptrs[instr->args[k].reg] = 1;
            }
        }
        vm_free(ptrs);
    }
}
