
#include "int3.h"

#include <stddef.h>
#include <stdio.h>

#include "../build.h"

#define vm_int_block_comp_put_ptr(arg_) buf.ops[buf.len++].ptr = ptrs[(arg_)]
#define vm_int_block_comp_put_out(out_) buf.ops[buf.len++].reg = (out_)
#define vm_int_block_comp_put_reg(vreg_) buf.ops[buf.len++].reg = (vreg_)
#define vm_int_block_comp_put_ival(val_) buf.ops[buf.len++].ival = (val_)
#define vm_int_block_comp_put_fval(val_) buf.ops[buf.len++].fval = (val_)
#define vm_int_block_comp_put_block(block_) buf.ops[buf.len++].block = (block_)
#define vm_int_block_comp_put_arg(arg_)            \
    ({                                             \
        vm_ir_arg_t arg = (arg_);                  \
        if (arg.type == VM_IR_ARG_REG) {           \
            vm_int_block_comp_put_reg(arg.reg);    \
        } else if (arg.type == VM_IR_ARG_NUM) {    \
            vm_int_block_comp_put_fval(arg.num);   \
        } else if (arg.type == VM_IR_ARG_FUNC) {   \
            vm_int_block_comp_put_block(arg.func); \
        } else {                                   \
            fprintf(stderr, "unknown arg type\n"); \
            __builtin_trap();                      \
        }                                          \
    })

struct vm_int_data_t;
typedef struct vm_int_data_t vm_int_data_t;

struct vm_int_data_t {
    vm_int_buf_t *bufs;
    uint8_t **types;
    size_t len;
    size_t alloc;
};

static void vm_int_data_push(vm_int_data_t *data, vm_int_buf_t buf, uint8_t *types) {
    if (data->len + 1 >= data->alloc) {
        data->alloc = (data->len + 1) * 2;
        data->bufs = vm_realloc(data->bufs, sizeof(vm_int_buf_t) * data->alloc);
        data->types = vm_realloc(data->types, sizeof(uint8_t *) * data->alloc);
    }
    data->bufs[data->len] = buf;
    data->types[data->len] = types;
    data->len += 1;
}

void *vm_int_block_comp(vm_int_state_t *state, void **ptrs, vm_ir_block_t *block) {
    vm_value_t *locals = state->locals;
    vm_int_data_t *data = block->data;
    if (data == NULL) {
        data = vm_malloc(sizeof(vm_int_data_t));
        block->data = data;
        data->alloc = 0;
        data->len = 0;
        data->bufs = NULL;
        data->types = NULL;
    } else {
        for (size_t i = 0; i < data->len; i++) {
            uint8_t *types = data->types[i];
            for (size_t a = 0; a < block->nargs; a++) {
                size_t reg = block->args[a];
                if (state->locals[reg].type != types[reg]) {
                    goto next;
                }
            }
            return data->bufs[i].ops;
        next:;
        }
    }
    vm_int_buf_t buf;
    buf.len = 0;
    buf.alloc = 16;
    buf.ops = vm_alloc0(sizeof(vm_int_opcode_t) * buf.alloc);
    uint8_t *types = vm_malloc(sizeof(uint8_t) * state->framesize);
    for (size_t i = 0; i < block->nargs; i++) {
        size_t reg = block->args[i];
        types[reg] = state->locals[reg].type;
    }
inline_jump:;
    // fprintf(stderr, " --- \n");
    // fprintf(stderr, "block %zu:\n", block->id);
    // vm_ir_print_block(stderr, block);
    ptrdiff_t typecheck = -1;
    for (size_t arg = 0; arg < block->len; arg++) {
        if (buf.len + 16 >= buf.alloc) {
            buf.alloc = buf.len * 2 + 16;
            buf.ops = vm_realloc(buf.ops, sizeof(vm_int_opcode_t) * buf.alloc);
        }
        vm_ir_instr_t *instr = block->instrs[arg];
        switch (instr->op) {
            case VM_IR_IOP_NOP: {
                break;
            }
            case VM_IR_IOP_MOVE: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    switch (instr->args[0].type) {
                        case VM_IR_ARG_REG: {
                            // r = move r
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_R);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            types[instr->out.reg] = types[instr->args[0].type];
                            break;
                        }
                        case VM_IR_ARG_NUM: {
                            // r = move i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                            break;
                        }
                        case VM_IR_ARG_STR: {
                            fprintf(stderr, "NO STRINGS YET\n");
                            __builtin_trap();
                        }
                        case VM_IR_ARG_FUNC: {
                            // r = move i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_T);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            types[instr->out.reg] = VM_TYPE_BLOCK;
                            break;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_ADD: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = add r r
                            vm_int_block_comp_put_ptr(VM_INT_OP_ADD_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = add r i
                            vm_int_block_comp_put_ptr(VM_INT_OP_ADD_RI);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = add i r
                            vm_int_block_comp_put_ptr(VM_INT_OP_ADD_RI);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_fval(instr->args[0].num + instr->args[1].num);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_SUB: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = sub r r
                            vm_int_block_comp_put_ptr(VM_INT_OP_SUB_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = sub r i
                            vm_int_block_comp_put_ptr(VM_INT_OP_SUB_RI);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = sub i r
                            vm_int_block_comp_put_ptr(VM_INT_OP_SUB_IR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_fval(instr->args[0].num - instr->args[1].num);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_MUL: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = mul r r
                            vm_int_block_comp_put_ptr(VM_INT_OP_MUL_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = mul r i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MUL_RI);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = mul i r
                            vm_int_block_comp_put_ptr(VM_INT_OP_MUL_RI);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_fval(instr->args[0].num * instr->args[1].num);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_DIV: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = div r r
                            vm_int_block_comp_put_ptr(VM_INT_OP_DIV_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = div r i
                            vm_int_block_comp_put_ptr(VM_INT_OP_DIV_RI);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = div i r
                            vm_int_block_comp_put_ptr(VM_INT_OP_DIV_IR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_fval(instr->args[0].num / instr->args[1].num);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_MOD: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = mod r r
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOD_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = mod r i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOD_RI);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = mod i r
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOD_IR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_fval(instr->args[0].num % instr->args[1].num);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_CALL: {
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
                    if (instr->args[i].type == VM_IR_ARG_NUM) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                        vm_int_block_comp_put_out(state->framesize + i);
                        vm_int_block_comp_put_arg(instr->args[0]);
                    }
                }
                size_t nargs = 0;
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
                    nargs += 1;
                }
                if (instr->args[0].type == VM_IR_ARG_FUNC) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_CALL_T0 + nargs);
                    vm_int_block_comp_put_block(instr->args[0].func);
                } else if (instr->args[0].type == VM_IR_ARG_EXTERN) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_CALL_X0 + nargs);
                    vm_int_block_comp_put_ival(instr->args[0].num);
                } else {
                    vm_int_block_comp_put_ptr(VM_INT_OP_CALL_R0 + nargs);
                    vm_int_block_comp_put_arg(instr->args[0]);
                }
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
                    if (instr->args[i].type == VM_IR_ARG_REG) {
                        vm_int_block_comp_put_arg(instr->args[i]);
                    } else {
                        vm_int_block_comp_put_reg(state->framesize + i);
                    }
                }
                if (instr->out.type == VM_IR_ARG_REG) {
                    vm_int_block_comp_put_out(instr->out.reg);
                    typecheck = instr->out.reg;
                } else {
                    vm_int_block_comp_put_out(block->nregs + 1);
                }
                break;
            }
            case VM_IR_IOP_ARR: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        // r = new r
                        vm_int_block_comp_put_ptr(VM_INT_OP_NEW_R);
                        vm_int_block_comp_put_out(instr->out.reg);
                        vm_int_block_comp_put_arg(instr->args[0]);
                        types[instr->out.reg] = VM_TYPE_STATIC;
                    } else {
                        // r = new i
                        vm_int_block_comp_put_ptr(VM_INT_OP_NEW_I);
                        vm_int_block_comp_put_out(instr->out.reg);
                        vm_int_block_comp_put_arg(instr->args[0]);
                        types[instr->out.reg] = VM_TYPE_STATIC;
                    }
                }
                break;
            }
            case VM_IR_IOP_GET: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = get r r
                            vm_int_block_comp_put_ptr(VM_INT_OP_GET_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            typecheck = instr->out.reg;
                        } else {
                            // r = get r i
                            vm_int_block_comp_put_ptr(VM_INT_OP_GET_RI);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            typecheck = instr->out.reg;
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_SET: {
                if (instr->args[0].type == VM_IR_ARG_REG) {
                    if (instr->args[1].type == VM_IR_ARG_REG) {
                        if (instr->args[2].type == VM_IR_ARG_REG) {
                            // set r r rbadtyped
                            vm_int_block_comp_put_arg(instr->args[1]);
                            vm_int_block_comp_put_arg(instr->args[2]);
                        } else {
                            // set r r i
                            vm_int_block_comp_put_ptr(VM_INT_OP_SET_RRI);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            vm_int_block_comp_put_arg(instr->args[2]);
                        }
                    } else {
                        if (instr->args[2].type == VM_IR_ARG_REG) {
                            // set r i r
                            vm_int_block_comp_put_ptr(VM_INT_OP_SET_RIR);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            vm_int_block_comp_put_arg(instr->args[2]);
                        } else {
                            // set r i i
                            vm_int_block_comp_put_ptr(VM_INT_OP_SET_RII);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            vm_int_block_comp_put_arg(instr->args[2]);
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_LEN: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        // r = len r
                        vm_int_block_comp_put_ptr(VM_INT_OP_LEN_R);
                        vm_int_block_comp_put_out(instr->out.reg);
                        vm_int_block_comp_put_arg(instr->args[0]);
                        types[instr->out.reg] = VM_TYPE_FLOAT;
                    }
                }
                break;
            }
            case VM_IR_IOP_TYPE: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        // r = type r
                        vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                        vm_int_block_comp_put_out(instr->out.reg);
                        vm_int_block_comp_put_fval(types[instr->args[0].reg]);
                        types[instr->out.reg] = VM_TYPE_FLOAT;
                    } else {
                        // r = move i
                        vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I);
                        vm_int_block_comp_put_out(instr->out.reg);
                        vm_int_block_comp_put_fval(VM_TYPE_FLOAT);
                        types[instr->out.reg] = VM_TYPE_FLOAT;
                    }
                }
                break;
            }
            case VM_IR_IOP_OUT: {
                if (instr->args[0].type == VM_IR_ARG_NUM) {
                    // out i
                    vm_int_block_comp_put_ptr(VM_INT_OP_OUT_I);
                    vm_int_block_comp_put_arg(instr->args[0]);
                } else {
                    // out r
                    vm_int_block_comp_put_ptr(VM_INT_OP_OUT_R);
                    vm_int_block_comp_put_arg(instr->args[0]);
                }
                break;
            }
        }
    }
    switch (block->branch->op) {
        case VM_IR_BOP_JUMP: {
            // jump l
            if (typecheck >= 0 && 0) {
                vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_C);
                vm_int_block_comp_put_ival(0);
                vm_int_block_comp_put_reg((size_t) typecheck);
                for (uint8_t i = 0; i < VM_TYPE_MAX; i++) {
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                }
            } else if (block->branch->targets[0]->id <= block->id) {
                vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
                vm_int_block_comp_put_block(block->branch->targets[0]);
            } else {
                block = block->branch->targets[0];
                goto inline_jump;
            }
            break;
        }
        case VM_IR_BOP_BOOL: {
            if (block->branch->args[0].type == VM_IR_ARG_NUM) {
                // jump l
                vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
                vm_int_block_comp_put_block(block->branch->targets[block->branch->args[0].num != 0]);
            } else {
                // bb r l l
                vm_int_block_comp_put_ptr(VM_INT_OP_BB_RTT);
                vm_int_block_comp_put_arg(block->branch->args[0]);
                vm_int_block_comp_put_block(block->branch->targets[0]);
                vm_int_block_comp_put_block(block->branch->targets[1]);
            }
            break;
        }
        case VM_IR_BOP_LESS: {
            if (block->branch->args[0].type == VM_IR_ARG_NUM) {
                if (block->branch->args[1].type == VM_IR_ARG_NUM) {
                    // jump l
                    vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
                    vm_int_block_comp_put_block(
                        block->branch->targets[block->branch->args[0].num < block->branch->args[1].num]);
                } else {
                    // blt i r l l
                    vm_int_block_comp_put_ptr(VM_INT_OP_BLT_IRTT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                    vm_int_block_comp_put_arg(block->branch->args[1]);
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                    vm_int_block_comp_put_block(block->branch->targets[1]);
                }
            } else {
                if (block->branch->args[1].type == VM_IR_ARG_NUM) {
                    // blt r i l l
                    vm_int_block_comp_put_ptr(VM_INT_OP_BLT_RITT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                    vm_int_block_comp_put_arg(block->branch->args[1]);
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                    vm_int_block_comp_put_block(block->branch->targets[1]);
                } else {
                    // blt r r l l
                    vm_int_block_comp_put_ptr(VM_INT_OP_BLT_RRTT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                    vm_int_block_comp_put_arg(block->branch->args[1]);
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                    vm_int_block_comp_put_block(block->branch->targets[1]);
                }
            }
            break;
        }
        case VM_IR_BOP_EQUAL: {
            if (block->branch->args[0].type == VM_IR_ARG_NUM) {
                if (block->branch->args[1].type == VM_IR_ARG_NUM) {
                    // jump l
                    vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
                    vm_int_block_comp_put_block(
                        block->branch->targets[block->branch->args[0].num == block->branch->args[1].num]);
                } else {
                    // beq i r l l
                    vm_int_block_comp_put_ptr(VM_INT_OP_BEQ_IRTT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                    vm_int_block_comp_put_arg(block->branch->args[1]);
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                    vm_int_block_comp_put_block(block->branch->targets[1]);
                }
            } else {
                if (block->branch->args[1].type == VM_IR_ARG_NUM) {
                    // beq r i l l
                    vm_int_block_comp_put_ptr(VM_INT_OP_BEQ_RITT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                    vm_int_block_comp_put_arg(block->branch->args[1]);
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                    vm_int_block_comp_put_block(block->branch->targets[1]);
                } else {
                    // beq r r l l
                    vm_int_block_comp_put_ptr(VM_INT_OP_BEQ_RRTT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                    vm_int_block_comp_put_arg(block->branch->args[1]);
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                    vm_int_block_comp_put_block(block->branch->targets[1]);
                }
            }
            break;
        }
        case VM_IR_BOP_RET: {
            vm_value_t val;
            if (block->branch->args[0].type == VM_IR_ARG_NUM) {
                // ret i
                vm_int_block_comp_put_ptr(VM_INT_OP_RET_I);
                vm_int_block_comp_put_arg(block->branch->args[0]);
            } else {
                // ret r
                vm_int_block_comp_put_ptr(VM_INT_OP_RET_R);
                vm_int_block_comp_put_arg(block->branch->args[0]);
            }
            break;
        }
        case VM_IR_BOP_EXIT: {
            // exit
            vm_int_block_comp_put_ptr(VM_INT_OP_EXIT);
            break;
        }
    }
    for (size_t a = 0; a < state->framesize; a++) {
        types[a] = state->locals[a].type;
    }
    vm_int_data_push(data, buf, types);
    // fprintf(stderr, "block(%zu).len = %zu\n", block->id, data->len);
    // vm_ir_print_block(stderr, block);
    return buf.ops;
}

#define vm_int_run_read()            \
    (*({                             \
        vm_int_opcode_t *ret = head; \
        head += 1;                   \
        ret;                         \
    }))
#if VM_INT_DEBUG_OPCODE
#define vm_int_debug(v)                                                          \
    ({                                                                           \
        typeof(v) v_ = v;                                                        \
        fprintf(stderr, "LINE = %zu\n", (size_t)__LINE__);                       \
        fprintf(stderr, "DEPTH = %zu\n", (size_t)(state->locals - init_locals)); \
        v_;                                                                      \
    })
#else
#define vm_int_debug(v) (v)
#endif
#define vm_int_run_next() goto *vm_int_debug(vm_int_run_read().ptr)
#define vm_int_not_implemented()                                       \
    ({                                                                 \
        fprintf(stderr, "UNIMPLEMENTED line=%zu\n", (size_t)__LINE__); \
        return;                                                        \
    })

#if VM_INT_DEBUG_STORE
#define vm_int_run_read_store() ({              \
    size_t reg = vm_int_run_read().reg;         \
    fprintf(stderr, "store r%zu:\n", reg, reg); \
    &state->locals[reg];                        \
})
#else
#define vm_int_run_read_store() &locals[vm_int_run_read().reg]
#endif

#if VM_INT_DEBUG_LOAD
#define vm_int_run_read_load() ({                               \
    size_t reg = vm_int_run_read().reg;                         \
    vm_value_t ret = locals[reg];                               \
    fprintf(stderr, "  load r%zu = %lf\n", reg, ret.data.ival); \
    ret;                                                        \
})
#else
#define vm_int_run_read_load() (locals[vm_int_run_read().reg])
#endif

#define vm_int_run_save()       \
    ({                          \
        state->locals = locals; \
        state;                  \
    })

vm_value_t vm_int_run(vm_int_state_t *state, vm_ir_block_t *block) {
    static void *ptrs[VM_INT_MAX_OP] = {
        [VM_INT_OP_EXIT] = &&do_exit,
        [VM_INT_OP_MOV_I] = &&do_mov_i,
        [VM_INT_OP_MOV_R] = &&do_mov_r,
        [VM_INT_OP_MOV_T] = &&do_mov_t,
        [VM_INT_OP_ADD_RR] = &&do_add_rr,
        [VM_INT_OP_ADD_RI] = &&do_add_ri,
        [VM_INT_OP_SUB_RR] = &&do_sub_rr,
        [VM_INT_OP_SUB_RI] = &&do_sub_ri,
        [VM_INT_OP_SUB_IR] = &&do_sub_ir,
        [VM_INT_OP_MUL_RR] = &&do_mul_rr,
        [VM_INT_OP_MUL_RI] = &&do_mul_ri,
        [VM_INT_OP_DIV_RR] = &&do_div_rr,
        [VM_INT_OP_DIV_RI] = &&do_div_ri,
        [VM_INT_OP_DIV_IR] = &&do_div_ir,
        [VM_INT_OP_MOD_RR] = &&do_mod_rr,
        [VM_INT_OP_MOD_RI] = &&do_mod_ri,
        [VM_INT_OP_MOD_IR] = &&do_mod_ir,
        [VM_INT_OP_CALL_L0] = &&do_call_l0,
        [VM_INT_OP_CALL_L1] = &&do_call_l1,
        [VM_INT_OP_CALL_L2] = &&do_call_l2,
        [VM_INT_OP_CALL_L3] = &&do_call_l3,
        [VM_INT_OP_CALL_L4] = &&do_call_l4,
        [VM_INT_OP_CALL_L5] = &&do_call_l5,
        [VM_INT_OP_CALL_L6] = &&do_call_l6,
        [VM_INT_OP_CALL_L7] = &&do_call_l7,
        [VM_INT_OP_CALL_L8] = &&do_call_l8,
        [VM_INT_OP_CALL_R0] = &&do_call_r0,
        [VM_INT_OP_CALL_R1] = &&do_call_r1,
        [VM_INT_OP_CALL_R2] = &&do_call_r2,
        [VM_INT_OP_CALL_R3] = &&do_call_r3,
        [VM_INT_OP_CALL_R4] = &&do_call_r4,
        [VM_INT_OP_CALL_R5] = &&do_call_r5,
        [VM_INT_OP_CALL_R6] = &&do_call_r6,
        [VM_INT_OP_CALL_R7] = &&do_call_r7,
        [VM_INT_OP_CALL_R8] = &&do_call_r8,
        [VM_INT_OP_CALL_X0] = &&do_call_x0,
        [VM_INT_OP_CALL_X1] = &&do_call_x1,
        [VM_INT_OP_CALL_X2] = &&do_call_x2,
        [VM_INT_OP_CALL_X3] = &&do_call_x3,
        [VM_INT_OP_CALL_X4] = &&do_call_x4,
        [VM_INT_OP_CALL_X5] = &&do_call_x5,
        [VM_INT_OP_CALL_X6] = &&do_call_x6,
        [VM_INT_OP_CALL_X7] = &&do_call_x7,
        [VM_INT_OP_CALL_X8] = &&do_call_x8,
        [VM_INT_OP_NEW_I] = &&do_new_i,
        [VM_INT_OP_NEW_R] = &&do_new_r,
        [VM_INT_OP_SET_RRR] = &&do_set_rrr,
        [VM_INT_OP_SET_RRI] = &&do_set_rri,
        [VM_INT_OP_SET_RIR] = &&do_set_rir,
        [VM_INT_OP_SET_RII] = &&do_set_rii,
        [VM_INT_OP_GET_RR] = &&do_get_rr,
        [VM_INT_OP_GET_RI] = &&do_get_ri,
        [VM_INT_OP_LEN_R] = &&do_len_r,
        [VM_INT_OP_OUT_I] = &&do_out_i,
        [VM_INT_OP_OUT_R] = &&do_out_r,
        [VM_INT_OP_JUMP_L] = &&do_jump_l,
        [VM_INT_OP_BB_RLL] = &&do_bb_rll,
        [VM_INT_OP_BLT_RRLL] = &&do_blt_rrll,
        [VM_INT_OP_BLT_RILL] = &&do_blt_rill,
        [VM_INT_OP_BLT_IRLL] = &&do_blt_irll,
        [VM_INT_OP_BEQ_RRLL] = &&do_beq_rrll,
        [VM_INT_OP_BEQ_RILL] = &&do_beq_rill,
        [VM_INT_OP_BEQ_IRLL] = &&do_beq_irll,
        [VM_INT_OP_RET_I] = &&do_ret_i,
        [VM_INT_OP_RET_R] = &&do_ret_r,
        [VM_INT_OP_CALL_T0] = &&do_call_t0,
        [VM_INT_OP_CALL_T1] = &&do_call_t1,
        [VM_INT_OP_CALL_T2] = &&do_call_t2,
        [VM_INT_OP_CALL_T3] = &&do_call_t3,
        [VM_INT_OP_CALL_T4] = &&do_call_t4,
        [VM_INT_OP_CALL_T5] = &&do_call_t5,
        [VM_INT_OP_CALL_T6] = &&do_call_t6,
        [VM_INT_OP_CALL_T7] = &&do_call_t7,
        [VM_INT_OP_CALL_T8] = &&do_call_t8,
        [VM_INT_OP_JUMP_T] = &&do_jump_t,
        [VM_INT_OP_BB_RTT] = &&do_bb_rtt,
        [VM_INT_OP_BLT_RRTT] = &&do_blt_rrtt,
        [VM_INT_OP_BLT_RITT] = &&do_blt_ritt,
        [VM_INT_OP_BLT_IRTT] = &&do_blt_irtt,
        [VM_INT_OP_BEQ_RRTT] = &&do_beq_rrtt,
        [VM_INT_OP_BEQ_RITT] = &&do_beq_ritt,
        [VM_INT_OP_BEQ_IRTT] = &&do_beq_irtt,
        [VM_INT_OP_JUMP_C] = &&do_jump_c,
    };
    vm_value_t *init_locals = state->locals;
    vm_value_t *locals = init_locals;
    vm_int_opcode_t *head = vm_int_block_comp(vm_int_run_save(), ptrs, block);
    static int n = 0;
    // if (++n == 2)
    // __builtin_trap();
    vm_int_run_next();
do_mov_i : {
    vm_value_t *out = vm_int_run_read_store();
    double value = vm_int_run_read().fval;
    *out = vm_value_from_int(value);
    vm_int_run_next();
}
do_mov_r : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t value = vm_int_run_read_load();
    *out = value;
    vm_int_run_next();
}
do_mov_t : {
    vm_value_t *out = vm_int_run_read_store();
    vm_ir_block_t *block = vm_int_run_read().block;
    *out = vm_value_from_block(block);
    vm_int_run_next();
}
do_add_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    // fprintf(stderr, "%lf = %lf + %lf\n", vm_value_to_int(lhs) + vm_value_to_int(rhs), vm_value_to_int(lhs), vm_value_to_int(rhs));
    *out = vm_value_from_int(vm_value_to_int(lhs) + vm_value_to_int(rhs));
    vm_int_run_next();
}
do_add_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    double rhs = vm_int_run_read().fval;
    *out = vm_value_from_int(vm_value_to_int(lhs) + rhs);
    vm_int_run_next();
}
do_sub_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(vm_value_to_int(lhs) - vm_value_to_int(rhs));
    vm_int_run_next();
}
do_sub_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    double rhs = vm_int_run_read().fval;
    *out = vm_value_from_int(vm_value_to_int(lhs) - rhs);
    vm_int_run_next();
}
do_sub_ir : {
    vm_value_t *out = vm_int_run_read_store();
    double lhs = vm_int_run_read().fval;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(lhs - vm_value_to_int(rhs));
    vm_int_run_next();
}
do_mul_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(vm_value_to_int(lhs) * vm_value_to_int(rhs));
    vm_int_run_next();
}
do_mul_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    double rhs = vm_int_run_read().fval;
    *out = vm_value_from_int(vm_value_to_int(lhs) * rhs);
    vm_int_run_next();
}
do_div_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(vm_value_to_int(lhs) / vm_value_to_int(rhs));
    vm_int_run_next();
}
do_div_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    double rhs = vm_int_run_read().fval;
    *out = vm_value_from_int(vm_value_to_int(lhs) / rhs);
    vm_int_run_next();
}
do_div_ir : {
    vm_value_t *out = vm_int_run_read_store();
    double lhs = vm_int_run_read().fval;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(lhs / vm_value_to_int(rhs));
    vm_int_run_next();
}
do_mod_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(fmod(vm_value_to_int(lhs), vm_value_to_int(rhs)));
    vm_int_run_next();
}
do_mod_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    double rhs = vm_int_run_read().fval;
    *out = vm_value_from_int(fmod(vm_value_to_int(lhs), rhs));
    vm_int_run_next();
}
do_mod_ir : {
    vm_value_t *out = vm_int_run_read_store();
    double lhs = vm_int_run_read().fval;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(fmod(lhs, vm_value_to_int(rhs)));
    vm_int_run_next();
}
do_call_l0 : {
    void *ptr = vm_int_run_read().ptr;
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l1 : {
    void *ptr = vm_int_run_read().ptr;
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    // fprintf(stderr, "%lf\n", locals[state->framesize + 1 + 0].data.ival);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l2 : {
    void *ptr = vm_int_run_read().ptr;
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l3 : {
    void *ptr = vm_int_run_read().ptr;
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l4 : {
    void *ptr = vm_int_run_read().ptr;
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l5 : {
    void *ptr = vm_int_run_read().ptr;
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    locals[state->framesize + 1 + 4] = vm_int_run_read_load();
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l6 : {
    void *ptr = vm_int_run_read().ptr;
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    locals[state->framesize + 1 + 4] = vm_int_run_read_load();
    locals[state->framesize + 1 + 5] = vm_int_run_read_load();
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l7 : {
    void *ptr = vm_int_run_read().ptr;
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    locals[state->framesize + 1 + 4] = vm_int_run_read_load();
    locals[state->framesize + 1 + 5] = vm_int_run_read_load();
    locals[state->framesize + 1 + 6] = vm_int_run_read_load();
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l8 : {
    void *ptr = vm_int_run_read().ptr;
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    locals[state->framesize + 1 + 4] = vm_int_run_read_load();
    locals[state->framesize + 1 + 5] = vm_int_run_read_load();
    locals[state->framesize + 1 + 6] = vm_int_run_read_load();
    locals[state->framesize + 1 + 7] = vm_int_run_read_load();
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r0 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r1 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r2 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r3 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r4 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r5 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    locals[state->framesize + 1 + 4] = vm_int_run_read_load();
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r6 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    locals[state->framesize + 1 + 4] = vm_int_run_read_load();
    locals[state->framesize + 1 + 5] = vm_int_run_read_load();
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r7 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    locals[state->framesize + 1 + 4] = vm_int_run_read_load();
    locals[state->framesize + 1 + 5] = vm_int_run_read_load();
    locals[state->framesize + 1 + 6] = vm_int_run_read_load();
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r8 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[state->framesize + 1 + 0] = vm_int_run_read_load();
    locals[state->framesize + 1 + 1] = vm_int_run_read_load();
    locals[state->framesize + 1 + 2] = vm_int_run_read_load();
    locals[state->framesize + 1 + 3] = vm_int_run_read_load();
    locals[state->framesize + 1 + 4] = vm_int_run_read_load();
    locals[state->framesize + 1 + 5] = vm_int_run_read_load();
    locals[state->framesize + 1 + 6] = vm_int_run_read_load();
    locals[state->framesize + 1 + 7] = vm_int_run_read_load();
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    *state->heads++ = head;
    locals += state->framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_x0 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 0, NULL);
    locals -= state->framesize;
    vm_int_run_next();
}
do_call_x1 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[1] = {vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 1, &values[0]);
    locals -= state->framesize;
    vm_int_run_next();
}
do_call_x2 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[2] = {vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 2, &values[0]);
    locals -= state->framesize;
    vm_int_run_next();
}
do_call_x3 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[3] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 3, &values[0]);
    locals -= state->framesize;
    vm_int_run_next();
}
do_call_x4 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[4] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 4, &values[0]);
    locals -= state->framesize;
    vm_int_run_next();
}
do_call_x5 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[5] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 5, &values[0]);
    locals -= state->framesize;
    vm_int_run_next();
}
do_call_x6 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[6] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 6, &values[0]);
    locals -= state->framesize;
    vm_int_run_next();
}
do_call_x7 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[7] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 7, &values[0]);
    locals -= state->framesize;
    vm_int_run_next();
}
do_call_x8 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[8] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += state->framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 8, &values[0]);
    locals -= state->framesize;
    vm_int_run_next();
}
do_new_i : {
    vm_value_t *out = vm_int_run_read_store();
    vm_int_opcode_t len = vm_int_run_read();
    *out = vm_gc_new(len.fval);
    vm_int_run_next();
}
do_new_r : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t len = vm_int_run_read_load();
    *out = vm_gc_new(vm_value_to_int(len));
    vm_int_run_next();
}
do_set_rrr : {
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t key = vm_int_run_read_load();
    vm_value_t val = vm_int_run_read_load();
    vm_gc_set_vv(obj, key, val);
    vm_int_run_next();
}
do_set_rri : {
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t key = vm_int_run_read_load();
    double val = vm_int_run_read().fval;
    vm_gc_set_vi(obj, key, val);
    vm_int_run_next();
}
do_set_rir : {
    vm_value_t obj = vm_int_run_read_load();
    double key = vm_int_run_read().fval;
    vm_value_t val = vm_int_run_read_load();
    vm_gc_set_iv(obj, key, val);
    vm_int_run_next();
}
do_set_rii : {
    vm_value_t obj = vm_int_run_read_load();
    double key = vm_int_run_read().fval;
    double val = vm_int_run_read().fval;
    vm_gc_set_ii(obj, key, val);
    vm_int_run_next();
}
do_get_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t key = vm_int_run_read_load();
    *out = vm_gc_get_v(obj, key);
    vm_int_run_next();
}
do_get_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t obj = vm_int_run_read_load();
    double key = vm_int_run_read().fval;
    *out = vm_gc_get_i(obj, key);
    vm_int_run_next();
}
do_len_r : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t obj = vm_int_run_read_load();
    *out = vm_value_from_int(vm_gc_len(obj));
    vm_int_run_next();
}
do_out_i : {
    fprintf(stdout, "%c", (int)vm_int_run_read().fval);
    vm_int_run_next();
}
do_out_r : {
    fprintf(stdout, "%c", (int)vm_value_to_int(vm_int_run_read_load()));
    vm_int_run_next();
}
do_jump_l : {
    head = head->ptr;
    vm_int_run_next();
}
do_bb_rll : {
    ptrdiff_t val = vm_value_to_int(vm_int_run_read_load());
    if (val != 0) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_blt_rrll : {
    ptrdiff_t lhs = vm_value_to_int(vm_int_run_read_load());
    ptrdiff_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_blt_rill : {
    ptrdiff_t lhs = vm_value_to_int(vm_int_run_read_load());
    double rhs = vm_int_run_read().fval;
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_blt_irll : {
    double lhs = vm_int_run_read().fval;
    ptrdiff_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_beq_rrll : {
    ptrdiff_t lhs = vm_value_to_int(vm_int_run_read_load());
    ptrdiff_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_beq_rill : {
    ptrdiff_t lhs = vm_value_to_int(vm_int_run_read_load());
    double rhs = vm_int_run_read().fval;
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_beq_irll : {
    double lhs = vm_int_run_read().fval;
    ptrdiff_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_ret_i : {
    vm_value_t value = vm_value_from_int(vm_int_run_read().fval);
    if (state->locals == init_locals) {
        return value;
    }
    head = *--state->heads;
    locals -= state->framesize;
    vm_int_opcode_t out = vm_int_run_read();
    locals[out.reg] = value;
    vm_int_run_next();
}
do_ret_r : {
    vm_value_t value = vm_int_run_read_load();
    if (state->locals == init_locals) {
        return value;
    }
    // fprintf(stderr, "%*cret %lf\n", state->locals - init_locals, ' ', vm_value_to_int(value));
    head = *--state->heads;
    locals -= state->framesize;
    vm_int_opcode_t out = vm_int_run_read();
    locals[out.reg] = value;
    vm_int_run_next();
}
do_exit : { return vm_value_from_int(0); }
do_call_t0 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l0;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_call_t1 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l1;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_call_t2 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l2;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_call_t3 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l3;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_call_t4 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l4;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_call_t5 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l5;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_call_t6 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l6;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_call_t7 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l7;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_call_t8 : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_call_l8;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_jump_t : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_jump_l;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_bb_rtt : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_bb_rll;
    head += 1;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_blt_rrtt : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_blt_rrll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_blt_ritt : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_blt_rill;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_blt_irtt : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_blt_irll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_beq_rrtt : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_beq_rrll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_beq_ritt : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_beq_rill;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_beq_irtt : {
    void *loc = --head;
    vm_int_run_read().ptr = &&do_beq_irll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_jump_c : {
    ptrdiff_t *flags = &vm_int_run_read().ival;
    uint8_t type = vm_int_run_read_load().type;
    vm_int_opcode_t *block = &head[type];
    if (!(*flags & (1 << type))) {
        block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
        *flags ^= 1 << type;
    }
    head = block->ptr;
    vm_int_run_next();
}
}

vm_value_t vm_ir_be_int3(size_t nblocks, vm_ir_block_t *blocks, vm_int_func_t *funcs) {
    vm_ir_block_t *cur = &blocks[0];
    vm_int_state_t state = (vm_int_state_t){0};
    size_t nregs = 1 << 16;
    vm_value_t *locals = vm_alloc0(sizeof(vm_value_t) * nregs);
    state.framesize = 1;
    state.funcs = funcs;
    for (size_t i = 0; i < nblocks; i++) {
        if (blocks[i].id == i) {
            if (blocks[i].nregs >= state.framesize) {
                state.framesize = blocks[i].nregs + 1;
            }
        }
    }
    state.locals = locals;
    state.heads = vm_malloc(sizeof(vm_int_opcode_t *) * (nregs / state.framesize + 1));
    vm_value_t ret = vm_int_run(&state, cur);
    return ret;
}

#include "../opt.h"
#include "../toir.h"

vm_value_t vm_run_arch_int(size_t nops, vm_opcode_t *ops, vm_int_func_t *funcs) {
    size_t nblocks = nops;
    vm_ir_block_t *blocks = vm_ir_parse(nblocks, ops);
    vm_ir_opt_all(&nblocks, &blocks);
    vm_value_t ret = vm_ir_be_int3(nblocks, blocks, funcs);
    vm_ir_blocks_free(nblocks, blocks);
    return ret;
}
