
#if VM_DEBUG_SPALL
#define SPALL_IMPLEMENTATION
#include "spall.h"
#endif

#include "int3.h"

#include "../build.h"
#include "debug.h"

#if VM_DEBUG_SPALL
#define VM_USE_SPALL 1
#else
#define VM_USE_SPALL 0
#endif

#define vm_int_block_comp_put_ptr(arg_)                 \
    ({                                                  \
        size_t arg = (arg_);                            \
        if (ptrs[arg] == NULL) {                        \
            fprintf(stderr, "bad ptr: ptrs[%zu]", arg); \
            __builtin_trap();                           \
        }                                               \
        if (state->debug_print_instrs || state->use_spall) {                      \
            buf.ops[buf.len++].ptr = ptrs[VM_INT_OP_DEBUG_PRINT_INSTRS];            \
            buf.ops[buf.len++].reg = (arg_);            \
            buf.ops[buf.len++].ptr = ptrs[(arg_)];            \
        } else {                                        \
            buf.ops[buf.len++].ptr = ptrs[(arg_)];      \
        }                                               \
    })
#define vm_int_block_comp_put_out(out_) buf.ops[buf.len++].reg = (out_)

#define vm_int_block_comp_put_reg(vreg_) buf.ops[buf.len++].reg = (vreg_).reg
#define vm_int_block_comp_put_bval(val_) buf.ops[buf.len++].bval = (val_).logic
#define vm_int_block_comp_put_ival(val_) buf.ops[buf.len++].ival = (val_).num
#define vm_int_block_comp_put_fval(val_) buf.ops[buf.len++].fval = (val_).num

#define vm_int_block_comp_put_regc(vreg_) buf.ops[buf.len++].reg = (vreg_)
#define vm_int_block_comp_put_bvalc(val_) buf.ops[buf.len++].bval = (val_)
#define vm_int_block_comp_put_ivalc(val_) buf.ops[buf.len++].ival = (val_)
#define vm_int_block_comp_put_fvalc(val_) buf.ops[buf.len++].fval = (val_)

#define vm_int_block_comp_put_block(block_) ({ \
    buf.ops[buf.len++].block = (block_);       \
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
        if (fmod(val, 1) == 0.0) {                      \
            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_I); \
            vm_int_block_comp_put_out(reg);             \
            vm_int_block_comp_put_ivalc(val);           \
            types[reg] = VM_TYPE_I32;                   \
        } else {                                        \
            vm_int_block_comp_put_ptr(VM_INT_OP_MOV_F); \
            vm_int_block_comp_put_out(reg);             \
            vm_int_block_comp_put_fvalc(val);           \
            types[reg] = VM_TYPE_F64;                   \
        }                                               \
    })

#define vm_int_block_comp_ensure_float_reg(reg_)                                             \
    ({                                                                                       \
        size_t reg = (reg_);                                                                 \
        if (types[reg] == VM_TYPE_F64) {                                                     \
            /* :) */                                                                         \
        } else if (types[reg] == VM_TYPE_I32) {                                              \
            vm_int_block_comp_put_ptr(VM_INT_OP_FMOV_R);                                     \
            vm_int_block_comp_put_out(reg);                                                  \
        } else {                                                                             \
            fprintf(stderr, "TYPE ERROR (reg: %zu) (type: %zu)\n", reg, (size_t)types[reg]); \
            __builtin_trap();                                                                \
        }                                                                                    \
    })

void *vm_int_block_comp(vm_int_state_t *state, void **ptrs, vm_ir_block_t *block) {
#if VM_DEBUG_SPALL
    if (state->use_spall) {
        uint64_t begin = vm_trace_time();
        SpallTraceBegin(&state->spall_ctx, NULL, begin, "Basic Block Compile");
        SpallTraceBegin(&state->spall_ctx, NULL, begin, "Basic Block Cache Check");
    }
#endif
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
#if VM_DEBUG_SPALL
            if (state->use_spall) {
                uint64_t end = vm_trace_time();
                SpallTraceEnd(&state->spall_ctx, NULL, end);
                SpallTraceEnd(&state->spall_ctx, NULL, end);
            }
#endif
            return data->bufs[i].ops;
        next:;
        }
    }
#if VM_DEBUG_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    vm_int_buf_t buf;
    buf.len = 0;
    buf.alloc = 16;
    buf.ops = vm_alloc0(sizeof(vm_int_opcode_t) * buf.alloc);
    uint8_t *types = vm_malloc(sizeof(uint8_t) * state->framesize);
    for (size_t i = 0; i < block->nargs; i++) {
        size_t reg = block->args[i];
        types[reg] = vm_typeof(state->locals[reg]);
    }
#if VM_DEBUG_COMP
    fprintf(stderr, "block .%zu(", block->id);
    for (size_t i = 0; i < block->nargs; i++) {
        if (i != 0) {
            fprintf(stderr, ", ");
        }
        size_t reg = block->args[i];
        uint8_t type = types[reg];
        static const char *const typenames[VM_TYPE_MAX] = {
            [VM_TYPE_NIL] = "nil",
            [VM_TYPE_BOOL] = "bool",
            [VM_TYPE_I32] = "int",
            [VM_TYPE_F64] = "float",
            [VM_TYPE_FUNC] = "func",
            [VM_TYPE_ARRAY] = "array",
            [VM_TYPE_TABLE] = "table",
        };
        const char *typename = typenames[type];
        if (!typename) __builtin_trap();
        fprintf(stderr, "r%zu : %s", reg, typename);
    }
    fprintf(stderr, ") {\n");
#endif
inline_jump:;
    for (size_t arg = 0; arg < block->len; arg++) {
        if (buf.len + 16 >= buf.alloc) {
            buf.alloc = buf.len * 2 + 16;
            buf.ops = vm_realloc(buf.ops, sizeof(vm_int_opcode_t) * buf.alloc);
        }
        vm_ir_instr_t *instr = block->instrs[arg];
#if VM_DEBUG_COMP
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
                            vm_int_block_comp_put_reg(instr->args[0]);
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
                            vm_int_block_comp_put_bval(instr->args[0]);
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
                            vm_int_block_comp_put_block(instr->args[0].func);
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
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && types[instr->args[1].reg] == VM_TYPE_I32) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32ADD_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FADD_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        } else {
                            // r = add r i
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && fmod(instr->args[1].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32ADD_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FADD_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = add i r
                            if (types[instr->args[1].reg] == VM_TYPE_I32 && fmod(instr->args[0].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32ADD_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                vm_int_block_comp_put_ival(instr->args[0]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FADD_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                vm_int_block_comp_put_fval(instr->args[0]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
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
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && types[instr->args[1].reg] == VM_TYPE_I32) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32SUB_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FSUB_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        } else {
                            // r = sub r i
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && fmod(instr->args[1].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32SUB_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FSUB_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = sub i r
                            if (types[instr->args[1].reg] == VM_TYPE_I32 && fmod(instr->args[0].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32SUB_IR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_ival(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FSUB_FR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_fval(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
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
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && types[instr->args[1].reg] == VM_TYPE_I32) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32MUL_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMUL_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        } else {
                            // r = mul r i
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && fmod(instr->args[1].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32MUL_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMUL_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = mul i r
                            if (types[instr->args[1].reg] == VM_TYPE_I32 && fmod(instr->args[0].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32MUL_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                vm_int_block_comp_put_ival(instr->args[0]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMUL_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                vm_int_block_comp_put_fval(instr->args[0]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        } else {
                            // r = move i
                            vm_int_block_comp_mov(instr->out.reg, instr->args[0].num + instr->args[1].num);
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
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && types[instr->args[1].reg] == VM_TYPE_I32) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32DIV_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_UNKNOWN;
                                vm_int_block_comp_put_block(block->branch->targets[0]);
                                vm_int_block_comp_put_block(NULL);
                                vm_int_block_comp_put_block(NULL);
                                goto retv;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FDIV_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        } else {
                            // r = div r i
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && fmod(instr->args[1].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32DIV_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_UNKNOWN;
                                vm_int_block_comp_put_block(block->branch->targets[0]);
                                vm_int_block_comp_put_block(NULL);
                                vm_int_block_comp_put_block(NULL);
                                goto retv;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FDIV_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = div i r
                            if (types[instr->args[1].reg] == VM_TYPE_I32 && fmod(instr->args[0].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32DIV_IR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_ival(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_UNKNOWN;
                                vm_int_block_comp_put_block(block->branch->targets[0]);
                                vm_int_block_comp_put_block(NULL);
                                vm_int_block_comp_put_block(NULL);
                                goto retv;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FDIV_FR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_fval(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
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
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && types[instr->args[1].reg] == VM_TYPE_I32) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32MOD_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMOD_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        } else {
                            // r = mod r i
                            if (types[instr->args[0].reg] == VM_TYPE_I32 && fmod(instr->args[1].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32MOD_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMOD_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
                        }
                    } else {
                        if (instr->args[1].type == VM_IR_ARG_REG) {
                            // r = mod i r
                            if (types[instr->args[1].reg] == VM_TYPE_I32 && fmod(instr->args[0].num, 1) == 0.0) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_I32MOD_IR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_ival(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_I32;
                            } else {
                                vm_int_block_comp_ensure_float_reg(instr->args[1].reg);
                                vm_int_block_comp_put_ptr(VM_INT_OP_FMOD_FR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_fval(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                types[instr->out.reg] = VM_TYPE_F64;
                            }
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
                    vm_int_block_comp_put_ival(instr->args[0]);
                } else {
                    if (types[instr->args[0].reg] == VM_TYPE_FUNC) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_CALL_R0 + nargs);
                        vm_int_block_comp_put_block(instr->args[0].func);
                    } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_CALL_C0 + nargs);
                        vm_int_block_comp_put_reg(instr->args[0]);
                    } else {
                        fprintf(stderr, "type error on call r%zu (type %zu)\n", instr->args[0].reg, (size_t)types[instr->args[0].reg]);
                        __builtin_trap();
                    }
                }
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++) {
                    if (instr->args[i].type == VM_IR_ARG_NUM) {
                        vm_int_block_comp_put_regc(state->framesize + i);
                    } else {
                        vm_int_block_comp_put_reg(instr->args[i]);
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
                        vm_int_block_comp_put_reg(instr->args[0]);
                        types[instr->out.reg] = VM_TYPE_ARRAY;
                    } else {
                        // r = new i
                        vm_int_block_comp_put_ptr(VM_INT_OP_ARR_F);
                        vm_int_block_comp_put_out(instr->out.reg);
                        vm_int_block_comp_put_fval(instr->args[0]);
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
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_GET_RR);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                            } else {
                                fprintf(stderr, "cannot get: r%zu (tag: %zu)\n", instr->args[0].reg, (size_t)types[instr->args[0].reg]);
                                __builtin_trap();
                            }
                        } else {
                            // r = get r i
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TGET_RF);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1]);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_GET_RI);
                                vm_int_block_comp_put_out(instr->out.reg);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1]);
                            } else {
                                fprintf(stderr, "cannot get: r%zu (tag: %zu)\n", instr->args[0].reg, (size_t)types[instr->args[0].reg]);
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
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                vm_int_block_comp_put_reg(instr->args[2]);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_SET_RRR);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                vm_int_block_comp_put_reg(instr->args[2]);
                            } else {
                                fprintf(stderr, "cannot set: r%zu\n", instr->args[0].reg);
                                __builtin_trap();
                            }
                        } else {
                            // set r r i
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TSET_RRF);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                vm_int_block_comp_put_fval(instr->args[2]);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_SET_RRI);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_reg(instr->args[1]);
                                vm_int_block_comp_put_ival(instr->args[2]);
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
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1]);
                                vm_int_block_comp_put_reg(instr->args[2]);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_SET_RIR);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1]);
                                vm_int_block_comp_put_reg(instr->args[2]);
                            } else {
                                fprintf(stderr, "cannot set: r%zu\n", instr->args[0].reg);
                                __builtin_trap();
                            }
                        } else {
                            // set r i i
                            if (types[instr->args[0].reg] == VM_TYPE_TABLE) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_TSET_RFF);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_fval(instr->args[1]);
                                vm_int_block_comp_put_fval(instr->args[2]);
                            } else if (types[instr->args[0].reg] == VM_TYPE_ARRAY) {
                                vm_int_block_comp_put_ptr(VM_INT_OP_SET_RII);
                                vm_int_block_comp_put_reg(instr->args[0]);
                                vm_int_block_comp_put_ival(instr->args[1]);
                                vm_int_block_comp_put_ival(instr->args[2]);
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
                            vm_int_block_comp_put_reg(instr->args[0]);
                            types[instr->out.reg] = VM_TYPE_I32;
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
                        vm_int_block_comp_mov(instr->out.reg, VM_TYPE_F64);
                    }
                }
                break;
            }
            case VM_IR_IOP_OUT: {
                if (instr->args[0].type == VM_IR_ARG_NUM) {
                    // out i
                    vm_int_block_comp_put_ptr(VM_INT_OP_OUT_I);
                    vm_int_block_comp_put_ival(instr->args[0]);
                } else {
                    // out r
                    vm_int_block_comp_ensure_float_reg(instr->args[0].reg);
                    vm_int_block_comp_put_ptr(VM_INT_OP_OUT_R);
                    vm_int_block_comp_put_reg(instr->args[0]);
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
                    vm_int_block_comp_put_reg(block->branch->args[0]);
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
                    if (types[block->branch->args[1].reg] == VM_TYPE_I32 && fmod(block->branch->args[0].num, 1) == 0.0) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_I32BLT_IRTT);
                        vm_int_block_comp_put_ival(block->branch->args[0]);
                        vm_int_block_comp_put_reg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    } else {
                        vm_int_block_comp_ensure_float_reg(block->branch->args[1].reg);
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBLT_FRTT);
                        vm_int_block_comp_put_fval(block->branch->args[0]);
                        vm_int_block_comp_put_reg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    }
                }
            } else {
                if (block->branch->args[1].type == VM_IR_ARG_NUM) {
                    // blt r i l l
                    if (types[block->branch->args[0].reg] == VM_TYPE_I32 && fmod(block->branch->args[1].num, 1) == 0.0) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_I32BLT_RITT);
                        vm_int_block_comp_put_reg(block->branch->args[0]);
                        vm_int_block_comp_put_ival(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    } else {
                        vm_int_block_comp_ensure_float_reg(block->branch->args[0].reg);
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBLT_RFTT);
                        vm_int_block_comp_put_reg(block->branch->args[0]);
                        vm_int_block_comp_put_fval(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    }
                } else {
                    // blt r r l l
                    if (types[block->branch->args[0].reg] == VM_TYPE_I32 && types[block->branch->args[1].reg] == VM_TYPE_I32) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_I32BLT_RRTT);
                        vm_int_block_comp_put_reg(block->branch->args[0]);
                        vm_int_block_comp_put_reg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    } else {
                        vm_int_block_comp_ensure_float_reg(block->branch->args[0].reg);
                        vm_int_block_comp_ensure_float_reg(block->branch->args[1].reg);
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBLT_RRTT);
                        vm_int_block_comp_put_reg(block->branch->args[0]);
                        vm_int_block_comp_put_reg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    }
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
                    if (types[block->branch->args[1].reg] == VM_TYPE_I32 && fmod(block->branch->args[0].num, 1) == 0.0) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_I32BEQ_IRTT);
                        vm_int_block_comp_put_ival(block->branch->args[0]);
                        vm_int_block_comp_put_reg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    } else {
                        vm_int_block_comp_ensure_float_reg(block->branch->args[1].reg);
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBEQ_FRTT);
                        vm_int_block_comp_put_fval(block->branch->args[0]);
                        vm_int_block_comp_put_reg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    }
                }
            } else {
                if (block->branch->args[1].type == VM_IR_ARG_NUM) {
                    // beq r i l l
                    if (types[block->branch->args[0].reg] == VM_TYPE_I32 && fmod(block->branch->args[1].num, 1) == 0.0) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_I32BEQ_RITT);
                        vm_int_block_comp_put_reg(block->branch->args[0]);
                        vm_int_block_comp_put_ival(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    } else {
                        vm_int_block_comp_ensure_float_reg(block->branch->args[0].reg);
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBEQ_RFTT);
                        vm_int_block_comp_put_reg(block->branch->args[0]);
                        vm_int_block_comp_put_fval(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    }
                } else {
                    // beq r r l l
                    if (types[block->branch->args[0].reg] == VM_TYPE_I32 && types[block->branch->args[1].reg] == VM_TYPE_I32) {
                        vm_int_block_comp_put_ptr(VM_INT_OP_I32BEQ_RRTT);
                        vm_int_block_comp_put_reg(block->branch->args[0]);
                        vm_int_block_comp_put_reg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    } else {
                        vm_int_block_comp_ensure_float_reg(block->branch->args[0].reg);
                        vm_int_block_comp_ensure_float_reg(block->branch->args[1].reg);
                        vm_int_block_comp_put_ptr(VM_INT_OP_FBEQ_RRTT);
                        vm_int_block_comp_put_reg(block->branch->args[0]);
                        vm_int_block_comp_put_reg(block->branch->args[1]);
                        vm_int_block_comp_put_block(block->branch->targets[0]);
                        vm_int_block_comp_put_block(block->branch->targets[1]);
                    }
                }
            }
            break;
        }
        case VM_IR_BOP_RET: {
            if (block->branch->args[0].type == VM_IR_ARG_NUM) {
                // ret i
                vm_int_block_comp_put_ptr(VM_INT_OP_RET_I);
                vm_int_block_comp_put_reg(block->branch->args[0]);
            } else {
                uint8_t type = types[block->branch->args[0].reg];
                if (type == VM_TYPE_NIL) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RV);
                }
                if (type == VM_TYPE_BOOL) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RB);
                    vm_int_block_comp_put_reg(block->branch->args[0]);
                }
                if (type == VM_TYPE_I32) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RI);
                    vm_int_block_comp_put_reg(block->branch->args[0]);
                }
                if (type == VM_TYPE_F64) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RF);
                    vm_int_block_comp_put_reg(block->branch->args[0]);
                }
                if (type == VM_TYPE_FUNC) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RF);
                    vm_int_block_comp_put_reg(block->branch->args[0]);
                }
                if (type == VM_TYPE_ARRAY) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RA);
                    vm_int_block_comp_put_reg(block->branch->args[0]);
                }
                if (type == VM_TYPE_TABLE) {
                    vm_int_block_comp_put_ptr(VM_INT_OP_RET_RT);
                    vm_int_block_comp_put_reg(block->branch->args[0]);
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
#if VM_DEBUG_COMP
    fprintf(stderr, "  ");
    vm_ir_print_branch(stderr, block->branch);
    fprintf(stderr, "\n}\n\n");
#endif
    for (size_t a = 0; a < state->framesize; a++) {
        types[a] = vm_typeof(state->locals[a]);
    }
    vm_int_data_push(data, buf, types);
#if VM_DEBUG_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    return buf.ops;
}

#define vm_int_run_read()              \
    (*({                               \
        vm_int_opcode_t *ret = (head); \
        head += 1;                     \
        ret;                           \
    }))

#define vm_int_run_next() goto *vm_int_run_read().ptr

#define vm_int_not_implemented()                                       \
    ({                                                                 \
        fprintf(stderr, "UNIMPLEMENTED line=%zu\n", (size_t)__LINE__); \
        return;                                                        \
    })

#define vm_int_run_read_store() (&locals[vm_int_run_read().reg])

#define vm_int_run_read_load() (locals[vm_int_run_read().reg])

#define vm_int_run_save()       \
    ({                          \
        state->locals = locals; \
        state->heads = heads;   \
        state;                  \
    })

vm_value_t vm_int_run(vm_int_state_t *state, vm_ir_block_t *block) {
    static void *ptrs[VM_INT_MAX_OP] = {
        [VM_INT_OP_EXIT] = &&do_exit,
        [VM_INT_OP_MOV_V] = &&do_mov_v,
        [VM_INT_OP_MOV_B] = &&do_mov_b,
        [VM_INT_OP_MOV_I] = &&do_mov_i,
        [VM_INT_OP_MOV_F] = &&do_mov_f,
        [VM_INT_OP_MOV_R] = &&do_mov_r,
        [VM_INT_OP_MOV_T] = &&do_mov_t,
        [VM_INT_OP_FMOV_R] = &&do_fmov_r,
        [VM_INT_OP_I32ADD_RR] = &&do_i32add_rr,
        [VM_INT_OP_I32ADD_RI] = &&do_i32add_ri,
        [VM_INT_OP_I32SUB_RR] = &&do_i32sub_rr,
        [VM_INT_OP_I32SUB_RI] = &&do_i32sub_ri,
        [VM_INT_OP_I32SUB_IR] = &&do_i32sub_ir,
        [VM_INT_OP_I32MUL_RR] = &&do_i32mul_rr,
        [VM_INT_OP_I32MUL_RI] = &&do_i32mul_ri,
        [VM_INT_OP_I32DIV_RR] = &&do_i32div_rr,
        [VM_INT_OP_I32DIV_RI] = &&do_i32div_ri,
        [VM_INT_OP_I32DIV_IR] = &&do_i32div_ir,
        [VM_INT_OP_I32MOD_RR] = &&do_i32mod_rr,
        [VM_INT_OP_I32MOD_RI] = &&do_i32mod_ri,
        [VM_INT_OP_I32MOD_IR] = &&do_i32mod_ir,
        [VM_INT_OP_I32BLT_RRLL] = &&do_i32blt_rrll,
        [VM_INT_OP_I32BLT_RILL] = &&do_i32blt_rill,
        [VM_INT_OP_I32BLT_IRLL] = &&do_i32blt_irll,
        [VM_INT_OP_I32BEQ_RRLL] = &&do_i32beq_rrll,
        [VM_INT_OP_I32BEQ_RILL] = &&do_i32beq_rill,
        [VM_INT_OP_I32BEQ_IRLL] = &&do_i32beq_irll,
        [VM_INT_OP_I32BLT_RRTT] = &&do_i32blt_rrtt,
        [VM_INT_OP_I32BLT_RITT] = &&do_i32blt_ritt,
        [VM_INT_OP_I32BLT_IRTT] = &&do_i32blt_irtt,
        [VM_INT_OP_I32BEQ_RRTT] = &&do_i32beq_rrtt,
        [VM_INT_OP_I32BEQ_RITT] = &&do_i32beq_ritt,
        [VM_INT_OP_I32BEQ_IRTT] = &&do_i32beq_irtt,
        [VM_INT_OP_FADD_RR] = &&do_fadd_rr,
        [VM_INT_OP_FADD_RF] = &&do_fadd_rf,
        [VM_INT_OP_FSUB_RR] = &&do_fsub_rr,
        [VM_INT_OP_FSUB_RF] = &&do_fsub_rf,
        [VM_INT_OP_FSUB_FR] = &&do_fsub_fr,
        [VM_INT_OP_FMUL_RR] = &&do_fmul_rr,
        [VM_INT_OP_FMUL_RF] = &&do_fmul_rf,
        [VM_INT_OP_FDIV_RR] = &&do_fdiv_rr,
        [VM_INT_OP_FDIV_RF] = &&do_fdiv_rf,
        [VM_INT_OP_FDIV_FR] = &&do_fdiv_fr,
        [VM_INT_OP_FMOD_RR] = &&do_fmod_rr,
        [VM_INT_OP_FMOD_RF] = &&do_fmod_rf,
        [VM_INT_OP_FMOD_FR] = &&do_fmod_fr,
        [VM_INT_OP_FBLT_RRLL] = &&do_fblt_rrll,
        [VM_INT_OP_FBLT_RFLL] = &&do_fblt_rfll,
        [VM_INT_OP_FBLT_FRLL] = &&do_fblt_frll,
        [VM_INT_OP_FBEQ_RRLL] = &&do_fbeq_rrll,
        [VM_INT_OP_FBEQ_RFLL] = &&do_fbeq_rfll,
        [VM_INT_OP_FBEQ_FRLL] = &&do_fbeq_frll,
        [VM_INT_OP_FBLT_RRTT] = &&do_fblt_rrtt,
        [VM_INT_OP_FBLT_RFTT] = &&do_fblt_rftt,
        [VM_INT_OP_FBLT_FRTT] = &&do_fblt_frtt,
        [VM_INT_OP_FBEQ_RRTT] = &&do_fbeq_rrtt,
        [VM_INT_OP_FBEQ_RFTT] = &&do_fbeq_rftt,
        [VM_INT_OP_FBEQ_FRTT] = &&do_fbeq_frtt,
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
        [VM_INT_OP_RET_I] = &&do_ret_i,
        [VM_INT_OP_RET_RV] = &&do_ret_rv,
        [VM_INT_OP_RET_RB] = &&do_ret_rb,
        [VM_INT_OP_RET_RI] = &&do_ret_ri,
        [VM_INT_OP_RET_RIF] = &&do_ret_rif,
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
        [VM_INT_OP_TAB] = &&do_tab,
        [VM_INT_OP_TSET_RRR] = &&do_tset_rrr,
        [VM_INT_OP_TSET_RRF] = &&do_tset_rrf,
        [VM_INT_OP_TSET_RFR] = &&do_tset_rfr,
        [VM_INT_OP_TSET_RFF] = &&do_tset_rff,
        [VM_INT_OP_TGET_RR] = &&do_tget_rr,
        [VM_INT_OP_TGET_RF] = &&do_tget_rf,
        [VM_INT_OP_DEBUG_PRINT_INSTRS] = &&do_debug_print_instrs,
    };
    vm_value_t *init_locals = state->locals;
    vm_value_t *locals = init_locals;
    vm_int_opcode_t **init_heads = state->heads;
    vm_int_opcode_t **heads = init_heads;
    size_t framesize = state->framesize;
    vm_int_opcode_t *head = vm_int_block_comp(vm_int_run_save(), ptrs, block);
#if VM_DEBUG_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "init");
    }
#endif
    vm_int_run_next();
do_debug_print_instrs : {
#if VM_DEBUG_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    size_t opcode = vm_int_run_read().reg;
    void *head0 = head;
    head += 1;
    const char *name = vm_int_debug_instr_name(opcode); 
    if (state->debug_print_instrs) {
        const char *fmt = vm_int_debug_instr_format(opcode);
        switch (*fmt) {
        case ':': {
            fprintf(state->debug_print_instrs, "r%zu <- ", vm_int_run_read().reg);
            fmt += 1;
            break;
        }
        case '.': {
            fmt += 1;
            break;
        }
        case '?': {
            fmt += 1;
            break;
        }
        default: {
            break;
        }
        }
        fprintf(state->debug_print_instrs, "%s", name, opcode);
        while (*fmt != '\0') {
            fprintf(state->debug_print_instrs, " ");
            switch (*fmt) {
            case 'a': {
                fprintf(state->debug_print_instrs, "[array %p]", vm_value_to_array(vm_int_run_read_load()));
                break;
            }
            case 'o': {
                fprintf(state->debug_print_instrs, "[table %p]", vm_value_to_table(vm_int_run_read_load()));
                break;
            }
            case 'd': {
                vm_value_t dyn = vm_int_run_read_load();
                switch (vm_typeof(dyn)) {
                case VM_TYPE_NIL:
                    fprintf(state->debug_print_instrs, "[any nil]");
                    break;
                case VM_TYPE_BOOL:
                    fprintf(state->debug_print_instrs, "[any bool %s]", vm_value_to_bool(dyn) ? "true" : "false");
                    break;
                case VM_TYPE_I32:
                    fprintf(state->debug_print_instrs, "[any int %zi]", (ptrdiff_t) vm_value_to_int(dyn));
                    break;
                case VM_TYPE_F64:
                    fprintf(state->debug_print_instrs, "[any float %lf]", vm_value_to_float(dyn));
                    break;
                case VM_TYPE_FUNC:
                    fprintf(state->debug_print_instrs, "[any func %p]", vm_value_to_block(dyn));
                    break;
                case VM_TYPE_ARRAY:
                    fprintf(state->debug_print_instrs, "[any array %p]", vm_value_to_array(dyn));
                    break;
                case VM_TYPE_TABLE:
                    fprintf(state->debug_print_instrs, "[any table %p]", vm_value_to_array(dyn));
                    break;
                }
                break;
            }
            case 'c': {
                fprintf(state->debug_print_instrs, "[closure %p]", vm_value_to_array(vm_int_run_read_load()));
                break;
            }
            case 'i': {
                fprintf(state->debug_print_instrs, "[int %zi]", (ptrdiff_t) vm_value_to_int(vm_int_run_read_load()));
                break;
            }
            case 'f': {
                fprintf(state->debug_print_instrs, "[float %lf]", vm_value_to_float(vm_int_run_read_load()));
                break;
            }
            case 't': {
                fprintf(state->debug_print_instrs, "[func %p]", vm_value_to_block(vm_int_run_read_load()));
                break;
            }
            case 'I': {
                fprintf(state->debug_print_instrs, "[const int %zi]", (ptrdiff_t) vm_int_run_read().ival);
                break;
            }
            case 'L': {
                fprintf(state->debug_print_instrs, "[const func %zu]", (size_t) vm_int_run_read().block->id);
                break;
            }
            case 'T': {
                fprintf(state->debug_print_instrs, "[const block %p]", vm_int_run_read().ptr);
                break;
            }
            case 'F': {
                fprintf(state->debug_print_instrs, "[const float %lf]", vm_int_run_read().fval);
                break;
            }
            default: {
                fprintf(state->debug_print_instrs, "\n<%c>\n", *fmt);
                // break;
                __builtin_trap();
            }
            }
            fmt += 1;
        }
        fprintf(state->debug_print_instrs, "\n");
    }
    head = head0;
#if VM_DEBUG_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), name);
    }
#endif
    vm_int_run_next();
}
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
do_mov_i : {
    vm_value_t *out = vm_int_run_read_store();
    vm_int_t value = vm_int_run_read().ival;
    *out = vm_value_from_int(value);
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
do_fmov_r : {
    vm_value_t *out = vm_int_run_read_store();
    *out = vm_value_from_float((vm_number_t)vm_value_to_int(*out));
    vm_int_run_next();
}

// int ops
do_i32add_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(vm_value_to_int(lhs) + vm_value_to_int(rhs));
    vm_int_run_next();
}
do_i32add_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_int_t rhs = vm_int_run_read().ival;
    *out = vm_value_from_int(vm_value_to_int(lhs) + rhs);
    vm_int_run_next();
}
do_i32sub_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(vm_value_to_int(lhs) - vm_value_to_int(rhs));
    vm_int_run_next();
}
do_i32sub_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_int_t rhs = vm_int_run_read().ival;
    *out = vm_value_from_int(vm_value_to_int(lhs) - rhs);
    vm_int_run_next();
}
do_i32sub_ir : {
    vm_value_t *out = vm_int_run_read_store();
    vm_int_t lhs = vm_int_run_read().ival;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(lhs - vm_value_to_int(rhs));
    vm_int_run_next();
}
do_i32mul_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(vm_value_to_int(lhs) * vm_value_to_int(rhs));
    vm_int_run_next();
}
do_i32mul_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_int_t rhs = vm_int_run_read().ival;
    *out = vm_value_from_int(vm_value_to_int(lhs) * rhs);
    vm_int_run_next();
}
do_i32div_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_int_t lhs = vm_value_to_int(vm_int_run_read_load());
    vm_int_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs % rhs == 0) {
        *out = vm_value_from_int(lhs / rhs);
        if (head[1].ptr == NULL) {
            head = head[1].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
        } else {
            head = head[1].ptr;
        }
    } else {
        *out = vm_value_from_float((double)lhs / (double)rhs);
        if (head[2].ptr == NULL) {
            head = head[2].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
        } else {
            head = head[2].ptr;
        }
    }
    vm_int_run_next();
}
do_i32div_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_int_t lhs = vm_value_to_int(vm_int_run_read_load());
    vm_int_t rhs = vm_int_run_read().ival;
    if (lhs % rhs == 0) {
        *out = vm_value_from_int(lhs / rhs);
        if (head[1].ptr == NULL) {
            head = head[1].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
        } else {
            head = head[1].ptr;
        }
    } else {
        *out = vm_value_from_float((double)lhs / (double)rhs);
        if (head[2].ptr == NULL) {
            head = head[2].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
        } else {
            head = head[2].ptr;
        }
    }
    vm_int_run_next();
}
do_i32div_ir : {
    vm_value_t *out = vm_int_run_read_store();
    vm_int_t lhs = vm_int_run_read().ival;
    vm_int_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs % rhs == 0) {
        *out = vm_value_from_int(lhs / rhs);
        if (head[1].ptr == NULL) {
            head = head[1].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
        } else {
            head = head[1].ptr;
        }
    } else {
        *out = vm_value_from_float((double)lhs / (double)rhs);
        if (head[2].ptr == NULL) {
            head = head[2].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
        } else {
            head = head[2].ptr;
        }
    }
    vm_int_run_next();
}
do_i32mod_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(vm_value_to_int(lhs) % vm_value_to_int(rhs));
    vm_int_run_next();
}
do_i32mod_ri : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_int_t rhs = vm_int_run_read().ival;
    *out = vm_value_from_int(vm_value_to_int(lhs) % rhs);
    vm_int_run_next();
}
do_i32mod_ir : {
    vm_value_t *out = vm_int_run_read_store();
    vm_int_t lhs = vm_int_run_read().ival;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_int(lhs % vm_value_to_int(rhs));
    vm_int_run_next();
}
do_i32blt_rrll : {
    vm_int_t lhs = vm_value_to_int(vm_int_run_read_load());
    vm_int_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_i32blt_rill : {
    vm_int_t lhs = vm_value_to_int(vm_int_run_read_load());
    vm_int_t rhs = vm_int_run_read().ival;
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_i32blt_irll : {
    vm_int_t lhs = vm_int_run_read().ival;
    vm_int_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_i32beq_rrll : {
    vm_int_t lhs = vm_value_to_int(vm_int_run_read_load());
    vm_int_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_i32beq_rill : {
    vm_int_t lhs = vm_value_to_int(vm_int_run_read_load());
    vm_int_t rhs = vm_int_run_read().ival;
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_i32beq_irll : {
    vm_int_t lhs = vm_int_run_read().ival;
    vm_int_t rhs = vm_value_to_int(vm_int_run_read_load());
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_i32blt_rrtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_i32blt_rrll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_i32blt_ritt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_i32blt_rill;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_i32blt_irtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_i32blt_irll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_i32beq_rrtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_i32beq_rrll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_i32beq_ritt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_i32beq_rill;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_i32beq_irtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_i32beq_irll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}

// float ops
do_fadd_rr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(vm_value_to_float(lhs) + vm_value_to_float(rhs));
    vm_int_run_next();
}
do_fadd_rf : {
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
do_fsub_rf : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_number_t rhs = vm_int_run_read().fval;
    *out = vm_value_from_float(vm_value_to_float(lhs) - rhs);
    vm_int_run_next();
}
do_fsub_fr : {
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
do_fmul_rf : {
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
do_fdiv_rf : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_number_t rhs = vm_int_run_read().fval;
    *out = vm_value_from_float(vm_value_to_float(lhs) / rhs);
    vm_int_run_next();
}
do_fdiv_fr : {
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
do_fmod_rf : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t lhs = vm_int_run_read_load();
    vm_number_t rhs = vm_int_run_read().fval;
    *out = vm_value_from_float(fmod(vm_value_to_float(lhs), rhs));
    vm_int_run_next();
}
do_fmod_fr : {
    vm_value_t *out = vm_int_run_read_store();
    vm_number_t lhs = vm_int_run_read().fval;
    vm_value_t rhs = vm_int_run_read_load();
    *out = vm_value_from_float(fmod(lhs, vm_value_to_float(rhs)));
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
do_fblt_rfll : {
    vm_number_t lhs = vm_value_to_float(vm_int_run_read_load());
    vm_number_t rhs = vm_int_run_read().fval;
    if (lhs < rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_fblt_frll : {
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
do_fbeq_rfll : {
    vm_number_t lhs = vm_value_to_float(vm_int_run_read_load());
    vm_number_t rhs = vm_int_run_read().fval;
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
do_fbeq_frll : {
    vm_number_t lhs = vm_int_run_read().fval;
    vm_number_t rhs = vm_value_to_float(vm_int_run_read_load());
    if (lhs == rhs) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
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
do_fblt_rftt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fblt_rfll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_fblt_frtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fblt_frll;
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
do_fbeq_rftt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fbeq_rfll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
do_fbeq_frtt : {
    vm_int_opcode_t *loc = --head;
    vm_int_run_read().ptr = &&do_fbeq_frll;
    head += 2;
    vm_int_opcode_t *block1 = &vm_int_run_read();
    vm_int_opcode_t *block2 = &vm_int_run_read();
    block1->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block1->block);
    block2->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block2->block);
    head = loc;
    vm_int_run_next();
}
// other
do_bb_rll : {
    vm_value_t value = vm_int_run_read_load();
    if (vm_value_to_bool(value)) {
        head = head[1].ptr;
    } else {
        head = head[0].ptr;
    }
    vm_int_run_next();
}
// calls
do_call_l0 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    void *ptr = vm_int_run_read().ptr;
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l1 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l2 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    void *ptr = vm_int_run_read().ptr;
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals[framesize + 1 + 1] = vm_int_run_read_load();
    *heads++ = head;
    locals += framesize;
    head = ptr;
    vm_int_run_next();
}
do_call_l3 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r1 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    vm_ir_block_t *func = vm_value_to_block(vm_int_run_read_load());
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    void *ptr = vm_int_block_comp(vm_int_run_save(), ptrs, func);
    head = ptr;
    vm_int_run_next();
}
do_call_r2 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 0, NULL);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x1 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[1] = {vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 1, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x2 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    vm_int_func_t ptr = state->funcs[vm_int_run_read().ival];
    vm_value_t values[2] = {vm_int_run_read_load(), vm_int_run_read_load()};
    vm_value_t *out = vm_int_run_read_store();
    locals += framesize;
    *out = ptr.func(ptr.data, vm_int_run_save(), 2, &values[0]);
    locals -= framesize;
    vm_int_run_next();
}
do_call_x3 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "gc");
    }
#endif
    vm_gc_run(&state->gc, locals + state->framesize);
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    *out = vm_gc_arr(&state->gc, (vm_int_t)len);
    vm_int_run_next();
}
do_arr_r : {
    vm_value_t *out = vm_int_run_read_store();
    vm_value_t len = vm_int_run_read_load();
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "gc");
    }
#endif
    vm_gc_run(&state->gc, locals + state->framesize);
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    *out = vm_gc_arr(&state->gc, (vm_int_t)vm_value_to_float(len));
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
        case VM_TYPE_F64:
            head = head[VM_TYPE_F64].ptr;
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
        case VM_TYPE_F64:
            head = head[VM_TYPE_F64].ptr;
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
    *out = vm_value_from_int(vm_gc_len(obj));
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
// ret
do_ret_i : {
    vm_value_t value = vm_value_from_float(vm_int_run_read().fval);
    head = *--heads;
    locals -= framesize;
    vm_int_opcode_t out = vm_int_run_read();
    locals[out.reg] = value;
    void *pblock = head[VM_TYPE_F64].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_F64].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    vm_int_run_next();
}
do_ret_ri : {
    vm_value_t value = locals[head->reg];
    locals -= framesize;
    head = *--heads;
    locals[vm_int_run_read().reg] = value;
    void *pblock = head[VM_TYPE_I32].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_I32].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    vm_int_run_next();
}
do_ret_rif : {
    vm_value_t value = locals[head->reg];
    locals -= framesize;
    head = *--heads;
    locals[vm_int_run_read().reg] = value;
    void *pblock = head[VM_TYPE_F64].ptr;
    if (pblock == NULL) {
        head = head[VM_TYPE_F64].ptr = vm_int_block_comp(vm_int_run_save(), ptrs, head[0].block);
    } else {
        head = pblock;
    }
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    vm_int_run_next();
}
do_exit : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
    return vm_value_nil();
}
// jmp/call tmp
do_call_t0 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    head[-1].ptr = &&do_call_l0;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t1 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
    head[-1].ptr = &&do_call_l1;
    vm_int_opcode_t *block = &vm_int_run_read();
    locals[framesize + 1 + 0] = vm_int_run_read_load();
    locals += framesize;
    *heads++ = head;
    head = block->ptr = vm_int_block_comp(vm_int_run_save(), ptrs, block->block);
    vm_int_run_next();
}
do_call_t2 : {
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "call");
    }
#endif
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
do_tab : {
    vm_value_t *out = vm_int_run_read_store();
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceBegin(&state->spall_ctx, NULL, vm_trace_time(), "gc");
    }
#endif
    vm_gc_run(&state->gc, locals + state->framesize);
#if VM_USE_SPALL
    if (state->use_spall) {
        SpallTraceEnd(&state->spall_ctx, NULL, vm_trace_time());
    }
#endif
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
        case VM_TYPE_I32:
            head = head[VM_TYPE_I32].ptr;
            vm_int_run_next();
        case VM_TYPE_F64:
            head = head[VM_TYPE_F64].ptr;
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
        case VM_TYPE_I32:
            head = head[VM_TYPE_I32].ptr;
            vm_int_run_next();
        case VM_TYPE_F64:
            head = head[VM_TYPE_F64].ptr;
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
