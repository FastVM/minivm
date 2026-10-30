
#include "ir.h"
#include "../value/string.h"

void *mx_ir_alloc(mx_t *vm, size_t size) {
    return mx_arena_alloc(&vm->arenas.ir, size);
}

void *mx_ir_alloc_bytes(mx_t *vm, size_t size) {
    return mx_arena_alloc_bytes(&vm->arenas.ir, size);
}

const char *mx_ir_strdup(mx_t *vm, const char *str) {
    return mx_arena_strdup(&vm->arenas.ir, str);
}

mx_ir_block_t *mx_ir_block_new(mx_t *vm, mx_arena_t *arena, mx_ir_func_t *func) {
    mx_ir_block_t *block = mx_arena_alloc(arena, sizeof(mx_ir_block_t));
    *block = (mx_ir_block_t) {
        .func = func,
        .id = func->nblocks,
    };
    func->nblocks += 1;
    vm->next_block_id += 1;
    return block;
}

mx_ir_func_t *mx_ir_func_new(mx_arena_t *arena, const mx_source_t *source) {
    mx_ir_func_t *func = mx_arena_alloc(arena, sizeof(mx_ir_func_t));
    *func = (mx_ir_func_t) { .source = source };
    return func;
}

size_t mx_ir_uv_size(uint64_t v) {
    size_t n = 1;
    while (v >= 0x80) {
        v >>= 7;
        n += 1;
    }
    return n;
}

uint8_t *mx_ir_put_uv(uint8_t *p, uint64_t v) {
    while (v >= 0x80) {
        p[0] = (uint8_t)(v | 0x80);
        p += 1;
        v >>= 7;
    }
    p[0] = (uint8_t)v;
    return p + 1;
}

uint8_t *mx_ir_put_uv_wide(uint8_t *p, uint64_t v, size_t n) {
    for (size_t i = 0; i + 1 < n; i++) {
        p[i] = (uint8_t)(v | 0x80);
        v >>= 7;
    }
    p[n - 1] = (uint8_t)v;
    return p + n;
}

mx_ir_uv_t mx_ir_get_uv(const uint8_t *p) {
    uint64_t v = 0;
    uint32_t shift = 0;
    while ((p[0] & 0x80) != 0) {
        v |= (uint64_t)(p[0] & 0x7f) << shift;
        shift += 7;
        p += 1;
    }
    return (mx_ir_uv_t) {
        .value = v | ((uint64_t)p[0] << shift),
        .next = p + 1,
    };
}

uint8_t *mx_ir_put_u16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    return p + 2;
}

uint16_t mx_ir_get_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

uint8_t *mx_ir_put_u64(uint8_t *p, uint64_t v) {
    for (uint32_t i = 0; i < 8; i++) {
        p[i] = (uint8_t)(v >> (i * 8));
    }
    return p + 8;
}

uint64_t mx_ir_get_u64(const uint8_t *p) {
    uint64_t v = 0;
    for (uint32_t i = 0; i < 8; i++) {
        v |= (uint64_t)p[i] << (i * 8);
    }
    return v;
}

uint64_t mx_ir_zigzag(int64_t v) {
    return ((uint64_t)v << 1) ^ (uint64_t)(v >> 63);
}

int64_t mx_ir_unzigzag(uint64_t v) {
    return (int64_t)(v >> 1) ^ -(int64_t)(v & 1);
}

uint8_t mx_ir_range_flags(const mx_location_range_t *range) {
    uint8_t flags = 0;
    for (uint32_t i = 0; i < 2; i++) {
        if (range->names[i].kind != MX_VARHINT_NONE) {
            flags |= (uint8_t)(1 << i);
        }
    }
    return flags;
}

size_t mx_ir_range_size(const mx_location_range_t *range, uint8_t flags) {
    size_t size = mx_ir_uv_size(range->start);
    for (uint32_t i = 0; i < 2; i++) {
        if ((flags & (1 << i)) != 0) {
            size += 1 + 8;
        }
    }
    return size;
}

uint8_t *mx_ir_range_write(uint8_t *p, const mx_location_range_t *range, uint8_t flags) {
    uint8_t *at = mx_ir_put_uv(p, range->start);
    for (uint32_t i = 0; i < 2; i++) {
        if ((flags & (1 << i)) != 0) {
            at[0] = range->names[i].kind;
            at = mx_ir_put_u64(at + 1, (uint64_t)(uintptr_t)range->names[i].name);
        }
    }
    return at;
}

mx_ir_range_read_t mx_ir_range_read(const uint8_t *p, uint8_t flags, const mx_source_t *source) {
    mx_ir_uv_t start = mx_ir_get_uv(p);
    mx_ir_range_read_t got = {
        .range = (mx_location_range_t) {
            .source = source,
            .start = (uint32_t)start.value,
        },
        .next = start.next,
    };
    for (uint32_t i = 0; i < 2; i++) {
        if ((flags & (1 << i)) != 0) {
            got.range.names[i] = (mx_varname_t) {
                .name = (const char *)(uintptr_t)mx_ir_get_u64(got.next + 1),
                .kind = got.next[0],
            };
            got.next += 1 + 8;
        }
    }
    return got;
}

size_t mx_ir_arg_size(mx_ir_arg_t arg) {
    switch (arg.type) {
        case MX_IR_ARG_TYPE_REG: {
            return 1 + mx_ir_uv_size(arg.reg);
        }
        case MX_IR_ARG_TYPE_LIT: {
            if (mx_obj_is_int(arg.lit)) {
                return 1 + mx_ir_uv_size(mx_ir_zigzag(mx_obj_get_int(arg.lit)));
            } else {
                return 1 + 2 + 8;
            }
        }
        case MX_IR_ARG_TYPE_SPREAD: {
            return 1;
        }
        default: {
            mx_fatal("ir: bad argument type");
            return 0;
        }
    }
}

uint8_t *mx_ir_arg_write(uint8_t *p, mx_ir_arg_t arg) {
    if (arg.type == MX_IR_ARG_TYPE_LIT && mx_obj_is_int(arg.lit)) {
        p[0] = MX_IR_ENC_INT;
        return mx_ir_put_uv(p + 1, mx_ir_zigzag(mx_obj_get_int(arg.lit)));
    }
    p[0] = arg.type;
    switch (arg.type) {
        case MX_IR_ARG_TYPE_REG: {
            return mx_ir_put_uv(p + 1, arg.reg);
        }
        case MX_IR_ARG_TYPE_LIT: {
            return mx_ir_put_u64(mx_ir_put_u16(p + 1, arg.lit.kind), arg.lit.val.bits);
        }
        default: {
            return p + 1;
        }
    }
}

mx_ir_arg_read_t mx_ir_arg_read(const uint8_t *p) {
    switch (p[0]) {
        case MX_IR_ENC_INT: {
            mx_ir_uv_t got = mx_ir_get_uv(p + 1);
            return (mx_ir_arg_read_t) {
                .next = got.next,
                .arg = (mx_ir_arg_t) {
                    .type = MX_IR_ARG_TYPE_LIT,
                    .lit = mx_obj_of_int(mx_ir_unzigzag(got.value)),
                },
            };
        }
        case MX_IR_ARG_TYPE_REG: {
            mx_ir_uv_t got = mx_ir_get_uv(p + 1);
            return (mx_ir_arg_read_t) {
                .next = got.next,
                .arg = (mx_ir_arg_t) {
                    .type = MX_IR_ARG_TYPE_REG,
                    .reg = got.value,
                },
            };
        }
        case MX_IR_ARG_TYPE_LIT: {
            return (mx_ir_arg_read_t) {
                .next = p + 11,
                .arg = (mx_ir_arg_t) {
                    .type = MX_IR_ARG_TYPE_LIT,
                    .lit = (mx_obj_t) {
                        .kind = mx_ir_get_u16(p + 1),
                        .val = (mx_val_t) {
                            .bits = mx_ir_get_u64(p + 3),
                        },
                    },
                },
            };
        }
        default: {
            return (mx_ir_arg_read_t) {
                .next = p + 1,
                .arg = (mx_ir_arg_t) {
                    .type = p[0],
                },
            };
        }
    }
}

uint32_t mx_ir_args_count(const mx_ir_arg_t *args) {
    uint32_t n = 0;
    while (args != NULL && args[n].type != MX_IR_ARG_TYPE_NONE) {
        n += 1;
    }
    return n;
}

void mx_ir_keep_lits(mx_t *vm, const mx_ir_arg_t *args) {
    for (uint32_t i = 0; args != NULL && args[i].type != MX_IR_ARG_TYPE_NONE; i++) {
        if (args[i].type == MX_IR_ARG_TYPE_LIT && mx_obj_is_string(args[i].lit)) {
            mx_vm_keep(vm, args[i].lit);
        }
    }
}

uint32_t mx_ir_block_emit(mx_ir_block_t *block, mx_ir_instr_t instr) {
    uint32_t nargs = mx_ir_args_count(instr.args);
    if (nargs > MX_IR_INSTR_MAX_ARGS) {
        mx_fatal("ir: instruction argument list too wide");
    }
    uint8_t flags = mx_ir_range_flags(&instr.range);
    bool has_out = instr.out.type == MX_IR_ARG_TYPE_REG;
    bool is_hint = instr.op == MX_IR_INSTR_OPCODE_TABLE_NEW;
    size_t out_size;
    if (has_out) {
        out_size = 2;
    } else {
        out_size = 0;
    }
    size_t size = 2 + mx_ir_range_size(&instr.range, flags) + out_size;
    for (uint32_t i = 0; i < nargs; i++) {
        if (is_hint) {
            size += 1 + MX_IR_TABLE_HINT_BYTES;
        } else {
            size += mx_ir_arg_size(instr.args[i]);
        }
    }
    mx_ir_block_grow(block, block->code_len + size);
    uint32_t at = block->code_len;
    uint8_t *head = &block->code[at];
    head[0] = instr.op;
    head[1] = (uint8_t)(nargs | (flags << 3) | (has_out << 7));
    uint8_t *range_end = mx_ir_range_write(head + 2, &instr.range, flags);
    uint8_t *p;
    if (has_out) {
        p = mx_ir_put_u16(range_end, (uint16_t)instr.out.reg);
    } else {
        p = range_end;
    }
    for (uint32_t i = 0; i < nargs; i++) {
        if (is_hint) {
            p[0] = MX_IR_ENC_INT;
            p = mx_ir_put_uv_wide(p + 1, mx_ir_zigzag(mx_obj_get_int(instr.args[i].lit)), MX_IR_TABLE_HINT_BYTES);
        } else {
            p = mx_ir_arg_write(p, instr.args[i]);
        }
    }
    block->code_len += (uint32_t)size;
    return at;
}

uint8_t *mx_ir_block_operands(mx_ir_block_t *block, uint32_t at) {
    uint8_t *head = &block->code[at];
    return (uint8_t *)mx_ir_range_read(head + 2, (uint8_t)((head[1] >> 3) & 3), NULL).next;
}

void mx_ir_block_patch_hint(mx_ir_block_t *block, uint32_t at, uint64_t hint) {
    uint8_t *operands = mx_ir_block_operands(block, at);
    uint8_t *arg;
    if ((block->code[at + 1] & 0x80) != 0) {
        arg = operands + 2;
    } else {
        arg = operands;
    }
    mx_ir_put_uv_wide(arg + 1, mx_ir_zigzag((int64_t)hint), MX_IR_TABLE_HINT_BYTES);
}

void mx_ir_block_retarget(mx_ir_block_t *block, uint32_t at, uint32_t reg) {
    mx_ir_put_u16(mx_ir_block_operands(block, at), (uint16_t)reg);
}

mx_ir_step_t mx_ir_block_next(const mx_ir_block_t *block, uint32_t limit, mx_ir_cursor_t *cur) {
    if (cur->off >= limit) {
        return (mx_ir_step_t) { .is_valid = false };
    }
    const uint8_t *head = &block->code[cur->off];
    uint8_t meta = head[1];
    mx_ir_range_read_t range = mx_ir_range_read(head + 2, (uint8_t)((meta >> 3) & 3), block->func->source);
    mx_ir_arg_t out;
    const uint8_t *p;
    if ((meta & 0x80) != 0) {
        out = (mx_ir_arg_t) {
            .type = MX_IR_ARG_TYPE_REG,
            .reg = mx_ir_get_u16(range.next),
        };
        p = range.next + 2;
    } else {
        out = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_NONE };
        p = range.next;
    }
    uint32_t nargs = meta & 7;
    for (uint32_t i = 0; i < nargs; i++) {
        mx_ir_arg_read_t got = mx_ir_arg_read(p);
        cur->args[i] = got.arg;
        p = got.next;
    }
    cur->args[nargs] = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_NONE };
    cur->off = (uint32_t)(p - block->code);
    return (mx_ir_step_t) {
        .instr = (mx_ir_instr_t) {
            .range = range.range,
            .args = cur->args,
            .out = out,
            .op = head[0],
        },
        .is_valid = true,
    };
}

void mx_ir_block_set_branch(mx_t *vm, mx_ir_block_t *block, mx_ir_branch_t branch) {
    block->branch_op = branch.op;
    block->branch_targets[0] = branch.targets[0];
    block->branch_targets[1] = branch.targets[1];
    uint32_t nargs = mx_ir_args_count(branch.args);
    if (nargs > MX_IR_BRANCH_MAX_ARGS) {
        mx_fatal("ir: branch argument list too wide");
    }
    uint8_t flags = mx_ir_range_flags(&branch.range);
    bool has_out = branch.out.type == MX_IR_ARG_TYPE_REG;
    bool has_want = branch.want != 0;
    size_t out_size;
    if (has_out) {
        out_size = mx_ir_uv_size(branch.out.reg);
    } else {
        out_size = 0;
    }
    size_t want_size;
    if (has_want) {
        want_size = mx_ir_uv_size(branch.want);
    } else {
        want_size = 0;
    }
    size_t size = 1 + mx_ir_range_size(&branch.range, flags) + out_size + want_size + mx_ir_uv_size(nargs);
    for (uint32_t i = 0; i < nargs; i++) {
        size += mx_ir_arg_size(branch.args[i]);
    }
    mx_ir_block_grow(block, block->code_len + size);
    vm->arenas.ir.branches += (uint32_t)size;
    block->branch_len = (uint32_t)size;
    uint8_t *head = block->code + block->code_len;
    head[0] = (uint8_t)(flags | (has_out << 2) | (has_want << 3));
    uint8_t *range_end = mx_ir_range_write(head + 1, &branch.range, flags);
    uint8_t *out_end;
    if (has_out) {
        out_end = mx_ir_put_uv(range_end, branch.out.reg);
    } else {
        out_end = range_end;
    }
    uint8_t *want_end;
    if (has_want) {
        want_end = mx_ir_put_uv(out_end, branch.want);
    } else {
        want_end = out_end;
    }
    uint8_t *p = mx_ir_put_uv(want_end, nargs);
    for (uint32_t i = 0; i < nargs; i++) {
        p = mx_ir_arg_write(p, branch.args[i]);
    }
}

mx_ir_branch_t mx_ir_block_branch(const mx_ir_block_t *block) {
    if (block->branch_len == 0) {
        return (mx_ir_branch_t) {
            .op = block->branch_op,
            .targets = {block->branch_targets[0], block->branch_targets[1]},
            .range = (mx_location_range_t) { .source = block->func->source },
            .out = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_NONE },
        };
    }
    const uint8_t *head = block->code + block->code_len;
    uint8_t meta = head[0];
    mx_ir_range_read_t range = mx_ir_range_read(head + 1, meta & 3, block->func->source);
    mx_ir_arg_t out;
    const uint8_t *out_end;
    if ((meta & 4) != 0) {
        mx_ir_uv_t reg = mx_ir_get_uv(range.next);
        out = (mx_ir_arg_t) {
            .type = MX_IR_ARG_TYPE_REG,
            .reg = reg.value,
        };
        out_end = reg.next;
    } else {
        out = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_NONE };
        out_end = range.next;
    }
    uint32_t want;
    const uint8_t *want_end;
    if ((meta & 8) != 0) {
        mx_ir_uv_t got = mx_ir_get_uv(out_end);
        want = (uint32_t)got.value;
        want_end = got.next;
    } else {
        want = 0;
        want_end = out_end;
    }
    mx_ir_uv_t count = mx_ir_get_uv(want_end);
    return (mx_ir_branch_t) {
        .op = block->branch_op,
        .targets = {block->branch_targets[0], block->branch_targets[1]},
        .range = range.range,
        .out = out,
        .want = want,
        .arg_count = (uint16_t)count.value,
        .args_enc = count.next,
    };
}

mx_location_range_t mx_ir_block_range(const mx_ir_block_t *block, uint32_t at) {
    if (at < block->code_len) {
        const uint8_t *head = &block->code[at];
        return mx_ir_range_read(head + 2, (uint8_t)((head[1] >> 3) & 3), block->func->source).range;
    } else if (block->branch_len != 0) {
        const uint8_t *head = block->code + block->code_len;
        return mx_ir_range_read(head + 1, head[0] & 3, block->func->source).range;
    } else {
        return (mx_location_range_t) { .source = block->func->source };
    }
}

uint16_t mx_ir_branch_args(const mx_ir_branch_t *br, mx_ir_arg_t *out_args) {
    const uint8_t *p = br->args_enc;
    for (uint16_t i = 0; i < br->arg_count; i++) {
        mx_ir_arg_read_t got = mx_ir_arg_read(p);
        out_args[i] = got.arg;
        p = got.next;
    }
    out_args[br->arg_count] = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_NONE };
    return br->arg_count;
}

void mx_ir_blocks_seal(mx_t *vm, mx_ir_blocks_t *blocks, uint32_t from) {
    for (uint32_t i = from; i < blocks->len; i++) {
        mx_ir_block_t *block = blocks->ptr[i];
        uint8_t *code = block->code;
        uint32_t size = block->code_len + block->branch_len;
        if (size != 0) {
            block->code = mx_ir_alloc_bytes(vm, size);
            vm->arenas.ir.sealed += block->code_len;
            memcpy(block->code, code, size);
        } else {
            block->code = NULL;
        }
        mx_mem_free(code);
    }
    blocks->len = from;
}

void mx_ir_live_use_arg(uint64_t *uevar, const uint64_t *kill, mx_ir_arg_t arg, uint32_t nregs) {
    if (arg.type == MX_IR_ARG_TYPE_REG && arg.reg < nregs && !mx_ir_live_test(kill, (uint32_t)arg.reg)) {
        mx_ir_live_set(uevar, (uint32_t)arg.reg);
    }
}

void mx_ir_live_use_range(uint64_t *uevar, const uint64_t *kill, uint32_t from, uint32_t to, uint32_t nregs) {
    for (uint32_t reg = from; reg < to && reg < nregs; reg++) {
        if (!mx_ir_live_test(kill, reg)) {
            mx_ir_live_set(uevar, reg);
        }
    }
}

void mx_ir_live_local(mx_ir_block_t *block, uint64_t *uevar, uint64_t *kill, uint32_t nregs) {
    mx_ir_cursor_t cur = {
        .off = 0,
    };
    if (nregs != 0) {
        mx_ir_live_set(uevar, 0);
    }
    mx_ir_step_t step = mx_ir_block_next(block, block->code_len, &cur);
    while (step.is_valid) {
        mx_ir_instr_t *in = &step.instr;
        if (in->op == MX_IR_INSTR_OPCODE_KEEP) {
            mx_ir_live_use_range(uevar, kill, (uint32_t)mx_obj_get_int(in->args[0].lit), (uint32_t)mx_obj_get_int(in->args[1].lit), nregs);
        } else if (in->op == MX_IR_INSTR_OPCODE_CLOSURE) {
            const mx_ir_func_t *child = mx_obj_get_block(in->args[0].lit)->func;
            for (uint32_t i = 0; i < child->nupvals; i++) {
                if (child->upvals[i].is_local) {
                    mx_ir_live_use_arg(uevar, kill, (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = child->upvals[i].index }, nregs);
                }
            }
        } else {
            for (mx_ir_arg_t *a = in->args; a->type != MX_IR_ARG_TYPE_NONE; a++) {
                mx_ir_live_use_arg(uevar, kill, *a, nregs);
            }
        }
        if (in->out.type == MX_IR_ARG_TYPE_REG && in->out.reg < nregs) {
            mx_ir_live_set(kill, (uint32_t)in->out.reg);
        }
        step = mx_ir_block_next(block, block->code_len, &cur);
    }
    if (block->branch_op != MX_IR_BRANCH_OPCODE_FALL) {
        mx_ir_branch_t br = mx_ir_block_branch(block);
        mx_ir_arg_t bargs[MX_IR_BRANCH_MAX_ARGS + 1];
        uint16_t nargs = mx_ir_branch_args(&br, bargs);
        for (uint16_t i = 0; i < nargs; i++) {
            mx_ir_live_use_arg(uevar, kill, bargs[i], nregs);
        }
        if (br.out.type == MX_IR_ARG_TYPE_REG) {
            uint32_t count;
            if (br.op != MX_IR_BRANCH_OPCODE_CALL) {
                count = 1;
            } else {
                count = br.want;
            }
            for (uint32_t r = (uint32_t)br.out.reg; r < br.out.reg + count && r < nregs; r++) {
                mx_ir_live_set(kill, r);
            }
        }
    }
}

void mx_ir_liveness(mx_t *vm, mx_ir_block_t *entry) {
    mx_ir_func_t *func = entry->func;
    mx_ir_blocks_t blocks = {
        .ptr = mx_mem_alloc(sizeof(mx_ir_block_t *) * 16),
        .len = 1,
        .cap = 16,
    };
    entry->is_marked = true;
    blocks.ptr[0] = entry;
    for (uint32_t i = 0; i < blocks.len; i++) {
        for (uint32_t t = 0; t < 2; t++) {
            mx_ir_block_t *target = blocks.ptr[i]->branch_targets[t];
            if (target != NULL && !target->is_marked) {
                target->is_marked = true;
                if (blocks.len == blocks.cap) {
                    blocks.cap *= 2;
                    blocks.ptr = mx_mem_realloc(blocks.ptr, sizeof(mx_ir_block_t *) * blocks.cap);
                }
                blocks.ptr[blocks.len] = target;
                blocks.len += 1;
            }
        }
    }
    uint32_t nregs = func->nregs;
    size_t words = mx_ir_live_words(func);
    func->live = mx_ir_alloc(vm, sizeof(uint64_t) * words * func->nblocks);
    memset(func->live, 0, sizeof(uint64_t) * words * func->nblocks);
    uint64_t *uevar = mx_mem_calloc(sizeof(uint64_t) * words * blocks.len);
    uint64_t *kill = mx_mem_calloc(sizeof(uint64_t) * words * blocks.len);
    for (uint32_t i = 0; i < blocks.len; i++) {
        mx_ir_live_local(blocks.ptr[i], &uevar[i * words], &kill[i * words], nregs);
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (uint32_t i = blocks.len; i > 0; i--) {
            mx_ir_block_t *b = blocks.ptr[i - 1];
            uint64_t *live = func->live + words * b->id;
            const uint64_t *ue = &uevar[(i - 1) * words];
            const uint64_t *kl = &kill[(i - 1) * words];
            for (size_t w = 0; w < words; w++) {
                uint64_t out = 0;
                for (uint32_t t = 0; t < 2; t++) {
                    mx_ir_block_t *target = b->branch_targets[t];
                    if (target != NULL) {
                        out |= func->live[words * target->id + w];
                    }
                }
                uint64_t next = ue[w] | (out & ~kl[w]);
                if (next != live[w]) {
                    live[w] = next;
                    changed = true;
                }
            }
        }
    }
    for (uint32_t i = 0; i < blocks.len; i++) {
        blocks.ptr[i]->is_marked = false;
    }
    mx_mem_free(uevar);
    mx_mem_free(kill);
    mx_mem_free(blocks.ptr);
}

const char *const mx_ir_branch_names[] = {
    [MX_IR_BRANCH_OPCODE_FALL] = "fall",
    [MX_IR_BRANCH_OPCODE_JUMP] = "jump",
    [MX_IR_BRANCH_OPCODE_BOOL] = "bool",
    [MX_IR_BRANCH_OPCODE_IF_LT] = "lt",
    [MX_IR_BRANCH_OPCODE_IF_LE] = "le",
    [MX_IR_BRANCH_OPCODE_IF_EQ] = "eq",
    [MX_IR_BRANCH_OPCODE_RETURN] = "return",
    [MX_IR_BRANCH_OPCODE_UPVAL_GET] = "upget",
    [MX_IR_BRANCH_OPCODE_TABLE_GET] = "get",
    [MX_IR_BRANCH_OPCODE_CALL] = "call",
};

const char *const mx_ir_instr_names[] = {
    [MX_IR_INSTR_OPCODE_MOVE] = "move",
    [MX_IR_INSTR_OPCODE_ADD] = "add",
    [MX_IR_INSTR_OPCODE_SUB] = "sub",
    [MX_IR_INSTR_OPCODE_MUL] = "mul",
    [MX_IR_INSTR_OPCODE_DIV] = "div",
    [MX_IR_INSTR_OPCODE_IDIV] = "idiv",
    [MX_IR_INSTR_OPCODE_MOD] = "mod",
    [MX_IR_INSTR_OPCODE_POW] = "pow",
    [MX_IR_INSTR_OPCODE_CONCAT] = "concat",
    [MX_IR_INSTR_OPCODE_BAND] = "band",
    [MX_IR_INSTR_OPCODE_BOR] = "bor",
    [MX_IR_INSTR_OPCODE_BXOR] = "bxor",
    [MX_IR_INSTR_OPCODE_SHL] = "shl",
    [MX_IR_INSTR_OPCODE_SHR] = "shr",
    [MX_IR_INSTR_OPCODE_NEG] = "neg",
    [MX_IR_INSTR_OPCODE_BNOT] = "bnot",
    [MX_IR_INSTR_OPCODE_TABLE_LEN] = "len",
    [MX_IR_INSTR_OPCODE_TABLE_NEW] = "new",
    [MX_IR_INSTR_OPCODE_TABLE_SET] = "set",
    [MX_IR_INSTR_OPCODE_CLOSURE] = "closure",
    [MX_IR_INSTR_OPCODE_VARARGS] = "varargs",
    [MX_IR_INSTR_OPCODE_UNPACK] = "unpack",
    [MX_IR_INSTR_OPCODE_UPVAL_SET] = "upset",
    [MX_IR_INSTR_OPCODE_CLOSE] = "close",
    [MX_IR_INSTR_OPCODE_KEEP] = "keep",
};

void mx_ir_format_arg(mx_str_t *out, mx_ir_arg_t val) {
    switch (val.type) {
        case MX_IR_ARG_TYPE_LIT: {
            mx_str_print_lit(out, val.lit);
            break;
        }
        case MX_IR_ARG_TYPE_REG: {
            mx_str_format(out, "%%%zu", (size_t)val.reg);
            break;
        }
        case MX_IR_ARG_TYPE_SPREAD: {
            mx_str_format(out, "...");
            break;
        }
        default: {
            break;
        }
    }
}

void mx_ir_format_branch(mx_str_t *out, mx_ir_branch_t val) {
    if (val.out.type != MX_IR_ARG_TYPE_NONE) {
        mx_ir_format_arg(out, val.out);
        mx_str_format(out, " <- ");
    }
    mx_str_format(out, "%s", mx_ir_branch_names[val.op]);
    mx_ir_arg_t bargs[MX_IR_BRANCH_MAX_ARGS + 1];
    uint16_t nargs = mx_ir_branch_args(&val, bargs);
    for (uint16_t i = 0; i < nargs; i++) {
        mx_str_format(out, " ");
        mx_ir_format_arg(out, bargs[i]);
    }
    if (val.op == MX_IR_BRANCH_OPCODE_CALL) {
        mx_str_format(out, " want %d", (int32_t)val.want);
    }
    for (uint32_t i = 0; i < 2; i++) {
        if (val.targets[i] != NULL) {
            mx_str_format(out, " .%zu", (size_t)val.targets[i]->id);
        }
    }
}

void mx_ir_format_instr(mx_str_t *out, mx_ir_instr_t val) {
    if (val.out.type != MX_IR_ARG_TYPE_NONE) {
        mx_ir_format_arg(out, val.out);
        mx_str_format(out, " <- ");
    }
    mx_str_format(out, "%s", mx_ir_instr_names[val.op]);
    for (size_t i = 0; val.args[i].type != MX_IR_ARG_TYPE_NONE; i++) {
        mx_str_format(out, " ");
        mx_ir_format_arg(out, val.args[i]);
    }
}

void mx_ir_format_block(mx_str_t *out, mx_ir_block_t *val) {
    mx_str_format(out, ".%zu:\n", (size_t)val->id);
    mx_ir_cursor_t cur = {
        .off = 0,
    };
    mx_ir_step_t step = mx_ir_block_next(val, val->code_len, &cur);
    while (step.is_valid) {
        mx_str_format(out, "    ");
        mx_ir_format_instr(out, step.instr);
        mx_str_format(out, "\n");
        step = mx_ir_block_next(val, val->code_len, &cur);
    }
    mx_str_format(out, "    ");
    mx_ir_format_branch(out, mx_ir_block_branch(val));
    mx_str_format(out, "\n");
}
