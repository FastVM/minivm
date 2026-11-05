
#pragma once

#if !defined(MX_HEADER_RUN_OP)
#define MX_HEADER_RUN_OP

#include "run.h"
#include "../ir/ir.h"
#include "../value/string.h"
#include "../value/table.h"
#include "../vm/gc.h"

#define MX_RUN_OP static inline __attribute__((always_inline))

MX_RUN_OP uint32_t mx_run_op_closure(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_ir_block_t *child = (mx_ir_block_t *)ip[2];
    mx_ir_func_t *func = child->func;
    uint32_t kslot = (uint32_t)ip[3];
    uint32_t n = func->nupvals;
    mx_thread_t *th = vm->thread;
    uint32_t boff = (uint32_t)(base - th->slots);
    mx_obj_closure_t *parent = mx_ref_obj(base[0].ref);
    mx_obj_closure_t *cl = mx_obj_closure_new(vm, child, n);
    for (uint32_t i = 0; i < n; i++) {
        const mx_ir_upval_t *up = &func->upvals[i];
        if (up->is_local) {
            if (ip[4 + i] != MX_KIND_SLOT) {
                base[kslot + up->index].bits = ip[4 + i];
            }
            cl->upvals[i] = mx_thread_upval_find(vm, th, boff + up->index, boff + kslot + up->index);
        } else {
            cl->upvals[i] = parent->upvals[up->index];
        }
        mx_ir_func_note_upval(func, i, mx_upval_get(cl->upvals[i]).kind);
    }
    base[ip[1]] = mx_val_ref(mx_ref_pin(cl));
    return n;
}

MX_RUN_OP void mx_run_op_table_init(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_obj_table_t *tbl = mx_ref_obj(base[ip[1]].ref);
    mx_obj_t k = mx_run_arg(ip + 2, base);
    mx_obj_t v = mx_run_arg(ip + 4, base);
    if (k.kind == MX_KIND_INT && v.kind != MX_KIND_NIL) {
        uint64_t at = (uint64_t)k.val.i64 - 1;
        if (at == (uint64_t)tbl->array_len && at < (uint64_t)tbl->array_cap && !tbl->has_int_hash) {
            mx_gc_write_barrier_slot(vm, tbl, v);
            tbl->array[at] = v;
            tbl->array_len += 1;
            if (tbl->len == (uint32_t)at) {
                tbl->len = tbl->array_len;
            }
            return;
        }
    }
    mx_run_set_raw(vm, tbl, k, v, NULL);
}

MX_RUN_OP void mx_run_op_table_init_ik(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_obj_table_t *tbl = mx_ref_obj(base[ip[1]].ref);
    mx_obj_t v = mx_run_arg(ip + 4, base);
    uint64_t at = (uint64_t)ip[2] - 1;
    if (v.kind != MX_KIND_NIL && at == (uint64_t)tbl->array_len && at < (uint64_t)tbl->array_cap && !tbl->has_int_hash) {
        tbl->array[at] = v;
        tbl->array_len += 1;
        if (tbl->len == (uint32_t)at) {
            tbl->len = tbl->array_len;
        }
        return;
    }
    mx_run_set_raw(vm, tbl, mx_obj_of_int((int64_t)ip[2]), v, NULL);
}

MX_RUN_OP void mx_run_op_table_init_sk(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    mx_obj_table_t *tbl = mx_ref_obj(base[ip[1]].ref);
    mx_obj_t v = mx_run_arg(ip + 3, base);
    size_t at = (size_t)ip[5];
    if (v.kind != MX_KIND_NIL && tbl->entries != NULL && at < mx_table_hash_cap(tbl)) {
        mx_table_entry_t *e = &tbl->entries[at];
        size_t cap = mx_table_hash_cap(tbl);
        uint32_t hash = ((mx_str_t *)(uintptr_t)ip[2])->hash;
        bool is_room = (tbl->used + 1) * MX_TABLE_LOAD_DEN <= cap * MX_TABLE_LOAD_NUM || cap <= MX_TABLE_HASH_MIN_CAP;
        if (e->key_kind == MX_KIND_NIL && hash != 0 && (hash & (cap - 1)) == at && is_room) {
            mx_gc_write_barrier(vm, tbl, v);
            tbl->used += 1;
            mx_table_entry_set_key(e, mx_obj_of_raw(MX_KIND_STRING, (mx_val_t) { .bits = ip[2] }), hash);
            mx_table_entry_set_val(e, v);
            return;
        }
    }
    mx_run_set_raw(vm, tbl, mx_obj_of_raw(MX_KIND_STRING, (mx_val_t) { .bits = ip[2] }), v, &ip[5]);
}

MX_RUN_OP bool mx_run_op_unpack(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_obj_table_t *pack = mx_ref_obj(base[ip[1]].ref);
    uint32_t n;
    if (ip[3] != 0) {
        mx_obj_t count = mx_table_get(pack, vm->char_strs[(uint8_t)'n']);
        if (!mx_obj_is_int(count) || (uint64_t)mx_obj_get_int(count) > INT32_MAX / 2) {
            return false;
        }
        n = (uint32_t)mx_obj_get_int(count);
    } else {
        n = pack->array_len;
    }
    mx_obj_t *out = mx_vm_rets_reserve(vm, n);
    for (uint32_t i = 0; i < n; i++) {
        if (i < pack->array_len) {
            out[i] = pack->array[i];
        } else {
            out[i] = mx_table_get(pack, mx_obj_of_int((int64_t)i + 1));
        }
    }
    vm->rets.len = n;
    return true;
}

MX_RUN_OP void mx_run_op_table_new(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    uint32_t hint = (uint32_t)ip[2];
    base[ip[1]] = mx_val_ref(mx_ref_pin(mx_table_new_sized(vm, hint & MX_IR_TABLE_HINT_MASK, hint >> MX_IR_TABLE_HINT_BITS)));
}

MX_RUN_OP void mx_run_op_concat(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_str_t *a = mx_ref_obj(base[ip[2]].ref);
    mx_str_t *b = mx_ref_obj(base[ip[3]].ref);
    base[ip[1]] = mx_val_ref(mx_ref_pin(mx_str_new_rope_gc(vm, a, b)));
}

MX_RUN_OP void mx_run_op_concat_si(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_str_t *a = mx_ref_obj(base[ip[2]].ref);
    base[ip[1]] = mx_val_ref(mx_ref_pin(mx_str_concat_int(vm, a, base[ip[3]].i64, false)));
}

MX_RUN_OP void mx_run_op_concat_is(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_str_t *b = mx_ref_obj(base[ip[3]].ref);
    base[ip[1]] = mx_val_ref(mx_ref_pin(mx_str_concat_int(vm, b, base[ip[2]].i64, true)));
}

MX_RUN_OP void mx_run_op_tostr_i64(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_str_t *out = mx_str_new_gc(vm);
    mx_str_append_int(out, base[ip[2]].i64);
    base[ip[1]] = mx_val_ref(mx_ref_pin(out));
}

MX_RUN_OP void mx_run_op_tostr_f64(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    mx_str_t *out = mx_str_new_gc(vm);
    mx_str_float(out, base[ip[2]].f64);
    base[ip[1]] = mx_val_ref(mx_ref_pin(out));
}

MX_RUN_OP void mx_run_op_varargs(mx_t *vm, mx_val_t *base, const uintptr_t *ip, uint32_t frame) {
    uint32_t n = (uint32_t)base[frame].bits;
    mx_obj_table_t *pack = mx_table_new_pack(vm, (const mx_obj_t *)(base + frame + 2), n, ip[2] != 0);
    base[ip[1]] = mx_val_ref(mx_ref_pin(pack));
}

#endif
