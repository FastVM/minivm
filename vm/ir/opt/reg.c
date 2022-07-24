
#include "../opt.h"
#include "../build.h"

void vm_ir_opt_reg_ssa(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
{
    size_t nops = *ptr_nops;
    vm_ir_block_t *blocks = *ptr_blocks;
    size_t *regs = NULL;
    for (size_t index = 0; index < nops; index++)
    {
    redo:;
        vm_ir_block_t *func = &blocks[index];
        if (func->id != index)
        {
            continue;
        }
        if (func->isfunc)
        {
            while (index < nops)
            {
                vm_ir_block_t *block = &blocks[index];
                if (block->id == index)
                {
                    size_t nregs = block->nregs;
                    regs = vm_realloc(regs, sizeof(size_t) * (block->nregs + block->len));
                    for (size_t i = 0; i < block->nregs + block->len; i++)
                    {
                        regs[i] = SIZE_MAX;
                    }
                    for (size_t arg = 0; arg < block->nregs; arg++)
                    {
                        regs[arg] = arg;
                    }
                    for (size_t ip = 0; ip < block->len; ip++)
                    {
                        vm_ir_instr_t *instr = block->instrs[ip];
                        for (size_t argno = 0; instr->args[argno] != NULL; argno++)
                        {
                            vm_ir_arg_t *arg = instr->args[argno];
                            if (arg->type == VM_IR_ARG_REG)
                            {
                                instr->args[argno] = vm_ir_arg_reg(regs[arg->reg]);
                            }
                        }
                        if (instr->out != NULL && instr->out->type == VM_IR_ARG_REG)
                        {
                            size_t reg = nregs++;
                            regs[instr->out->reg] = reg;
                            instr->out = vm_ir_arg_reg(reg);
                        }
                    }
                    for (size_t argno = 0; argno < 2; argno++)
                    {
                        vm_ir_arg_t *arg = block->branch->args[argno];
                        if (arg != NULL && arg->type == VM_IR_ARG_REG)
                        {
                            block->branch->args[argno] = vm_ir_arg_reg(regs[arg->reg]);
                        }
                    }
                    for (size_t n = 0; n < 2; n++)
                    {
                        vm_ir_block_t *target = block->branch->targets[n];
                        if (target == NULL)
                        {
                            break;
                        }
                        for (size_t argno = 0; argno < target->nargs; argno++)
                        {
                            size_t arg = target->args[argno];
                            if (arg != regs[arg] && regs[arg] != SIZE_MAX)
                            {
                                vm_ir_block_add_move(block, vm_ir_arg_reg(arg), vm_ir_arg_reg(regs[arg]));
                                regs[arg] = SIZE_MAX;
                            }
                        }
                    }
                    if (nregs > func->nregs)
                    {
                        func->nregs = nregs;
                    }
                    block->nregs = nregs;
                }
                index += 1;
                if (index < nops && blocks[index].isfunc)
                {
                    goto redo;
                }
            }
        }
    }
}

void vm_ir_opt_reg_print(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
{
    size_t nops = *ptr_nops;
    vm_ir_block_t *blocks = *ptr_blocks;
    printf("%%entry%% {\n");
    for (size_t i = 0; i < nops; i++)
    {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i)
        {
            continue;
        }
        if (block->isfunc)
        {
            printf("}\nfunc .%zu(", i);
            for (size_t i = 0; i < block->nargs; i++)
            {
                if (i != 0)
                {
                    printf(", ");
                }
                printf("r%zu", block->args[i]);
            }
            printf(") {\n");
        }
        else
        {
            printf("  .%zu(", i);
            for (size_t i = 0; i < block->nargs; i++)
            {
                if (i != 0)
                {
                    printf(", ");
                }
                printf("r%zu", block->args[i]);
            }
            printf("):\n");
        }
        vm_ir_print_block(stderr, block);
    }
    printf("}\n");
}

void vm_ir_opt_reg_alloc(size_t *ptr_nops, vm_ir_block_t **ptr_blocks)
{
    vm_ir_opt_reg_print(ptr_nops, ptr_blocks);
    size_t nops = *ptr_nops;
    vm_ir_block_t *blocks = *ptr_blocks;
    size_t *regs = NULL;
    for (size_t index = 0; index < nops; index++)
    {
    redo:;
        vm_ir_block_t *func = &blocks[index];
        if (func->id != index)
        {
            continue;
        }
        if (func->isfunc)
        {
            regs = realloc(regs, sizeof(size_t) * func->nregs);
            for (size_t reg = 0; reg < func->nargs; reg++)
            {
                regs[reg] = reg;
            }
            for (size_t reg = func->nargs; reg < func->nregs; reg++)
            {
                regs[reg] = SIZE_MAX;
            }
            size_t nregs = func->nargs;
            fprintf(stderr, "FUNC: %zu\n", index);
            while (index < nops)
            {
                vm_ir_block_t *block = &blocks[index];
                if (block->id == index)
                {
                    for (size_t ip = 0; ip < block->len; ip++)
                    {
                        vm_ir_instr_t *instr = block->instrs[ip];
                        for (size_t argno = 0; instr->args[argno] != NULL; argno++)
                        {
                            vm_ir_arg_t *arg = instr->args[argno];
                            if (arg->type == VM_IR_ARG_REG)
                            {
                                arg = vm_ir_arg_reg(regs[arg->reg]);
                                // fprintf(stderr, "REG: %zu\n", arg->reg);
                            }
                        }
                        if (instr->out != NULL && instr->out->type == VM_IR_ARG_REG)
                        {
                        }
                    }
                }
                index += 1;
                if (blocks[index].isfunc)
                {
                    goto redo;
                }
            }
        }
    }
    vm_ir_opt_reg_print(ptr_nops, ptr_blocks);
}

