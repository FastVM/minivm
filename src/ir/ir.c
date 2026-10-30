
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

// Writes v in exactly n bytes, padding with continuation bytes, so it can be
// rewritten in place later. v must fit in 7 * n bits.
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
    v |= (uint64_t)p[0] << shift;
    return (mx_ir_uv_t) { .value = v, .next = p + 1 };
}

uint64_t mx_ir_zigzag(int64_t v) {
    return ((uint64_t)v << 1) ^ (uint64_t)(v >> 63);
}

int64_t mx_ir_unzigzag(uint64_t v) {
    return (int64_t)(v >> 1) ^ -(int64_t)(v & 1);
}

size_t mx_ir_range_size(const mx_location_range_t *range, uint8_t *flags) {
    size_t size = mx_ir_uv_size(range->start);
    uint8_t f = 0;
    for (uint32_t i = 0; i < 2; i++) {
        if (range->names[i].kind != MX_VARHINT_NONE) {
            f |= (uint8_t)(1 << i);
            size += 1 + sizeof(const char *);
        }
    }
    *flags = f;
    return size;
}

uint8_t *mx_ir_range_write(uint8_t *p, const mx_location_range_t *range, uint8_t flags) {
    p = mx_ir_put_uv(p, range->start);
    for (uint32_t i = 0; i < 2; i++) {
        if ((flags & (1 << i)) != 0) {
            p[0] = range->names[i].kind;
            memcpy(p + 1, &range->names[i].name, sizeof(const char *));
            p += 1 + sizeof(const char *);
        }
    }
    return p;
}

const uint8_t *mx_ir_range_read(const uint8_t *p, mx_location_range_t *range, uint8_t flags) {
    mx_ir_uv_t start = mx_ir_get_uv(p);
    p = start.next;
    range->start = (uint32_t)start.value;
    for (uint32_t i = 0; i < 2; i++) {
        if ((flags & (1 << i)) != 0) {
            range->names[i].kind = p[0];
            memcpy(&range->names[i].name, p + 1, sizeof(const char *));
            p += 1 + sizeof(const char *);
        } else {
            range->names[i] = (mx_varname_t) { .name = NULL, .kind = MX_VARHINT_NONE };
        }
    }
    return p;
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
                return 1 + sizeof(uint16_t) + sizeof(mx_val_t);
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
    p += 1;
    switch (arg.type) {
        case MX_IR_ARG_TYPE_REG: {
            return mx_ir_put_uv(p, arg.reg);
        }
        case MX_IR_ARG_TYPE_LIT: {
            uint16_t kind = arg.lit.kind;
            memcpy(p, &kind, sizeof(uint16_t));
            memcpy(p + sizeof(uint16_t), &arg.lit.val, sizeof(mx_val_t));
            return p + sizeof(uint16_t) + sizeof(mx_val_t);
        }
        default: {
            return p;
        }
    }
}

mx_ir_arg_read_t mx_ir_arg_read(const uint8_t *p) {
    mx_ir_arg_t arg;
    if (p[0] == MX_IR_ENC_INT) {
        mx_ir_uv_t got = mx_ir_get_uv(p + 1);
        arg = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_LIT, .lit = mx_obj_of_int(mx_ir_unzigzag(got.value)) };
        return (mx_ir_arg_read_t) { .next = got.next, .arg = arg };
    }
    arg.type = p[0];
    p += 1;
    switch (arg.type) {
        case MX_IR_ARG_TYPE_REG: {
            mx_ir_uv_t got = mx_ir_get_uv(p);
            arg.reg = got.value;
            return (mx_ir_arg_read_t) { .next = got.next, .arg = arg };
        }
        case MX_IR_ARG_TYPE_LIT: {
            uint16_t kind;
            memcpy(&kind, p, sizeof(uint16_t));
            arg.lit.kind = kind;
            memcpy(&arg.lit.val, p + sizeof(uint16_t), sizeof(mx_val_t));
            return (mx_ir_arg_read_t) { .next = p + sizeof(uint16_t) + sizeof(mx_val_t), .arg = arg };
        }
        default: {
            arg.reg = 0;
            return (mx_ir_arg_read_t) { .next = p, .arg = arg };
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
    uint8_t flags;
    size_t size = 2 + mx_ir_range_size(&instr.range, &flags);
    bool has_out = instr.out.type == MX_IR_ARG_TYPE_REG;
    if (has_out) {
        size += sizeof(uint16_t);
    }
    // TABLE_NEW's size hint is patched once the constructor has been parsed,
    // so it is written at a fixed width.
    bool is_hint = instr.op == MX_IR_INSTR_OPCODE_TABLE_NEW;
    for (uint32_t i = 0; i < nargs; i++) {
        if (is_hint) {
            size += 1 + MX_IR_TABLE_HINT_BYTES;
        } else {
            size += mx_ir_arg_size(instr.args[i]);
        }
    }
    mx_ir_block_grow(block, block->code_len + size);
    uint32_t at = block->code_len;
    uint8_t *p = &block->code[at];
    uint8_t meta = (uint8_t)(nargs | (flags << 3));
    if (has_out) {
        meta |= 0x80;
    }
    p[0] = instr.op;
    p[1] = meta;
    p = mx_ir_range_write(p + 2, &instr.range, flags);
    if (has_out) {
        uint16_t out = (uint16_t)instr.out.reg;
        memcpy(p, &out, sizeof(uint16_t));
        p += sizeof(uint16_t);
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
    uint8_t *p = &block->code[at];
    mx_location_range_t range;
    return (uint8_t *)mx_ir_range_read(p + 2, &range, (uint8_t)((p[1] >> 3) & 3));
}

void mx_ir_block_patch_hint(mx_ir_block_t *block, uint32_t at, uint64_t hint) {
    uint8_t *p = mx_ir_block_operands(block, at);
    if ((block->code[at + 1] & 0x80) != 0) {
        p += sizeof(uint16_t);
    }
    mx_ir_put_uv_wide(p + 1, mx_ir_zigzag((int64_t)hint), MX_IR_TABLE_HINT_BYTES);
}

void mx_ir_block_retarget(mx_ir_block_t *block, uint32_t at, uint32_t reg) {
    uint16_t out = (uint16_t)reg;
    memcpy(mx_ir_block_operands(block, at), &out, sizeof(uint16_t));
}

mx_ir_step_t mx_ir_block_next(const mx_ir_block_t *block, uint32_t limit, mx_ir_cursor_t *cur) {
    if (cur->off >= limit) {
        return (mx_ir_step_t) { .is_valid = false };
    }
    const uint8_t *p = &block->code[cur->off];
    mx_ir_instr_t instr;
    instr.op = p[0];
    uint8_t meta = p[1];
    instr.range.source = block->func->source;
    p = mx_ir_range_read(p + 2, &instr.range, (uint8_t)((meta >> 3) & 3));
    if ((meta & 0x80) != 0) {
        uint16_t out;
        memcpy(&out, p, sizeof(uint16_t));
        instr.out = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = out };
        p += sizeof(uint16_t);
    } else {
        instr.out = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_NONE };
    }
    uint32_t nargs = meta & 7;
    for (uint32_t i = 0; i < nargs; i++) {
        mx_ir_arg_read_t got = mx_ir_arg_read(p);
        cur->args[i] = got.arg;
        p = got.next;
    }
    cur->args[nargs].type = MX_IR_ARG_TYPE_NONE;
    instr.args = cur->args;
    cur->off = (uint32_t)(p - block->code);
    return (mx_ir_step_t) { .instr = instr, .is_valid = true };
}

void mx_ir_block_set_branch(mx_t *vm, mx_ir_block_t *block, mx_ir_branch_t branch) {
    block->branch_op = branch.op;
    block->branch_targets[0] = branch.targets[0];
    block->branch_targets[1] = branch.targets[1];
    uint32_t nargs = mx_ir_args_count(branch.args);
    if (nargs > MX_IR_BRANCH_MAX_ARGS) {
        mx_fatal("ir: branch argument list too wide");
    }
    uint8_t flags;
    size_t size = 1 + mx_ir_range_size(&branch.range, &flags) + mx_ir_uv_size(nargs);
    uint8_t meta = flags;
    if (branch.out.type == MX_IR_ARG_TYPE_REG) {
        meta |= 4;
        size += mx_ir_uv_size(branch.out.reg);
    }
    if (branch.want != 0) {
        meta |= 8;
        size += mx_ir_uv_size(branch.want);
    }
    for (uint32_t i = 0; i < nargs; i++) {
        size += mx_ir_arg_size(branch.args[i]);
    }
    mx_ir_block_grow(block, block->code_len + size);
    vm->arenas.ir.branches += (uint32_t)size;
    block->branch_len = (uint32_t)size;
    uint8_t *p = block->code + block->code_len;
    p[0] = meta;
    p = mx_ir_range_write(p + 1, &branch.range, flags);
    if ((meta & 4) != 0) {
        p = mx_ir_put_uv(p, branch.out.reg);
    }
    if ((meta & 8) != 0) {
        p = mx_ir_put_uv(p, branch.want);
    }
    p = mx_ir_put_uv(p, nargs);
    for (uint32_t i = 0; i < nargs; i++) {
        p = mx_ir_arg_write(p, branch.args[i]);
    }
}

mx_ir_branch_t mx_ir_block_branch(const mx_ir_block_t *block) {
    mx_ir_branch_t br = {
        .op = block->branch_op,
        .targets = {block->branch_targets[0], block->branch_targets[1]},
        .range = {
            .source = block->func->source,
        },
        .out = {
            .type = MX_IR_ARG_TYPE_NONE,
        },
    };
    if (block->branch_len != 0) {
        const uint8_t *p = block->code + block->code_len;
        uint8_t meta = p[0];
        p = mx_ir_range_read(p + 1, &br.range, meta & 3);
        if ((meta & 4) != 0) {
            mx_ir_uv_t out = mx_ir_get_uv(p);
            br.out = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = out.value };
            p = out.next;
        }
        if ((meta & 8) != 0) {
            mx_ir_uv_t want = mx_ir_get_uv(p);
            br.want = (uint32_t)want.value;
            p = want.next;
        }
        mx_ir_uv_t count = mx_ir_get_uv(p);
        br.arg_count = (uint16_t)count.value;
        br.args_enc = count.next;
    }
    return br;
}

mx_location_range_t mx_ir_block_range(const mx_ir_block_t *block, uint32_t at) {
    mx_location_range_t range = { .source = block->func->source };
    if (at < block->code_len) {
        const uint8_t *p = &block->code[at];
        mx_ir_range_read(p + 2, &range, (uint8_t)((p[1] >> 3) & 3));
    } else if (block->branch_len != 0) {
        const uint8_t *p = block->code + block->code_len;
        mx_ir_range_read(p + 1, &range, p[0] & 3);
    }
    return range;
}

uint16_t mx_ir_branch_args(const mx_ir_branch_t *br, mx_ir_arg_t *out_args) {
    const uint8_t *p = br->args_enc;
    for (uint16_t i = 0; i < br->arg_count; i++) {
        mx_ir_arg_read_t got = mx_ir_arg_read(p);
        out_args[i] = got.arg;
        p = got.next;
    }
    out_args[br->arg_count].type = MX_IR_ARG_TYPE_NONE;
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
    mx_ir_blocks_t blocks = {0};
    uint32_t cap = 16;
    blocks.ptr = mx_mem_alloc(sizeof(mx_ir_block_t *) * cap);
    entry->is_marked = true;
    blocks.ptr[0] = entry;
    blocks.len = 1;
    for (uint32_t i = 0; i < blocks.len; i++) {
        for (uint32_t t = 0; t < 2; t++) {
            mx_ir_block_t *target = blocks.ptr[i]->branch_targets[t];
            if (target != NULL && !target->is_marked) {
                target->is_marked = true;
                if (blocks.len == cap) {
                    cap *= 2;
                    blocks.ptr = mx_mem_realloc(blocks.ptr, sizeof(mx_ir_block_t *) * cap);
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
