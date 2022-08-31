
#include "int3.h"
#include "../build.h"
#include "../../int/gc.h"
#include <stddef.h>
#include <stdio.h>

#define VM_IR_BE_INT3_FRAME_SIZE 256

enum
{
    VM_IR_BE_INT3_OP_ARGS1,
    VM_IR_BE_INT3_OP_ARGS2,
    VM_IR_BE_INT3_OP_ARGS3,
    VM_IR_BE_INT3_OP_ARGS4,
    VM_IR_BE_INT3_OP_ARGS5,
    VM_IR_BE_INT3_OP_ARGS6,
    VM_IR_BE_INT3_OP_ARGS7,
    VM_IR_BE_INT3_OP_ARGS8,

    VM_IR_BE_INT3_OP_MOV_I,
    VM_IR_BE_INT3_OP_MOV_R,

    VM_IR_BE_INT3_OP_ADD_RR,
    VM_IR_BE_INT3_OP_ADD_RI,
    VM_IR_BE_INT3_OP_ADD_IR,
    VM_IR_BE_INT3_OP_SUB_RR,
    VM_IR_BE_INT3_OP_SUB_RI,
    VM_IR_BE_INT3_OP_SUB_IR,
    VM_IR_BE_INT3_OP_MUL_RR,
    VM_IR_BE_INT3_OP_MUL_RI,
    VM_IR_BE_INT3_OP_MUL_IR,
    VM_IR_BE_INT3_OP_DIV_RR,
    VM_IR_BE_INT3_OP_DIV_RI,
    VM_IR_BE_INT3_OP_DIV_IR,
    VM_IR_BE_INT3_OP_MOD_RR,
    VM_IR_BE_INT3_OP_MOD_RI,
    VM_IR_BE_INT3_OP_MOD_IR,

    VM_IR_BE_INT3_OP_CALL_L0,
    VM_IR_BE_INT3_OP_CALL_L1,
    VM_IR_BE_INT3_OP_CALL_L2,
    VM_IR_BE_INT3_OP_CALL_L3,
    VM_IR_BE_INT3_OP_CALL_L4,
    VM_IR_BE_INT3_OP_CALL_L5,
    VM_IR_BE_INT3_OP_CALL_L6,
    VM_IR_BE_INT3_OP_CALL_L7,
    VM_IR_BE_INT3_OP_CALL_L8,
    VM_IR_BE_INT3_OP_CALL_R0,
    VM_IR_BE_INT3_OP_CALL_R1,
    VM_IR_BE_INT3_OP_CALL_R2,
    VM_IR_BE_INT3_OP_CALL_R3,
    VM_IR_BE_INT3_OP_CALL_R4,
    VM_IR_BE_INT3_OP_CALL_R5,
    VM_IR_BE_INT3_OP_CALL_R6,
    VM_IR_BE_INT3_OP_CALL_R7,
    VM_IR_BE_INT3_OP_CALL_R8,

    VM_IR_BE_INT3_OP_NEW_I,
    VM_IR_BE_INT3_OP_NEW_R,
    VM_IR_BE_INT3_OP_SET_RRR,
    VM_IR_BE_INT3_OP_SET_RRI,
    VM_IR_BE_INT3_OP_SET_RIR,
    VM_IR_BE_INT3_OP_SET_RII,
    VM_IR_BE_INT3_OP_GET_RR,
    VM_IR_BE_INT3_OP_GET_RI,
    VM_IR_BE_INT3_OP_LEN_R,

    VM_IR_BE_INT3_OP_OUT_I,
    VM_IR_BE_INT3_OP_OUT_R,

    VM_IR_BE_INT3_OP_JUMP_L,
    VM_IR_BE_INT3_OP_BB_RLL,
    VM_IR_BE_INT3_OP_BLT_RRLL,
    VM_IR_BE_INT3_OP_BLT_RILL,
    VM_IR_BE_INT3_OP_BLT_IRLL,
    VM_IR_BE_INT3_OP_BEQ_RRLL,
    VM_IR_BE_INT3_OP_BEQ_RILL,
    VM_IR_BE_INT3_OP_BEQ_IRLL,

    VM_IR_BE_INT3_OP_RET_I,
    VM_IR_BE_INT3_OP_RET_R,
    VM_IR_BE_INT3_OP_EXIT,

    VM_IR_BE_INT3_OP_CALL_T0,
    VM_IR_BE_INT3_OP_CALL_T1,
    VM_IR_BE_INT3_OP_CALL_T2,
    VM_IR_BE_INT3_OP_CALL_T3,
    VM_IR_BE_INT3_OP_CALL_T4,
    VM_IR_BE_INT3_OP_CALL_T5,
    VM_IR_BE_INT3_OP_CALL_T6,
    VM_IR_BE_INT3_OP_CALL_T7,
    VM_IR_BE_INT3_OP_CALL_T8,

    VM_IR_BE_INT3_OP_JUMP_T,
    VM_IR_BE_INT3_OP_BB_RTT,
    VM_IR_BE_INT3_OP_BLT_RRTT,
    VM_IR_BE_INT3_OP_BLT_RITT,
    VM_IR_BE_INT3_OP_BLT_IRTT,
    VM_IR_BE_INT3_OP_BEQ_RRTT,
    VM_IR_BE_INT3_OP_BEQ_RITT,
    VM_IR_BE_INT3_OP_BEQ_IRTT,

    VM_IR_BE_INT3_MAX_OP,
};

struct vm_ir_be_int3_state_t;
typedef struct vm_ir_be_int3_state_t vm_ir_be_int3_state_t;

struct vm_ir_be_int3_buf_t;
typedef struct vm_ir_be_int3_buf_t vm_ir_be_int3_buf_t;

struct vm_ir_be_int3_opcode_t;
typedef struct vm_ir_be_int3_opcode_t vm_ir_be_int3_opcode_t;

struct vm_ir_be_int3_state_t {
    size_t nblocks;
    vm_ir_block_t *blocks;
    vm_ir_be_int3_opcode_t **heads;
    vm_value_t **locals;
    vm_gc_t gc;
    void **ptrs;
};

struct vm_ir_be_int3_buf_t {
    size_t len;
    size_t alloc;
    vm_ir_be_int3_opcode_t *ops;
};

struct vm_ir_be_int3_opcode_t {
    union {
        void *ptr;
        vm_ir_block_t *block;
        size_t reg;
        ptrdiff_t val;
    };
};

#define vm_ir_be_int3_block_comp_put_ptr(arg_) buf.ops[buf.len++].ptr = state->ptrs[(arg_)]
#define vm_ir_be_int3_block_comp_put_out(out_) buf.ops[buf.len++].reg = (out_)
#define vm_ir_be_int3_block_comp_put_reg(reg_) buf.ops[buf.len++].reg = (reg_)
#define vm_ir_be_int3_block_comp_put_val(val_) buf.ops[buf.len++].val = (val_)
#define vm_ir_be_int3_block_comp_put_block(block_) buf.ops[buf.len++].block = (block_)
#define vm_ir_be_int3_block_comp_put_arg(arg_) ({ \
        vm_ir_arg_t arg = (arg_); \
        if (arg.type == VM_IR_ARG_REG) { \
            vm_ir_be_int3_block_comp_put_reg(arg.reg); \
        } else if (arg.type == VM_IR_ARG_NUM) { \
            vm_ir_be_int3_block_comp_put_val(arg.num); \
        } else if (arg.type == VM_IR_ARG_FUNC) { \
            vm_ir_be_int3_block_comp_put_block(arg.func); \
        } else { \
            fprintf(stderr, "unknown arg type\n"); \
            __builtin_trap(); \
        } \
    })

vm_ir_be_int3_buf_t vm_ir_be_int3_block_comp(vm_ir_be_int3_state_t *state, vm_ir_block_t *block) {
    vm_ir_be_int3_buf_t buf;
    buf.len = 0;
    buf.alloc = 16;
    buf.ops = vm_alloc0(sizeof(vm_ir_be_int3_opcode_t) * buf.alloc);
    if (block->nargs != 0 && block->isfunc) {
        vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_ARGS1 + (block->nargs - 1));
        for (size_t i = 0; i < block->nargs; i++) {
            vm_ir_be_int3_block_comp_put_reg(block->args[i]);
        }
    }
    for (size_t arg = 0; arg < block->len; arg++) {
        if (buf.len + 8 >= buf.alloc) {
            buf.alloc = buf.len * 2 + 8;
            buf.ops = vm_realloc(buf.ops, sizeof(vm_ir_be_int3_opcode_t) * buf.alloc);
        }
        vm_ir_instr_t *instr = block->instrs[arg];
        switch (instr->op) {
            case VM_IR_IOP_NOP: {
                break;
            }
            case VM_IR_IOP_MOVE: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    switch (instr->args[0].type) 
                    {
                    case VM_IR_ARG_REG:
                    {
                        // r = move r
                        vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOV_R);
                        vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                        vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                        break;
                    }
                    case VM_IR_ARG_NUM:
                    {
                        // r = move i
                        vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOV_I);
                        vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                        vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                        break;
                    }
                    case VM_IR_ARG_STR:
                    {
                        fprintf(stderr, "NO STRINGS YET\n");
                        __builtin_trap();
                    }
                    case VM_IR_ARG_FUNC:
                    {
                        // r = move i
                        break;
                    }
                    }
                }
                break;
            }
            case VM_IR_IOP_ADD: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = add r r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_ADD_RR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = add r i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_ADD_RI);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = add i r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_ADD_IR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = move i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOV_I);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_val(instr->args[0].num + instr->args[0].num);
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_SUB: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = sub r r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_SUB_RR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = sub r i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_SUB_RI);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = sub i r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_SUB_IR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = move i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOV_I);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_val(instr->args[0].num + instr->args[0].num);
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_MUL: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = mul r r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MUL_RR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = mul r i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MUL_RI);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = mul i r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MUL_IR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = move i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOV_I);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_val(instr->args[0].num + instr->args[0].num);
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_DIV: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = div r r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_DIV_RR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = div r i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_DIV_RI);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = div i r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_DIV_IR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = move i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOV_I);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_val(instr->args[0].num + instr->args[0].num);
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_MOD: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = mod r r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOD_RR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = mod r i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOD_RI);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = mod i r
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOD_IR);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                            vm_ir_be_int3_block_comp_put_arg(instr->args[1]);
                        }
                        else
                        {
                            // r = move i
                            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_MOV_I);
                            vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                            vm_ir_be_int3_block_comp_put_val(instr->args[0].num + instr->args[0].num);
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_CALL: {
                size_t nargs = 0;
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++)
                {
                    nargs += 1;
                }
                if (instr->args[0].type == VM_IR_ARG_FUNC)
                {
                    vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_CALL_T0 + nargs);
                    vm_ir_be_int3_block_comp_put_block(instr->args[0].func);
                }
                else
                {
                    vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_CALL_R0 + nargs);
                    vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                }
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE; i++)
                {
                    if (instr->args[i].type == VM_IR_ARG_REG)
                    {
                        vm_ir_be_int3_block_comp_put_arg(instr->args[i]);
                    }
                    else
                    {
                        fprintf(stderr, "internal error: cannot handle call with int arg\n");
                        __builtin_trap();
                    }
                }
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    vm_ir_be_int3_block_comp_put_out(instr->out.reg);
                }
                else
                {
                    vm_ir_be_int3_block_comp_put_out(block->nregs + 1);
                }
                break;
            }
            case VM_IR_IOP_ARR: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        // r = new r
                    }
                    else
                    {
                        // r = new i
                    }
                }
                break;
            }
            case VM_IR_IOP_GET: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = get r r
                        }
                        else
                        {
                            // r = get r i
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_SET: {
                if (instr->args[0].type == VM_IR_ARG_REG)
                {
                    if (instr->args[1].type == VM_IR_ARG_REG)
                    {
                        if (instr->args[2].type == VM_IR_ARG_REG)
                        {
                            // set r r r
                        }
                        else
                        {
                            // set r r i
                        }
                    }
                    else
                    {
                        if (instr->args[2].type == VM_IR_ARG_REG)
                        {
                            // set r i r
                        }
                        else
                        {
                            // set r i i
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_LEN: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        // r = len r
                    }
                }
                break;
            }
            case VM_IR_IOP_TYPE: {
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        // r = type r
                    }
                    else
                    {
                        // r = move i
                    }
                }
                break;
            }
            case VM_IR_IOP_OUT: {
                if (instr->args[0].type == VM_IR_ARG_NUM)
                {
                    // out i
                    vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_OUT_I);
                    vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                }
                else
                {
                    // out r
                    vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_OUT_R);
                    vm_ir_be_int3_block_comp_put_arg(instr->args[0]);
                }
                break;
            }
            }   
        }
        switch(block->branch->op)
        {
        case VM_IR_BOP_JUMP:
        {
            // jump l
            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_JUMP_T);
            vm_ir_be_int3_block_comp_put_block(block->branch->targets[0]);
            break;
        }
        case VM_IR_BOP_BOOL:
        {
            if (block->branch->args[0].type == VM_IR_ARG_NUM)
            {
                // jump l
            }
            else
            {
                vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_BB_RTT);
                vm_ir_be_int3_block_comp_put_arg(block->branch->args[0]);
                vm_ir_be_int3_block_comp_put_block(block->branch->targets[0]);
                vm_ir_be_int3_block_comp_put_block(block->branch->targets[1]);
                // bb r l l
            }
            break;
        }
        case VM_IR_BOP_LESS:
        {
            if (block->branch->args[0].type == VM_IR_ARG_NUM)
            {
                if (block->branch->args[1].type == VM_IR_ARG_NUM)
                {
                    // jump l
                }
                else
                {
                    // blt i r l l
                    vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_BLT_IRTT);
                    vm_ir_be_int3_block_comp_put_arg(block->branch->args[0]);
                    vm_ir_be_int3_block_comp_put_arg(block->branch->args[1]);
                    vm_ir_be_int3_block_comp_put_block(block->branch->targets[0]);
                    vm_ir_be_int3_block_comp_put_block(block->branch->targets[1]);
                }
            }
            else
            {
                if (block->branch->args[1].type == VM_IR_ARG_NUM)
                {
                    // blt r i l l
                    vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_BLT_RITT);
                    vm_ir_be_int3_block_comp_put_arg(block->branch->args[0]);
                    vm_ir_be_int3_block_comp_put_arg(block->branch->args[1]);
                    vm_ir_be_int3_block_comp_put_block(block->branch->targets[0]);
                    vm_ir_be_int3_block_comp_put_block(block->branch->targets[1]);
                }
                else
                {
                    // blt r r l l
                    vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_BLT_RRTT);
                    vm_ir_be_int3_block_comp_put_arg(block->branch->args[0]);
                    vm_ir_be_int3_block_comp_put_arg(block->branch->args[1]);
                    vm_ir_be_int3_block_comp_put_block(block->branch->targets[0]);
                    vm_ir_be_int3_block_comp_put_block(block->branch->targets[1]);
                }
            }
            break;
        }
        case VM_IR_BOP_EQUAL:
        {
            if (block->branch->args[0].type == VM_IR_ARG_NUM)
            {
                if (block->branch->args[1].type == VM_IR_ARG_NUM)
                {
                    // jump l
                }
                else
                {
                    // beq i r l l
                }
            }
            else
            {
                if (block->branch->args[1].type == VM_IR_ARG_NUM)
                {
                    // beq r i l l
                }
                else
                {
                    // beq r r l l
                }
            }
            break;
        }
        case VM_IR_BOP_RET:
        {
            vm_value_t val;
            if (block->branch->args[0].type == VM_IR_ARG_NUM)
            {
                // ret i
                vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_RET_I);
                vm_ir_be_int3_block_comp_put_arg(block->branch->args[0]);
            }
            else
            {
                // ret r
                vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_RET_R);
                vm_ir_be_int3_block_comp_put_arg(block->branch->args[0]);
            }
            break;
        }
        case VM_IR_BOP_EXIT:
        {
            // exit
            vm_ir_be_int3_block_comp_put_ptr(VM_IR_BE_INT3_OP_EXIT);
            break;
        }
    }
#if 0
    fprintf(stderr, "%s", "----------------\n");
    fprintf(stderr, ".%zu(", block->id);
    for (size_t i = 0; i < block->nargs; i++)
    {
        if (i != 0)
        {
            fprintf(stderr, ", ");
        }
        fprintf(stderr, "r%zu", block->args[i]);
    }
    fprintf(stderr, "):\n");
    vm_ir_print_block(stderr, block);
    fprintf(stderr, "%s", "----------------\n");
#endif
    {
    // fprintf(stderr, "\n.len = %zu {\n", buf.len);
    // for (size_t i = 0; i < buf.len; i++) {
    //     fprintf(stderr, "  %zX: %zX\n", i, buf.ops[i].val);
    // }
    // fprintf(stderr, "}\n");
    }
    return buf;
}

#define vm_ir_be_int3_run_read() (*head++)
#if 0
#define vm_ir_be_int3_debug(v) ({typeof(v) v_ = v; fprintf(stderr, "DEBUG: %zX\n", (size_t) v_); v_;})
#else
#define vm_ir_be_int3_debug(v) (v)
#endif
#define vm_ir_be_int3_run_next() goto *vm_ir_be_int3_debug(vm_ir_be_int3_run_read().ptr)
#define vm_ir_be_int3_not_implemented() ({fprintf(stderr, "UNIMPLEMENTED %zu\n", (size_t) __LINE__); return;})

void vm_ir_be_int3_run(vm_ir_be_int3_state_t *state, vm_ir_block_t *block)
{
    void *ptrs[VM_IR_BE_INT3_MAX_OP] = {
        [VM_IR_BE_INT3_OP_ARGS1] = &&do_args1,
        [VM_IR_BE_INT3_OP_ARGS2] = &&do_args2,
        [VM_IR_BE_INT3_OP_ARGS3] = &&do_args3,
        [VM_IR_BE_INT3_OP_ARGS4] = &&do_args4,
        [VM_IR_BE_INT3_OP_ARGS5] = &&do_args5,
        [VM_IR_BE_INT3_OP_ARGS6] = &&do_args6,
        [VM_IR_BE_INT3_OP_ARGS7] = &&do_args7,
        [VM_IR_BE_INT3_OP_ARGS8] = &&do_args8,

        [VM_IR_BE_INT3_OP_MOV_I] = &&do_mov_i,
        [VM_IR_BE_INT3_OP_MOV_R] = &&do_mov_r,
        [VM_IR_BE_INT3_OP_ADD_RR] = &&do_add_rr,
        [VM_IR_BE_INT3_OP_ADD_RI] = &&do_add_ri,
        [VM_IR_BE_INT3_OP_ADD_IR] = &&do_add_ir,
        [VM_IR_BE_INT3_OP_SUB_RR] = &&do_sub_rr,
        [VM_IR_BE_INT3_OP_SUB_RI] = &&do_sub_ri,
        [VM_IR_BE_INT3_OP_SUB_IR] = &&do_sub_ir,
        [VM_IR_BE_INT3_OP_MUL_RR] = &&do_mul_rr,
        [VM_IR_BE_INT3_OP_MUL_RI] = &&do_mul_ri,
        [VM_IR_BE_INT3_OP_MUL_IR] = &&do_mul_ir,
        [VM_IR_BE_INT3_OP_DIV_RR] = &&do_div_rr,
        [VM_IR_BE_INT3_OP_DIV_RI] = &&do_div_ri,
        [VM_IR_BE_INT3_OP_DIV_IR] = &&do_div_ir,
        [VM_IR_BE_INT3_OP_MOD_RR] = &&do_mod_rr,
        [VM_IR_BE_INT3_OP_MOD_RI] = &&do_mod_ri,
        [VM_IR_BE_INT3_OP_MOD_IR] = &&do_mod_ir,
        [VM_IR_BE_INT3_OP_CALL_L0] = &&do_call_l0,
        [VM_IR_BE_INT3_OP_CALL_L1] = &&do_call_l1,
        [VM_IR_BE_INT3_OP_CALL_L2] = &&do_call_l2,
        [VM_IR_BE_INT3_OP_CALL_L3] = &&do_call_l3,
        [VM_IR_BE_INT3_OP_CALL_L4] = &&do_call_l4,
        [VM_IR_BE_INT3_OP_CALL_L5] = &&do_call_l5,
        [VM_IR_BE_INT3_OP_CALL_L6] = &&do_call_l6,
        [VM_IR_BE_INT3_OP_CALL_L7] = &&do_call_l7,
        [VM_IR_BE_INT3_OP_CALL_L8] = &&do_call_l8,
        [VM_IR_BE_INT3_OP_CALL_R0] = &&do_call_r0,
        [VM_IR_BE_INT3_OP_CALL_R1] = &&do_call_r1,
        [VM_IR_BE_INT3_OP_CALL_R2] = &&do_call_r2,
        [VM_IR_BE_INT3_OP_CALL_R3] = &&do_call_r3,
        [VM_IR_BE_INT3_OP_CALL_R4] = &&do_call_r4,
        [VM_IR_BE_INT3_OP_CALL_R5] = &&do_call_r5,
        [VM_IR_BE_INT3_OP_CALL_R6] = &&do_call_r6,
        [VM_IR_BE_INT3_OP_CALL_R7] = &&do_call_r7,
        [VM_IR_BE_INT3_OP_CALL_R8] = &&do_call_r8,
        [VM_IR_BE_INT3_OP_NEW_I] = &&do_new_i,
        [VM_IR_BE_INT3_OP_NEW_R] = &&do_new_r,
        [VM_IR_BE_INT3_OP_SET_RRR] = &&do_set_rrr,
        [VM_IR_BE_INT3_OP_SET_RRI] = &&do_set_rri,
        [VM_IR_BE_INT3_OP_SET_RIR] = &&do_set_rir,
        [VM_IR_BE_INT3_OP_SET_RII] = &&do_set_rii,
        [VM_IR_BE_INT3_OP_GET_RR] = &&do_get_rr,
        [VM_IR_BE_INT3_OP_GET_RI] = &&do_get_ri,
        [VM_IR_BE_INT3_OP_LEN_R] = &&do_len_r,
        [VM_IR_BE_INT3_OP_OUT_I] = &&do_out_i,
        [VM_IR_BE_INT3_OP_OUT_R] = &&do_out_r,
        [VM_IR_BE_INT3_OP_JUMP_L] = &&do_jump_l,
        [VM_IR_BE_INT3_OP_BB_RLL] = &&do_bb_rll,
        [VM_IR_BE_INT3_OP_BLT_RRLL] = &&do_blt_rrll,
        [VM_IR_BE_INT3_OP_BLT_RILL] = &&do_blt_rill,
        [VM_IR_BE_INT3_OP_BLT_IRLL] = &&do_blt_irll,
        [VM_IR_BE_INT3_OP_BEQ_RRLL] = &&do_beq_rrll,
        [VM_IR_BE_INT3_OP_BEQ_RILL] = &&do_beq_rill,
        [VM_IR_BE_INT3_OP_BEQ_IRLL] = &&do_beq_irll,
        [VM_IR_BE_INT3_OP_RET_I] = &&do_ret_i,
        [VM_IR_BE_INT3_OP_RET_R] = &&do_ret_r,
        [VM_IR_BE_INT3_OP_CALL_T0] = &&do_call_t0,
        [VM_IR_BE_INT3_OP_CALL_T1] = &&do_call_t1,
        [VM_IR_BE_INT3_OP_CALL_T2] = &&do_call_t2,
        [VM_IR_BE_INT3_OP_CALL_T3] = &&do_call_t3,
        [VM_IR_BE_INT3_OP_CALL_T4] = &&do_call_t4,
        [VM_IR_BE_INT3_OP_CALL_T5] = &&do_call_t5,
        [VM_IR_BE_INT3_OP_CALL_T6] = &&do_call_t6,
        [VM_IR_BE_INT3_OP_CALL_T7] = &&do_call_t7,
        [VM_IR_BE_INT3_OP_CALL_T8] = &&do_call_t8,
        [VM_IR_BE_INT3_OP_JUMP_T] = &&do_jump_t,
        [VM_IR_BE_INT3_OP_BB_RTT] = &&do_bb_rtt,
        [VM_IR_BE_INT3_OP_BLT_RRTT] = &&do_blt_rrtt,
        [VM_IR_BE_INT3_OP_BLT_RITT] = &&do_blt_ritt,
        [VM_IR_BE_INT3_OP_BLT_IRTT] = &&do_blt_irtt,
        [VM_IR_BE_INT3_OP_BEQ_RRTT] = &&do_beq_rrtt,
        [VM_IR_BE_INT3_OP_BEQ_RITT] = &&do_beq_ritt,
        [VM_IR_BE_INT3_OP_BEQ_IRTT] = &&do_beq_irtt,
        [VM_IR_BE_INT3_OP_EXIT] = &&do_exit,
    };
    state->ptrs = &ptrs[0];
    vm_ir_be_int3_opcode_t *head = vm_ir_be_int3_block_comp(state, block).ops;
    vm_value_t *locals = state->gc.regs;
    vm_value_t args[8];
    vm_ir_be_int3_run_next();
    do_args1: {
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        vm_ir_be_int3_run_next();
    }
    do_args2: {
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        locals[vm_ir_be_int3_run_read().reg] = args[1];
        vm_ir_be_int3_run_next();
    }
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        locals[vm_ir_be_int3_run_read().reg] = args[1];
        locals[vm_ir_be_int3_run_read().reg] = args[2];
    do_args3: {
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        locals[vm_ir_be_int3_run_read().reg] = args[1];
        locals[vm_ir_be_int3_run_read().reg] = args[2];
        vm_ir_be_int3_run_next();
    }
    do_args4: {
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        locals[vm_ir_be_int3_run_read().reg] = args[1];
        locals[vm_ir_be_int3_run_read().reg] = args[2];
        locals[vm_ir_be_int3_run_read().reg] = args[3];
        vm_ir_be_int3_run_next();
    }
    do_args5: {
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        locals[vm_ir_be_int3_run_read().reg] = args[1];
        locals[vm_ir_be_int3_run_read().reg] = args[2];
        locals[vm_ir_be_int3_run_read().reg] = args[3];
        locals[vm_ir_be_int3_run_read().reg] = args[4];
        vm_ir_be_int3_run_next();
    }
    do_args6: {
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        locals[vm_ir_be_int3_run_read().reg] = args[1];
        locals[vm_ir_be_int3_run_read().reg] = args[2];
        locals[vm_ir_be_int3_run_read().reg] = args[3];
        locals[vm_ir_be_int3_run_read().reg] = args[4];
        locals[vm_ir_be_int3_run_read().reg] = args[5];
        vm_ir_be_int3_run_next();
    }
    do_args7: {
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        locals[vm_ir_be_int3_run_read().reg] = args[1];
        locals[vm_ir_be_int3_run_read().reg] = args[2];
        locals[vm_ir_be_int3_run_read().reg] = args[3];
        locals[vm_ir_be_int3_run_read().reg] = args[4];
        locals[vm_ir_be_int3_run_read().reg] = args[5];
        locals[vm_ir_be_int3_run_read().reg] = args[6];
        vm_ir_be_int3_run_next();
    }
    do_args8: {
        locals[vm_ir_be_int3_run_read().reg] = args[0];
        locals[vm_ir_be_int3_run_read().reg] = args[1];
        locals[vm_ir_be_int3_run_read().reg] = args[2];
        locals[vm_ir_be_int3_run_read().reg] = args[3];
        locals[vm_ir_be_int3_run_read().reg] = args[4];
        locals[vm_ir_be_int3_run_read().reg] = args[5];
        locals[vm_ir_be_int3_run_read().reg] = args[6];
        locals[vm_ir_be_int3_run_read().reg] = args[7];
        vm_ir_be_int3_run_next();
    }
    do_mov_i: {
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        vm_ir_be_int3_opcode_t value = vm_ir_be_int3_run_read();
        locals[out.reg] = vm_value_from_int(state->gc, value.val);
        vm_ir_be_int3_run_next();
    }
    do_mov_r: {
        vm_ir_be_int3_not_implemented();
    }
    do_add_rr: {
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        vm_value_t lhs = locals[vm_ir_be_int3_run_read().reg];
        vm_value_t rhs = locals[vm_ir_be_int3_run_read().reg];
        locals[out.reg] = vm_value_add(gc, lhs, rhs);
        vm_ir_be_int3_run_next();
    }
    do_add_ri: {
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        vm_value_t lhs = locals[vm_ir_be_int3_run_read().reg];
        ptrdiff_t rhs = vm_ir_be_int3_run_read().val;
        locals[out.reg] = vm_value_addi(gc, lhs, rhs);
        vm_ir_be_int3_run_next();
    }
    do_add_ir: {
        vm_ir_be_int3_not_implemented();
    }
    do_sub_rr: {
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        vm_value_t lhs = locals[vm_ir_be_int3_run_read().reg];
        vm_value_t rhs = locals[vm_ir_be_int3_run_read().reg];
        locals[out.reg] = vm_value_sub(gc, lhs, rhs);
        vm_ir_be_int3_run_next();
    }
    do_sub_ri: {
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        vm_value_t lhs = locals[vm_ir_be_int3_run_read().reg];
        ptrdiff_t rhs = vm_ir_be_int3_run_read().val;
        locals[out.reg] = vm_value_subi(gc, lhs, rhs);
        vm_ir_be_int3_run_next();
    }
    do_sub_ir: {
        vm_ir_be_int3_not_implemented();
    }
    do_mul_rr: {
        vm_ir_be_int3_not_implemented();
    }
    do_mul_ri: {
        vm_ir_be_int3_not_implemented();
    }
    do_mul_ir: {
        vm_ir_be_int3_not_implemented();
    }
    do_div_rr: {
        vm_ir_be_int3_not_implemented();
    }
    do_div_ri: {
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        vm_value_t lhs = locals[vm_ir_be_int3_run_read().reg];
        ptrdiff_t rhs = vm_ir_be_int3_run_read().val;
        locals[out.reg] = vm_value_divi(gc, lhs, rhs);
        vm_ir_be_int3_run_next();
    }
    do_div_ir: {
        vm_ir_be_int3_not_implemented();
    }
    do_mod_rr: {
        vm_ir_be_int3_not_implemented();
    }
    do_mod_ri: {
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        vm_value_t lhs = locals[vm_ir_be_int3_run_read().reg];
        ptrdiff_t rhs = vm_ir_be_int3_run_read().val;
        locals[out.reg] = vm_value_modi(gc, lhs, rhs);
        vm_ir_be_int3_run_next();
    }
    do_mod_ir: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_l0: {
        vm_ir_be_int3_opcode_t block = vm_ir_be_int3_run_read();
        *state->heads++ = head;
        locals += VM_IR_BE_INT3_FRAME_SIZE;
        head = block.ptr;
        vm_ir_be_int3_run_next();
    }
    do_call_l1: {
        vm_ir_be_int3_opcode_t block = vm_ir_be_int3_run_read();
        args[0] = locals[vm_ir_be_int3_run_read().reg];
        // fprintf(stderr, "%zi\n", args[0].ival);
        *state->heads++ = head;
        locals += VM_IR_BE_INT3_FRAME_SIZE;
        // fprintf(stderr, "ARG(1): %zi\n", vm_value_to_int(state.gc, a0));
        head = block.ptr;
        vm_ir_be_int3_run_next();
    }
    do_call_l2: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_l3: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_l4: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_l5: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_l6: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_l7: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_l8: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r0: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r1: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r2: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r3: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r4: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r5: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r6: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r7: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_r8: {
        vm_ir_be_int3_not_implemented();
    }
    do_new_i: {
        vm_ir_be_int3_not_implemented();
    }
    do_new_r: {
        vm_ir_be_int3_not_implemented();
    }
    do_set_rrr: {
        vm_ir_be_int3_not_implemented();
    }
    do_set_rri: {
        vm_ir_be_int3_not_implemented();
    }
    do_set_rir: {
        vm_ir_be_int3_not_implemented();
    }
    do_set_rii: {
        vm_ir_be_int3_not_implemented();
    }
    do_get_rr: {
        vm_ir_be_int3_not_implemented();
    }
    do_get_ri: {
        vm_ir_be_int3_not_implemented();
    }
    do_len_r: {
        vm_ir_be_int3_not_implemented();
    }
    do_out_i: {
        fprintf(stdout, "%c", (int) vm_ir_be_int3_run_read().val);
        vm_ir_be_int3_run_next();
    }
    do_out_r: {
        fprintf(stdout, "%c", (int) vm_value_to_int(state.gc, locals[vm_ir_be_int3_run_read().reg]));
        vm_ir_be_int3_run_next();
    }
    do_jump_l: {
        head = head->ptr;
        vm_ir_be_int3_run_next();
    }
    do_bb_rll: {
        ptrdiff_t val = vm_value_to_int(state.gc, locals[vm_ir_be_int3_run_read().reg]);
        head = (head + (size_t) (val != 0))->ptr;
        vm_ir_be_int3_run_next();
    }
    do_blt_rrll: {
        vm_ir_be_int3_not_implemented();
    }
    do_blt_rill: {
        ptrdiff_t lhs = vm_value_to_int(state.gc, locals[vm_ir_be_int3_run_read().reg]);
        ptrdiff_t rhs = vm_ir_be_int3_run_read().val;
        // fprintf(stderr, "test %zi < %zi\n", lhs, rhs);
        head = (head + (size_t) (lhs < rhs))->ptr;
        vm_ir_be_int3_run_next();
    }
    do_blt_irll: {
        vm_ir_be_int3_not_implemented();
    }
    do_beq_rrll: {
        vm_ir_be_int3_not_implemented();
    }
    do_beq_rill: {
        vm_ir_be_int3_not_implemented();
    }
    do_beq_irll: {
        vm_ir_be_int3_not_implemented();
    }
    do_ret_i: {
        vm_value_t value = vm_value_from_int(gc, vm_ir_be_int3_run_read().val);
        head = *--state->heads;
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        locals -= VM_IR_BE_INT3_FRAME_SIZE;
        locals[out.reg] = value;
        vm_ir_be_int3_run_next();
    }
    do_ret_r: {
        vm_value_t value = locals[vm_ir_be_int3_run_read().reg];
        head = *--state->heads;
        vm_ir_be_int3_opcode_t out = vm_ir_be_int3_run_read();
        locals -= VM_IR_BE_INT3_FRAME_SIZE;
        locals[out.reg] = value;
        // fprintf(stderr, "ret %li / 2\n", value.ival);
        vm_ir_be_int3_run_next();
    }
    do_exit: {
        return;
    }
    do_call_t0: {
        void *loc = --head;
        vm_ir_be_int3_run_read().ptr = &&do_call_l0;
        vm_ir_be_int3_opcode_t *block = &vm_ir_be_int3_run_read();
        block->ptr = vm_ir_be_int3_block_comp(state, block->block).ops;
        head = loc;
        vm_ir_be_int3_run_next();
    }
    do_call_t1: {
        void *loc = --head;
        vm_ir_be_int3_run_read().ptr = &&do_call_l1;
        vm_ir_be_int3_opcode_t *block = &vm_ir_be_int3_run_read();
        block->ptr = vm_ir_be_int3_block_comp(state, block->block).ops;
        head = loc;
        vm_ir_be_int3_run_next();
    }
    do_call_t2: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_t3: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_t4: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_t5: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_t6: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_t7: {
        vm_ir_be_int3_not_implemented();
    }
    do_call_t8: {
        vm_ir_be_int3_not_implemented();
    }
    do_jump_t: {
        void *loc = --head;
        vm_ir_be_int3_run_read().ptr = &&do_jump_l;
        vm_ir_be_int3_opcode_t *block = &vm_ir_be_int3_run_read();
        block->ptr = vm_ir_be_int3_block_comp(state, block->block).ops;
        head = loc;
        vm_ir_be_int3_run_next();
    }
    do_bb_rtt: {
        void *loc = --head;
        vm_ir_be_int3_run_read().ptr = &&do_bb_rll;
        head += 1;
        vm_ir_be_int3_opcode_t *block1 = &vm_ir_be_int3_run_read();
        vm_ir_be_int3_opcode_t *block2 = &vm_ir_be_int3_run_read();
        block1->ptr = vm_ir_be_int3_block_comp(state, block1->block).ops;
        block2->ptr = vm_ir_be_int3_block_comp(state, block2->block).ops;
        head = loc;
        vm_ir_be_int3_run_next();
    }
    do_blt_rrtt: {
        vm_ir_be_int3_not_implemented();
    }
    do_blt_ritt: {
        void *loc = --head;
        vm_ir_be_int3_run_read().ptr = &&do_blt_rill;
        head += 2;
        vm_ir_be_int3_opcode_t *block1 = &vm_ir_be_int3_run_read();
        vm_ir_be_int3_opcode_t *block2 = &vm_ir_be_int3_run_read();
        block1->ptr = vm_ir_be_int3_block_comp(state, block1->block).ops;
        block2->ptr = vm_ir_be_int3_block_comp(state, block2->block).ops;
        head = loc;
        vm_ir_be_int3_run_next();
    }
    do_blt_irtt: {
        vm_ir_be_int3_not_implemented();
    }
    do_beq_rrtt: {
        vm_ir_be_int3_not_implemented();
    }
    do_beq_ritt: {
        vm_ir_be_int3_not_implemented();
    }
    do_beq_irtt: {
        vm_ir_be_int3_not_implemented();
    }
}


void vm_ir_be_int3(size_t nblocks, vm_ir_block_t *blocks)
{
    vm_ir_block_t *cur = &blocks[0];
    vm_ir_be_int3_state_t state;
    vm_gc_init(&state.gc);
    state.gc.nregs = (1 << 16);
    vm_value_t *locals = vm_malloc(sizeof(vm_value_t) * state.gc.nregs);
    state.gc.regs = locals;
    state.nblocks = nblocks;
    state.blocks = blocks; 
    state.locals = vm_malloc(sizeof(vm_value_t *) * (state.gc.nregs / VM_IR_BE_INT3_FRAME_SIZE + 1));
    state.heads = vm_malloc(sizeof(vm_ir_be_int3_opcode_t *) * (state.gc.nregs / VM_IR_BE_INT3_FRAME_SIZE + 1));
    vm_ir_be_int3_run(&state, cur);
    vm_gc_stop(state.gc);
}
