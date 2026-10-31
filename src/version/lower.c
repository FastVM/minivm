
#include "lower.h"
#include "../value/obj.h"
#include "../value/string.h"

void mx_lower_reserve(mx_lower_t *l, size_t n) {
    if (l->w + n > l->end) {
        size_t used = (size_t)(l->w - l->start);
        size_t last;
        if (l->last_at != NULL) {
            last = (size_t)(l->last_at - l->start);
        } else {
            last = 0;
        }
        size_t cap = (size_t)(l->end - l->start) * 2 + n + 256;
        l->start = mx_mem_realloc(l->start, sizeof(uintptr_t) * cap);
        l->w = l->start + used;
        l->end = l->start + cap;
        if (l->last_at != NULL) {
            l->last_at = l->start + last;
        }
        l->vm->code_scratch.ptr = l->start;
        l->vm->code_scratch.cap = cap;
    }
}

void mx_lower_word(mx_lower_t *l, uintptr_t v) {
    mx_lower_reserve(l, 1);
    l->w[0] = v;
    l->w += 1;
}

void mx_lower_hop(mx_lower_t *l, mx_hop_t hop) {
    mx_lower_reserve(l, 1);
    l->last_at = l->w;
    l->w[0] = (uintptr_t)l->vm->hops[hop];
    l->w += 1;
}

mx_kind_t mx_lower_kind(mx_lower_t *l, mx_ir_arg_t arg) {
    if (arg.type == MX_IR_ARG_TYPE_LIT) {
        return arg.lit.kind;
    } else if (arg.type == MX_IR_ARG_TYPE_REG && arg.reg < l->nregs) {
        return mx_kind_plain(l->types[arg.reg]);
    } else {
        return MX_KIND_NIL;
    }
}

uintptr_t mx_lower_tag(mx_lower_t *l, mx_ir_arg_t arg) {
    uintptr_t kind = (uintptr_t)mx_lower_kind(l, arg) << 1;
    if (arg.type == MX_IR_ARG_TYPE_LIT) {
        return kind | 1;
    } else {
        return kind;
    }
}

uintptr_t mx_lower_value(mx_ir_arg_t arg) {
    if (arg.type == MX_IR_ARG_TYPE_LIT) {
        return (uintptr_t)arg.lit.val.bits;
    } else {
        return (uintptr_t)arg.reg;
    }
}

void mx_lower_arg(mx_lower_t *l, mx_ir_arg_t arg) {
    mx_lower_word(l, mx_lower_value(arg));
    mx_lower_word(l, mx_lower_tag(l, arg));
}

void mx_lower_hot(mx_lower_t *l) {
    mx_lower_word(l, MX_KIND_MIXED);
    mx_lower_word(l, 0);
}

void mx_lower_move_i(mx_lower_t *l, uint32_t out, mx_obj_t v) {
    mx_lower_hop(l, MX_HOP_MOVE_I);
    mx_lower_word(l, out);
    mx_lower_word(l, (uintptr_t)v.val.bits);
}

void mx_lower_write(mx_lower_t *l, uint32_t out, mx_kind_t kind) {
    if (!mx_kind_is_open(l->types[out])) {
        l->types[out] = kind;
    } else if (l->types[out] != (mx_kind_t)(MX_KIND_CELL | kind)) {
        mx_lower_hop(l, MX_HOP_SET_KIND);
        mx_lower_word(l, l->kslot + out);
        mx_lower_word(l, kind);
        l->types[out] = MX_KIND_CELL | kind;
    }
}

mx_lower_snap_t mx_lower_snapshot(mx_lower_t *l) {
    uint32_t len = l->nregs;
    while (len > 0 && l->types[len - 1] == MX_KIND_NIL) {
        len -= 1;
    }
    if (l->snap == NULL || len != l->nsnap || memcmp(l->snap, l->types, sizeof(mx_kind_t) * len) != 0) {
        l->snap = mx_version_kinds_intern(l->vm, l->types, len);
        l->nsnap = len;
    }
    return (mx_lower_snap_t) { .kinds = l->snap, .nkinds = len };
}

mx_resume_t *mx_lower_rec(mx_lower_t *l, mx_resume_t init) {
    mx_resume_t *rec = mx_arena_alloc(&l->vm->arenas.scratch, sizeof(mx_resume_t));
    if (l->recs.len == l->recs.cap) {
        l->recs.cap = l->recs.cap * 2 + 8;
        l->recs.ptr = mx_mem_realloc(l->recs.ptr, sizeof(mx_resume_t *) * l->recs.cap);
    }
    l->recs.ptr[l->recs.len] = rec;
    l->recs.len += 1;
    mx_lower_snap_t snap = mx_lower_snapshot(l);
    *rec = (mx_resume_t) {
        .want = init.want,
        .out = init.out,
        .kind = init.kind,
        .meta = init.meta,
        .has_cell_out = init.has_cell_out,
        .is_meta = init.is_meta,
        .nkinds = (uint16_t)snap.nkinds,
        .site_at = l->site,
        .resume_at = init.resume_at,
        .rb = l->rb,
        .kinds = snap.kinds,
        .site = l->block,
        .target = init.target,
        .target2 = init.target2,
    };
    l->vm->stats.version_bytes += sizeof(mx_resume_t);
    l->vm->stats.recs[init.kind] += 1;
    return rec;
}

bool mx_lower_has_cell(mx_lower_t *l, uint32_t out, uint32_t count) {
    for (uint32_t r = out; r < out + count && r < l->nregs; r++) {
        if (mx_kind_is_open(l->types[r])) {
            return true;
        }
    }
    return false;
}

uint32_t mx_lower_rec_index(const mx_lower_t *l, const mx_resume_t *rec) {
    uint32_t i = l->recs.len - 1;
    while (l->recs.ptr[i] != rec) {
        i -= 1;
    }
    return i;
}

void mx_lower_fixup_push(mx_lower_fixups_t *fixups, uint32_t rec, size_t at) {
    if (fixups->len == fixups->cap) {
        fixups->cap = fixups->cap * 2 + 8;
        fixups->ptr = mx_mem_realloc(fixups->ptr, sizeof(mx_lower_fixup_t) * fixups->cap);
    }
    fixups->ptr[fixups->len] = (mx_lower_fixup_t) { .rec = rec, .at = at };
    fixups->len += 1;
}

void mx_lower_rec_word(mx_lower_t *l, mx_resume_t *rec) {
    mx_lower_fixup_push(&l->refs, mx_lower_rec_index(l, rec), (size_t)(l->w - l->start));
    mx_lower_word(l, 0);
}

void mx_lower_void(mx_lower_t *l) {
    mx_lower_snap_t snap = mx_lower_snapshot(l);
    mx_lower_word(l, 0);
    mx_lower_word(l, UINTPTR_MAX);
    mx_lower_word(l, (uintptr_t)snap.kinds);
    mx_lower_word(l, snap.nkinds);
    mx_lower_word(l, (uintptr_t)l->block);
    mx_lower_word(l, (uintptr_t)l->site | ((uintptr_t)l->next_at << 32));
}

uint32_t mx_lower_unknown_reg(mx_lower_t *l, mx_ir_arg_t *args, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        if (args[i].type == MX_IR_ARG_TYPE_REG && args[i].reg < l->nregs && !mx_kind_is_known(l->types[args[i].reg])) {
            return (uint32_t)args[i].reg;
        }
    }
    return UINT32_MAX;
}

void mx_lower_cell(mx_lower_t *l, uint32_t reg) {
    mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
        .out = (uint16_t)reg,
        .kind = MX_RESUME_AT,
        .resume_at = l->at,
        .target = l->block,
    });
    mx_lower_hop(l, MX_HOP_CELL);
    mx_lower_word(l, reg);
    mx_lower_word(l, l->kslot + reg);
    mx_lower_rec_word(l, rec);
}

void mx_lower_hook(mx_lower_t *l, uintptr_t event) {
    mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
        .kind = MX_RESUME_ENTRY,
        .resume_at = l->at,
        .target = l->block,
    });
    mx_lower_hop(l, MX_HOP_HOOK);
    mx_lower_rec_word(l, rec);
    mx_lower_word(l, event);
}

mx_op_t mx_lower_op_of(mx_ir_instr_opcode_t op) {
    switch (op) {
        case MX_IR_INSTR_OPCODE_ADD: {
            return MX_OP_ADD;
        }
        case MX_IR_INSTR_OPCODE_SUB: {
            return MX_OP_SUB;
        }
        case MX_IR_INSTR_OPCODE_MUL: {
            return MX_OP_MUL;
        }
        case MX_IR_INSTR_OPCODE_DIV: {
            return MX_OP_DIV;
        }
        case MX_IR_INSTR_OPCODE_IDIV: {
            return MX_OP_IDIV;
        }
        case MX_IR_INSTR_OPCODE_MOD: {
            return MX_OP_MOD;
        }
        case MX_IR_INSTR_OPCODE_POW: {
            return MX_OP_POW;
        }
        case MX_IR_INSTR_OPCODE_CONCAT: {
            return MX_OP_CONCAT;
        }
        case MX_IR_INSTR_OPCODE_BAND: {
            return MX_OP_BAND;
        }
        case MX_IR_INSTR_OPCODE_BOR: {
            return MX_OP_BOR;
        }
        case MX_IR_INSTR_OPCODE_BXOR: {
            return MX_OP_BXOR;
        }
        case MX_IR_INSTR_OPCODE_SHL: {
            return MX_OP_SHL;
        }
        case MX_IR_INSTR_OPCODE_SHR: {
            return MX_OP_SHR;
        }
        case MX_IR_INSTR_OPCODE_NEG: {
            return MX_OP_UNM;
        }
        case MX_IR_INSTR_OPCODE_BNOT: {
            return MX_OP_BNOT;
        }
        default: {
            return MX_OP_LEN;
        }
    }
}

void mx_lower_meta_op(mx_lower_t *l, mx_op_t op, mx_ir_instr_t *in, uint32_t nargs) {
    mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
        .out = (uint16_t)in->out.reg,
        .kind = MX_RESUME_AT,
        .meta = op,
        .has_cell_out = mx_lower_has_cell(l, (uint32_t)in->out.reg, 1),
        .is_meta = true,
        .resume_at = l->next_at,
        .target = l->block,
    });
    mx_lower_hop(l, MX_HOP_META_OP);
    mx_lower_word(l, op);
    mx_lower_word(l, nargs);
    mx_lower_rec_word(l, rec);
    for (uint32_t i = 0; i < nargs; i++) {
        mx_lower_arg(l, in->args[i]);
    }
}

mx_kind_t mx_lower_arith_kind(mx_ir_instr_opcode_t op, mx_kind_t a, mx_kind_t b) {
    if (!mx_kind_is_number(a) || !mx_kind_is_number(b)) {
        return MX_KIND_NIL;
    }
    switch (op) {
        case MX_IR_INSTR_OPCODE_DIV:
        case MX_IR_INSTR_OPCODE_POW: {
            return MX_KIND_FLOAT;
        }
        case MX_IR_INSTR_OPCODE_BAND:
        case MX_IR_INSTR_OPCODE_BOR:
        case MX_IR_INSTR_OPCODE_BXOR:
        case MX_IR_INSTR_OPCODE_SHL:
        case MX_IR_INSTR_OPCODE_SHR: {
            if (a == MX_KIND_INT && b == MX_KIND_INT) {
                return MX_KIND_INT;
            } else {
                return MX_KIND_NIL;
            }
        }
        default: {
            if (a == MX_KIND_INT && b == MX_KIND_INT) {
                return MX_KIND_INT;
            } else {
                return MX_KIND_FLOAT;
            }
        }
    }
}

mx_hop_t mx_lower_binop_hop(mx_ir_instr_opcode_t op, mx_kind_t want) {
    if (want == MX_KIND_INT) {
        switch (op) {
            case MX_IR_INSTR_OPCODE_ADD: {
                return MX_HOP_ADD_I64_RR;
            }
            case MX_IR_INSTR_OPCODE_SUB: {
                return MX_HOP_SUB_I64_RR;
            }
            case MX_IR_INSTR_OPCODE_MUL: {
                return MX_HOP_MUL_I64_RR;
            }
            case MX_IR_INSTR_OPCODE_IDIV: {
                return MX_HOP_IDIV_I64_RR;
            }
            case MX_IR_INSTR_OPCODE_MOD: {
                return MX_HOP_MOD_I64_RR;
            }
            case MX_IR_INSTR_OPCODE_BAND: {
                return MX_HOP_BAND_I64_RR;
            }
            case MX_IR_INSTR_OPCODE_BOR: {
                return MX_HOP_BOR_I64_RR;
            }
            case MX_IR_INSTR_OPCODE_BXOR: {
                return MX_HOP_BXOR_I64_RR;
            }
            case MX_IR_INSTR_OPCODE_SHL: {
                return MX_HOP_SHL_I64_RR;
            }
            default: {
                return MX_HOP_SHR_I64_RR;
            }
        }
    } else {
        switch (op) {
            case MX_IR_INSTR_OPCODE_ADD: {
                return MX_HOP_ADD_F64_RR;
            }
            case MX_IR_INSTR_OPCODE_SUB: {
                return MX_HOP_SUB_F64_RR;
            }
            case MX_IR_INSTR_OPCODE_MUL: {
                return MX_HOP_MUL_F64_RR;
            }
            case MX_IR_INSTR_OPCODE_DIV: {
                return MX_HOP_DIV_F64_RR;
            }
            case MX_IR_INSTR_OPCODE_IDIV: {
                return MX_HOP_IDIV_F64_RR;
            }
            case MX_IR_INSTR_OPCODE_MOD: {
                return MX_HOP_MOD_F64_RR;
            }
            default: {
                return MX_HOP_POW_F64_RR;
            }
        }
    }
}

mx_ir_arg_t mx_lower_as_float(mx_lower_t *l, mx_ir_arg_t arg, uint32_t scratch) {
    if (arg.type == MX_IR_ARG_TYPE_LIT && mx_obj_is_int(arg.lit)) {
        return (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_LIT, .lit = mx_obj_of_number((double)mx_obj_get_int(arg.lit)) };
    } else if (arg.type == MX_IR_ARG_TYPE_REG && mx_lower_kind(l, arg) == MX_KIND_INT) {
        mx_lower_hop(l, MX_HOP_TO_F64);
        mx_lower_word(l, scratch);
        mx_lower_word(l, (uintptr_t)arg.reg);
        return (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = scratch };
    } else {
        return arg;
    }
}

void mx_lower_arith(mx_lower_t *l, mx_ir_instr_t *in, mx_kind_t want) {
    mx_ir_arg_t a = in->args[0];
    mx_ir_arg_t b = in->args[1];
    uint32_t out = (uint32_t)in->out.reg;
    bool is_div = in->op == MX_IR_INSTR_OPCODE_DIV;
    bool is_pow = in->op == MX_IR_INSTR_OPCODE_POW;
    bool is_ints = mx_lower_kind(l, a) == MX_KIND_INT && mx_lower_kind(l, b) == MX_KIND_INT;
    if ((is_div || is_pow) && is_ints && a.type == MX_IR_ARG_TYPE_REG && b.type == MX_IR_ARG_TYPE_REG) {
        if (is_div) {
            mx_lower_hop(l, MX_HOP_DIV_INT_INT);
        } else {
            mx_lower_hop(l, MX_HOP_POW_INT_INT);
        }
        mx_lower_word(l, out);
        mx_lower_word(l, (uintptr_t)a.reg);
        mx_lower_word(l, (uintptr_t)b.reg);
        mx_lower_write(l, out, want);
        return;
    }
    mx_ir_arg_t fa;
    mx_ir_arg_t fb;
    if (want == MX_KIND_FLOAT) {
        fa = mx_lower_as_float(l, a, l->frame);
        fb = mx_lower_as_float(l, b, l->frame + 1);
    } else {
        fa = a;
        fb = b;
    }
    mx_ir_arg_t ra;
    if (fa.type == MX_IR_ARG_TYPE_LIT && fb.type == MX_IR_ARG_TYPE_LIT) {
        mx_lower_move_i(l, l->frame, fa.lit);
        ra = (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = l->frame };
    } else {
        ra = fa;
    }
    mx_hop_t hop = mx_lower_binop_hop(in->op, want);
    bool can_fail = want == MX_KIND_INT && (in->op == MX_IR_INSTR_OPCODE_IDIV || in->op == MX_IR_INSTR_OPCODE_MOD);
    mx_resume_t *rec;
    if (can_fail) {
        rec = mx_lower_rec(l, (mx_resume_t) {
            .kind = MX_RESUME_ENTRY,
            .resume_at = l->at,
            .target = l->block,
        });
    } else {
        rec = NULL;
    }
    if (ra.type == MX_IR_ARG_TYPE_REG && fb.type == MX_IR_ARG_TYPE_REG) {
        mx_lower_hop(l, hop);
        mx_lower_word(l, out);
        mx_lower_word(l, (uintptr_t)ra.reg);
        mx_lower_word(l, (uintptr_t)fb.reg);
    } else if (ra.type == MX_IR_ARG_TYPE_REG) {
        mx_lower_hop(l, hop + 1);
        mx_lower_word(l, out);
        mx_lower_word(l, (uintptr_t)ra.reg);
        mx_lower_word(l, (uintptr_t)fb.lit.val.bits);
    } else {
        mx_lower_hop(l, hop + 2);
        mx_lower_word(l, out);
        mx_lower_word(l, (uintptr_t)fb.reg);
        mx_lower_word(l, (uintptr_t)ra.lit.val.bits);
    }
    if (can_fail) {
        mx_lower_rec_word(l, rec);
    }
    mx_lower_write(l, out, want);
}

mx_obj_t mx_lower_lit_string(mx_lower_t *l, mx_obj_t v) {
    if (mx_obj_is_string(v)) {
        return v;
    } else {
        mx_str_t *buf = mx_str_new();
        if (mx_obj_is_int(v)) {
            mx_str_append_int(buf, mx_obj_get_int(v));
        } else {
            mx_str_float(buf, mx_obj_get_float(v));
        }
        mx_obj_t s = mx_obj_intern_strn(l->vm, buf->buf, buf->len);
        mx_str_free(buf);
        mx_vm_keep(l->vm, s);
        return s;
    }
}

mx_ir_arg_t mx_lower_to_string(mx_lower_t *l, mx_ir_arg_t arg, uint32_t scratch) {
    if (arg.type == MX_IR_ARG_TYPE_LIT) {
        mx_lower_move_i(l, scratch, mx_lower_lit_string(l, arg.lit));
        return (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = scratch };
    }
    mx_kind_t kind = mx_lower_kind(l, arg);
    if (kind == MX_KIND_STRING) {
        return arg;
    } else {
        if (kind == MX_KIND_INT) {
            mx_lower_hop(l, MX_HOP_TOSTR_I64);
        } else {
            mx_lower_hop(l, MX_HOP_TOSTR_F64);
        }
        mx_lower_word(l, scratch);
        mx_lower_word(l, (uintptr_t)arg.reg);
        return (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = scratch };
    }
}

bool mx_lower_is_int_reg(mx_lower_t *l, mx_ir_arg_t arg) {
    return arg.type == MX_IR_ARG_TYPE_REG && mx_lower_kind(l, arg) == MX_KIND_INT;
}

void mx_lower_concat(mx_lower_t *l, mx_ir_instr_t *in) {
    bool is_si = !mx_lower_is_int_reg(l, in->args[0]) && mx_lower_is_int_reg(l, in->args[1]);
    bool is_is = mx_lower_is_int_reg(l, in->args[0]) && !mx_lower_is_int_reg(l, in->args[1]);
    mx_ir_arg_t a;
    if (is_is) {
        a = in->args[0];
    } else {
        a = mx_lower_to_string(l, in->args[0], l->frame);
    }
    mx_ir_arg_t b;
    if (is_si) {
        b = in->args[1];
    } else {
        b = mx_lower_to_string(l, in->args[1], l->frame + 1);
    }
    if (is_si) {
        mx_lower_hop(l, MX_HOP_CONCAT_SI);
    } else if (is_is) {
        mx_lower_hop(l, MX_HOP_CONCAT_IS);
    } else {
        mx_lower_hop(l, MX_HOP_CONCAT);
    }
    mx_lower_word(l, (uintptr_t)in->out.reg);
    mx_lower_word(l, (uintptr_t)a.reg);
    mx_lower_word(l, (uintptr_t)b.reg);
    mx_lower_write(l, (uint32_t)in->out.reg, MX_KIND_STRING);
}

bool mx_lower_needs_meta(mx_lower_t *l, mx_ir_instr_t *in, mx_kind_t a, mx_kind_t b) {
    switch (in->op) {
        case MX_IR_INSTR_OPCODE_TABLE_LEN: {
            return a != MX_KIND_STRING && a != l->vm->kinds.table;
        }
        case MX_IR_INSTR_OPCODE_ADD:
        case MX_IR_INSTR_OPCODE_SUB:
        case MX_IR_INSTR_OPCODE_MUL:
        case MX_IR_INSTR_OPCODE_DIV:
        case MX_IR_INSTR_OPCODE_IDIV:
        case MX_IR_INSTR_OPCODE_MOD:
        case MX_IR_INSTR_OPCODE_POW:
        case MX_IR_INSTR_OPCODE_BAND:
        case MX_IR_INSTR_OPCODE_BOR:
        case MX_IR_INSTR_OPCODE_BXOR:
        case MX_IR_INSTR_OPCODE_SHL:
        case MX_IR_INSTR_OPCODE_SHR: {
            return mx_lower_arith_kind(in->op, a, b) == MX_KIND_NIL;
        }
        case MX_IR_INSTR_OPCODE_CONCAT: {
            return !(a == MX_KIND_STRING || mx_kind_is_number(a)) || !(b == MX_KIND_STRING || mx_kind_is_number(b));
        }
        case MX_IR_INSTR_OPCODE_NEG: {
            return !mx_kind_is_number(a);
        }
        case MX_IR_INSTR_OPCODE_BNOT: {
            return a != MX_KIND_INT;
        }
        default: {
            return false;
        }
    }
}

void mx_lower_closure(mx_lower_t *l, mx_ir_instr_t *in) {
    mx_ir_block_t *child = mx_obj_get_block(in->args[0].lit);
    const mx_ir_func_t *func = child->func;
    mx_lower_hop(l, MX_HOP_CLOSURE);
    mx_lower_word(l, (uintptr_t)in->out.reg);
    mx_lower_word(l, (uintptr_t)child);
    mx_lower_word(l, l->kslot);
    for (uint32_t i = 0; i < func->nupvals; i++) {
        const mx_ir_upval_t *up = &func->upvals[i];
        if (!up->is_local) {
            mx_lower_word(l, MX_KIND_SLOT);
        } else if (l->types[up->index] == MX_KIND_SLOT) {
            mx_lower_word(l, MX_KIND_SLOT);
            l->types[up->index] = MX_KIND_CELL_ANY;
        } else if (mx_kind_is_cell(l->types[up->index])) {
            mx_lower_word(l, MX_KIND_SLOT);
        } else {
            mx_lower_word(l, l->types[up->index]);
            l->types[up->index] = MX_KIND_CELL | l->types[up->index];
        }
    }
    mx_lower_write(l, (uint32_t)in->out.reg, MX_KIND_CLOSURE);
}

void mx_lower_instr(mx_lower_t *l, mx_ir_instr_t *in) {
    if (in->out.type == MX_IR_ARG_TYPE_REG && in->out.reg < l->nregs) {
        l->fresh[in->out.reg] = false;
    }
    switch (in->op) {
        case MX_IR_INSTR_OPCODE_MOVE: {
            mx_ir_arg_t a = in->args[0];
            if (a.type == MX_IR_ARG_TYPE_LIT) {
                mx_lower_move_i(l, (uint32_t)in->out.reg, a.lit);
            } else if (a.reg != in->out.reg) {
                mx_lower_hop(l, MX_HOP_MOVE_R);
                mx_lower_word(l, (uintptr_t)in->out.reg);
                mx_lower_word(l, (uintptr_t)a.reg);
            }
            mx_lower_write(l, (uint32_t)in->out.reg, mx_lower_kind(l, a));
            break;
        }
        case MX_IR_INSTR_OPCODE_CONCAT: {
            mx_lower_concat(l, in);
            break;
        }
        case MX_IR_INSTR_OPCODE_NEG: {
            mx_kind_t kind = mx_lower_kind(l, in->args[0]);
            if (in->args[0].type == MX_IR_ARG_TYPE_LIT) {
                mx_obj_t v = in->args[0].lit;
                if (kind == MX_KIND_INT) {
                    mx_lower_move_i(l, (uint32_t)in->out.reg, mx_obj_of_int((int64_t)(0 - (uint64_t)mx_obj_get_int(v))));
                } else {
                    mx_lower_move_i(l, (uint32_t)in->out.reg, mx_obj_of_number(-mx_obj_get_float(v)));
                }
            } else {
                if (kind == MX_KIND_INT) {
                    mx_lower_hop(l, MX_HOP_NEG_I64);
                } else {
                    mx_lower_hop(l, MX_HOP_NEG_F64);
                }
                mx_lower_word(l, (uintptr_t)in->out.reg);
                mx_lower_word(l, (uintptr_t)in->args[0].reg);
            }
            mx_lower_write(l, (uint32_t)in->out.reg, kind);
            break;
        }
        case MX_IR_INSTR_OPCODE_BNOT: {
            if (in->args[0].type == MX_IR_ARG_TYPE_LIT) {
                mx_lower_move_i(l, (uint32_t)in->out.reg, mx_obj_of_int((int64_t)~(uint64_t)mx_obj_get_int(in->args[0].lit)));
            } else {
                mx_lower_hop(l, MX_HOP_BNOT_I64);
                mx_lower_word(l, (uintptr_t)in->out.reg);
                mx_lower_word(l, (uintptr_t)in->args[0].reg);
            }
            mx_lower_write(l, (uint32_t)in->out.reg, MX_KIND_INT);
            break;
        }
        case MX_IR_INSTR_OPCODE_TABLE_LEN: {
            mx_ir_arg_t a = in->args[0];
            uint32_t out = (uint32_t)in->out.reg;
            if (a.type == MX_IR_ARG_TYPE_LIT) {
                mx_lower_move_i(l, out, mx_obj_of_int((int64_t)mx_obj_get_string(a.lit)->len));
            } else if (mx_lower_kind(l, a) == MX_KIND_STRING) {
                mx_lower_hop(l, MX_HOP_LEN_STR);
                mx_lower_word(l, out);
                mx_lower_word(l, (uintptr_t)a.reg);
            } else {
                mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
                    .out = (uint16_t)out,
                    .kind = MX_RESUME_AT,
                    .meta = MX_OP_LEN,
                    .has_cell_out = mx_lower_has_cell(l, out, 1),
                    .is_meta = true,
                    .resume_at = l->next_at,
                    .target = l->block,
                });
                mx_lower_hop(l, MX_HOP_LEN_TABLE);
                mx_lower_word(l, out);
                mx_lower_word(l, (uintptr_t)a.reg);
                mx_lower_rec_word(l, rec);
            }
            mx_lower_write(l, out, MX_KIND_INT);
            break;
        }
        case MX_IR_INSTR_OPCODE_TABLE_NEW: {
            mx_lower_hop(l, MX_HOP_TABLE_NEW);
            mx_lower_word(l, (uintptr_t)in->out.reg);
            mx_lower_word(l, (uintptr_t)mx_obj_get_int(in->args[0].lit));
            mx_lower_write(l, (uint32_t)in->out.reg, l->vm->kinds.table);
            l->fresh[in->out.reg] = !l->is_hooked;
            break;
        }
        case MX_IR_INSTR_OPCODE_TABLE_SET: {
            mx_ir_arg_t t = in->args[0];
            mx_ir_arg_t k = in->args[1];
            bool is_fresh = t.type == MX_IR_ARG_TYPE_REG && t.reg < l->nregs && l->fresh[t.reg] && k.type == MX_IR_ARG_TYPE_LIT && !mx_obj_is_nil(k.lit);
            if (is_fresh && mx_obj_is_string(k.lit)) {
                mx_lower_hop(l, MX_HOP_TABLE_INIT_SK);
                mx_lower_word(l, (uintptr_t)t.reg);
                mx_lower_word(l, (uintptr_t)k.lit.val.bits);
                mx_lower_arg(l, in->args[2]);
                mx_lower_word(l, UINTPTR_MAX);
                break;
            } else if (is_fresh) {
                mx_lower_hop(l, MX_HOP_TABLE_INIT);
                mx_lower_word(l, (uintptr_t)t.reg);
                mx_lower_arg(l, k);
                mx_lower_arg(l, in->args[2]);
                break;
            }
            memset(l->fresh, 0, sizeof(bool) * l->nregs);
            if (t.type == MX_IR_ARG_TYPE_REG && mx_lower_kind(l, t) == l->vm->kinds.table && k.type == MX_IR_ARG_TYPE_LIT && mx_obj_is_string(k.lit)) {
                mx_lower_hop(l, MX_HOP_TABLE_SET_SK);
                mx_lower_word(l, (uintptr_t)t.reg);
                mx_lower_word(l, (uintptr_t)k.lit.val.bits);
            } else {
                mx_lower_hop(l, MX_HOP_TABLE_SET);
                mx_lower_arg(l, t);
                mx_lower_arg(l, k);
            }
            mx_lower_arg(l, in->args[2]);
            mx_lower_void(l);
            break;
        }
        case MX_IR_INSTR_OPCODE_CLOSURE: {
            mx_lower_closure(l, in);
            break;
        }
        case MX_IR_INSTR_OPCODE_VARARGS: {
            mx_lower_hop(l, MX_HOP_VARARGS);
            mx_lower_word(l, (uintptr_t)in->out.reg);
            mx_lower_word(l, !mx_ir_func_has_hidden_pack(l->block->func));
            mx_lower_write(l, (uint32_t)in->out.reg, l->vm->kinds.table);
            break;
        }
        case MX_IR_INSTR_OPCODE_UNPACK: {
            mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
                .kind = MX_RESUME_AT,
                .resume_at = l->at,
                .target = l->block,
            });
            mx_lower_hop(l, MX_HOP_UNPACK);
            mx_lower_word(l, (uintptr_t)in->args[0].reg);
            mx_lower_rec_word(l, rec);
            mx_lower_word(l, !mx_ir_func_has_hidden_pack(l->block->func));
            break;
        }
        case MX_IR_INSTR_OPCODE_UPVAL_SET: {
            mx_lower_hop(l, MX_HOP_UPVAL_SET);
            mx_lower_word(l, (uintptr_t)mx_obj_get_int(in->args[0].lit));
            mx_lower_arg(l, in->args[1]);
            break;
        }
        case MX_IR_INSTR_OPCODE_CLOSE: {
            uint32_t from = (uint32_t)mx_obj_get_int(in->args[0].lit);
            bool is_open = false;
            for (uint32_t r = from; r < l->nregs; r++) {
                if (l->types[r] == MX_KIND_CELL_ANY) {
                    is_open = true;
                    l->types[r] = MX_KIND_SLOT;
                } else if (mx_kind_is_open(l->types[r])) {
                    is_open = true;
                    l->types[r] = mx_kind_plain(l->types[r]);
                }
            }
            if (is_open) {
                mx_lower_hop(l, MX_HOP_CLOSE);
                mx_lower_word(l, from);
            }
            break;
        }
        case MX_IR_INSTR_OPCODE_KEEP: {
            break;
        }
        default: {
            mx_kind_t want = mx_lower_arith_kind(in->op, mx_lower_kind(l, in->args[0]), mx_lower_kind(l, in->args[1]));
            mx_lower_arith(l, in, want);
            break;
        }
    }
}

void mx_lower_goto(mx_lower_t *l, mx_ir_block_t *target) {
    mx_lower_hop(l, MX_HOP_GOTO);
    mx_lower_word(l, (uintptr_t)target);
    mx_lower_word(l, 0);
}

void mx_lower_cmp_hop(mx_lower_t *l, mx_hop_t hop, mx_ir_branch_t *br, uintptr_t a, uintptr_t b) {
    mx_lower_hop(l, hop);
    mx_lower_word(l, (uintptr_t)br->targets[0]);
    mx_lower_word(l, (uintptr_t)br->targets[1]);
    mx_lower_word(l, 0);
    mx_lower_word(l, 0);
    mx_lower_word(l, a);
    mx_lower_word(l, b);
}

mx_ir_arg_t mx_lower_lit_reg(mx_lower_t *l, mx_ir_arg_t arg, uint32_t scratch) {
    if (arg.type == MX_IR_ARG_TYPE_LIT) {
        mx_lower_move_i(l, scratch, arg.lit);
        return (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_REG, .reg = scratch };
    } else {
        return arg;
    }
}

void mx_lower_cmp_form(mx_lower_t *l, mx_hop_t rr, mx_ir_branch_t *br, mx_ir_arg_t a, mx_ir_arg_t b) {
    if (a.type == MX_IR_ARG_TYPE_REG && b.type == MX_IR_ARG_TYPE_REG) {
        mx_lower_cmp_hop(l, rr, br, (uintptr_t)a.reg, (uintptr_t)b.reg);
    } else if (a.type == MX_IR_ARG_TYPE_REG) {
        mx_lower_cmp_hop(l, rr + 1, br, (uintptr_t)a.reg, (uintptr_t)b.lit.val.bits);
    } else if (b.type == MX_IR_ARG_TYPE_REG) {
        mx_lower_cmp_hop(l, rr + 2, br, (uintptr_t)b.reg, (uintptr_t)a.lit.val.bits);
    } else {
        mx_ir_arg_t ra = mx_lower_lit_reg(l, a, l->frame);
        mx_lower_cmp_hop(l, rr + 1, br, (uintptr_t)ra.reg, (uintptr_t)b.lit.val.bits);
    }
}

void mx_lower_meta_cmp(mx_lower_t *l, mx_ir_branch_t *br, mx_op_t op, mx_ir_arg_t a, mx_ir_arg_t b) {
    mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
        .kind = MX_RESUME_BRANCH,
        .meta = op,
        .is_meta = true,
        .resume_at = l->at,
        .target = br->targets[0],
        .target2 = br->targets[1],
    });
    mx_lower_hop(l, MX_HOP_META_CMP);
    mx_lower_word(l, op);
    mx_lower_rec_word(l, rec);
    mx_lower_arg(l, a);
    mx_lower_arg(l, b);
}

mx_ir_arg_t mx_lower_exact_float(mx_ir_arg_t arg) {
    if (arg.type == MX_IR_ARG_TYPE_LIT && (mx_obj_is_float(arg.lit) || (mx_obj_get_int(arg.lit) >= -((int64_t)1 << 53) && mx_obj_get_int(arg.lit) <= ((int64_t)1 << 53)))) {
        return (mx_ir_arg_t) { .type = MX_IR_ARG_TYPE_LIT, .lit = mx_obj_of_number(mx_obj_get_number(arg.lit)) };
    } else {
        return arg;
    }
}

void mx_lower_compare(mx_lower_t *l, mx_ir_branch_t *br, mx_ir_arg_t a, mx_ir_arg_t b) {
    mx_kind_t ka = mx_lower_kind(l, a);
    mx_kind_t kb = mx_lower_kind(l, b);
    uint32_t rel;
    mx_op_t mop;
    if (br->op == MX_IR_BRANCH_OPCODE_IF_LT) {
        rel = 0;
        mop = MX_OP_LT;
    } else if (br->op == MX_IR_BRANCH_OPCODE_IF_LE) {
        rel = 1;
        mop = MX_OP_LE;
    } else {
        rel = 2;
        mop = MX_OP_EQ;
    }
    if (ka == MX_KIND_INT && kb == MX_KIND_INT) {
        mx_lower_cmp_form(l, MX_HOP_BR_LT_I64_RR + 3 * rel, br, a, b);
    } else if (ka == MX_KIND_FLOAT && kb == MX_KIND_FLOAT) {
        mx_lower_cmp_form(l, MX_HOP_BR_LT_F64_RR + 3 * rel, br, a, b);
    } else if (mx_kind_is_number(ka) && mx_kind_is_number(kb)) {
        mx_ir_arg_t fa = mx_lower_exact_float(a);
        mx_ir_arg_t fb = mx_lower_exact_float(b);
        mx_kind_t fka = mx_lower_kind(l, fa);
        mx_kind_t fkb = mx_lower_kind(l, fb);
        if (fka == MX_KIND_FLOAT && fkb == MX_KIND_FLOAT) {
            mx_lower_cmp_form(l, MX_HOP_BR_LT_F64_RR + 3 * rel, br, fa, fb);
        } else {
            mx_ir_arg_t ra = mx_lower_lit_reg(l, fa, l->frame);
            mx_ir_arg_t rb = mx_lower_lit_reg(l, fb, l->frame + 1);
            if (fka == MX_KIND_INT) {
                mx_lower_cmp_hop(l, MX_HOP_BR_LT_INT_F64 + rel, br, (uintptr_t)ra.reg, (uintptr_t)rb.reg);
            } else if (rel == 2) {
                mx_lower_cmp_hop(l, MX_HOP_BR_EQ_INT_F64, br, (uintptr_t)rb.reg, (uintptr_t)ra.reg);
            } else {
                mx_lower_cmp_hop(l, MX_HOP_BR_LT_F64_INT + rel, br, (uintptr_t)ra.reg, (uintptr_t)rb.reg);
            }
        }
    } else if (ka == MX_KIND_STRING && kb == MX_KIND_STRING) {
        mx_ir_arg_t ra = mx_lower_lit_reg(l, a, l->frame);
        mx_ir_arg_t rb = mx_lower_lit_reg(l, b, l->frame + 1);
        mx_lower_cmp_hop(l, MX_HOP_BR_LT_STR + rel, br, (uintptr_t)ra.reg, (uintptr_t)rb.reg);
    } else if (rel == 2 && ka != kb && (ka < MX_KIND_FIRST_DATA || kb < MX_KIND_FIRST_DATA)) {
        mx_lower_goto(l, br->targets[1]);
    } else if (rel == 2 && ka == MX_KIND_NIL) {
        mx_lower_goto(l, br->targets[0]);
    } else if (rel == 2 && ka < MX_KIND_FIRST_DATA && ka != MX_KIND_ERROR) {
        mx_ir_arg_t ra = mx_lower_lit_reg(l, a, l->frame);
        mx_ir_arg_t rb = mx_lower_lit_reg(l, b, l->frame + 1);
        mx_lower_cmp_hop(l, MX_HOP_BR_EQ_RAW, br, (uintptr_t)ra.reg, (uintptr_t)rb.reg);
    } else {
        mx_lower_meta_cmp(l, br, mop, a, b);
    }
}

void mx_lower_args(mx_lower_t *l, mx_ir_arg_t *args, uint32_t from, uint32_t nargs) {
    for (uint32_t i = from; i < nargs; i++) {
        if (args[i].type != MX_IR_ARG_TYPE_SPREAD) {
            mx_lower_arg(l, args[i]);
        }
    }
}

void mx_lower_close_all(mx_lower_t *l) {
    for (uint32_t r = 0; r < l->nregs; r++) {
        if (mx_kind_is_open(l->types[r])) {
            mx_lower_hop(l, MX_HOP_CLOSE);
            mx_lower_word(l, 0);
            return;
        }
    }
}

void mx_lower_call(mx_lower_t *l, mx_ir_branch_t *br, mx_ir_arg_t *args, uint32_t nargs) {
    bool is_spread = nargs > 1 && args[nargs - 1].type == MX_IR_ARG_TYPE_SPREAD;
    uint32_t n;
    if (is_spread) {
        n = nargs - 2;
    } else {
        n = nargs - 1;
    }
    mx_ir_arg_t fn = args[0];
    bool is_lua = fn.type == MX_IR_ARG_TYPE_REG && mx_lower_kind(l, fn) == MX_KIND_CLOSURE;
    bool is_tail = br->targets[0] == NULL;
    if (is_tail && is_lua) {
        mx_lower_close_all(l);
        if (l->is_hooked) {
            mx_lower_hook(l, 3);
        }
        mx_lower_hop(l, MX_HOP_TAILCALL_LUA);
        mx_lower_word(l, (uintptr_t)fn.reg);
        mx_lower_word(l, n);
        mx_lower_word(l, is_spread);
        for (uint32_t i = 0; i < MX_RESUME_CACHE_WORDS; i++) {
            mx_lower_word(l, 0);
        }
        mx_lower_args(l, args, 1, nargs);
        return;
    }
    mx_resume_t *rec;
    if (is_tail) {
        mx_lower_close_all(l);
        rec = mx_lower_rec(l, (mx_resume_t) {
            .want = MX_IR_WANT_MULTI,
            .kind = MX_RESUME_RETURN,
            .resume_at = l->at,
            .target = l->block,
        });
    } else if (br->out.type == MX_IR_ARG_TYPE_REG) {
        rec = mx_lower_rec(l, (mx_resume_t) {
            .want = br->want,
            .out = (uint16_t)br->out.reg,
            .kind = MX_RESUME_VALUES,
            .has_cell_out = mx_lower_has_cell(l, (uint32_t)br->out.reg, br->want),
            .resume_at = l->at,
            .target = br->targets[0],
        });
    } else {
        rec = mx_lower_rec(l, (mx_resume_t) {
            .want = br->want,
            .kind = MX_RESUME_VALUES,
            .resume_at = l->at,
            .target = br->targets[0],
        });
    }
    if (is_lua) {
        mx_lower_hop(l, MX_HOP_CALL_LUA);
        mx_lower_word(l, (uintptr_t)fn.reg);
        mx_lower_word(l, n);
        mx_lower_word(l, is_spread);
        mx_lower_rec_word(l, rec);
        for (uint32_t i = 0; i < MX_RESUME_CACHE_WORDS; i++) {
            mx_lower_word(l, 0);
        }
        mx_lower_args(l, args, 1, nargs);
        return;
    }
    mx_kind_t fk = mx_lower_kind(l, fn);
    bool is_callable = fn.type == MX_IR_ARG_TYPE_REG && !is_spread && (fk == l->vm->kinds.table || fk == l->vm->kinds.udata);
    if (is_callable) {
        mx_lower_hop(l, MX_HOP_CALL_META);
    } else {
        mx_lower_hop(l, MX_HOP_CALL);
    }
    mx_lower_arg(l, fn);
    mx_lower_word(l, n);
    mx_lower_word(l, is_spread);
    mx_lower_rec_word(l, rec);
    mx_lower_word(l, 0);
    mx_lower_word(l, 0);
    mx_lower_args(l, args, 1, nargs);
    for (uint32_t i = 0; i < 12 && is_callable; i++) {
        mx_lower_word(l, 0);
    }
}

void mx_lower_return(mx_lower_t *l, mx_ir_arg_t *args, uint32_t nargs) {
    mx_lower_close_all(l);
    bool is_spread = nargs > 0 && args[nargs - 1].type == MX_IR_ARG_TYPE_SPREAD;
    if (nargs == 0) {
        mx_lower_hop(l, MX_HOP_RETURN0);
    } else if (nargs == 1 && !is_spread) {
        mx_lower_hop(l, MX_HOP_RETURN1);
        mx_lower_arg(l, args[0]);
    } else {
        uint32_t n;
        if (is_spread) {
            n = nargs - 1;
        } else {
            n = nargs;
        }
        mx_lower_hop(l, MX_HOP_RETURN);
        mx_lower_word(l, n);
        mx_lower_word(l, is_spread);
        mx_lower_args(l, args, 0, nargs);
    }
}

void mx_lower_table_get(mx_lower_t *l, mx_ir_branch_t *br, mx_ir_arg_t *args) {
    mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
        .want = 1,
        .out = (uint16_t)br->out.reg,
        .kind = MX_RESUME_VALUES,
        .meta = MX_OP_GET,
        .has_cell_out = mx_lower_has_cell(l, (uint32_t)br->out.reg, 1),
        .is_meta = true,
        .resume_at = l->at,
        .target = br->targets[0],
    });
    mx_kind_t tk = mx_lower_kind(l, args[0]);
    mx_kind_t kk = mx_lower_kind(l, args[1]);
    bool is_table = args[0].type == MX_IR_ARG_TYPE_REG && tk == l->vm->kinds.table;
    if (is_table && args[1].type == MX_IR_ARG_TYPE_LIT && kk == MX_KIND_STRING) {
        mx_lower_hop(l, MX_HOP_TABLE_GET_SK);
        mx_lower_word(l, (uintptr_t)br->out.reg);
        mx_lower_word(l, (uintptr_t)args[0].reg);
        mx_lower_word(l, (uintptr_t)args[1].lit.val.bits);
        mx_lower_rec_word(l, rec);
        mx_lower_word(l, UINTPTR_MAX);
        mx_lower_hot(l);
    } else if (args[0].type == MX_IR_ARG_TYPE_REG && tk < MX_KIND_FIRST_DATA && mx_obj_is_table(l->vm->kinds.ptr[tk].get_methods) && args[1].type == MX_IR_ARG_TYPE_LIT && kk == MX_KIND_STRING) {
        mx_lower_hop(l, MX_HOP_METHOD_GET_SK);
        mx_lower_word(l, (uintptr_t)br->out.reg);
        mx_lower_word(l, (uintptr_t)args[0].reg);
        mx_lower_word(l, (uintptr_t)args[1].lit.val.bits);
        mx_lower_rec_word(l, rec);
        mx_lower_word(l, UINTPTR_MAX);
        mx_lower_word(l, tk);
        mx_lower_hot(l);
    } else if (is_table && args[1].type == MX_IR_ARG_TYPE_REG && kk == MX_KIND_INT) {
        mx_lower_hop(l, MX_HOP_TABLE_GET_IR);
        mx_lower_word(l, (uintptr_t)br->out.reg);
        mx_lower_word(l, (uintptr_t)args[0].reg);
        mx_lower_word(l, (uintptr_t)args[1].reg);
        mx_lower_rec_word(l, rec);
        mx_lower_hot(l);
    } else {
        mx_lower_hop(l, MX_HOP_TABLE_GET);
        mx_lower_word(l, (uintptr_t)br->out.reg);
        mx_lower_arg(l, args[0]);
        mx_lower_arg(l, args[1]);
        mx_lower_rec_word(l, rec);
        mx_lower_word(l, UINTPTR_MAX);
    }
}

void mx_lower_branch(mx_lower_t *l, mx_ir_branch_t *br) {
    mx_ir_arg_t args[MX_IR_BRANCH_MAX_ARGS + 1];
    uint32_t nargs = mx_ir_branch_args(br, args);
    switch (br->op) {
        case MX_IR_BRANCH_OPCODE_BOOL: {
            mx_kind_t kind = mx_lower_kind(l, args[0]);
            if (kind == MX_KIND_NIL) {
                mx_lower_goto(l, br->targets[1]);
            } else if (kind != MX_KIND_BOOLEAN) {
                mx_lower_goto(l, br->targets[0]);
            } else if (args[0].type == MX_IR_ARG_TYPE_LIT && mx_obj_get_boolean(args[0].lit)) {
                mx_lower_goto(l, br->targets[0]);
            } else if (args[0].type == MX_IR_ARG_TYPE_LIT) {
                mx_lower_goto(l, br->targets[1]);
            } else {
                mx_lower_hop(l, MX_HOP_BR_BOOL);
                mx_lower_word(l, (uintptr_t)br->targets[0]);
                mx_lower_word(l, (uintptr_t)br->targets[1]);
                mx_lower_word(l, 0);
                mx_lower_word(l, 0);
                mx_lower_word(l, (uintptr_t)args[0].reg);
            }
            break;
        }
        case MX_IR_BRANCH_OPCODE_IF_LT:
        case MX_IR_BRANCH_OPCODE_IF_LE:
        case MX_IR_BRANCH_OPCODE_IF_EQ: {
            mx_lower_compare(l, br, args[0], args[1]);
            break;
        }
        case MX_IR_BRANCH_OPCODE_RETURN: {
            if (l->is_hooked) {
                mx_lower_hook(l, 0);
                mx_lower_hook(l, 2);
            }
            mx_lower_return(l, args, nargs);
            break;
        }
        case MX_IR_BRANCH_OPCODE_UPVAL_GET: {
            mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
                .want = 1,
                .out = (uint16_t)br->out.reg,
                .kind = MX_RESUME_VALUES,
                .has_cell_out = mx_lower_has_cell(l, (uint32_t)br->out.reg, 1),
                .resume_at = l->at,
                .target = br->targets[0],
            });
            uint32_t index = (uint32_t)mx_obj_get_int(args[0].lit);
            mx_kind_t *seen = &l->block->func->upval_seen[index];
            mx_lower_hop(l, MX_HOP_UPVAL_GET);
            mx_lower_word(l, (uintptr_t)br->out.reg);
            mx_lower_word(l, index);
            mx_lower_rec_word(l, rec);
            if (*seen == MX_KIND_MIXED) {
                mx_lower_word(l, 0);
            } else {
                mx_lower_word(l, (uintptr_t)seen);
            }
            mx_lower_hot(l);
            break;
        }
        case MX_IR_BRANCH_OPCODE_TABLE_GET: {
            mx_lower_table_get(l, br, args);
            break;
        }
        case MX_IR_BRANCH_OPCODE_CALL: {
            mx_lower_call(l, br, args, nargs);
            break;
        }
        default: {
            mx_lower_goto(l, br->targets[0]);
            break;
        }
    }
}

bool mx_lower_chain(mx_lower_t *l, mx_ir_block_t *target) {
    if (target == NULL || target->is_func || target->id < l->block->id || l->nvisited >= MX_LOWER_CHAIN_MAX) {
        return false;
    }
    for (uint32_t i = 0; i < l->nvisited; i++) {
        if (l->visited[i] == target) {
            return false;
        }
    }
    l->visited[l->nvisited] = target;
    l->nvisited += 1;
    l->block = target;
    l->at = 0;
    return true;
}

bool mx_lower_guard(mx_lower_t *l, mx_ir_branch_t *br) {
    const mx_ir_func_t *func = l->block->func;
    mx_ir_arg_t args[2];
    mx_ir_branch_args(br, args);
    uint32_t index = (uint32_t)mx_obj_get_int(args[0].lit);
    mx_kind_t seen = func->upval_seen[index];
    mx_ir_block_t *target = br->targets[0];
    bool is_chainable = target != NULL && !target->is_func && l->nvisited < MX_LOWER_CHAIN_MAX;
    for (uint32_t i = 0; i < l->nvisited && is_chainable; i++) {
        if (l->visited[i] == target) {
            is_chainable = false;
        }
    }
    if (seen == MX_KIND_NIL || seen == MX_KIND_MIXED || !is_chainable) {
        return false;
    }
    mx_resume_t *rec = mx_lower_rec(l, (mx_resume_t) {
        .want = 1,
        .out = (uint16_t)br->out.reg,
        .kind = MX_RESUME_VALUES,
        .has_cell_out = mx_lower_has_cell(l, (uint32_t)br->out.reg, 1),
        .resume_at = l->at,
        .target = target,
    });
    mx_lower_hop(l, MX_HOP_UPVAL_GUARD);
    mx_lower_word(l, (uintptr_t)br->out.reg);
    mx_lower_word(l, index);
    mx_lower_word(l, seen);
    mx_lower_rec_word(l, rec);
    mx_lower_write(l, (uint32_t)br->out.reg, seen);
    mx_lower_chain(l, target);
    return true;
}

void mx_lower_try_fuse(mx_lower_t *l, uintptr_t *br_at) {
    uintptr_t *last = l->last_at;
    void *const *hops = l->vm->hops;
    if (last == NULL || br_at - last != 4 || last[0] != (uintptr_t)hops[MX_HOP_ADD_I64_RI]) {
        return;
    }
    mx_hop_t fused;
    if (br_at[0] == (uintptr_t)hops[MX_HOP_BR_LT_I64_RR]) {
        fused = MX_HOP_ADD_I64_RI_BR_LT_RR;
    } else if (br_at[0] == (uintptr_t)hops[MX_HOP_BR_LT_I64_RI]) {
        fused = MX_HOP_ADD_I64_RI_BR_LT_RI;
    } else if (br_at[0] == (uintptr_t)hops[MX_HOP_BR_LE_I64_RR]) {
        fused = MX_HOP_ADD_I64_RI_BR_LE_RR;
    } else if (br_at[0] == (uintptr_t)hops[MX_HOP_BR_LE_I64_RI]) {
        fused = MX_HOP_ADD_I64_RI_BR_LE_RI;
    } else if (br_at[0] == (uintptr_t)hops[MX_HOP_BR_EQ_I64_RR]) {
        fused = MX_HOP_ADD_I64_RI_BR_EQ_RR;
    } else if (br_at[0] == (uintptr_t)hops[MX_HOP_BR_EQ_I64_RI]) {
        fused = MX_HOP_ADD_I64_RI_BR_EQ_RI;
    } else {
        return;
    }
    last[0] = (uintptr_t)hops[fused];
}

void mx_lower_finish(mx_lower_t *l) {
    mx_t *vm = l->vm;
    mx_version_t *rb = l->rb;
    mx_lower_snap_t snap = mx_lower_snapshot(l);
    rb->out = snap.kinds;
    rb->nout = snap.nkinds;
    size_t used = (size_t)(l->w - l->start);
    uintptr_t *stream = mx_arena_alloc(&vm->arenas.code, sizeof(uintptr_t) * used + sizeof(mx_resume_t) * l->recs.len);
    memcpy(stream, l->start, sizeof(uintptr_t) * used);
    vm->arenas.code.hops += (uint32_t)(sizeof(uintptr_t) * used);
    stream[0] = (uintptr_t)rb;
    rb->code = stream + 1;
    mx_resume_t *recs = (mx_resume_t *)(stream + used);
    uint32_t last = l->recs.len - 1;
    for (uint32_t i = 0; i < l->recs.len; i++) {
        recs[last - i] = *l->recs.ptr[i];
    }
    for (uint32_t i = 0; i < l->refs.len; i++) {
        stream[l->refs.ptr[i].at] = (uintptr_t)&recs[last - l->refs.ptr[i].rec];
    }
    vm->lower_scratch.recs = l->recs;
    vm->lower_scratch.refs = l->refs;
    mx_arena_release(&vm->arenas.scratch, l->mark);
}

void mx_lower_version(mx_t *vm, mx_version_t *rb) {
    uint32_t nregs = rb->block->func->nregs;
    if (vm->lower_scratch.cap < nregs) {
        vm->lower_scratch.cap = nregs * 2;
        mx_mem_free(vm->lower_scratch.types);
        mx_mem_free(vm->lower_scratch.fresh);
        vm->lower_scratch.types = mx_mem_alloc(sizeof(mx_kind_t) * vm->lower_scratch.cap);
        vm->lower_scratch.fresh = mx_mem_alloc(sizeof(bool) * vm->lower_scratch.cap);
    }
    memset(vm->lower_scratch.fresh, 0, sizeof(bool) * nregs);
    if (vm->code_scratch.cap == 0) {
        vm->code_scratch.cap = 1024;
        vm->code_scratch.ptr = mx_mem_alloc(sizeof(uintptr_t) * vm->code_scratch.cap);
    }
    mx_lower_t l = {
        .vm = vm,
        .rb = rb,
        .types = vm->lower_scratch.types,
        .fresh = vm->lower_scratch.fresh,
        .block = rb->block,
        .visited = {rb->block},
        .start = vm->code_scratch.ptr,
        .w = vm->code_scratch.ptr + 1,
        .end = vm->code_scratch.ptr + vm->code_scratch.cap,
        .mark = mx_arena_mark(&vm->arenas.scratch),
        .recs = (mx_lower_recs_t) { .ptr = vm->lower_scratch.recs.ptr, .cap = vm->lower_scratch.recs.cap },
        .refs = (mx_lower_fixups_t) { .ptr = vm->lower_scratch.refs.ptr, .cap = vm->lower_scratch.refs.cap },
        .nvisited = 1,
        .nregs = nregs,
        .kslot = nregs,
        .frame = rb->frame,
        .at = rb->start_at,
        .is_hooked = rb->is_hooked,
        .needs_call_hook = rb->is_hooked && rb->start_at == 0 && rb->block->is_func,
    };
    for (uint32_t i = 0; i < nregs; i++) {
        if (i < rb->nkinds) {
            l.types[i] = rb->kinds[i];
        } else {
            l.types[i] = MX_KIND_NIL;
        }
    }
    while (true) {
        mx_ir_block_t *block = l.block;
        mx_ir_cursor_t cur = {
            .off = l.at,
        };
        mx_ir_step_t step = mx_ir_block_next(block, block->code_len, &cur);
        while (step.is_valid) {
            mx_ir_instr_t *in = &step.instr;
            l.site = l.at;
            l.next_at = cur.off;
            uint32_t nargs = mx_ir_args_count(in->args);
            bool is_untyped = in->op == MX_IR_INSTR_OPCODE_CLOSURE || in->op == MX_IR_INSTR_OPCODE_KEEP || in->op == MX_IR_INSTR_OPCODE_CLOSE;
            uint32_t unknown;
            if (is_untyped) {
                unknown = UINT32_MAX;
            } else {
                unknown = mx_lower_unknown_reg(&l, in->args, nargs);
            }
            if (unknown != UINT32_MAX) {
                mx_lower_cell(&l, unknown);
                mx_lower_finish(&l);
                return;
            }
            if (l.needs_call_hook && in->op != MX_IR_INSTR_OPCODE_VARARGS) {
                l.needs_call_hook = false;
                mx_lower_hook(&l, 1);
            }
            if (l.is_hooked && in->op != MX_IR_INSTR_OPCODE_KEEP && in->op != MX_IR_INSTR_OPCODE_VARARGS) {
                mx_lower_hook(&l, 0);
            }
            mx_kind_t a;
            if (nargs >= 1) {
                a = mx_lower_kind(&l, in->args[0]);
            } else {
                a = MX_KIND_NIL;
            }
            mx_kind_t b;
            if (nargs >= 2) {
                b = mx_lower_kind(&l, in->args[1]);
            } else {
                b = MX_KIND_NIL;
            }
            if (mx_lower_needs_meta(&l, in, a, b)) {
                mx_lower_meta_op(&l, mx_lower_op_of(in->op), in, nargs);
                mx_lower_finish(&l);
                return;
            }
            mx_lower_instr(&l, in);
            l.at = cur.off;
            step = mx_ir_block_next(block, block->code_len, &cur);
        }
        l.at = block->code_len;
        l.site = block->code_len;
        mx_ir_branch_t br = mx_ir_block_branch(block);
        bool is_flow = br.op == MX_IR_BRANCH_OPCODE_FALL || br.op == MX_IR_BRANCH_OPCODE_JUMP;
        if (is_flow) {
            if (mx_lower_chain(&l, br.targets[0])) {
                continue;
            }
            if (l.is_hooked) {
                mx_lower_hook(&l, 0);
            }
            mx_lower_goto(&l, br.targets[0]);
            mx_lower_finish(&l);
            return;
        }
        mx_ir_arg_t bargs[MX_IR_BRANCH_MAX_ARGS + 1];
        uint32_t nbargs = mx_ir_branch_args(&br, bargs);
        uint32_t unknown = mx_lower_unknown_reg(&l, bargs, nbargs);
        if (unknown != UINT32_MAX) {
            mx_lower_cell(&l, unknown);
            mx_lower_finish(&l);
            return;
        }
        if (l.needs_call_hook) {
            l.needs_call_hook = false;
            mx_lower_hook(&l, 1);
        }
        bool is_tail = br.op == MX_IR_BRANCH_OPCODE_CALL && br.targets[0] == NULL;
        if (l.is_hooked && br.op != MX_IR_BRANCH_OPCODE_RETURN && !is_tail) {
            mx_lower_hook(&l, 0);
        }
        if (br.op == MX_IR_BRANCH_OPCODE_UPVAL_GET && mx_lower_guard(&l, &br)) {
            continue;
        }
        uintptr_t *br_at = l.w;
        mx_lower_branch(&l, &br);
        if (!l.is_hooked) {
            mx_lower_try_fuse(&l, br_at);
        }
        mx_lower_finish(&l);
        return;
    }
}
