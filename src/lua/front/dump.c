
#include "dump.h"
#include "parse.h"
#include "../rt/prim.h"
#include "../../value/error.h"
#include "../../value/obj.h"

mx_ffi_t *const mx_dump_prims[] = {
    mx_prim_table_append,
    mx_prim_pack,
    mx_prim_forprep_initial,
    mx_prim_forprep_limit,
    mx_prim_forprep_step,
    mx_prim_global_fresh,
    mx_prim_close_check,
    mx_prim_close_call,
};

const uint32_t mx_dump_nprims = sizeof(mx_dump_prims) / sizeof(mx_dump_prims[0]);

void mx_dump_uv(mx_dump_writer_t *w, uint64_t v) {
    uint8_t tmp[10];
    uint8_t *end = mx_ir_put_uv(tmp, v);
    mx_str_append(w->out, (const char *)tmp, (uint32_t)(end - tmp));
}

void mx_dump_bytes(mx_dump_writer_t *w, const char *data, size_t len) {
    mx_dump_uv(w, len);
    mx_str_append(w->out, data, (uint32_t)len);
}

void mx_dump_name(mx_dump_writer_t *w, const char *name) {
    if (name == NULL) {
        mx_dump_uv(w, 0);
    } else {
        size_t len = strlen(name);
        mx_dump_uv(w, len + 1);
        mx_str_append(w->out, name, (uint32_t)len);
    }
}

void mx_dump_range(mx_dump_writer_t *w, const mx_location_range_t *range) {
    mx_dump_uv(w, range->start);
    mx_dump_uv(w, range->stop);
    for (uint32_t i = 0; i < 2; i++) {
        if (w->is_strip) {
            mx_str_append_byte(w->out, (char)MX_VARHINT_NONE);
        } else {
            mx_str_append_byte(w->out, (char)range->names[i].kind);
        }
        if (!w->is_strip && range->names[i].kind != MX_VARHINT_NONE) {
            mx_dump_name(w, range->names[i].name);
        }
    }
}

void mx_dump_arg(mx_dump_writer_t *w, mx_ir_arg_t arg) {
    mx_str_append_byte(w->out, (char)arg.type);
    if (arg.type == MX_IR_ARG_TYPE_REG) {
        mx_dump_uv(w, arg.reg);
        return;
    } else if (arg.type != MX_IR_ARG_TYPE_LIT) {
        return;
    }
    mx_obj_t v = arg.lit;
    switch (v.kind) {
        case MX_KIND_NIL: {
            mx_str_append_byte(w->out, MX_DUMP_LIT_NIL);
            break;
        }
        case MX_KIND_BOOLEAN: {
            if (mx_obj_get_boolean(v)) {
                mx_str_append_byte(w->out, MX_DUMP_LIT_TRUE);
            } else {
                mx_str_append_byte(w->out, MX_DUMP_LIT_FALSE);
            }
            break;
        }
        case MX_KIND_INT: {
            mx_str_append_byte(w->out, MX_DUMP_LIT_INT);
            mx_dump_uv(w, mx_ir_zigzag(mx_obj_get_int(v)));
            break;
        }
        case MX_KIND_FLOAT: {
            mx_str_append_byte(w->out, MX_DUMP_LIT_FLOAT);
            mx_str_append(w->out, (const char *)&v.val.bits, sizeof(v.val.bits));
            break;
        }
        case MX_KIND_STRING: {
            mx_str_t *s = mx_obj_get_string(v);
            mx_str_append_byte(w->out, MX_DUMP_LIT_STRING);
            mx_dump_bytes(w, s->buf, s->len);
            break;
        }
        case MX_KIND_BLOCK: {
            mx_str_append_byte(w->out, MX_DUMP_LIT_FUNC);
            mx_dump_func(w, mx_obj_get_block(v));
            break;
        }
        case MX_KIND_FFI: {
            uint32_t id = 0;
            while (id < mx_dump_nprims && mx_dump_prims[id] != mx_obj_get_ffi(v)) {
                id += 1;
            }
            w->is_bad = w->is_bad || id == mx_dump_nprims;
            mx_str_append_byte(w->out, MX_DUMP_LIT_PRIM);
            mx_dump_uv(w, id);
            break;
        }
        default: {
            w->is_bad = true;
            break;
        }
    }
}

uint32_t mx_dump_index(const mx_ir_blocks_t *blocks, const mx_ir_block_t *block) {
    for (uint32_t i = 0; i < blocks->len; i++) {
        if (blocks->ptr[i] == block) {
            return i;
        }
    }
    return UINT32_MAX;
}

void mx_dump_func(mx_dump_writer_t *w, mx_ir_block_t *head) {
    mx_ir_blocks_t blocks = { .ptr = mx_mem_alloc(sizeof(mx_ir_block_t *) * 16), .len = 1, .cap = 16 };
    blocks.ptr[0] = head;
    for (uint32_t i = 0; i < blocks.len; i++) {
        for (uint32_t t = 0; t < 2; t++) {
            mx_ir_block_t *target = blocks.ptr[i]->branch_targets[t];
            if (target != NULL && mx_dump_index(&blocks, target) == UINT32_MAX) {
                if (blocks.len == blocks.cap) {
                    blocks.cap *= 2;
                    blocks.ptr = mx_mem_realloc(blocks.ptr, sizeof(mx_ir_block_t *) * blocks.cap);
                }
                blocks.ptr[blocks.len] = target;
                blocks.len += 1;
            }
        }
    }
    const mx_ir_func_t *func = head->func;
    mx_dump_uv(w, func->start);
    mx_dump_uv(w, func->stop);
    mx_str_append_byte(w->out, (char)(head->is_main | (func->is_vararg << 1) | (func->has_cells << 2)));
    mx_dump_uv(w, func->nregs);
    mx_dump_uv(w, func->nparams);
    mx_dump_uv(w, func->nupvals);
    for (uint32_t i = 0; i < func->nupvals; i++) {
        if (w->is_strip) {
            mx_dump_name(w, NULL);
        } else {
            mx_dump_name(w, func->upvals[i].name);
        }
        mx_dump_uv(w, func->upvals[i].index);
        mx_str_append_byte(w->out, (char)func->upvals[i].is_local);
    }
    uint32_t nlocals;
    if (w->is_strip) {
        nlocals = 0;
    } else {
        nlocals = func->locals.len;
    }
    mx_dump_uv(w, nlocals);
    for (uint32_t i = 0; i < nlocals; i++) {
        const mx_ir_local_t *local = &func->locals.ptr[i];
        mx_dump_name(w, local->name);
        mx_dump_uv(w, local->reg);
        mx_dump_uv(w, local->start);
        mx_dump_uv(w, local->stop);
        mx_str_append_byte(w->out, (char)(local->is_param | (local->is_pack << 1)));
    }
    mx_dump_uv(w, blocks.len);
    for (uint32_t i = 0; i < blocks.len; i++) {
        mx_ir_block_t *block = blocks.ptr[i];
        uint32_t ninstrs = 0;
        mx_ir_cursor_t count = { .off = 0 };
        while (mx_ir_block_next(block, block->code_len, &count).is_valid) {
            ninstrs += 1;
        }
        mx_dump_uv(w, ninstrs);
        mx_ir_cursor_t cur = { .off = 0 };
        for (mx_ir_step_t step = mx_ir_block_next(block, block->code_len, &cur); step.is_valid; step = mx_ir_block_next(block, block->code_len, &cur)) {
            mx_str_append_byte(w->out, (char)step.instr.op);
            mx_dump_range(w, &step.instr.range);
            if (step.instr.out.type == MX_IR_ARG_TYPE_REG) {
                mx_dump_uv(w, step.instr.out.reg + 1);
            } else {
                mx_dump_uv(w, 0);
            }
            uint32_t nargs = mx_ir_args_count(step.instr.args);
            mx_dump_uv(w, nargs);
            for (uint32_t a = 0; a < nargs; a++) {
                mx_dump_arg(w, step.instr.args[a]);
            }
        }
        mx_str_append_byte(w->out, (char)block->branch_op);
        for (uint32_t t = 0; t < 2; t++) {
            if (block->branch_targets[t] == NULL) {
                mx_dump_uv(w, 0);
            } else {
                mx_dump_uv(w, (uint64_t)mx_dump_index(&blocks, block->branch_targets[t]) + 1);
            }
        }
        mx_str_append_byte(w->out, (char)(block->branch_enc != NULL));
        if (block->branch_enc != NULL) {
            mx_ir_branch_t br = mx_ir_block_branch(block);
            mx_ir_arg_t args[MX_IR_BRANCH_MAX_ARGS + 1];
            uint16_t nargs = mx_ir_branch_args(&br, args);
            mx_dump_range(w, &br.range);
            if (br.out.type == MX_IR_ARG_TYPE_REG) {
                mx_dump_uv(w, br.out.reg + 1);
            } else {
                mx_dump_uv(w, 0);
            }
            mx_dump_uv(w, br.want);
            mx_dump_uv(w, nargs);
            for (uint16_t a = 0; a < nargs; a++) {
                mx_dump_arg(w, args[a]);
            }
        }
    }
    mx_mem_free(blocks.ptr);
}

mx_str_t *mx_dump_function(mx_t *vm, mx_ir_block_t *head, bool is_strip) {
    mx_dump_writer_t w = { .vm = vm, .out = mx_str_new(), .is_strip = is_strip };
    const mx_source_t *source = head->func->source;
    uint32_t len = head->func->stop;
    if (len > source->len) {
        len = source->len;
    }
    mx_str_append(w.out, MX_DUMP_MAGIC, MX_DUMP_MAGIC_LEN);
    mx_dump_name(&w, source->file);
    mx_dump_bytes(&w, source->src, len);
    mx_dump_func(&w, head);
    if (w.is_bad) {
        mx_str_free(w.out);
        return NULL;
    } else {
        return w.out;
    }
}

uint64_t mx_dump_read_uv(mx_dump_reader_t *r) {
    uint64_t v = 0;
    for (uint32_t shift = 0; shift < 64 && r->p < r->end; shift += 7) {
        uint8_t byte = r->p[0];
        r->p += 1;
        v |= (uint64_t)(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) {
            return v;
        }
    }
    r->is_bad = true;
    return 0;
}

const uint8_t *mx_dump_read_bytes(mx_dump_reader_t *r, size_t len) {
    if (r->is_bad) {
        return NULL;
    } else if (len > (size_t)(r->end - r->p)) {
        r->is_bad = true;
        r->is_truncated = true;
        return NULL;
    }
    const uint8_t *at = r->p;
    r->p += len;
    return at;
}

const char *mx_dump_read_name(mx_dump_reader_t *r) {
    uint64_t n = mx_dump_read_uv(r);
    if (n == 0) {
        return NULL;
    }
    const uint8_t *at = mx_dump_read_bytes(r, n - 1);
    if (at == NULL) {
        return NULL;
    }
    char *out = mx_ir_alloc_bytes(r->vm, n);
    memcpy(out, at, n - 1);
    out[n - 1] = '\0';
    return out;
}

mx_location_range_t mx_dump_read_range(mx_dump_reader_t *r) {
    mx_location_range_t range = { .source = r->source };
    range.start = (uint32_t)mx_dump_read_uv(r);
    range.stop = (uint32_t)mx_dump_read_uv(r);
    for (uint32_t i = 0; i < 2; i++) {
        const uint8_t *kind = mx_dump_read_bytes(r, 1);
        if (kind != NULL && kind[0] > MX_VARHINT_FOR_ITERATOR) {
            r->is_bad = true;
        } else if (kind != NULL && kind[0] != MX_VARHINT_NONE) {
            range.names[i] = (mx_varname_t) { .kind = kind[0], .name = mx_dump_read_name(r) };
        }
    }
    return range;
}

mx_ir_arg_t mx_dump_read_arg(mx_dump_reader_t *r, uint32_t nregs) {
    mx_ir_arg_t arg = { .type = MX_IR_ARG_TYPE_SPREAD };
    const uint8_t *type = mx_dump_read_bytes(r, 1);
    if (type == NULL) {
        return arg;
    } else if (type[0] == MX_IR_ARG_TYPE_REG) {
        arg = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = mx_dump_read_uv(r) };
        r->is_bad = r->is_bad || arg.reg >= nregs;
        return arg;
    } else if (type[0] != MX_IR_ARG_TYPE_LIT) {
        r->is_bad = r->is_bad || type[0] != MX_IR_ARG_TYPE_SPREAD;
        return arg;
    }
    arg = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_LIT, .lit = mx_obj_of_nil() };
    const uint8_t *tag = mx_dump_read_bytes(r, 1);
    if (tag == NULL) {
        return arg;
    }
    switch (tag[0]) {
        case MX_DUMP_LIT_NIL: {
            break;
        }
        case MX_DUMP_LIT_FALSE:
        case MX_DUMP_LIT_TRUE: {
            arg.lit = mx_obj_of_boolean(tag[0] == MX_DUMP_LIT_TRUE);
            break;
        }
        case MX_DUMP_LIT_INT: {
            arg.lit = mx_obj_of_int(mx_ir_unzigzag(mx_dump_read_uv(r)));
            break;
        }
        case MX_DUMP_LIT_FLOAT: {
            const uint8_t *bits = mx_dump_read_bytes(r, sizeof(uint64_t));
            if (bits != NULL) {
                mx_val_t v;
                memcpy(&v.bits, bits, sizeof(uint64_t));
                arg.lit = mx_obj_of_raw(MX_KIND_FLOAT, v);
            }
            break;
        }
        case MX_DUMP_LIT_STRING: {
            uint64_t len = mx_dump_read_uv(r);
            const uint8_t *bytes = mx_dump_read_bytes(r, len);
            if (bytes != NULL && len <= UINT32_MAX) {
                arg.lit = mx_obj_intern_strn(r->vm, (const char *)bytes, (uint32_t)len);
            } else {
                r->is_bad = true;
            }
            break;
        }
        case MX_DUMP_LIT_FUNC: {
            mx_ir_block_t *child = mx_dump_read_func(r);
            if (child != NULL) {
                arg.lit = mx_obj_of_block(child);
            }
            break;
        }
        case MX_DUMP_LIT_PRIM: {
            uint64_t id = mx_dump_read_uv(r);
            if (id < mx_dump_nprims) {
                arg.lit = mx_obj_of_ffi(mx_dump_prims[id]);
            } else {
                r->is_bad = true;
            }
            break;
        }
        default: {
            r->is_bad = true;
            break;
        }
    }
    return arg;
}

bool mx_dump_is_lit(mx_ir_arg_t arg, mx_kind_t kind) {
    return arg.type == MX_IR_ARG_TYPE_LIT && arg.lit.kind == kind;
}

bool mx_dump_is_index(mx_ir_arg_t arg, uint64_t limit) {
    return mx_dump_is_lit(arg, MX_KIND_INT) && mx_obj_get_int(arg.lit) >= 0 && (uint64_t)mx_obj_get_int(arg.lit) < limit;
}

bool mx_dump_args_ok(const mx_ir_arg_t *args, uint32_t nargs, bool can_spread, bool can_prim, bool can_block) {
    for (uint32_t i = 0; i < nargs; i++) {
        if (args[i].type == MX_IR_ARG_TYPE_SPREAD && (!can_spread || i + 1 != nargs)) {
            return false;
        } else if (mx_dump_is_lit(args[i], MX_KIND_FFI) && (!can_prim || i != 0)) {
            return false;
        } else if (mx_dump_is_lit(args[i], MX_KIND_BLOCK) && !can_block) {
            return false;
        }
    }
    return true;
}

bool mx_dump_instr_ok(mx_dump_reader_t *r, mx_ir_instr_opcode_t op, bool has_out, const mx_ir_arg_t *args, uint32_t nargs, bool is_entry) {
    const mx_ir_func_t *func = r->func;
    if (!mx_dump_args_ok(args, nargs, false, false, op == MX_IR_INSTR_OPCODE_CLOSURE)) {
        return false;
    }
    switch (op) {
        case MX_IR_INSTR_OPCODE_MOVE:
        case MX_IR_INSTR_OPCODE_NEG:
        case MX_IR_INSTR_OPCODE_BNOT:
        case MX_IR_INSTR_OPCODE_TABLE_LEN: {
            return has_out && nargs == 1;
        }
        case MX_IR_INSTR_OPCODE_CLOSURE: {
            return has_out && nargs == 1 && mx_dump_is_lit(args[0], MX_KIND_BLOCK);
        }
        case MX_IR_INSTR_OPCODE_TABLE_NEW: {
            return has_out && nargs == 1 && mx_dump_is_index(args[0], (uint64_t)1 << 32);
        }
        case MX_IR_INSTR_OPCODE_TABLE_SET: {
            return !has_out && nargs == 3;
        }
        case MX_IR_INSTR_OPCODE_VARARGS: {
            return has_out && nargs == 0 && func->is_vararg && is_entry;
        }
        case MX_IR_INSTR_OPCODE_UNPACK: {
            return !has_out && nargs == 1 && args[0].type == MX_IR_ARG_TYPE_REG;
        }
        case MX_IR_INSTR_OPCODE_UPVAL_SET: {
            return !has_out && nargs == 2 && mx_dump_is_index(args[0], func->nupvals);
        }
        case MX_IR_INSTR_OPCODE_CLOSE: {
            return !has_out && nargs == 1 && mx_dump_is_index(args[0], (uint64_t)func->nregs + 1);
        }
        case MX_IR_INSTR_OPCODE_KEEP: {
            return !has_out && nargs == 2 && mx_dump_is_index(args[0], (uint64_t)func->nregs + 1) && mx_dump_is_index(args[1], (uint64_t)func->nregs + 1);
        }
        default: {
            return has_out && nargs == 2;
        }
    }
}

bool mx_dump_prim_ok(const mx_ir_arg_t *args, uint32_t nargs) {
    mx_ffi_t *fn = mx_obj_get_ffi(args[0].lit);
    bool is_spread = nargs > 0 && args[nargs - 1].type == MX_IR_ARG_TYPE_SPREAD;
    if (fn == mx_prim_pack) {
        return true;
    } else if (fn == mx_prim_table_append) {
        return nargs >= 3 && args[1].type == MX_IR_ARG_TYPE_REG && mx_dump_is_lit(args[2], MX_KIND_INT);
    } else if (is_spread) {
        return false;
    } else if (fn == mx_prim_forprep_initial) {
        return nargs == 3;
    } else if (fn == mx_prim_forprep_limit) {
        return nargs == 4;
    } else if (fn == mx_prim_forprep_step || fn == mx_prim_close_call) {
        return nargs == 2;
    } else {
        return nargs == 3 && mx_dump_is_lit(args[2], MX_KIND_STRING);
    }
}

bool mx_dump_branch_ok(mx_dump_reader_t *r, const mx_ir_branch_t *br, const mx_ir_arg_t *args, uint32_t nargs) {
    const mx_ir_func_t *func = r->func;
    bool has_out = br->out.type == MX_IR_ARG_TYPE_REG;
    bool has_t0 = br->targets[0] != NULL;
    bool has_t1 = br->targets[1] != NULL;
    bool is_call = br->op == MX_IR_BRANCH_OPCODE_CALL;
    if (!mx_dump_args_ok(args, nargs, is_call || br->op == MX_IR_BRANCH_OPCODE_RETURN, is_call, false)) {
        return false;
    }
    switch (br->op) {
        case MX_IR_BRANCH_OPCODE_FALL:
        case MX_IR_BRANCH_OPCODE_JUMP: {
            return !has_out && nargs == 0 && has_t0 && !has_t1;
        }
        case MX_IR_BRANCH_OPCODE_BOOL: {
            return !has_out && nargs == 1 && has_t0 && has_t1;
        }
        case MX_IR_BRANCH_OPCODE_IF_LT:
        case MX_IR_BRANCH_OPCODE_IF_LE:
        case MX_IR_BRANCH_OPCODE_IF_EQ: {
            return !has_out && nargs == 2 && has_t0 && has_t1;
        }
        case MX_IR_BRANCH_OPCODE_RETURN: {
            return !has_out && !has_t0 && !has_t1;
        }
        case MX_IR_BRANCH_OPCODE_UPVAL_GET: {
            return has_out && nargs == 1 && mx_dump_is_index(args[0], func->nupvals) && has_t0 && !has_t1;
        }
        case MX_IR_BRANCH_OPCODE_TABLE_GET: {
            return has_out && nargs == 2 && has_t0 && !has_t1;
        }
        case MX_IR_BRANCH_OPCODE_CALL: {
            bool is_shaped;
            if (!has_t0) {
                is_shaped = br->want == MX_IR_WANT_MULTI && !has_out;
            } else if (br->want == MX_IR_WANT_MULTI || br->want == 0) {
                is_shaped = !has_out;
            } else {
                is_shaped = has_out && br->out.reg + br->want <= func->nregs;
            }
            return is_shaped && !has_t1 && nargs >= 1 && (!mx_dump_is_lit(args[0], MX_KIND_FFI) || mx_dump_prim_ok(args, nargs));
        }
        default: {
            return false;
        }
    }
}

mx_ir_block_t *mx_dump_read_func(mx_dump_reader_t *r) {
    mx_t *vm = r->vm;
    const mx_ir_func_t *parent = r->func;
    uint32_t start = (uint32_t)mx_dump_read_uv(r);
    uint32_t stop = (uint32_t)mx_dump_read_uv(r);
    const uint8_t *flags = mx_dump_read_bytes(r, 1);
    uint64_t nregs = mx_dump_read_uv(r);
    uint64_t nparams = mx_dump_read_uv(r);
    uint64_t nupvals = mx_dump_read_uv(r);
    if (r->is_bad || nregs > 0xffff || nparams > nregs || nupvals > 255 || r->depth > 200) {
        r->is_bad = true;
        return NULL;
    }
    r->depth += 1;
    mx_ir_upval_t *upvals = NULL;
    mx_kind_t *seen = NULL;
    if (nupvals != 0) {
        upvals = mx_ir_alloc(vm, sizeof(mx_ir_upval_t) * nupvals);
        seen = mx_ir_alloc(vm, sizeof(mx_kind_t) * nupvals);
    }
    for (uint32_t i = 0; i < nupvals; i++) {
        const char *name = mx_dump_read_name(r);
        uint32_t index = (uint32_t)mx_dump_read_uv(r);
        const uint8_t *is_local = mx_dump_read_bytes(r, 1);
        upvals[i] = (mx_ir_upval_t) { .name = name, .index = index, .is_local = is_local != NULL && is_local[0] != 0 };
        seen[i] = MX_KIND_NIL;
        if (parent != NULL && upvals[i].is_local) {
            r->is_bad = r->is_bad || index >= parent->nregs || !parent->has_cells;
        } else if (parent != NULL) {
            r->is_bad = r->is_bad || index >= parent->nupvals;
        }
    }
    uint64_t nlocals = mx_dump_read_uv(r);
    if (nlocals > (uint64_t)(r->end - r->p)) {
        r->is_bad = true;
    }
    mx_ir_local_t *locals = NULL;
    if (!r->is_bad && nlocals != 0) {
        locals = mx_ir_alloc(vm, sizeof(mx_ir_local_t) * nlocals);
    }
    for (uint32_t i = 0; i < nlocals && !r->is_bad; i++) {
        const char *name = mx_dump_read_name(r);
        uint64_t reg = mx_dump_read_uv(r);
        uint32_t lstart = (uint32_t)mx_dump_read_uv(r);
        uint32_t lstop = (uint32_t)mx_dump_read_uv(r);
        const uint8_t *lflags = mx_dump_read_bytes(r, 1);
        r->is_bad = r->is_bad || reg >= nregs;
        if (lflags != NULL) {
            locals[i] = (mx_ir_local_t) {
                .name = name,
                .reg = (uint32_t)reg,
                .start = lstart,
                .stop = lstop,
                .is_param = (lflags[0] & 1) != 0,
                .is_pack = (lflags[0] & 2) != 0,
            };
        }
    }
    uint64_t nblocks = mx_dump_read_uv(r);
    if (r->is_bad || nblocks == 0 || nblocks > (uint64_t)(r->end - r->p)) {
        r->is_bad = true;
        return NULL;
    }
    mx_ir_func_t *func = mx_ir_func_new(&vm->arenas.ir, r->source);
    mx_ir_block_t **blocks = mx_mem_alloc(sizeof(mx_ir_block_t *) * nblocks);
    for (uint32_t i = 0; i < nblocks; i++) {
        blocks[i] = mx_ir_block_new(vm, &vm->arenas.ir, func);
        if (r->blocks.len == r->blocks.cap) {
            r->blocks.cap = r->blocks.cap * 2 + 64;
            r->blocks.ptr = mx_mem_realloc(r->blocks.ptr, sizeof(mx_ir_block_t *) * r->blocks.cap);
        }
        r->blocks.ptr[r->blocks.len] = blocks[i];
        r->blocks.len += 1;
    }
    mx_ir_block_t *head = blocks[0];
    head->is_func = true;
    head->is_main = (flags[0] & 1) != 0;
    *func = (mx_ir_func_t) {
        .head = head,
        .source = r->source,
        .start = start,
        .stop = stop,
        .upval_seen = seen,
        .upvals = upvals,
        .locals = { .ptr = locals, .len = (uint32_t)nlocals },
        .nupvals = (uint32_t)nupvals,
        .nregs = (uint32_t)nregs,
        .nparams = (uint32_t)nparams,
        .is_vararg = (flags[0] & 2) != 0,
        .has_cells = (flags[0] & 4) != 0,
    };
    r->func = func;
    for (uint32_t i = 0; i < nblocks && !r->is_bad; i++) {
        mx_ir_block_t *block = blocks[i];
        uint64_t ninstrs = mx_dump_read_uv(r);
        for (uint64_t n = 0; n < ninstrs && !r->is_bad; n++) {
            const uint8_t *op = mx_dump_read_bytes(r, 1);
            mx_location_range_t range = mx_dump_read_range(r);
            uint64_t out = mx_dump_read_uv(r);
            uint64_t nargs = mx_dump_read_uv(r);
            if (r->is_bad || op[0] > MX_IR_INSTR_OPCODE_KEEP || out > nregs || nargs > MX_IR_INSTR_MAX_ARGS) {
                r->is_bad = true;
                break;
            }
            mx_ir_arg_t args[MX_IR_INSTR_MAX_ARGS + 1];
            for (uint32_t a = 0; a < nargs; a++) {
                args[a] = mx_dump_read_arg(r, (uint32_t)nregs);
            }
            args[nargs].type = MX_IR_ARG_TYPE_NONE;
            mx_ir_arg_t oarg = { .type = MX_IR_ARG_TYPE_NONE };
            if (out != 0) {
                oarg = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = out - 1 };
            }
            r->is_bad = r->is_bad || !mx_dump_instr_ok(r, op[0], out != 0, args, (uint32_t)nargs, i == 0 && n == 0);
            if (!r->is_bad) {
                mx_ir_keep_lits(vm, args);
                mx_ir_block_emit(block, (mx_ir_instr_t) { .op = op[0], .range = range, .out = oarg, .args = args });
            }
        }
        const uint8_t *bop = mx_dump_read_bytes(r, 1);
        uint64_t t0 = mx_dump_read_uv(r);
        uint64_t t1 = mx_dump_read_uv(r);
        const uint8_t *has_enc = mx_dump_read_bytes(r, 1);
        bool is_reentry = func->is_vararg && (t0 == 1 || t1 == 1);
        if (r->is_bad || bop[0] > MX_IR_BRANCH_OPCODE_CALL || t0 > nblocks || t1 > nblocks || is_reentry) {
            r->is_bad = true;
            break;
        }
        mx_ir_branch_t br = { .op = bop[0] };
        if (t0 != 0) {
            br.targets[0] = blocks[t0 - 1];
        }
        if (t1 != 0) {
            br.targets[1] = blocks[t1 - 1];
        }
        bool is_flow = br.op == MX_IR_BRANCH_OPCODE_FALL || br.op == MX_IR_BRANCH_OPCODE_JUMP;
        if ((has_enc[0] == 0 && !is_flow) || (is_flow && (t0 == 0 || t1 != 0))) {
            r->is_bad = true;
            break;
        } else if (has_enc[0] == 0) {
            block->branch_op = br.op;
            block->branch_targets[0] = br.targets[0];
            block->branch_targets[1] = br.targets[1];
            continue;
        }
        br.range = mx_dump_read_range(r);
        uint64_t out = mx_dump_read_uv(r);
        br.want = (uint32_t)mx_dump_read_uv(r);
        uint64_t nargs = mx_dump_read_uv(r);
        if (r->is_bad || out > nregs || nargs > MX_IR_BRANCH_MAX_ARGS) {
            r->is_bad = true;
            break;
        }
        if (out != 0) {
            br.out = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = out - 1 };
        }
        mx_ir_arg_t *args = mx_mem_alloc(sizeof(mx_ir_arg_t) * (nargs + 1));
        for (uint32_t a = 0; a < nargs; a++) {
            args[a] = mx_dump_read_arg(r, (uint32_t)nregs);
        }
        args[nargs].type = MX_IR_ARG_TYPE_NONE;
        br.args = args;
        r->is_bad = r->is_bad || !mx_dump_branch_ok(r, &br, args, (uint32_t)nargs);
        if (!r->is_bad) {
            mx_ir_keep_lits(vm, args);
            mx_ir_block_set_branch(vm, &vm->arenas.ir, block, br);
        }
        mx_mem_free(args);
    }
    mx_mem_free(blocks);
    r->depth -= 1;
    r->func = parent;
    if (r->is_bad) {
        return NULL;
    } else {
        return head;
    }
}

mx_dump_loaded_t mx_dump_load(mx_t *vm, const char *data, size_t len) {
    if (len < MX_DUMP_MAGIC_LEN || memcmp(data, MX_DUMP_MAGIC, MX_DUMP_MAGIC_LEN) != 0) {
        return (mx_dump_loaded_t) { .entry = NULL, .error = "not a MiniVM chunk" };
    }
    mx_dump_reader_t r = {
        .vm = vm,
        .p = (const uint8_t *)data + MX_DUMP_MAGIC_LEN,
        .end = (const uint8_t *)data + len,
    };
    const char *file = mx_dump_read_name(&r);
    uint64_t srclen = mx_dump_read_uv(&r);
    const uint8_t *src = mx_dump_read_bytes(&r, srclen);
    mx_ir_block_t *entry = NULL;
    if (!r.is_bad && file != NULL && srclen < UINT32_MAX) {
        char *copy = mx_ir_alloc_bytes(vm, srclen + 1);
        memcpy(copy, src, srclen);
        copy[srclen] = '\0';
        vm->arenas.ir.sources += (uint32_t)srclen + 1;
        r.source = mx_source_new(vm, file, copy, (uint32_t)srclen);
        entry = mx_dump_read_func(&r);
    }
    if (entry == NULL || r.p != r.end) {
        for (uint32_t i = 0; i < r.blocks.len; i++) {
            mx_mem_free(r.blocks.ptr[i]->code);
            r.blocks.ptr[i]->code = NULL;
            r.blocks.ptr[i]->code_len = 0;
        }
        r.blocks.len = 0;
        entry = NULL;
    }
    mx_ir_blocks_seal(vm, &r.blocks, 0);
    mx_mem_free(r.blocks.ptr);
    if (entry == NULL && r.is_truncated) {
        return (mx_dump_loaded_t) { .entry = NULL, .error = "truncated chunk" };
    } else if (entry == NULL) {
        return (mx_dump_loaded_t) { .entry = NULL, .error = "malformed chunk" };
    } else {
        return (mx_dump_loaded_t) { .entry = entry, .error = NULL };
    }
}
