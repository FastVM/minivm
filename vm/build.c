
#include "build.h"

void vm_block_realloc(vm_block_t *block, vm_instr_t *instr) {
    if (block->len + 4 >= block->alloc) {
        block->alloc = (block->len + 4) * 4;
        block->instrs = vm_realloc(block->instrs, sizeof(vm_arg_t) * block->alloc);
    }
    block->instrs[block->len++] = instr;
}

vm_arg_t vm_arg_nil(void) { return (vm_arg_t){.type = VM_ARG_NIL}; }
vm_arg_t vm_arg_bool(bool t) { return (vm_arg_t){.type = VM_ARG_BOOL, .logic = t}; }
vm_arg_t vm_arg_reg(size_t reg) { return (vm_arg_t){.type = VM_ARG_REG, .reg = reg}; }

vm_arg_t vm_arg_extern(size_t num) { return (vm_arg_t){.type = VM_ARG_EXTERN, .num = (double)num}; }

vm_arg_t vm_arg_func(vm_block_t *func) { return (vm_arg_t){.type = VM_ARG_FUNC, .func = func}; }

vm_arg_t vm_arg_num(double num) { return (vm_arg_t){.type = VM_ARG_NUM, .num = num}; }

vm_arg_t vm_arg_str(const char *str) { return (vm_arg_t){.type = VM_ARG_STR, .str = str}; }

void vm_block_add_move(vm_block_t *block, vm_arg_t out, vm_arg_t arg) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_MOVE;
    instr->out = out;
    instr->args[0] = arg;
    vm_block_realloc(block, instr);
}

void vm_block_add_add(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_ADD;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_sub(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_SUB;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_mul(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_MUL;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_div(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_DIV;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_mod(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_MOD;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_bor(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_BOR;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_band(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_BAND;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_bxor(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_BXOR;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_bshl(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_BSHL;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_bshr(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_BSHR;
    instr->out = out;
    instr->args[0] = lhs;
    instr->args[1] = rhs;
    vm_block_realloc(block, instr);
}
void vm_block_add_call(vm_block_t *block, vm_arg_t out, vm_arg_t func, size_t nargs, vm_arg_t *args) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_CALL;
    instr->out = out;
    instr->args[0] = func;
    size_t i = 1;
    while (i <= nargs) {
        instr->args[i++] = *args++;
    }
    vm_block_realloc(block, instr);
}
void vm_block_add_arr(vm_block_t *block, vm_arg_t out, vm_arg_t num) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_ARR;
    instr->out = out;
    instr->args[0] = num;
    vm_block_realloc(block, instr);
}
void vm_block_add_tab(vm_block_t *block, vm_arg_t out) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_TAB;
    instr->out = out;
    vm_block_realloc(block, instr);
}
void vm_block_add_get(vm_block_t *block, vm_arg_t out, vm_arg_t obj, vm_arg_t index) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_GET;
    instr->out = out;
    instr->args[0] = obj;
    instr->args[1] = index;
    vm_block_realloc(block, instr);
}
void vm_block_add_len(vm_block_t *block, vm_arg_t out, vm_arg_t obj) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_LEN;
    instr->out = out;
    instr->args[0] = obj;
    vm_block_realloc(block, instr);
}
void vm_block_add_type(vm_block_t *block, vm_arg_t out, vm_arg_t obj) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_TYPE;
    instr->out = out;
    instr->args[0] = obj;
    vm_block_realloc(block, instr);
}
void vm_block_add_set(vm_block_t *block, vm_arg_t obj, vm_arg_t index, vm_arg_t value) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_SET;
    instr->args[0] = obj;
    instr->args[1] = index;
    instr->args[2] = value;
    vm_block_realloc(block, instr);
}
void vm_block_add_out(vm_block_t *block, vm_arg_t arg) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_OUT;
    instr->args[0] = arg;
    vm_block_realloc(block, instr);
}
void vm_block_add_in(vm_block_t *block, vm_arg_t arg) {
    vm_instr_t *instr = vm_alloc0(sizeof(vm_instr_t));
    instr->op = VM_IOP_IN;
    instr->out = arg;
    vm_block_realloc(block, instr);
}

void vm_block_end_jump(vm_block_t *block, vm_block_t *target) {
    block->branch = vm_alloc0(sizeof(vm_branch_t));
    block->branch->op = VM_BOP_JUMP;
    block->branch->targets[0] = target;
}
void vm_block_end_bb(vm_block_t *block, vm_arg_t val, vm_block_t *iffalse, vm_block_t *iftrue) {
    block->branch = vm_alloc0(sizeof(vm_branch_t));
    block->branch->op = VM_BOP_BOOL;
    block->branch->args[0] = val;
    block->branch->targets[0] = iffalse;
    block->branch->targets[1] = iftrue;
}
void vm_block_end_blt(vm_block_t *block, vm_arg_t lhs, vm_arg_t rhs, vm_block_t *iffalse, vm_block_t *iftrue) {
    block->branch = vm_alloc0(sizeof(vm_branch_t));
    block->branch->op = VM_BOP_LESS;
    block->branch->args[0] = lhs;
    block->branch->args[1] = rhs;
    block->branch->targets[0] = iffalse;
    block->branch->targets[1] = iftrue;
}
void vm_block_end_beq(vm_block_t *block, vm_arg_t lhs, vm_arg_t rhs, vm_block_t *iffalse, vm_block_t *iftrue) {
    block->branch = vm_alloc0(sizeof(vm_branch_t));
    block->branch->op = VM_BOP_EQUAL;
    block->branch->args[0] = lhs;
    block->branch->args[1] = rhs;
    block->branch->targets[0] = iffalse;
    block->branch->targets[1] = iftrue;
}
void vm_block_end_ret(vm_block_t *block, vm_arg_t arg) {
    block->branch = vm_alloc0(sizeof(vm_branch_t));
    block->branch->op = VM_BOP_RET;
    block->branch->args[0] = arg;
}
void vm_block_end_exit(vm_block_t *block) {
    block->branch = vm_alloc0(sizeof(vm_branch_t));
    block->branch->op = VM_BOP_EXIT;
}

void vm_instr_free(vm_instr_t *instr) { vm_free(instr); }

void vm_block_free(vm_block_t *block) {
    if (block == NULL) {
        return;
    }
    for (size_t i = 0; i < block->len; i++) {
        vm_instr_free(block->instrs[i]);
    }
    if (block->branch != NULL) {
        vm_free(block->branch);
    }
    vm_free(block->instrs);
    vm_free(block->args);
}

void vm_blocks_free(size_t nblocks, vm_block_t *blocks) {
    for (size_t i = 0; i < nblocks; i++) {
        vm_block_t *block = &blocks[i];
        if (block->id < 0) {
            continue;
        }
        vm_block_free(block);
    }
    vm_free(blocks);
}

void vm_print_arg(FILE *out, vm_arg_t val) {
    switch (val.type) {
        case VM_ARG_NIL: {
            fprintf(out, "nil");
            break;
        }
        case VM_ARG_BOOL: {
            fprintf(out, "%s", val.logic ? "true" : "false");
            break;
        }
        case VM_ARG_NUM: {
            fprintf(out, "%lf", val.num);
            break;
        }
        case VM_ARG_STR: {
            fprintf(out, "\"%s\"", val.str);
            break;
        }
        case VM_ARG_REG: {
            fprintf(out, "r%zu", val.reg);
            break;
        }
        case VM_ARG_FUNC: {
            fprintf(out, ".%zu", val.func->id);
            break;
        }
    }
}
void vm_print_branch(FILE *out, vm_branch_t *val) {
    switch (val->op) {
        case VM_BOP_JUMP: {
            fprintf(out, "jump");
            break;
        }
        case VM_BOP_BOOL: {
            fprintf(out, "bb");
            break;
        }
        case VM_BOP_LESS: {
            fprintf(out, "blt");
            break;
        }
        case VM_BOP_EQUAL: {
            fprintf(out, "beq");
            break;
        }
        case VM_BOP_RET: {
            fprintf(out, "ret");
            break;
        }
        case VM_BOP_EXIT: {
            fprintf(out, "exit");
            break;
        }
    }
    if (val->args[0].type != VM_ARG_NONE) {
        fprintf(out, " ");
        vm_print_arg(out, val->args[0]);
    }
    if (val->args[1].type != VM_ARG_NONE) {
        fprintf(out, " ");
        vm_print_arg(out, val->args[1]);
    }
    if (val->targets[0]) {
        fprintf(out, " .%zu", (size_t)val->targets[0]->id);
        fprintf(out, "(");
        for (size_t i = 0; i < val->targets[0]->nargs; i++) {
            if (i != 0) {
                fprintf(out, ", ");
            }
            fprintf(out, "r%zu", val->targets[0]->args[i]);
        }
        fprintf(out, ")");
    }
    if (val->targets[1]) {
        fprintf(out, " .%zu", (size_t)val->targets[1]->id);
        fprintf(out, "(");
        for (size_t i = 0; i < val->targets[1]->nargs; i++) {
            if (i != 0) {
                fprintf(out, ", ");
            }
            fprintf(out, "r%zu", val->targets[1]->args[i]);
        }
        fprintf(out, ")");
    }
}
void vm_print_instr(FILE *out, vm_instr_t *val) {
    if (val->op == VM_IOP_NOP) {
        fprintf(out, "nop");
        return;
    }
    if (val->out.type != VM_ARG_NONE) {
        vm_print_arg(out, val->out);
        fprintf(out, " <- ");
    }
    switch (val->op) {
        case VM_IOP_MOVE: {
            fprintf(out, "move");
            break;
        }
        case VM_IOP_ADD: {
            fprintf(out, "add");
            break;
        }
        case VM_IOP_SUB: {
            fprintf(out, "sub");
            break;
        }
        case VM_IOP_MUL: {
            fprintf(out, "mul");
            break;
        }
        case VM_IOP_DIV: {
            fprintf(out, "div");
            break;
        }
        case VM_IOP_MOD: {
            fprintf(out, "mod");
            break;
        }
        case VM_IOP_CALL: {
            fprintf(out, "call");
            break;
        }
        case VM_IOP_ARR: {
            fprintf(out, "arr");
            break;
        }
        case VM_IOP_TAB: {
            fprintf(out, "tab");
            break;
        }
        case VM_IOP_GET: {
            fprintf(out, "get");
            break;
        }
        case VM_IOP_SET: {
            fprintf(out, "set");
            break;
        }
        case VM_IOP_LEN: {
            fprintf(out, "len");
            break;
        }
        case VM_IOP_TYPE: {
            fprintf(out, "type");
            break;
        }
        case VM_IOP_OUT: {
            fprintf(out, "out");
            break;
        }
    }
    for (size_t i = 0; val->args[i].type != VM_ARG_NONE; i++) {
        fprintf(out, " ");
        vm_print_arg(out, val->args[i]);
    }
}
void vm_print_block(FILE *out, vm_block_t *val) {
    for (size_t i = 0; i < val->len; i++) {
        if (val->instrs[i]->op == VM_IOP_NOP) {
            continue;
        }
        fprintf(out, "    ");
        vm_print_instr(out, val->instrs[i]);
        fprintf(out, "\n");
    }
    if (val->branch != NULL) {
        fprintf(out, "    ");
        vm_print_branch(out, val->branch);
        fprintf(out, "\n");
    } else {
        fprintf(out, "    <fall>\n");
    }
}

void vm_print_blocks(FILE *out, size_t nblocks, vm_block_t *blocks) {
    for (size_t i = 0; i < nblocks; i++) {
        vm_block_t *block = &blocks[i];
        if (block->id < 0) {
            continue;
        }
        if (block->isfunc) {
            fprintf(out, "\nfunc .%zu(", i);
        } else {
            fprintf(out, ".%zu(", i);
        }
        for (size_t j = 0; j < block->nargs; j++) {
            if (i != 0) {
                fprintf(out, ", ");
            }
            fprintf(out, "r%zu", block->args[j]);
        }
        fprintf(out, ")\n");
        vm_print_block(out, block);
    }
}
