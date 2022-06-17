
#include "build.h"

static void vm_ir_block_realloc(vm_ir_block_t *block, vm_ir_instr_t *instr)
{
    if (block->len + 4 >= block->alloc)
    {
        block->alloc = (block->len + 4) * 4;
        block->instrs = vm_realloc(block->instrs, sizeof(vm_ir_arg_t *) * block->alloc);
    }
    block->instrs[block->len++] = instr;
}

vm_ir_arg_t *vm_ir_arg_reg(size_t reg)
{
    return vm_ir_new(vm_ir_arg_t, .type = VM_IR_ARG_REG, .reg = reg);
}

vm_ir_arg_t *vm_ir_arg_num(size_t reg)
{
    return vm_ir_new(vm_ir_arg_t, .type = VM_IR_ARG_NUM, .reg = reg);
}

vm_ir_block_t *vm_ir_block_new(void)
{
    return vm_ir_new(vm_ir_block_t);
}

void vm_ir_block_add_reg(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *arg)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_MOVE, .out = out, .args[0] = arg));
}

void vm_ir_block_add_add(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *lhs, vm_ir_arg_t *rhs)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_ADD, .out = out, .args[0] = lhs, .args[1] = rhs));
}
void vm_ir_block_add_sub(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *lhs, vm_ir_arg_t *rhs)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_SUB, .out = out, .args[0] = lhs, .args[1] = rhs));
}
void vm_ir_block_add_mul(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *lhs, vm_ir_arg_t *rhs)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_MUL, .out = out, .args[0] = lhs, .args[1] = rhs));
}
void vm_ir_block_add_div(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *lhs, vm_ir_arg_t *rhs)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_DIV, .out = out, .args[0] = lhs, .args[1] = rhs));
}
void vm_ir_block_add_mod(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *lhs, vm_ir_arg_t *rhs)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_MOD, .out = out, .args[0] = lhs, .args[1] = rhs));
}
void vm_ir_block_add_addr(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_func_t *func)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_ADDR, .out = out, .args[0] = vm_ir_new(vm_ir_arg_t, .type = VM_IR_ARG_FUNC, .func = func)));
}
void vm_ir_block_add_call(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *func, vm_ir_arg_t **args)
{
    vm_ir_instr_t *instr = vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_CALL, .args[0] = out, .args[1] = func);
    size_t i = 2;
    while (*args != NULL)
    {
        instr->args[i++] = *args++;
    }
    vm_ir_block_realloc(block, instr);
}
void vm_ir_block_add_arr(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *num)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_ARR, .out = out, .args[0] = num));
}
void vm_ir_block_add_get(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *obj, vm_ir_arg_t *index)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_GET, .out = out, .args[0] = obj, .args[1] = index));
}
void vm_ir_block_add_len(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *obj)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_LEN, .out = out, .args[0] = obj));
}
void vm_ir_block_add_type(vm_ir_block_t *block, vm_ir_arg_t *out, vm_ir_arg_t *obj)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_TYPE, .out = out, .args[0] = obj));
}
void vm_ir_block_add_set(vm_ir_block_t *block, vm_ir_arg_t *obj, vm_ir_arg_t *index, vm_ir_arg_t *value)
{
    vm_ir_block_realloc(block, vm_ir_new(vm_ir_instr_t, .op = VM_IR_IOP_SET, .out = NULL, .args[0] = obj, .args[1] = index, .args[2] = value));
}

void vm_ir_block_end_jump(vm_ir_block_t *block, vm_ir_block_t *target)
{
    block->branch = vm_ir_new(vm_ir_branch_t, .op = VM_IR_BOP_GOTO, .targets[0] = target - block->func->blocks);
}
void vm_ir_block_end_bb(vm_ir_block_t *block, vm_ir_arg_t *val, vm_ir_block_t *iffalse, vm_ir_block_t *iftrue)
{
    block->branch = vm_ir_new(vm_ir_branch_t, .op = VM_IR_BOP_BOOL, .args[0] = val, .targets[0] = iffalse - block->func->blocks, .targets[1] = iftrue - block->func->blocks);
}
void vm_ir_block_end_blt(vm_ir_block_t *block, vm_ir_arg_t *lhs, vm_ir_arg_t *rhs, vm_ir_block_t *iffalse, vm_ir_block_t *iftrue)
{
    block->branch = vm_ir_new(vm_ir_branch_t, .op = VM_IR_BOP_LESS, .args[0] = lhs, .args[1] = rhs, .targets[0] = iffalse - block->func->blocks, .targets[1] = iftrue - block->func->blocks);
}
void vm_ir_block_end_beq(vm_ir_block_t *block, vm_ir_arg_t *lhs, vm_ir_arg_t *rhs, vm_ir_block_t *iffalse, vm_ir_block_t *iftrue)
{
    block->branch = vm_ir_new(vm_ir_branch_t, .op = VM_IR_BOP_EQUAL, .args[0] = lhs, .args[1] = rhs, .targets[0] = iffalse - block->func->blocks, .targets[1] = iftrue - block->func->blocks);
}
void vm_ir_block_end_ret(vm_ir_block_t *block, vm_ir_arg_t *arg)
{
    block->branch = vm_ir_new(vm_ir_branch_t, .op = VM_IR_BOP_RET, .args[0] = arg);
}
void vm_ir_block_end_exit(vm_ir_block_t *block, vm_ir_arg_t *arg)
{
    block->branch = vm_ir_new(vm_ir_branch_t, .op = VM_IR_BOP_EXIT, .args[0] = arg);
}

void vm_ir_print_arg(FILE *out, vm_ir_arg_t *val)
{
    switch (val->type)
    {
    case VM_IR_ARG_NUM:
    {
        fprintf(out, "%zu", val->num);
        break;
    }
    case VM_IR_ARG_REG:
    {
        fprintf(out, "r%zu", val->reg);
        break;
    }
    case VM_IR_ARG_FUNC:
    {
        fprintf(out, "<func>");
        break;
    }
    }
}
void vm_ir_print_branch(FILE *out, vm_ir_branch_t *val)
{
    switch (val->op)
    {
    case VM_IR_BOP_GOTO:
    {

        fprintf(out, "goto");
    }
    case VM_IR_BOP_BOOL:
    {
        fprintf(out, "bb");
        break;
    }
    case VM_IR_BOP_LESS:
    {
        fprintf(out, "blt");
        break;
    }
    case VM_IR_BOP_EQUAL:
    {
        fprintf(out, "beq");
        break;
    }
    case VM_IR_BOP_RET:
    {
        fprintf(out, "ret");
        break;
    }
    case VM_IR_BOP_EXIT:
    {
        fprintf(out, "exit");
        break;
    }
    }
    if (val->args[0] != NULL)
    {
        fprintf(out, " ");
        vm_ir_print_arg(out, val->args[0]);
    }
    if (val->args[1] != NULL)
    {
        fprintf(out, " ");
        vm_ir_print_arg(out, val->args[1]);
    }
    if (val->targets[0] >= 0)
    {
        fprintf(out, " {.L%zu}", (size_t) val->targets[0]);
    }
    if (val->targets[1] >= 0)
    {
        fprintf(out, " {.L%zu}", (size_t) val->targets[1]);
    }
}
void vm_ir_print_instr(FILE *out, vm_ir_instr_t *val)
{
    if (out != NULL)
    {
        vm_ir_print_arg(out, val->out);
        fprintf(out, " <- ");
    }
    switch (val->op)
    {
    case VM_IR_IOP_MOVE:
    {
        fprintf(out, "move");
        break;
    }
    case VM_IR_IOP_ADD:
    {
        fprintf(out, "add");
        break;
    }
    case VM_IR_IOP_SUB:
    {
        fprintf(out, "sub");
        break;
    }
    case VM_IR_IOP_MUL:
    {
        fprintf(out, "mul");
        break;
    }
    case VM_IR_IOP_DIV:
    {
        fprintf(out, "div");
        break;
    }
    case VM_IR_IOP_MOD:
    {
        fprintf(out, "mod");
        break;
    }
    case VM_IR_IOP_ADDR:
    {
        fprintf(out, "addr");
        break;
    }
    case VM_IR_IOP_CALL:
    {
        fprintf(out, "call");
        break;
    }
    case VM_IR_IOP_ARR:
    {
        fprintf(out, "arr");
        break;
    }
    case VM_IR_IOP_GET:
    {
        fprintf(out, "get");
        break;
    }
    case VM_IR_IOP_SET:
    {
        fprintf(out, "set");
        break;
    }
    case VM_IR_IOP_LEN:
    {
        fprintf(out, "len");
        break;
    }
    case VM_IR_IOP_TYPE:
    {
        fprintf(out, "type");
        break;
    }
    }
    for (size_t i = 0; val->args[i] != NULL; i++)
    {
        fprintf(out, " ");
        vm_ir_print_arg(out, val->args[i]);
    }
}
void vm_ir_print_block(FILE *out, vm_ir_block_t *val)
{
    for (size_t i = 0; i < val->len; i++)
    {
        fprintf(out, "    ");
        vm_ir_print_instr(out, val->instrs[i]);
        fprintf(out, "\n");
    }
}
void vm_ir_print_func(FILE *out, vm_ir_func_t *val)
{
    for (size_t i = 0; i < val->len; i++)
    {
        fprintf(out, ".L%zu:\n", i);
        vm_ir_print_block(out, &val->blocks[i]);
    }
}
