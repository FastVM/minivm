
#include "int3.h"
#include "../build.h"
#include "../../int/gc.h"

enum
{
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
    VM_IR_BE_INT3_OP_CALL_R0,
    VM_IR_BE_INT3_OP_CALL_R1,
    VM_IR_BE_INT3_OP_CALL_R2,
    VM_IR_BE_INT3_OP_CALL_R3,
    VM_IR_BE_INT3_OP_NEW_I,
    VM_IR_BE_INT3_OP_NEW_R,
    VM_IR_BE_INT3_OP_SET_RRR,
    VM_IR_BE_INT3_OP_SET_RRI,
    VM_IR_BE_INT3_OP_SET_RIR,
    VM_IR_BE_INT3_OP_SET_RII,
    VM_IR_BE_INT3_OP_GET_RR,
    VM_IR_BE_INT3_OP_GET_RI,
    VM_IR_BE_INT3_OP_LEN_R,
    VM_IR_BE_INT3_OP_JUMP_I,
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
    VM_IR_BE_INT3_MAX_OP,
};

struct vm_ir_be_int3_state_t;
typedef struct vm_ir_be_int3_state_t vm_ir_be_int3_state_t;

struct vm_ir_be_int3_state_t {
    size_t nblocks;
    vm_ir_block_t *blocks;
    vm_gc_t gc;
    void *ptrs[VM_IR_BE_INT3_MAX_OP];
};

vm_value_t vm_ir_be_int3_block(vm_ir_be_int3_state_t *state, vm_ir_block_t *block, vm_value_t *locals) {
    vm_gc_t *gc = &state->gc;
    while (true) {
        for (size_t arg = 0; arg < block->len; arg++) {
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
                        locals[instr->out.reg] = locals[instr->args[0].reg];
                        break;
                    }
                    case VM_IR_ARG_NUM:
                    {
                        // r = move i
                        locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num);
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
                        locals[instr->out.reg] = vm_value_from_func(gc, instr->args[0].func->id);
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
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) + vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = add r i
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) + instr->args[1].num);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = add i r
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num + vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = move i
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num +  instr->args[1].num);
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
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) - vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = sub r i
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) - instr->args[1].num);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = sub i r
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num - vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = move i
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num -  instr->args[1].num);
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
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) * vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = mul r i
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) * instr->args[1].num);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = mul i r
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num * vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = move i
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num *  instr->args[1].num);
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
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) / vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = div r i
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) / instr->args[1].num);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = div i r
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num / vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = move i
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num /  instr->args[1].num);
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
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) % vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = mod r r
                            locals[instr->out.reg] = vm_value_from_int(gc, vm_value_to_int(gc, locals[instr->args[0].reg]) % instr->args[1].num);
                        }
                    }
                    else
                    {
                        if (instr->args[1].type == VM_IR_ARG_REG)
                        {
                            // r = mod i r
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num % vm_value_to_int(gc, locals[instr->args[1].reg]));
                        }
                        else
                        {
                            // r = move i
                            locals[instr->out.reg] = vm_value_from_int(gc, instr->args[0].num %  instr->args[1].num);
                        }
                    }
                }
                break;
            }
            case VM_IR_IOP_CALL: {
                vm_ir_block_t *next;
                if (instr->args[0].type == VM_IR_ARG_FUNC)
                {
                    // r = call r{0:8}
                    next = instr->args[0].func;
                }
                else
                {
                    // r = dcall r{0:8}
                    next = &state->blocks[vm_value_to_func(gc, locals[instr->args[0].reg])];
                }
                vm_value_t *regs = locals + block->nregs;
                for (size_t i = 1; instr->args[i].type != VM_IR_ARG_NONE&& i-1 < next->nargs; i++)
                {
                    if (instr->args[i].type == VM_IR_ARG_REG)
                    {
                        regs[next->args[i-1]] = locals[instr->args[i].reg];
                    }
                    else
                    {
                        regs[next->args[i-1]] = vm_value_from_int(gc, instr->args[i].num);
                    }
                }
                vm_value_t res = vm_ir_be_int3_block(state, next, regs);
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    locals[instr->out.reg] = res;
                }
                break;
            }
            case VM_IR_IOP_ARR: {
                vm_gc_run(gc);
                if (instr->out.type == VM_IR_ARG_REG)
                {
                    if (instr->args[0].type == VM_IR_ARG_REG)
                    {
                        // r = new r
                        locals[instr->out.reg] = VM_VALUE_SET_ARR(vm_gc_arr(gc, vm_value_to_int(gc, locals[instr->args[0].reg])));
                    }
                    else
                    {
                        // r = new i
                        locals[instr->out.reg] = VM_VALUE_SET_ARR(vm_gc_arr(gc, instr->args[0].num));
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
                            locals[instr->out.reg] = vm_gc_get_v(gc, locals[instr->args[0].reg], locals[instr->args[1].reg]);
                        }
                        else
                        {
                            // r = get r i
                            locals[instr->out.reg] = vm_gc_get_i(gc, locals[instr->args[0].reg], instr->args[1].num);
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
                            vm_gc_set_vv(gc, locals[instr->args[0].reg], locals[instr->args[1].reg], locals[instr->args[2].reg]);
                        }
                        else
                        {
                            // set r r i
                            vm_gc_set_vi(gc, locals[instr->args[0].reg], locals[instr->args[1].reg], instr->args[2].num);
                        }
                    }
                    else
                    {
                        if (instr->args[2].type == VM_IR_ARG_REG)
                        {
                            // set r i r
                            vm_gc_set_iv(gc, locals[instr->args[0].reg], instr->args[1].num, locals[instr->args[2].reg]);
                        }
                        else
                        {
                            // set r i i
                            vm_gc_set_ii(gc, locals[instr->args[0].reg], instr->args[1].num, instr->args[2].num);
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
                        locals[instr->out.reg] = vm_value_from_int(gc, vm_gc_len(gc, locals[instr->args[0].reg]));
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
                        locals[instr->out.reg] = vm_value_from_int(gc, vm_value_typeof(gc, locals[instr->args[0].reg]));
                    }
                    else
                    {
                        // r = move i
                        locals[instr->out.reg] = vm_value_from_int(gc, vm_value_typeof(gc, vm_value_from_int(gc, instr->args[0].num)));
                    }
                }
                break;
            }
            case VM_IR_IOP_OUT: {
                if (instr->args[0].type == VM_IR_ARG_NUM)
                {
                    // out i
                    putchar((int) instr->args[0].num);
                }
                else
                {
                    // out r
                    putchar((int) vm_value_to_int(gc, locals[instr->args[0].reg]));
                }
                break;
            }
            }   
        }
        uint8_t next;
        switch(block->branch->op)
        {
        case VM_IR_BOP_JUMP:
        {
            // jump l
            next = 0;
            break;
        }
        case VM_IR_BOP_BOOL:
        {
            if (block->branch->args[0].type == VM_IR_ARG_NUM)
            {
                // jump l
                next = block->branch->args[0].num != 0 ? 1 : 0;
            }
            else
            {
                // bb r l l
                next = vm_value_to_int(gc, locals[block->branch->args[0].reg]) != 0 ? 1 : 0;
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
                    next = block->branch->args[0].num < block->branch->args[1].num ? 1 : 0;
                }
                else
                {
                    // blt i r l l
                    next = block->branch->args[0].num < vm_value_to_int(gc, locals[block->branch->args[1].reg]) ? 1 : 0;
                }
            }
            else
            {
                if (block->branch->args[1].type == VM_IR_ARG_NUM)
                {
                    // blt r i l l
                    next = vm_value_to_int(gc, locals[block->branch->args[0].reg]) < block->branch->args[1].num ? 1 : 0;
                }
                else
                {
                    // blt r r l l
                    next = vm_value_to_int(gc, locals[block->branch->args[0].reg]) <vm_value_to_int(gc, locals[block->branch->args[1].reg]) ? 1 : 0;
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
                    next = block->branch->args[0].num == block->branch->args[1].num ? 1 : 0;
                }
                else
                {
                    // beq i r l l
                    next = block->branch->args[0].num == vm_value_to_int(gc, locals[block->branch->args[1].reg]) ? 1 : 0;
                }
            }
            else
            {
                if (block->branch->args[1].type == VM_IR_ARG_NUM)
                {
                    // beq r i l l
                    next = vm_value_to_int(gc, locals[block->branch->args[0].reg]) == block->branch->args[1].num ? 1 : 0;
                }
                else
                {
                    // beq r r l l
                    next = vm_value_to_int(gc, locals[block->branch->args[0].reg]) == vm_value_to_int(gc, locals[block->branch->args[1].reg]) ? 1 : 0;
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
                val = vm_value_from_int(gc, block->branch->args[0].num);
            }
            else
            {
                // ret r
                val = locals[block->branch->args[0].reg];
            }
            return val;
        }
        case VM_IR_BOP_EXIT:
        {
            // exit
            exit(0);
        }
        }
        vm_value_t *args = locals + block->nregs;
        for (size_t i = 0; i < block->branch->targets[next]->nargs; i++)
        {
            vm_ir_arg_t arg = block->branch->pass[next][i];
            if (arg.type == VM_IR_ARG_REG)
            {
                args[i] = locals[arg.reg];
            }
            else
            {
                args[i] = vm_value_from_int(gc, arg.num);
            }
        }
        for (size_t i = 0; i < block->branch->targets[next]->nargs; i++)
        {
            locals[block->branch->targets[next]->args[i]] = args[i];
        }
        block = block->branch->targets[next];
        continue;
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
    vm_ir_be_int3_block(&state, cur, locals);
    vm_gc_stop(state.gc);
}
