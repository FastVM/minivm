
#include "../ir.h"

void vm_ir_be_racket_block(FILE *of, vm_ir_block_t *arg) {
    fprintf(of, "(l%zi return", arg->id);
    for (size_t i = 0; i < arg->nargs; i++) {
        fprintf(of, " r%zi", arg->args[i]);
    }
    fprintf(of, ")");
}

void vm_ir_be_racket_arg(FILE *of, vm_ir_arg_t arg) {
    switch (arg.type) {
        case VM_IR_ARG_NIL: {
            fprintf(of, "(void)");
            break;
        }
        case VM_IR_ARG_BOOL: {
            if (arg.logic) {
                fprintf(of, "#t");
            } else {
                fprintf(of, "#f");
            }
            break;
        }
        case VM_IR_ARG_REG: {
            fprintf(of, "r%zu", arg.reg);
            break;
        }
        case VM_IR_ARG_NUM: {
            fprintf(of, "%lf", arg.num);
            break;
        }
        case VM_IR_ARG_STR: {
            __builtin_trap();
        }
        case VM_IR_ARG_EXTERN: {
            __builtin_trap();
        }
        case VM_IR_ARG_FUNC: {
            fprintf(of, "l%zi", arg.func->id);
            break;
        }
    }
}

void vm_ir_be_racket(FILE *of, size_t nargs, vm_ir_block_t *blocks) {
    fprintf(of, "#lang racket/base\n");
    fprintf(of, "(define (fmod x m) (- x (* (floor (/ x m)) m)))\n");
    fprintf(of, "(define (int x) (inexact->exact (floor x)))\n");
    for (size_t nblock = 0; nblock < nargs; nblock++) {
        vm_ir_block_t *block = &blocks[nblock];
        if (block->id < 0) {
            continue;
        }
        uint8_t *known = vm_alloc0(sizeof(size_t) * block->nregs);
        fprintf(of, "(define (l%zi return", block->id);
        for (int i = 0; i < block->nargs; i++) {
            fprintf(of, " r%zu", block->args[i]);
            known[block->args[i]] = 1;
        }
        fprintf(of, ") ");
        for (size_t i = 0; i < block->nregs; i++) {
            if (!known[i]) {
                fprintf(of, "(define r%zu (void))", i);
            }
        }
        for (size_t ninstr = 0; ninstr < block->len; ninstr++) {
            vm_ir_instr_t *instr = block->instrs[ninstr];
            switch (instr->op) {
                case VM_IR_IOP_NOP: {
                    break;
                }
                case VM_IR_IOP_MOVE: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_ADD: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (+ ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, " ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, ")");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_SUB: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (- ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, " ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, ")");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_MUL: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (* ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, " ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, ")");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_DIV: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (/ ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, " ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, ")");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_MOD: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (fmod ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, " ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, ")");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_CALL: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (let/cc return (");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, " return");
                    for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
                        fprintf(of, " ");
                        vm_ir_be_racket_arg(of, instr->args[i]);
                    }
                    fprintf(of, ")");
                    fprintf(of, ")");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_ARR: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (make-vector (int ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, " ) (void))");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_TAB: {
                    // fprintf(of, "(set! ");
                    // vm_ir_be_racket_arg(of, instr->out);
                    // fprintf(of, " (make-vector ");
                    // vm_ir_be_racket_arg(of, instr->args[0]);
                    // fprintf(of, " (void))");
                    // fprintf(of, ")");
                    __builtin_trap();
                    break;
                }
                case VM_IR_IOP_GET: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (let ((thing ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ") (index ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, "))");
                    fprintf(of, "(if (vector? thing) (vector-ref thing (int index)) (hash-ref thing index))");
                    fprintf(of, ")");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_SET: {
                    fprintf(of, " (let ((thing ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ") (index ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, ") (value ");
                    vm_ir_be_racket_arg(of, instr->args[2]);
                    fprintf(of, "))");
                    fprintf(of, "(if (vector? thing) (vector-set! thing (int index) value) (hash-set! thing index value))");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_LEN: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (vector-length ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ")");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_TYPE: {
                    __builtin_trap();
                }
                case VM_IR_IOP_OUT: {
                    fprintf(of, "(display (integer->char (int ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ")))");
                    break;
                }
                case VM_IR_IOP_IN: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (char->integer (read-char)))");
                    break;
                }
                case VM_IR_IOP_BOR: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (bitwise-ior (int ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ") (int ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, "))");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_BAND: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (bitwise-and (int ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ") (int ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, "))");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_BXOR: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (bitwise-xor (int ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ") (int ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, "))");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_BSHL: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (arithmetic-shift (int ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ") (");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, "))");
                    fprintf(of, ")");
                    break;
                }
                case VM_IR_IOP_BSHR: {
                    fprintf(of, "(set! ");
                    vm_ir_be_racket_arg(of, instr->out);
                    fprintf(of, " (arithmetic-shift (int ");
                    vm_ir_be_racket_arg(of, instr->args[0]);
                    fprintf(of, ") (- (int ");
                    vm_ir_be_racket_arg(of, instr->args[1]);
                    fprintf(of, ")))");
                    fprintf(of, ")");
                    break;
                }
            }
        }
        switch (block->branch->op) {
            case VM_IR_BOP_JUMP: {
                vm_ir_be_racket_block(of, block->branch->targets[0]);
                break;
            }
            case VM_IR_BOP_BOOL: {
                fprintf(of, "(if ");
                vm_ir_be_racket_arg(of, block->branch->args[0]);
                vm_ir_be_racket_block(of, block->branch->targets[1]);
                vm_ir_be_racket_block(of, block->branch->targets[0]);
                fprintf(of, ")");
                break;
            }
            case VM_IR_BOP_LESS: {
                fprintf(of, "(if (< ");
                vm_ir_be_racket_arg(of, block->branch->args[0]);
                fprintf(of, " ");
                vm_ir_be_racket_arg(of, block->branch->args[1]);
                fprintf(of, ") ");
                vm_ir_be_racket_block(of, block->branch->targets[1]);
                vm_ir_be_racket_block(of, block->branch->targets[0]);
                fprintf(of, ")");
                break;
            }
            case VM_IR_BOP_EQUAL: {
                fprintf(of, "(if (equal? ");
                vm_ir_be_racket_arg(of, block->branch->args[0]);
                fprintf(of, " ");
                vm_ir_be_racket_arg(of, block->branch->args[1]);
                fprintf(of, ") ");
                vm_ir_be_racket_block(of, block->branch->targets[1]);
                vm_ir_be_racket_block(of, block->branch->targets[0]);
                fprintf(of, ")");
                break;
            }
            case VM_IR_BOP_RET: {
                fprintf(of, "(return ");
                vm_ir_be_racket_arg(of, block->branch->args[0]);
                fprintf(of, ")");
                break;
            }
            case VM_IR_BOP_EXIT: {
                fprintf(of, "(exit)");
                break;
            }
        }
        fprintf(of, ")\n");
        vm_free(known);
    }
    fprintf(of, "(let/cc return (l0 return))");
}
