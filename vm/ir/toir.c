
#include "toir.h"

#include "build.h"

void vm_ir_read_from(vm_ir_read_t *state, size_t index) { vm_ir_read(state, &index); }

void vm_ir_read(vm_ir_read_t *state, size_t *index) {
    vm_ir_block_t *blocks = state->blocks;
    size_t nops = state->nops;
    const vm_opcode_t *ops = state->ops;
    vm_ir_block_t *block = &blocks[*index];
    if (block->id == *index) {
        return;
    }
    block->nregs = state->nregs;
    block->id = *index;
    vm_opcode_t op = ops[(*index)++];
    switch (op) {
        case VM_OPCODE_EXIT: {
            vm_ir_block_end_exit(block);
            return;
        }
        case VM_OPCODE_REG: {
            size_t out = ops[(*index)++];
            size_t in = ops[(*index)++];
            vm_ir_block_add_move(block, vm_ir_arg_reg(out), vm_ir_arg_reg(in));
            break;
        }
        case VM_OPCODE_JUMP: {
            size_t loc = ops[(*index)++];
            vm_ir_read_from(state, loc);
            vm_ir_block_end_jump(block, &blocks[loc]);
            return;
        }
        case VM_OPCODE_FUNC: {
            vm_opcode_t over = ops[(*index)++];
            vm_opcode_t nargs = ops[(*index)++];
            vm_opcode_t nregs = ops[(*index)++];
            size_t tmp1 = state->nops;
            size_t tmp2 = state->nregs;
            state->nops = over;
            state->nregs = nregs;
            vm_ir_read_from(state, *index);
            state->nops = tmp1;
            state->nregs = tmp2;
            *index = over;
            break;
        }
        case VM_OPCODE_CALL: {
            vm_opcode_t rreg = ops[(*index)++];
            vm_opcode_t func = ops[(*index)++];
            blocks[func].isfunc = true;
            vm_opcode_t nargs = ops[(*index)++];
            vm_ir_arg_t *args = vm_malloc(sizeof(vm_ir_arg_t) * nargs);
            for (size_t i = 0; i < nargs; i++) {
                args[i] = vm_ir_arg_reg(ops[(*index)++]);
            }
            size_t tmp = state->nops;
            state->nops = ops[func - 3];
            vm_ir_read_from(state, func);
            state->nops = tmp;
            vm_ir_block_add_call(block, vm_ir_arg_reg(rreg), vm_ir_arg_func(&blocks[func]), nargs, args);
            vm_free(args);
            break;
        }
        case VM_OPCODE_DCALL: {
            vm_opcode_t rreg = ops[(*index)++];
            vm_opcode_t func = ops[(*index)++];
            vm_opcode_t nargs = ops[(*index)++];
            vm_ir_arg_t *args = vm_malloc(sizeof(vm_ir_arg_t) * nargs);
            for (size_t i = 0; i < nargs; i++) {
                args[i] = vm_ir_arg_reg(ops[(*index)++]);
            }
            vm_ir_block_add_call(block, vm_ir_arg_reg(rreg), vm_ir_arg_reg(func), nargs, args);
            vm_free(args);
            break;
        }
        case VM_OPCODE_XCALL: {
            vm_opcode_t rreg = ops[(*index)++];
            vm_opcode_t func = ops[(*index)++];
            vm_opcode_t nargs = ops[(*index)++];
            vm_ir_arg_t *args = vm_malloc(sizeof(vm_ir_arg_t) * nargs);
            for (size_t i = 0; i < nargs; i++) {
                args[i] = vm_ir_arg_reg(ops[(*index)++]);
            }
            vm_ir_block_add_call(block, vm_ir_arg_reg(rreg), vm_ir_arg_extern(func), nargs, args);
            vm_free(args);
            break;
        }
        case VM_OPCODE_ADDR: {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t func = ops[(*index)++];
            blocks[func].isfunc = true;
            size_t tmp = state->nops;
            state->nops = ops[func - 3];
            vm_ir_read_from(state, func);
            state->nops = tmp;
            vm_ir_block_add_move(block, vm_ir_arg_reg(reg), vm_ir_arg_func(&blocks[func]));
            break;
        }
        case VM_OPCODE_RET: {
            vm_opcode_t reg = ops[(*index)++];
            vm_ir_block_end_ret(block, vm_ir_arg_reg(reg));
            return;
        }
        case VM_OPCODE_PUTCHAR: {
            vm_opcode_t reg = ops[(*index)++];
            vm_ir_block_add_out(block, vm_ir_arg_reg(reg));
            break;
        }
        case VM_OPCODE_INT: {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t val = ops[(*index)++];
            vm_ir_block_add_move(block, vm_ir_arg_reg(reg), vm_ir_arg_num(val));
            break;
        }
        case VM_OPCODE_NEG: {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t val = ops[(*index)++];
            vm_ir_block_add_move(block, vm_ir_arg_reg(reg), vm_ir_arg_num(-(ptrdiff_t)val));
            break;
        }
        case VM_OPCODE_ADD: {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_ir_block_add_add(block, vm_ir_arg_reg(out), vm_ir_arg_reg(lhs), vm_ir_arg_reg(rhs));
            break;
        }
        case VM_OPCODE_SUB: {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_ir_block_add_sub(block, vm_ir_arg_reg(out), vm_ir_arg_reg(lhs), vm_ir_arg_reg(rhs));
            break;
        }
        case VM_OPCODE_MUL: {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_ir_block_add_mul(block, vm_ir_arg_reg(out), vm_ir_arg_reg(lhs), vm_ir_arg_reg(rhs));

            break;
        }
        case VM_OPCODE_DIV: {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_ir_block_add_div(block, vm_ir_arg_reg(out), vm_ir_arg_reg(lhs), vm_ir_arg_reg(rhs));
            break;
        }
        case VM_OPCODE_MOD: {
            vm_opcode_t out = ops[(*index)++];
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_ir_block_add_mod(block, vm_ir_arg_reg(out), vm_ir_arg_reg(lhs), vm_ir_arg_reg(rhs));
            break;
        }
        case VM_OPCODE_BB: {
            vm_opcode_t val = ops[(*index)++];
            vm_opcode_t iff = ops[(*index)++];
            vm_opcode_t ift = ops[(*index)++];
            vm_ir_read_from(state, iff);
            vm_ir_read_from(state, ift);
            vm_ir_block_end_bb(block, vm_ir_arg_reg(val), &blocks[iff], &blocks[ift]);
            return;
        }
        case VM_OPCODE_BEQ: {
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_opcode_t iff = ops[(*index)++];
            vm_opcode_t ift = ops[(*index)++];
            vm_ir_read_from(state, iff);
            vm_ir_read_from(state, ift);
            vm_ir_block_end_beq(block, vm_ir_arg_reg(lhs), vm_ir_arg_reg(rhs), &blocks[iff], &blocks[ift]);
            return;
        }
        case VM_OPCODE_BLT: {
            vm_opcode_t lhs = ops[(*index)++];
            vm_opcode_t rhs = ops[(*index)++];
            vm_opcode_t iff = ops[(*index)++];
            vm_opcode_t ift = ops[(*index)++];
            vm_ir_read_from(state, iff);
            vm_ir_read_from(state, ift);
            vm_ir_block_end_blt(block, vm_ir_arg_reg(lhs), vm_ir_arg_reg(rhs), &blocks[iff], &blocks[ift]);
            return;
        }
        case VM_OPCODE_STR: {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t len = ops[(*index)++];
            char *s = vm_malloc(sizeof(char) * (len + 1));
            for (size_t i = 0; i < len; i++) {
                s[i] = (char)ops[(*index)++];
            }
            s[len] = '\0';
            vm_ir_block_add_move(block, vm_ir_arg_reg(reg), vm_ir_arg_str(s));
            break;
        }
        case VM_OPCODE_ARR: {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t len = ops[(*index)++];
            vm_ir_block_add_arr(block, vm_ir_arg_reg(reg), vm_ir_arg_reg(len));
            break;
        }
        case VM_OPCODE_SET: {
            vm_opcode_t obj = ops[(*index)++];
            vm_opcode_t ind = ops[(*index)++];
            vm_opcode_t val = ops[(*index)++];
            vm_ir_block_add_set(block, vm_ir_arg_reg(obj), vm_ir_arg_reg(ind), vm_ir_arg_reg(val));
            break;
        }
        case VM_OPCODE_GET: {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t obj = ops[(*index)++];
            vm_opcode_t ind = ops[(*index)++];
            vm_ir_block_add_get(block, vm_ir_arg_reg(reg), vm_ir_arg_reg(obj), vm_ir_arg_reg(ind));
            break;
        }
        case VM_OPCODE_LEN: {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t obj = ops[(*index)++];
            vm_ir_block_add_len(block, vm_ir_arg_reg(reg), vm_ir_arg_reg(obj));
            break;
        }
        case VM_OPCODE_TYPE: {
            vm_opcode_t reg = ops[(*index)++];
            vm_opcode_t obj = ops[(*index)++];
            vm_ir_block_add_type(block, vm_ir_arg_reg(reg), vm_ir_arg_reg(obj));
            break;
        }
    }
    vm_ir_read_from(state, *index);
    vm_ir_block_end_jump(block, &blocks[*index]);
}

vm_ir_block_t *vm_ir_parse(size_t nops, const vm_opcode_t *ops) {
    size_t index = 0;
    vm_ir_block_t *blocks = vm_malloc(sizeof(vm_ir_block_t) * nops);
    for (size_t i = 0; i < nops; i++) {
        blocks[i] = (vm_ir_block_t){
            .id = -1,
        };
    }
    vm_ir_read_t state;
    state.blocks = blocks;
    state.nregs = 0;
    state.nops = nops;
    state.ops = ops;
    vm_ir_read_from(&state, index);
    vm_ir_info(&nops, &blocks);
    for (size_t i = 0; i < nops; i++) {
        vm_ir_block_t *block = &blocks[i];
        if (block->id != i) {
            continue;
        }
        for (size_t i = 0; i < 2; i++) {
            vm_ir_block_t *next = block->branch->targets[i];
            if (next == NULL) {
                continue;
            }
        }
    }
    return &blocks[0];
}
