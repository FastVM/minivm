
#include "../opcode.h"
#include "./build.h"

void vm_ir_read(size_t nops, const vm_opcode_t *ops, size_t *index, vm_ir_func_t *func)
{
    while (*index < nops)
    {
        switch (ops[(*index)++])
        {
        case VM_OPCODE_EXIT:
        {
            break;
        }
        case VM_OPCODE_REG:
        {
            size_t in = ops[(*index)++];
            size_t out = ops[(*index)++];
            break;
        }
        case VM_OPCODE_JUMP:
        {
            size_t loc = ops[(*index)++];
            break;
        }
        case VM_OPCODE_FUNC:
        {
            vm_opcode_t over = ops[(*index)++];
            vm_opcode_t nargs = ops[(*index)++];
            vm_opcode_t nregs = ops[(*index)++];
            *index = (size_t) over;
            break;
        }
        case VM_OPCODE_CALL:
        {
            vm_opcode_t rreg = ops[(*index)++];
            vm_opcode_t func = ops[(*index)++];
            vm_opcode_t nargs = ops[(*index)++];
            *index += nargs;
            break;
        }
        case VM_OPCODE_DCALL:
        {
            vm_opcode_t rreg = ops[(*index)++];
            vm_opcode_t func = ops[(*index)++];
            vm_opcode_t nargs = ops[(*index)++];
            *index += nargs;
            break;
        }
        case VM_OPCODE_ADDR:
        {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t func = ops[(*index)++];
            break;
        }
        case VM_OPCODE_RET:
        {
            vm_opcode_t reg = ops[(*index)++];
            break;
        }
        case VM_OPCODE_PUTCHAR:
        {
            vm_opcode_t reg = ops[(*index)++];
            break;
        }
        case VM_OPCODE_INT:
        {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t val = ops[(*index)++];
            break;
        }
        case VM_OPCODE_NEG:
        {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t val = ops[(*index)++];
            break;
        }
        case VM_OPCODE_ADD:
        {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            break;
        }
        case VM_OPCODE_SUB:
        {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            break;
        }
        case VM_OPCODE_MUL:
        {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            
            break;
        }
        case VM_OPCODE_DIV:
        {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            break;
        }
        case VM_OPCODE_MOD:
        {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            break;
        }
        case VM_OPCODE_BB:
        {
            vm_opcode_t val = ops[(*index)++];
            vm_opcode_t ift = ops[(*index)++];
            vm_opcode_t iff = ops[(*index)++];
            break;
        }
        case VM_OPCODE_BEQ:
        {
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_opcode_t ift = ops[(*index)++];
            vm_opcode_t iff = ops[(*index)++];
            break;
        }
        case VM_OPCODE_BLT:
        {
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_opcode_t ift = ops[(*index)++];
            vm_opcode_t iff = ops[(*index)++];
            break;
        }
        case VM_OPCODE_STR:
        {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t len = ops[(*index)++];
            *index += len;
            break;
        }
        case VM_OPCODE_ARR:
        {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t len = ops[(*index)++];
            *index += len;
            break;
        }
        case VM_OPCODE_SET:
        {
            vm_opcode_t obj = ops[(*index)++];
            vm_opcode_t ind = ops[(*index)++];
            vm_opcode_t val = ops[(*index)++];
            break;
        }
        case VM_OPCODE_GET:
        {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t obj = ops[(*index)++];
            vm_opcode_t ind = ops[(*index)++];
            break;
        }
        case VM_OPCODE_LEN:
        {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t obj = ops[(*index)++];
            break;
        }
        case VM_OPCODE_TYPE:
        {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t obj = ops[(*index)++];
            break;
        }
        }
    }
}

void vm_test_toir(size_t nops, const vm_opcode_t *ops)
{
    size_t index = 0;
    vm_ir_func_t *func = vm_ir_new(vm_ir_func_t);
    vm_ir_read(nops, ops, &index, func);
}
