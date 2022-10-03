
#include "../ir.h"

void vm_ir_be_racket_arg(FILE *of, vm_ir_arg_t arg) {
    switch (arg->type) {
        case VM_IR_ARG_NIL: {
            break;
        }
        case VM_IR_ARG_BOOL: {
            break;
        }
        case VM_IR_ARG_REG: {
            break;
        }
        case VM_IR_ARG_NUM: {
            break;
        }
        case VM_IR_ARG_STR: {
            break;
        }
        case VM_IR_ARG_EXTERN: {
            break;
        }
        case VM_IR_ARG_FUNC: {
            break;
        }
    }
}

void vm_ir_be_racket(FILE *of, size_t nargs, vm_ir_block_t *blocks) {
    for (size_t nblock = 0; nblock < nargs; nblock++) {
        vm_ir_block_t *block = &blocks[nblock];
        if (block->id < 0) {
            continue;
        }
        for (size_t ninstr = 0; ninstr < block->len; ninstr++) {
            vm_ir_instr_t *instr = block->instrs[ninstr];
            switch (instr->op) {
                case VM_IR_IOP_NOP: {
                    break;
                }
                case VM_IR_IOP_MOVE: {
                    break;
                }
                case VM_IR_IOP_ADD: {
                    break;
                }
                case VM_IR_IOP_SUB: {
                    break;
                }
                case VM_IR_IOP_MUL: {
                    break;
                }
                case VM_IR_IOP_DIV: {
                    break;
                }
                case VM_IR_IOP_MOD: {
                    break;
                }
                case VM_IR_IOP_CALL: {
                    break;
                }
                case VM_IR_IOP_ARR: {
                    break;
                }
                case VM_IR_IOP_TAB: {
                    break;
                }
                case VM_IR_IOP_GET: {
                    break;
                }
                case VM_IR_IOP_SET: {
                    break;
                }
                case VM_IR_IOP_LEN: {
                    break;
                }
                case VM_IR_IOP_TYPE: {
                    break;
                }
                case VM_IR_IOP_OUT: {
                    break;
                }
                case VM_IR_IOP_IN: {
                    break;
                }
                case VM_IR_IOP_BOR: {
                    break;
                }
                case VM_IR_IOP_BAND: {
                    break;
                }
                case VM_IR_IOP_BXOR: {
                    break;
                }
                case VM_IR_IOP_BSHL: {
                    break;
                }
                case VM_IR_IOP_BSHR: {
                    break;
                }
            }
        }
        switch (block->branch->op) {
            case VM_IR_BOP_JUMP: {
                break;
            }
            case VM_IR_BOP_BOOL: {
                break;
            }
            case VM_IR_BOP_LESS: {
                break;
            }
            case VM_IR_BOP_EQUAL: {
                break;
            }
            case VM_IR_BOP_RET: {
                break;
            }
            case VM_IR_BOP_EXIT: {
                break;
            }
        }
    }
}
