
#include "int3.h"

#include "../build.h"

#define vm_int_block_comp_put_ptr(arg_) buf.ops[buf.len++].ptr = ptrs[(arg_)]
#define vm_int_block_comp_put_out(out_) buf.ops[buf.len++].reg = (out_)
#define vm_int_block_comp_put_reg(vreg_) buf.ops[buf.len++].reg = (vreg_)
#define vm_int_block_comp_put_bval(val_) buf.ops[buf.len++].bval = (val_)
#define vm_int_block_comp_put_ival(val_) buf.ops[buf.len++].ival = (val_)
#define vm_int_block_comp_put_fval(val_) buf.ops[buf.len++].fval = (val_)
#define vm_int_block_comp_put_block(block_) ({ \
    buf.ops[buf.len++].block = (block_);       \
})
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

#define vm_int_block_comp_mov(vreg_, fval_)             \
    ({                                                  \
        size_t reg = vreg_;                             \
        double val = fval_;                             \
        vm_int_block_comp_put_ptr(VM_INT_OP_MOV_F); \
        vm_int_block_comp_put_out(reg);             \
        vm_int_block_comp_put_fval(val);            \
        types[reg] = VM_TYPE_FLOAT;                 \
    })

#define vm_int_block_comp_ensure_float_reg(reg_)                                                       \
    ({                                                                                                 \
        size_t reg = (reg_);                                                                           \
        if (types[reg] == VM_TYPE_FLOAT) {                                                             \
            /* :) */                                                                                   \
        } else {                                                                                       \
            fprintf(stderr, "TYPE ERROR (reg: %zu) (type: %zu)\n", reg, (size_t)types[reg]); \
            __builtin_trap();                                                                          \
        }                                                                                              \
    })

void *vm_int_block_comp(vm_int_state_t *state, void **ptrs, vm_ir_block_t *block) {
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
                if (vm_typeof(state->locals[reg]) != types[reg]) {
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
        types[reg] = vm_typeof(state->locals[reg]);
    }
#if VM_INT_DEBUG_COMP
    fprintf(stderr, "block .%zu(", block->id);
    for (size_t i = 0; i < block->nargs; i++) {
        if (i != 0) {
            fprintf(stderr, " ");
        }
        size_t reg = block->args[i];
        uint8_t type = types[reg];
        const char *typename;
        switch (type) {
        case VM_TYPE_NIL: {
            typename = "nil";
            break;
        }
        case VM_TYPE_BOOL: {
            typename = "bool";
            break;
        }
        case VM_TYPE_FLOAT: {
            typename = "float";
            break;
        }
        case VM_TYPE_FUNC: {
            typename = "func";
            break;
        }
        case VM_TYPE_ARRAY: {
            typename = "array";
            break;
        }
        case VM_TYPE_TABLE: {
            typename = "table";
            break;
        }
        }
        fprintf(stderr, "r%zu : %s", reg, typename);
    }
    fprintf(stderr, ") {\n", block->id);
#endif
inline_jump:;
    for (size_t arg = 0; arg < block->len; arg++) {
        if (buf.len + 16 >= buf.alloc) {
            buf.alloc = buf.len * 2 + 16;
            buf.ops = vm_realloc(buf.ops, sizeof(vm_int_opcode_t) * buf.alloc);
        }
        vm_ir_instr_t *instr = block->instrs[arg];
#if VM_INT_DEBUG_COMP
        if (instr->op != VM_IR_IOP_NOP) {
            fprintf(stderr, "  ");
            vm_ir_print_instr(stderr, instr);
            fprintf(stderr, "\n");
        }
#endif
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
                            types[instr->out.reg] = types[instr->args[0].reg];
                            break;
                        }
                        case VM_IR_ARG_NIL: {
                            // r = move i                             
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_V);
                            vm_int_block_comp_put_out(instr->out.reg);
                            types[instr->out.reg] = VM_TYPE_NIL;
                            break;
                        }
                        case VM_IR_ARG_BOOL: {
                            // r = move i                             
                            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_B);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_bval(instr->args[0].logic);
                            types[instr->out.reg] = VM_TYPE_BOOL;
                            break;
                        }
                        case VM_IR_ARG_NUM: {
                            // r = move i
                            vm_int_block_comp_mov(instr->out.reg, instr->args[0].num);
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
                            types[instr->out.reg] = VM_TYPE_FUNC;
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
                            vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                            vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                            vm_int_block_comp_put_ptr(VM_INT_OP_FADD_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = add r i
                                vm_int_block_comp_put_ptr(VM_INT_OP_FADD_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = add i r
                                vm_int_block_comp_put_ptr(VM_INT_OP_FADD_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_mov(instr->out.reg, instr->args[0].num + instr->args[1].num);
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
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FSUB_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = sub r i
                                vm_int_block_comp_put_ptr(VM_INT_OP_FSUB_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = sub i r
                            vm_int_block_comp_put_ptr(VM_INT_OP_FSUB_FR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_fval(instr->args[0].num);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_mov(instr->out.reg, instr->args[0].num - instr->args[1].num);
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
                            vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                            vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                            vm_int_block_comp_put_ptr(VM_INT_OP_FMUL_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = mul r i
                            vm_int_block_comp_put_ptr(VM_INT_OP_FMUL_RF);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = mul i r
                            vm_int_block_comp_put_ptr(VM_INT_OP_FMUL_RF);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_mov(instr->out.reg, instr->args[0].num * instr->args[1].num);
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
                            vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                            vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                            vm_int_block_comp_put_ptr(VM_INT_OP_FDIV_RR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = div r i
                            vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                            vm_int_block_comp_put_ptr(VM_INT_OP_FDIV_RF);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = div i r
                            vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                            vm_int_block_comp_put_ptr(VM_INT_OP_FDIV_FR);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            vm_int_block_comp_put_arg(instr->args[1]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_mov(instr->out.reg, instr->args[0].num / instr->args[1].num);
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
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMOD_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = mod r i
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMOD_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_FLOAT;
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = mod i r
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMOD_FR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            // r = move i
                            vm_int_block_comp_mov(instr->out.reg, fmod(instr->args[0].num, instr->args[1].num));
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_CALL: {
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
                    if (instr->args[i].type == VM_IR_ARG_NUM) {
                        vm_int_block_comp_mov(state->framesize + i, instr->args[0].num);
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
                    if (types[instr->args[0].reg] == VM_TYPE_FUNC) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_CALL_R0 + nargs);
                        vm_int_block_comp_put_arg(instr->args[0]);
                    } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_CALL_C0 + nargs);
                        vm_int_block_comp_put_arg(instr->args[0]);
                    } else {
                        fprintf(stderr, "type error on call r%zu (type %zu)\n", instr->args[0].reg, (size_t)types[instr->args[0].reg]);
                        __builtin_trap();
                    }
                }
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
                    if (instr->args[i].type == VM_IR_ARG_NUM) {
                        vm_int_block_comp_put_reg(state->framesize + i);
                    } else {
                        vm_int_block_comp_put_arg(instr->args[i]);
                    }
                }
                if (instr->out.type == VM_IR_ARG_REG) {
                    vm_int_block_comp_put_out(instr->out.reg);
                } else {
                    vm_int_block_comp_put_out(block->nregs + 1);
                }
                vm_int_block_comp_put_block(block->branch->targets[0]);
                for (uint8_t i = 1; i < VM_TYPE_MAX; i++) {
                    vm_int_block_comp_put_block(NULL);
                }
                goto retv;
                break;
            }
            case VM_IR_IOP_TAB: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_TAB);
                    vm_int_block_comp_put_out(instr->out.reg);
                    types[instr->out.reg] = VM_TYPE_TABLE;
                }
                break;
            }
            case VM_IR_IOP_ARR: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        // r = new r
                        vm_int_block_comp_put_ptr(VM_INT_OP_ARR_R);
                        vm_int_block_comp_put_out(instr->out.reg);
                        vm_int_block_comp_put_arg(instr->args[0]);
                        types[instr->out.reg] = VM_TYPE_ARRAY;
                    } else {
                        // r = new i
                        vm_int_block_comp_put_ptr(VM_INT_OP_ARR_F);
                        vm_int_block_comp_put_out(instr->out.reg);
                        vm_int_block_comp_put_fval(instr->args[0].num);
                        types[instr->out.reg] = VM_TYPE_ARRAY;
                    }
                }
                break;
            }
            case VM_IR_IOP_GET: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = get r r
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TGET_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_GET_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                            } else {
                                fprintf(stderr, "cannot get: r%zu (tag: %zu)\n", instr->args[0].reg, (size_t) types[instr->args[0].reg]);
                                __builtin_trap();
                            }
                        } else {
                            // r = get r i
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TGET_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1].num);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_GET_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1].num);
                            } else {
                                fprintf(stderr, "cannot get: r%zu (tag: %zu)\n", instr->args[0].reg, (size_t) types[ instr->args[0].reg]);
                                __builtin_trap();
                            }
                        }
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        for (uint8_t i = 1; i < VM_TYPE_MAX; i++) {
                            vm_int_block_comp_put_block(NULL);
                        }
                        goto retv;
                    }
                }
                break;
            }
            case VM_IR_IOP_SET: {
                if (instr->args[0].type == VM_IR_ARG_REG) {
                    if (instr->args[1].type == VM_IR_ARG_REG) {
                        if (instr->args[2].type == VM_IR_ARG_REG) {
                            // set r r r
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TSET_RRR);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                vm_int_block_comp_put_arg(instr->args[2]);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_SET_RRR);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                vm_int_block_comp_put_arg(instr->args[2]);
                            } else {
                                fprintf(stderr, "cannot set: r%zu\n", instr->args[0].reg);
                                __builtin_trap();
                            }
                        } else {
                            // set r r i
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TSET_RRF);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                vm_int_block_comp_put_fval(instr->args[2].num);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_SET_RRI);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_arg(instr->args[1]);
                                vm_int_block_comp_put_ival(instr->args[2].num);
                            } else {
                                fprintf(stderr, "cannot set: r%zu\n", instr->args[0].reg);
                                __builtin_trap();
                            }
                        }
                    } else {
                        if (instr->args[2].type == VM_IR_ARG_REG) {
                            // set r i r
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TSET_RFR);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1].num);
                                vm_int_block_comp_put_arg(instr->args[2]); 
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_SET_RIR);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1].num);
                                vm_int_block_comp_put_arg(instr->args[2]); 
                            } else {
                                fprintf(stderr, "cannot set: r%zu\n", instr->args[0].reg);
                                __builtin_trap();
                            }
                        } else {
                            // set r i i
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TSET_RFF);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1].num);
                                vm_int_block_comp_put_fval(instr->args[2].num); 
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_SET_RII);
                                vm_int_block_comp_put_arg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1].num);
                                vm_int_block_comp_put_ival(instr->args[2].num); 
                            } else {
                                fprintf(stderr, "cannot set: r%zu\n", instr->args[0].reg);
                                __builtin_trap();
                            }
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_LEN: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        // r = len r
                        if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                            vm_int_block_comp_put_ptr(VM_INT_OP_LEN_R);
                            vm_int_block_comp_put_out(instr->out.reg);
                            vm_int_block_comp_put_arg(instr->args[0]);
                            types[instr->out.reg] = VM_TYPE_FLOAT;
                        } else {
                            fprintf(stderr, "cannot len: r%zu\n", instr->args[0].reg);
                            __builtin_trap();
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_TYPE: {
                if (instr->out.type == VM_IR_ARG_REG) {
                    if (instr->args[0].type == VM_IR_ARG_REG) {
                        // r = type r
                        vm_int_block_comp_mov(instr->out.reg, types[instr->args[0].reg]);
                    } else {
                        // r = move i
                        vm_int_block_comp_mov(instr->out.reg, VM_TYPE_FLOAT);
                    }
                }
                break;
            }
            case VM_IR_IOP_OUT: {
                if (instr->args[0].type == VM_IR_ARG_NUM) {
                    // out i
                    vm_int_block_comp_put_ptr(VM_INT_OP_OUT_I);
                    vm_int_block_comp_put_ival(instr->args[0].num);
                } else {
                    // out r
                    vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
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
            if (block->branch->targets[0]->id <= block->id) {
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
                if (types[block->branch->args[0].reg] == VM_TYPE_BOOL) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_BB_RTT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                    vm_int_block_comp_put_block(block->branch->targets[1]);
                } else if (types[block->branch->args[0].reg] == VM_TYPE_NIL) {
                    if (block->branch->targets[0]->id <= block->id) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                    } else {
                        block = block->branch->targets[0];
                        goto inline_jump;
                    }
                } else {
                    if (block->branch->targets[1]->id <= block->id) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_JUMP_T);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    } else {
                        block = block->branch->targets[1];
                        goto inline_jump;
                    }
                }
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
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBLT_IRTT);
                        vm_int_block_comp_put_fval(block->branch->args[0].num);
                        vm_int_block_comp_put_arg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                }
            } else {
                if (block->branch->args[1].type == VM_IR_ARG_NUM) {
                    // blt r i l l
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBLT_RITT);
                        vm_int_block_comp_put_arg(block->branch->args[0]);
                        vm_int_block_comp_put_fval(block->branch->args[1].num);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                } else {
                    // blt r r l l
                    vm_int_block_comp_ensure_float_reg(block->branch->args[0].reg);
                    vm_int_block_comp_ensure_float_reg(block->branch->args[1].reg);
                    vm_int_block_comp_put_ptr(VM_INT_OP_FBLT_RRTT);
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
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBEQ_IRTT);
                        vm_int_block_comp_put_fval(block->branch->args[0].num);
                        vm_int_block_comp_put_arg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                }
            } else {
                if (block->branch->args[1].type == VM_IR_ARG_NUM) {
                    // beq r i l l
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBEQ_RITT);
                        vm_int_block_comp_put_arg(block->branch->args[0]);
                        vm_int_block_comp_put_fval(block->branch->args[1].num);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                } else {
                    // beq r r l l
                    vm_int_block_comp_ensure_float_reg(block->branch->args[0].reg);
                    vm_int_block_comp_ensure_float_reg(block->branch->args[1].reg);
                    vm_int_block_comp_put_ptr(VM_INT_OP_FBEQ_RRTT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                    vm_int_block_comp_put_arg(block->branch->args[1]);
                    vm_int_block_comp_put_block(block->branch->targets[0]);
                    vm_int_block_comp_put_block(block->branch->targets[1]);
                }
            }
            break;
        }
        case VM_IR_BOP_RET: {
            if (block->branch->args[0].type == VM_IR_ARG_NUM) {
                // ret i
                vm_int_block_comp_put_ptr(VM_INT_OP_RET_I);
                vm_int_block_comp_put_arg(block->branch->args[0]);
            } else {
                uint8_t type = types[block->branch->args[0].reg];
                if (type == VM_TYPE_NIL) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RV);
                }
                if (type == VM_TYPE_BOOL) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RB);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                }
                if (type == VM_TYPE_FLOAT) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RI);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                }
                if (type == VM_TYPE_FUNC) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RF);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                }
                if (type == VM_TYPE_ARRAY) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RA);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                }
                if (type == VM_TYPE_TABLE) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RT);
                    vm_int_block_comp_put_arg(block->branch->args[0]);
                }
                // ret r
            }
            break;
        }
        case VM_IR_BOP_EXIT: {
            // exit
            vm_int_block_comp_put_ptr(VM_INT_OP_EXIT);
            break;
        }
    }
retv:
#if VM_INT_DEBUG_COMP
    fprintf(stderr, "  ");
    vm_ir_print_branch(stderr, block->branch);
    fprintf(stderr, "\n}\n\n");
#endif
    for (size_t a = 0; a < state->framesize; a++) {
        types[a] = vm_typeof(state->locals[a]);
    }
    vm_int_data_push(data, buf, types);
    return buf.ops;
}

#define vm_int_run_read()              \
    (*({                               \
        vm_int_opcode_t *ret = (head); \
        head += 1;                     \
        ret;                           \
    }))
#if VM_INT_DEBUG_OPCODE
#define vm_int_debug(v)                                                   \
    ({                                                                    \
        typeof(v) v_ = v;                                                 \
        fprintf(stderr, "LINE = %zu\n", (size_t)__LINE__);                \
        fprintf(stderr, "DEPTH = %zu\n", (size_t)(locals - init_locals)); \
        fprintf(stderr, "GOTO PTR = %zX\n", (size_t)(v_));                \
        v_;                                                               \
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
        state->heads = heads; \
        state;                  \
    })

vm_value_t vm_int_run(vm_int_state_t *state, vm_ir_block_t *block) {
    static void *ptrs[VM_INT_MAX_OP] = {
        [VM_INT_OP_EXIT] = &&do_exit, 
        [VM_INT_OP_MOV_V] = &&do_mov_v,
        [VM_INT_OP_MOV_B] = &&do_mov_b,
        [VM_INT_OP_MOV_F] = &&do_mov_f,
        [VM_INT_OP_MOV_R] = &&do_mov_r,
        [VM_INT_OP_MOV_T] = &&do_mov_t,
        [VM_INT_OP_FADD_RR] = &&do_fadd_rr,
        [VM_INT_OP_FADD_RF] = &&do_fadd_ri,
        [VM_INT_OP_FSUB_RR] = &&do_fsub_rr,
        [VM_INT_OP_FSUB_RF] = &&do_fsub_ri,
        [VM_INT_OP_FSUB_FR] = &&do_fsub_ir,
        [VM_INT_OP_FMUL_RR] = &&do_fmul_rr,
        [VM_INT_OP_FMUL_RF] = &&do_fmul_ri,
        [VM_INT_OP_FDIV_RR] = &&do_fdiv_rr,
        [VM_INT_OP_FDIV_RF] = &&do_fdiv_ri,
        [VM_INT_OP_FDIV_FR] = &&do_fdiv_ir,
        [VM_INT_OP_FMOD_RR] = &&do_fmod_rr,
        [VM_INT_OP_FMOD_RF] = &&do_fmod_ri,
        [VM_INT_OP_FMOD_FR] = &&do_fmod_ir,
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
        [VM_INT_OP_CALL_C0] = &&do_call_c0,
        [VM_INT_OP_CALL_C1] = &&do_call_c1,
        [VM_INT_OP_CALL_C2] = &&do_call_c2,
        [VM_INT_OP_CALL_C3] = &&do_call_c3,
        [VM_INT_OP_CALL_C4] = &&do_call_c4,
        [VM_INT_OP_CALL_C5] = &&do_call_c5,
        [VM_INT_OP_CALL_C6] = &&do_call_c6,
        [VM_INT_OP_CALL_C7] = &&do_call_c7,
        [VM_INT_OP_ARR_F] = &&do_arr_f,
        [VM_INT_OP_ARR_R] = &&do_arr_r,
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
        [VM_INT_OP_FBLT_RRLL] = &&do_fblt_rrll,
        [VM_INT_OP_FBLT_RILL] = &&do_fblt_rill,
        [VM_INT_OP_FBLT_IRLL] = &&do_fblt_irll,
        [VM_INT_OP_FBEQ_RRLL] = &&do_fbeq_rrll,
        [VM_INT_OP_FBEQ_RILL] = &&do_fbeq_rill,
        [VM_INT_OP_FBEQ_IRLL] = &&do_fbeq_irll,
        [VM_INT_OP_RET_I] = &&do_ret_i,
        [VM_INT_OP_RET_RV] = &&do_ret_rv,
        [VM_INT_OP_RET_RB] = &&do_ret_rb,
        [VM_INT_OP_RET_RI] = &&do_ret_ri,
        [VM_INT_OP_RET_RF] = &&do_ret_rf,
        [VM_INT_OP_RET_RA] = &&do_ret_ra,
        [VM_INT_OP_RET_RT] = &&do_ret_rt,
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
        [VM_INT_OP_FBLT_RRTT] = &&do_fblt_rrtt,
        [VM_INT_OP_FBLT_RITT] = &&do_fblt_ritt,
        [VM_INT_OP_FBLT_IRTT] = &&do_fblt_irtt,
        [VM_INT_OP_FBEQ_RRTT] = &&do_fbeq_rrtt,
        [VM_INT_OP_FBEQ_RITT] = &&do_fbeq_ritt,
        [VM_INT_OP_FBEQ_IRTT] = &&do_fbeq_irtt,
        [VM_INT_OP_TAB] = &&do_tab,
        [VM_INT_OP_TSET_RRR] = &&do_tset_rrr,
        [VM_INT_OP_TSET_RRF] = &&do_tset_rrf,
        [VM_INT_OP_TSET_RFR] = &&do_tset_rfr,
        [VM_INT_OP_TSET_RFF] = &&do_tset_rff,
        [VM_INT_OP_TGET_RR] = &&do_tget_rr,
        [VM_INT_OP_TGET_RF] = &&do_tget_rf,
    };
    vm_value_t *init_locals = state->locals;
    vm_value_t *locals = init_locals;
    vm_int_opcode_t **init_heads = state->heads;
    vm_int_opcode_t **heads = init_heads;
    size_t framesize = state->framesize;
    vm_int_opcode_t *head = vm_int_block_comp(vm_int_run_save(), ptrs, block);
    vm_int_run_next();
// movs
do_mov_v : {
    vm_value_t *out = vm_int_run_read_store();
    *out = vm_value_nil();
    vm_int_run_next();
}
do_mov_b : {
    vm_value_t *out = vm_int_run_read_store();
    bool value = vm_int_run_read().bval;
    *out = vm_value_from_bool(value);
    vm_int_run_next();
}
do_mov_f : {
    vm_value_t *out = vm_int_run_read_store();
    vm_number_t value = vm_int_run_read().fval;
    *out = vm_value_from_float(value);
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
do_float : {
    vm_value_t *out = vm_int_run_read_store();
    *out = vm_value_from_float((vm_number_t)vm_value_to_float(*out));
    vm_int_run_next();
}
// math ops
do_fadd_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(vm_value_to_float(lhs) + vm_value_to_float(rhs));
    vm_int_run_next();
}
do_fadd_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_number_t rhs = vm_int_run_read().fval;
    *out = vm_value_from_float(vm_value_to_float(lhs) + rhs);
    vm_int_run_next();
}
do_fsub_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(vm_value_to_float(lhs) - vm_value_to_float(rhs));
    vm_int_run_next();
}
do_fsub_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_number_t rhs = vm_int_run_read().fval;
    *out = vm_value_from_float(vm_value_to_float(lhs) - rhs);
    vm_int_run_next();
}
do_fsub_ir : {
    vm_value_t *out = vm_int_run_read_store();
    vm_number_t lhs = vm_int_run_read().fval;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(lhs - vm_value_to_float(rhs));
    vm_int_run_next();
}
do_fmul_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(vm_value_to_float(lhs) * vm_value_to_float(rhs));
    vm_int_run_next();
}
do_fmul_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_number_t rhs = vm_int_run_read().fval;
    *out = vm_value_from_float(vm_value_to_float(lhs) * rhs);
    vm_int_run_next();
}
do_fdiv_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(vm_value_to_float(lhs) / vm_value_to_float(rhs));
    vm_int_run_next();
}
do_fdiv_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_number_t rhs = vm_int_run_read().fval;
    *out = vm_value_from_float(vm_value_to_float(lhs) / rhs);
    vm_int_run_next();
}
do_fdiv_ir : {
    vm_value_t *out = vm_int_run_read_store();
    vm_number_t lhs = vm_int_run_read().fval;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(lhs / vm_value_to_float(rhs));
    vm_int_run_next();
}
do_fmod_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(fmod(vm_value_to_float(lhs), vm_value_to_float(rhs)));
    vm_int_run_next();
}
do_fmod_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_number_t rhs = vm_int_run_read().fval;
    *out = vm_value_from_float(fmod(vm_value_to_float(lhs), rhs));
    vm_int_run_next();
}
do_fmod_ir : {
    vm_value_t *out = vm_int_run_read_store();
    vm_number_t lhs = vm_int_run_read().fval;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(fmod(lhs, vm_value_to_float(rhs)));
    vm_int_run_next();
}
// calls
do_call_l0 : {
    void *ptr = vm_int_run_read().ptr;
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l1 : {
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l2 : {
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l3 : {
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l4 : {
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l5 : {
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l6 : {
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l7 : {
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals[framesize + 1 + 6] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l8 : {
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals[framesize + 1 + 6] = vm_int_run_read_load();
    locals[framesize + 1 + 7] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_r0 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r1 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r2 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r3 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r4 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r5 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r6 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r7 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals[framesize + 1 + 6] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r8 : {
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals[framesize + 1 + 6] = vm_int_run_read_load();
    locals[framesize + 1 + 7] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
// extern
do_call_x0 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 0, NULL);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x1 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[1] = {vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 1, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x2 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[2] = {vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 2, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x3 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[3] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 3, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x4 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[4] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 4, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x5 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[5] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 5, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x6 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[6] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 6, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x7 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[7] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 7, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x8 : {
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[8] = {vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load(),
                            vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 8, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_c0 : {
    vm_value_t obj = vm_int_run_read_load();
    locals[framesize + 1 + 0] = obj;
    locals += framesize;
    *heads++ = head;
    vm_ir_block_t *func = vm_value_to_block(vm_gc_get_i(obj, 0));
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_c1 : {
    vm_value_t obj = vm_int_run_read_load();
    locals[framesize + 1 + 0] = obj;
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    vm_ir_block_t *func = vm_value_to_block(vm_gc_get_i(obj, 0));
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_c2 : {
    vm_value_t obj = vm_int_run_read_load();
    locals[framesize + 1 + 0] = obj;
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    vm_ir_block_t *func = vm_value_to_block(vm_gc_get_i(obj, 0));
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_c3 : {
    vm_value_t obj = vm_int_run_read_load();
    locals[framesize + 1 + 0] = obj;
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    vm_ir_block_t *func = vm_value_to_block(vm_gc_get_i(obj, 0));
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_c4 : {
    vm_value_t obj = vm_int_run_read_load();
    locals[framesize + 1 + 0] = obj;
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    vm_ir_block_t *func = vm_value_to_block(vm_gc_get_i(obj, 0));
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_c5 : {
    vm_value_t obj = vm_int_run_read_load();
    locals[framesize + 1 + 0] = obj;
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    vm_ir_block_t *func = vm_value_to_block(vm_gc_get_i(obj, 0));
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_c6 : {
    vm_value_t obj = vm_int_run_read_load();
    locals[framesize + 1 + 0] = obj;
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals[framesize + 1 + 6] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    vm_ir_block_t *func = vm_value_to_block(vm_gc_get_i(obj, 0));
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_c7 : {
    vm_value_t obj = vm_int_run_read_load();
    locals[framesize + 1 + 0] = obj;
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals[framesize + 1 + 6] = vm_int_run_read_load();
    locals[framesize + 1 + 7] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    vm_ir_block_t *func = vm_value_to_block(vm_gc_get_i(obj, 0));
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
// memorys
do_arr_f : {
    vm_value_t *out = vm_int_run_read_store();
    double len = vm_int_run_read().fval;
    vm_gc_run(&state->gc, locals + state->framesize);
    *out = vm_gc_arr(&state->gc, (vm_int_t) len);
    vm_int_run_next();
}
do_arr_r : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t len = vm_int_run_read_load();
    vm_gc_run(&state->gc, locals + state->framesize);
    *out = vm_gc_arr(&state->gc, (vm_int_t) vm_value_to_float(len));
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
    vm_int_t val = vm_int_run_read().ival;
    vm_gc_set_vi(obj, key, val);
    vm_int_run_next();
}
do_set_rir : {
    vm_value_t obj = vm_int_run_read_load();
    vm_int_t key = vm_int_run_read().ival;
    vm_value_t val = vm_int_run_read_load();
    vm_gc_set_iv(obj, key, val);
    vm_int_run_next();
}
do_set_rii : {
    vm_value_t obj = vm_int_run_read_load();
    vm_int_t key = vm_int_run_read().ival;
    vm_int_t val = vm_int_run_read().ival;
    vm_gc_set_ii(obj, key, val);
    vm_int_run_next();
}
do_get_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t key = vm_int_run_read_load();
    vm_value_t data = vm_gc_get_v(obj, key);
    *out = data;
    uint8_t type = vm_typeof(data);
    if (head[type].ptr == NULL) {
        head[type].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    }
    switch (type) {
        case VM_TYPE_NIL: 
            head = head[VM_TYPE_NIL].ptr;
            vm_int_run_next();
        case VM_TYPE_BOOL:
            head = head[VM_TYPE_BOOL].ptr;
            vm_int_run_next();
        case VM_TYPE_FLOAT:
            head = head[VM_TYPE_FLOAT].ptr;
            vm_int_run_next();
        case VM_TYPE_FUNC:
            head = head[VM_TYPE_FUNC].ptr;
            vm_int_run_next();
        case VM_TYPE_ARRAY:
            head = head[VM_TYPE_ARRAY].ptr;
            vm_int_run_next();
        case VM_TYPE_TABLE:
            head = head[VM_TYPE_TABLE].ptr;
            vm_int_run_next();
    }
    __builtin_unreachable();
}
do_get_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t obj = vm_int_run_read_load();
    vm_int_t key = vm_int_run_read().ival;
    vm_value_t data = vm_gc_get_i(obj, key);
    *out = data;
    uint8_t type = vm_typeof(data);
    if (head[type].ptr == NULL) {
        head[type].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    }
    switch (type) {
        case VM_TYPE_NIL: 
            head = head[VM_TYPE_NIL].ptr;
            vm_int_run_next();
        case VM_TYPE_BOOL:
            head = head[VM_TYPE_BOOL].ptr;
            vm_int_run_next();
        case VM_TYPE_FLOAT:
            head = head[VM_TYPE_FLOAT].ptr;
            vm_int_run_next();
        case VM_TYPE_FUNC:
            head = head[VM_TYPE_FUNC].ptr;
            vm_int_run_next();
        case VM_TYPE_ARRAY:
            head = head[VM_TYPE_ARRAY].ptr;
            vm_int_run_next();
        case VM_TYPE_TABLE:
            head = head[VM_TYPE_TABLE].ptr;
            vm_int_run_next();
    }
    __builtin_unreachable();
}
do_len_r : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t obj = vm_int_run_read_load();
    *out = vm_value_from_float(vm_gc_len(obj));
    vm_int_run_next();
}
// io
do_out_i : {
    fprintf(stdout, "%c", (int)vm_int_run_read().ival);
    vm_int_run_next();
}
do_out_r : {
    fprintf(stdout, "%c", (int)vm_value_to_float(vm_int_run_read_load()));
    vm_int_run_next();
}
// jump compiled
do_jump_l : {
    head = head->ptr;
    vm_int_run_next();
}
// float branch compiled
do_bb_rll : {
    vm_value_t value = vm_int_run_read_load();
    if (vm_value_to_bool(value)) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_fblt_rrll : {
    vm_number_t lhs = vm_value_to_float(vm_int_run_read_load());
    vm_number_t rhs = vm_value_to_float(vm_int_run_read_load());
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_fblt_rill : {
    vm_number_t lhs = vm_value_to_float(vm_int_run_read_load());
    vm_number_t rhs = vm_int_run_read().fval;
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_fblt_irll : {
    vm_number_t lhs = vm_int_run_read().fval;
    vm_number_t rhs = vm_value_to_float(vm_int_run_read_load());
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_fbeq_rrll : {
    vm_number_t lhs = vm_value_to_float(vm_int_run_read_load());
    vm_number_t rhs = vm_value_to_float(vm_int_run_read_load());
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_fbeq_rill : {
    vm_number_t lhs = vm_value_to_float(vm_int_run_read_load());
    vm_number_t rhs = vm_int_run_read().fval;
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_fbeq_irll : {
    vm_number_t lhs = vm_int_run_read().fval;
    vm_number_t rhs = vm_value_to_float(vm_int_run_read_load());
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
// ret
do_ret_i : {
    vm_value_t value = vm_value_from_float(vm_int_run_read().fval);
    head = *--heads;
    locals -= framesize;
    vm_int_opcode_t out = vm_int_run_read();
    locals[out.reg] = value;
    void *pblock = head[VM_TYPE_FLOAT].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_FLOAT].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
    vm_int_run_next();
}
do_ret_rv : {
    vm_value_t value = locals[head->reg];
    locals -= framesize;
    head = *--heads;
    locals[vm_int_run_read().reg] = value;
    void *pblock = head[VM_TYPE_NIL].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_NIL].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
    vm_int_run_next();
}
do_ret_rb : {
    vm_value_t value = locals[head->reg];
    locals -= framesize;
    head = *--heads;
    locals[vm_int_run_read().reg] = value;
    void *pblock = head[VM_TYPE_BOOL].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_BOOL].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
    vm_int_run_next();
}
do_ret_ri : {
    vm_value_t value = locals[head->reg];
    locals -= framesize;
    head = *--heads;
    locals[vm_int_run_read().reg] = value;
    void *pblock = head[VM_TYPE_FLOAT].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_FLOAT].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
    vm_int_run_next();
}
do_ret_rf : {
    vm_value_t value = locals[head->reg];
    locals -= framesize;
    head = *--heads;
    locals[vm_int_run_read().reg] = value;
    void *pblock = head[VM_TYPE_FUNC].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_FUNC].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
    vm_int_run_next();
}
do_ret_ra : {
    vm_value_t value = locals[head->reg];
    locals -= framesize;
    head = *--heads;
    locals[vm_int_run_read().reg] = value;
    void *pblock = head[VM_TYPE_ARRAY].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_ARRAY].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
    vm_int_run_next();
}
do_ret_rt : {
    vm_value_t value = locals[head->reg];
    locals -= framesize;
    head = *--heads;
    locals[vm_int_run_read().reg] = value;
    void *pblock = head[VM_TYPE_TABLE].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_TABLE].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
    vm_int_run_next();
}
do_exit : { return vm_value_nil(); }
// jmp/call tmp
do_call_t0 : {
    head[-1].ptr = &&do_call_l0;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t1 : {
    head[-1].ptr = &&do_call_l1;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t2 : {
    head[-1].ptr = &&do_call_l2;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t3 : {
    head[-1].ptr = &&do_call_l3;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t4 : {
    head[-1].ptr = &&do_call_l4;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t5 : {
    head[-1].ptr = &&do_call_l5;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t6 : {
    head[-1].ptr = &&do_call_l6;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t7 : {
    head[-1].ptr = &&do_call_l7;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals[framesize + 1 + 6] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t8 : {
    head[-1].ptr = &&do_call_l8;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    locals[framesize + 1 + 2] = vm_int_run_read_load();
    locals[framesize + 1 + 3] = vm_int_run_read_load();
    locals[framesize + 1 + 4] = vm_int_run_read_load();
    locals[framesize + 1 + 5] = vm_int_run_read_load();
    locals[framesize + 1 + 6] = vm_int_run_read_load();
    locals[framesize + 1 + 7] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_jump_t : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_jump_l;
    vm_int_opcode_t *block = &vm_int_run_read();
    block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    head = loc;
    vm_int_run_next();
}
do_bb_rtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_bb_rll;
    head += 1;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_fblt_rrtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fblt_rrll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_fblt_ritt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fblt_rill;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_fblt_irtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fblt_irll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_fbeq_rrtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fbeq_rrll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_fbeq_ritt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fbeq_rill;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_fbeq_irtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fbeq_irll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_tab : {
    vm_value_t *out = vm_int_run_read_store();
    vm_gc_run(&state->gc, locals + state->framesize);
    *out = vm_gc_tab(&state->gc);
    vm_int_run_next();
}
do_tget_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t ind = vm_int_run_read_load();
    vm_value_t data = vm_gc_table_get(vm_value_to_table(obj), ind);
    *out = data;
    uint8_t type = vm_typeof(data);
    if (head[type].ptr == NULL) {
        head[type].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    }
    switch (type) {
        case VM_TYPE_NIL: 
            head = head[VM_TYPE_NIL].ptr;
            vm_int_run_next();
        case VM_TYPE_BOOL:
            head = head[VM_TYPE_BOOL].ptr;
            vm_int_run_next();
        case VM_TYPE_FLOAT:
            head = head[VM_TYPE_FLOAT].ptr;
            vm_int_run_next();
        case VM_TYPE_FUNC:
            head = head[VM_TYPE_FUNC].ptr;
            vm_int_run_next();
        case VM_TYPE_ARRAY:
            head = head[VM_TYPE_ARRAY].ptr;
            vm_int_run_next();
        case VM_TYPE_TABLE:
            head = head[VM_TYPE_TABLE].ptr;
            vm_int_run_next();
    }
    __builtin_unreachable();
}
do_tget_rf : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t ind = vm_value_from_float(vm_int_run_read().fval);
    vm_value_t data = vm_gc_table_get(vm_value_to_table(obj), ind);
    *out = data;
    uint8_t type = vm_typeof(data);
    if (head[type].ptr == NULL) {
        head[type].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    }
    switch (type) {
        case VM_TYPE_NIL: 
            head = head[VM_TYPE_NIL].ptr;
            vm_int_run_next();
        case VM_TYPE_BOOL:
            head = head[VM_TYPE_BOOL].ptr;
            vm_int_run_next();
        case VM_TYPE_FLOAT:
            head = head[VM_TYPE_FLOAT].ptr;
            vm_int_run_next();
        case VM_TYPE_FUNC:
            head = head[VM_TYPE_FUNC].ptr;
            vm_int_run_next();
        case VM_TYPE_ARRAY:
            head = head[VM_TYPE_ARRAY].ptr;
            vm_int_run_next();
        case VM_TYPE_TABLE:
            head = head[VM_TYPE_TABLE].ptr;
            vm_int_run_next();
    }
    __builtin_unreachable();
}
do_tset_rrr : {
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t ind = vm_int_run_read_load();
    vm_value_t val = vm_int_run_read_load();
    vm_gc_table_set(vm_value_to_table(obj), ind, val);
    vm_int_run_next();
}
do_tset_rrf : {
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t ind = vm_int_run_read_load();
    vm_value_t val = vm_value_from_float(vm_int_run_read().fval);
    vm_gc_table_set(vm_value_to_table(obj), ind, val);
    vm_int_run_next();
}
do_tset_rfr : {
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t ind = vm_value_from_float(vm_int_run_read().fval);
    vm_value_t val = vm_int_run_read_load();
    vm_gc_table_set(vm_value_to_table(obj), ind, val);
    vm_int_run_next();
}
do_tset_rff : {
    vm_value_t obj = vm_int_run_read_load();
    vm_value_t ind = vm_value_from_float(vm_int_run_read().fval);
    vm_value_t val = vm_value_from_float(vm_int_run_read().fval);
    vm_gc_table_set(vm_value_to_table(obj), ind, val);
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
        if (blocks[i].id >= 0) {
            if (blocks[i].nregs >= state.framesize) {
                state.framesize = blocks[i].nregs + 1;
            }
        }
    }
    state.locals = locals;
    state.heads = vm_malloc(sizeof(vm_int_opcode_t *) * (nregs / state.framesize + 1));
    vm_gc_init(&state.gc, nregs, locals);
    vm_value_t ret = vm_int_run(&state, cur);
    vm_gc_deinit(&state.gc);
    return ret;
}

#include "../toir.h"

vm_value_t vm_run_arch_int(size_t nops, vm_opcode_t *ops, vm_int_func_t *funcs) {
    size_t nblocks = nops;
    vm_ir_block_t *blocks = vm_ir_parse(nblocks, ops);
    vm_value_t ret = vm_ir_be_int3(nblocks, blocks, funcs);
    vm_ir_blocks_free(nblocks, blocks);
    return ret;
}
