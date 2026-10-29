
#include "run.h"
#include "hop.h"
#include "../vm/gc.h"
#include "../ir/ir.h"
#include "../value/error.h"
#include "../value/kind.h"
#include "../value/num.h"
#include "../value/obj.h"
#include "../value/string.h"
#include "../value/table.h"

uintptr_t *mx_run_cache_find(const mx_resume_cache_t *cache, uintptr_t key) {
    if (cache->code != 0 && cache->key == key) {
        return (uintptr_t *)cache->code;
    } else if (cache->more != NULL) {
        for (uint32_t i = 0; i < MX_RESUME_WAYS - 1; i++) {
            if (cache->more[2 * i + 1] != 0 && cache->more[2 * i] == key) {
                return (uintptr_t *)cache->more[2 * i + 1];
            }
        }
    }
    return NULL;
}

void mx_run_cache_put(mx_t *vm, mx_resume_cache_t *cache, uintptr_t key, uintptr_t code) {
    if (cache->code != 0) {
        if (cache->more == NULL) {
            cache->more = mx_ir_alloc(vm, sizeof(uintptr_t) * 2 * (MX_RESUME_WAYS - 1));
            memset(cache->more, 0, sizeof(uintptr_t) * 2 * (MX_RESUME_WAYS - 1));
            vm->stats.version_bytes += (uint32_t)(sizeof(uintptr_t) * 2 * (MX_RESUME_WAYS - 1));
        }
        for (uint32_t i = MX_RESUME_WAYS - 2; i > 0; i--) {
            cache->more[2 * i] = cache->more[2 * i - 2];
            cache->more[2 * i + 1] = cache->more[2 * i - 1];
        }
        cache->more[0] = cache->key;
        cache->more[1] = cache->code;
    }
    cache->key = key;
    cache->code = code;
}

mx_ffi_fast_t *mx_run_fast_of(const mx_t *vm, mx_ffi_t *fn) {
    for (const mx_ffi_fast_reg_t *r = vm->fasts; r != NULL && r->ffi != NULL; r++) {
        if (r->ffi == fn) {
            return r->fast;
        }
    }
    return NULL;
}

bool mx_run_cells_stale(const mx_resume_t *rec, const mx_val_t *base) {
    const mx_ir_func_t *func = rec->target->func;
    if (!func->has_cells) {
        return false;
    }
    for (uint32_t r = 0; r < rec->nkinds; r++) {
        mx_kind_t k = rec->kinds[r];
        if (mx_kind_is_open(k) && k != MX_KIND_CELL_ANY && base[func->nregs + r].bits != mx_kind_plain(k)) {
            return true;
        }
    }
    return false;
}

void mx_run_cells_write(const mx_resume_t *rec, mx_val_t *base, const mx_kind_t *kinds, uint32_t n) {
    uint32_t nregs = rec->target->func->nregs;
    for (uint32_t i = 0; i < n; i++) {
        uint32_t r = rec->out + i;
        if (r < rec->nkinds && mx_kind_is_open(rec->kinds[r])) {
            base[nregs + r].bits = kinds[i];
        }
    }
}

mx_run_found_t mx_run_index_chain(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key) {
    mx_obj_table_t *cur = tbl;
    for (uint32_t steps = 0; steps < MX_RUN_MAX_CHAIN; steps++) {
        mx_obj_t handler = mx_table_get(cur->meta, vm->meta_get.key);
        if (mx_obj_is_nil(handler)) {
            return (mx_run_found_t) { .value = mx_obj_of_nil(), .is_found = true };
        } else if (!mx_obj_is_table(handler) || !mx_obj_get_table(handler)->is_plain) {
            return (mx_run_found_t) { .value = mx_obj_of_nil(), .is_found = false };
        }
        cur = mx_obj_get_table(handler);
        mx_obj_t raw = mx_table_get(cur, key);
        if (!mx_obj_is_nil(raw) || cur->meta == NULL) {
            return (mx_run_found_t) { .value = raw, .is_found = true };
        }
    }
    return (mx_run_found_t) { .value = mx_obj_of_nil(), .is_found = false };
}

mx_run_found_t mx_run_get(mx_t *vm, mx_obj_t t, mx_obj_t key) {
    if (mx_obj_is_table(t)) {
        mx_obj_table_t *tbl = mx_obj_get_table(t);
        if (tbl->is_plain) {
            mx_obj_t v = mx_table_get_inline(tbl, key);
            mx_obj_t handler = vm->kinds.ptr[t.kind].ops[MX_OP_GET];
            if (!mx_obj_is_nil(v) || tbl->meta == NULL) {
                return (mx_run_found_t) { .value = v, .is_found = true };
            } else if (mx_obj_is_ffi(handler) && mx_obj_get_ffi(handler) == vm->meta_get.fn) {
                return mx_run_index_chain(vm, tbl, key);
            }
        }
    } else if (t.kind < vm->kinds.len) {
        mx_obj_t methods = vm->kinds.ptr[t.kind].get_methods;
        if (mx_obj_is_table(methods)) {
            return (mx_run_found_t) { .value = mx_table_get_inline(mx_obj_get_table(methods), key), .is_found = true };
        }
    }
    return (mx_run_found_t) { .value = mx_obj_of_nil(), .is_found = false };
}

mx_obj_t mx_run_call_handler(mx_t *vm, mx_obj_t fn) {
    if (!mx_obj_is_table(fn) || mx_obj_get_table(fn)->meta == NULL) {
        return mx_obj_of_nil();
    }
    mx_obj_t op = vm->kinds.ptr[fn.kind].ops[MX_OP_CALL];
    if (mx_obj_is_ffi(op) && mx_obj_get_ffi(op) == vm->meta_call.fn) {
        return mx_table_get(mx_obj_get_table(fn)->meta, vm->meta_call.key);
    } else {
        return mx_obj_of_nil();
    }
}

bool mx_run_set_is_raw(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key) {
    if (!tbl->is_plain) {
        return false;
    } else if (tbl->meta == NULL) {
        return true;
    }
    mx_obj_t handler = vm->kinds.ptr[tbl->head.kind].ops[MX_OP_SET];
    if (!mx_obj_is_ffi(handler) || mx_obj_get_ffi(handler) != vm->meta_set.fn) {
        return false;
    } else if (!mx_obj_is_nil(mx_table_get_inline(tbl, key))) {
        return true;
    } else {
        return mx_obj_is_nil(mx_table_get(tbl->meta, vm->meta_set.key));
    }
}

int32_t mx_run_set_str(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key, mx_obj_t value) {
    mx_str_t *buf = (mx_str_t *)mx_ref_obj(key.val.ref);
    if (buf->is_rope || buf->hash == 0 || tbl->entries == NULL) {
        return -1;
    }
    size_t len = mx_table_hash_cap(tbl);
    size_t mask = len - 1;
    size_t look = buf->hash & mask;
    for (size_t probe = 0; probe < len; probe++) {
        mx_table_entry_t *e = &tbl->entries[look];
        if (e->key_kind == MX_KIND_NIL) {
            if ((tbl->used + 1) * MX_TABLE_LOAD_DEN > len * MX_TABLE_LOAD_NUM && len > MX_TABLE_HASH_MIN_CAP) {
                return -1;
            }
            mx_gc_write_barrier(vm, tbl, value);
            tbl->used += 1;
            mx_table_entry_set_key(e, key, buf->hash);
            mx_table_entry_set_val(e, value);
            return (int32_t)look;
        } else if (e->key_kind == MX_KIND_STRING && e->hash == buf->hash) {
            if (e->key.bits == key.val.bits) {
                mx_gc_write_barrier_slot(vm, tbl, value);
                mx_table_entry_set_val(e, value);
                return (int32_t)look;
            }
            return -1;
        }
        look = (look + 1) & mask;
    }
    return -1;
}

mx_obj_t mx_run_set_raw(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key, mx_obj_t value, uintptr_t *slot) {
    if (mx_obj_is_nil(key)) {
        return mx_error_raw(vm, "table index is nil");
    } else if (mx_obj_is_float(key) && mx_obj_get_float(key) != mx_obj_get_float(key)) {
        return mx_error_raw(vm, "table index is NaN");
    }
    if (key.kind == MX_KIND_INT && value.kind != MX_KIND_NIL) {
        uint64_t at = (uint64_t)(key.val.i64 - 1);
        if (at < (uint64_t)tbl->array_len && !mx_obj_is_nil(tbl->array[at])) {
            mx_gc_write_barrier_slot(vm, tbl, value);
            tbl->array[at] = value;
            return mx_obj_of_nil();
        } else if (at == (uint64_t)tbl->array_len && tbl->used == 0 && tbl->len == tbl->array_len) {
            mx_table_array_push(vm, tbl, value);
            tbl->len = tbl->array_len;
            return mx_obj_of_nil();
        }
    } else if (key.kind == MX_KIND_STRING && value.kind != MX_KIND_NIL && tbl->entries != NULL) {
        size_t at = (size_t)*slot;
        if (at < mx_table_hash_cap(tbl)) {
            mx_table_entry_t *e = &tbl->entries[at];
            if (e->key_kind == MX_KIND_STRING && e->key.bits == key.val.bits && e->val_kind != MX_KIND_NIL) {
                mx_gc_write_barrier_slot(vm, tbl, value);
                mx_table_entry_set_val(e, value);
                return mx_obj_of_nil();
            }
        }
        int32_t got = mx_run_set_str(vm, tbl, key, value);
        if (got < 0) {
            got = mx_table_hash_set(vm, tbl, key, value);
        }
        if (got >= 0) {
            *slot = (uintptr_t)got;
        }
        return mx_obj_of_nil();
    }
    mx_table_set(vm, tbl, key, value);
    return mx_obj_of_nil();
}

bool mx_run_eq_raw(mx_t *vm, mx_obj_t a, mx_obj_t b, bool *out) {
    if (a.kind != b.kind) {
        if (mx_obj_is_number(a) && mx_obj_is_number(b)) {
            *out = mx_obj_num_eq(a, b);
        } else {
            *out = false;
        }
        return true;
    } else if (a.kind < MX_KIND_FIRST_DATA || a.val.bits == b.val.bits) {
        *out = mx_table_key_eq(a, b);
        return true;
    }
    mx_obj_t handler = vm->kinds.ptr[a.kind].ops[MX_OP_EQ];
    bool is_std = mx_obj_is_ffi(handler) && mx_obj_get_ffi(handler) == vm->meta_eq.fn;
    if (is_std && mx_obj_is_table(a) && mx_obj_is_table(b)) {
        mx_obj_table_t *ta = mx_obj_get_table(a);
        mx_obj_table_t *tb = mx_obj_get_table(b);
        bool has_a = ta->meta != NULL && !mx_obj_is_nil(mx_table_get(ta->meta, vm->meta_eq.key));
        bool has_b = tb->meta != NULL && !mx_obj_is_nil(mx_table_get(tb->meta, vm->meta_eq.key));
        if (!has_a && !has_b) {
            *out = false;
            return true;
        }
    }
    return false;
}

void mx_run_spill(mx_val_t *base, uint32_t frame, const mx_obj_t *extra, uint32_t n) {
    base[frame].bits = n;
    mx_obj_t *spill = (mx_obj_t *)(base + frame + 2);
    for (uint32_t i = 0; i < n; i++) {
        spill[i] = extra[i];
    }
}

mx_version_t *mx_run_push(mx_t *vm, mx_thread_t *th, uint32_t p, uintptr_t link, const mx_resume_t *rec, mx_obj_t fn, const mx_obj_t *args, uint32_t n) {
    mx_ir_block_t *block = mx_obj_get_closure(fn)->block;
    const mx_ir_func_t *func = block->func;
    uint32_t frame = mx_ir_func_frame(func);
    uint32_t need = p + 2 + frame + 2 + 2 * n + MX_RUN_SLACK;
    if (need > th->limit) {
        return NULL;
    }
    uintptr_t at = (uintptr_t)args;
    uintptr_t lo = (uintptr_t)th->slots;
    bool is_inside = th->slots != NULL && at >= lo && at < lo + sizeof(mx_val_t) * th->cap;
    mx_thread_reserve(vm, th, need);
    if (is_inside) {
        args = (const mx_obj_t *)((uintptr_t)th->slots + (at - lo));
    }
    mx_version_t *rb = mx_version_entry(vm, block, args, n);
    mx_val_t *base = th->slots + p + 2;
    base[-2].bits = link;
    base[-1].bits = (uintptr_t)rec;
    base[0] = fn.val;
    for (uint32_t i = 0; i < func->nparams; i++) {
        if (i < n) {
            base[i + 1] = args[i].val;
        } else {
            base[i + 1].bits = 0;
        }
    }
    if (func->is_vararg) {
        if (n > func->nparams) {
            mx_run_spill(base, frame, args + func->nparams, n - func->nparams);
        } else {
            mx_run_spill(base, frame, NULL, 0);
        }
    }
    return rb;
}

bool mx_run_site_is_prim(const mx_resume_t *rec) {
    if (rec == NULL || rec->site_at < rec->site->code_len) {
        return false;
    }
    mx_ir_branch_t br = mx_ir_block_branch(rec->site);
    if (br.op != MX_IR_BRANCH_OPCODE_CALL) {
        return false;
    }
    mx_ir_arg_t args[MX_IR_BRANCH_MAX_ARGS + 1];
    mx_ir_branch_args(&br, args);
    return args[0].type == MX_IR_ARG_TYPE_LIT;
}

mx_obj_t mx_run_msgh(mx_t *vm, mx_obj_t handler, mx_obj_t err) {
    mx_obj_t payload = mx_error_payload(err);
    mx_thread_t *th = vm->thread;
    uint32_t limit = th->limit;
    th->limit = MX_THREAD_MAX_WORDS - MX_RUN_ERROR_ROOM / 2;
    mx_vm_hold(vm, err);
    mx_obj_t got = mx_call(vm, handler, 1, &payload);
    mx_vm_release(vm);
    th->limit = limit;
    mx_obj_t result;
    if (mx_obj_is_error(got)) {
        result = mx_error_value(vm, mx_obj_new_string(vm, "error in error handling"));
    } else {
        result = mx_error_value(vm, got);
    }
    mx_obj_get_error(result)->is_handled = true;
    return result;
}

mx_obj_t mx_run_wrap_error(mx_t *vm, const mx_resume_t *rec, mx_obj_t err) {
    mx_obj_t payload = mx_error_payload(err);
    if (!mx_obj_is_string(payload) || rec == NULL) {
        return err;
    }
    mx_str_t *msg = mx_str_new_gc(vm);
    mx_location_range_t range = mx_resume_range(rec);
    mx_error_where(msg, &range);
    mx_str_t *src = mx_obj_get_string(payload);
    mx_str_append(msg, src->buf, src->len);
    return mx_error_value(vm, mx_obj_of_string(msg));
}

bool mx_run_collect(mx_t *vm) {
    mx_vm_rets_push(vm);
    bool ran = mx_gc_collect(vm);
    mx_vm_rets_pop(vm);
    return ran;
}

#if MX_GC_STAT
#define MX_STAT(field) (vm->stats.field += 1)
#else
#define MX_STAT(field) ((void)0)
#endif

#define MX_DISPATCH() goto *(void *)ip[0]

#define MX_NEXT() \
    do { \
        if (vm->interrupt) { \
            goto safepoint; \
        } \
        MX_DISPATCH(); \
    } while (0)

#define MX_ENTER(code) \
    do { \
        ip = (uintptr_t *)(code); \
        rb = (mx_version_t *)ip[-1]; \
        MX_NEXT(); \
    } while (0)

#define MX_BRANCH(cond) \
    do { \
        if (cond) { \
            bc = ip + 3; \
        } else { \
            bc = ip + 4; \
        } \
        goto branch; \
    } while (0)

#define MX_DELIVER1(recp, value, hot) \
    do { \
        const mx_resume_t *d1_rec = (recp); \
        mx_obj_t d1_v = (value); \
        uintptr_t *d1_hot = (hot); \
        if (!d1_rec->has_cell_out) { \
            base[d1_rec->out] = d1_v.val; \
            if (d1_rec->cache.key == mx_run_key1(vm, d1_v.kind, false)) { \
                MX_STAT(deliver_fast); \
                if (d1_hot != NULL && d1_hot[0] == MX_KIND_MIXED) { \
                    d1_hot[0] = d1_v.kind; \
                    d1_hot[1] = d1_rec->cache.code; \
                } \
                MX_ENTER(d1_rec->cache.code); \
            } \
        } \
        small[0] = d1_v; \
        drec = d1_rec; \
        dvals = small; \
        dn = 1; \
        dresumed = false; \
        goto deliver; \
    } while (0)

#define MX_DELIVER_RESUMED(recp, vals, n) \
    do { \
        const mx_resume_t *dr_rec = (recp); \
        const mx_obj_t *dr_vals = (vals); \
        uint32_t dr_n = (n); \
        if (dr_rec->kind == MX_RESUME_VALUES && dr_rec->want <= 1 && !dr_rec->has_cell_out) { \
            uintptr_t dr_key; \
            if (dr_rec->want == 0) { \
                dr_key = mx_version_kinds_key(NULL, 0) | mx_run_key_flags(vm, true); \
            } else if (dr_n == 0) { \
                base[dr_rec->out] = mx_val_nil(); \
                dr_key = mx_run_key1(vm, MX_KIND_NIL, true); \
            } else { \
                base[dr_rec->out] = dr_vals[0].val; \
                dr_key = mx_run_key1(vm, dr_vals[0].kind, true); \
            } \
            if (dr_rec->cache.key == dr_key) { \
                MX_STAT(deliver_resumed_fast); \
                MX_ENTER(dr_rec->cache.code); \
            } \
        } \
        drec = dr_rec; \
        dvals = dr_vals; \
        dn = dr_n; \
        dresumed = true; \
        goto deliver; \
    } while (0)

mx_run_result_t mx_run_body(mx_t *vm, const mx_run_entry_t *entry) {
    static void *const labels[MX_HOP_COUNT] = {
        [MX_HOP_MOVE_R] = &&L_MOVE_R,
        [MX_HOP_MOVE_I] = &&L_MOVE_I,
        [MX_HOP_SET_KIND] = &&L_SET_KIND,
        [MX_HOP_ADD_I64_RR] = &&L_ADD_I64_RR,
        [MX_HOP_ADD_I64_RI] = &&L_ADD_I64_RI,
        [MX_HOP_ADD_I64_IR] = &&L_ADD_I64_IR,
        [MX_HOP_SUB_I64_RR] = &&L_SUB_I64_RR,
        [MX_HOP_SUB_I64_RI] = &&L_SUB_I64_RI,
        [MX_HOP_SUB_I64_IR] = &&L_SUB_I64_IR,
        [MX_HOP_MUL_I64_RR] = &&L_MUL_I64_RR,
        [MX_HOP_MUL_I64_RI] = &&L_MUL_I64_RI,
        [MX_HOP_MUL_I64_IR] = &&L_MUL_I64_IR,
        [MX_HOP_IDIV_I64_RR] = &&L_IDIV_I64_RR,
        [MX_HOP_IDIV_I64_RI] = &&L_IDIV_I64_RI,
        [MX_HOP_IDIV_I64_IR] = &&L_IDIV_I64_IR,
        [MX_HOP_MOD_I64_RR] = &&L_MOD_I64_RR,
        [MX_HOP_MOD_I64_RI] = &&L_MOD_I64_RI,
        [MX_HOP_MOD_I64_IR] = &&L_MOD_I64_IR,
        [MX_HOP_BAND_I64_RR] = &&L_BAND_I64_RR,
        [MX_HOP_BAND_I64_RI] = &&L_BAND_I64_RI,
        [MX_HOP_BAND_I64_IR] = &&L_BAND_I64_IR,
        [MX_HOP_BOR_I64_RR] = &&L_BOR_I64_RR,
        [MX_HOP_BOR_I64_RI] = &&L_BOR_I64_RI,
        [MX_HOP_BOR_I64_IR] = &&L_BOR_I64_IR,
        [MX_HOP_BXOR_I64_RR] = &&L_BXOR_I64_RR,
        [MX_HOP_BXOR_I64_RI] = &&L_BXOR_I64_RI,
        [MX_HOP_BXOR_I64_IR] = &&L_BXOR_I64_IR,
        [MX_HOP_SHL_I64_RR] = &&L_SHL_I64_RR,
        [MX_HOP_SHL_I64_RI] = &&L_SHL_I64_RI,
        [MX_HOP_SHL_I64_IR] = &&L_SHL_I64_IR,
        [MX_HOP_SHR_I64_RR] = &&L_SHR_I64_RR,
        [MX_HOP_SHR_I64_RI] = &&L_SHR_I64_RI,
        [MX_HOP_SHR_I64_IR] = &&L_SHR_I64_IR,
        [MX_HOP_ADD_F64_RR] = &&L_ADD_F64_RR,
        [MX_HOP_ADD_F64_RI] = &&L_ADD_F64_RI,
        [MX_HOP_ADD_F64_IR] = &&L_ADD_F64_IR,
        [MX_HOP_SUB_F64_RR] = &&L_SUB_F64_RR,
        [MX_HOP_SUB_F64_RI] = &&L_SUB_F64_RI,
        [MX_HOP_SUB_F64_IR] = &&L_SUB_F64_IR,
        [MX_HOP_MUL_F64_RR] = &&L_MUL_F64_RR,
        [MX_HOP_MUL_F64_RI] = &&L_MUL_F64_RI,
        [MX_HOP_MUL_F64_IR] = &&L_MUL_F64_IR,
        [MX_HOP_DIV_F64_RR] = &&L_DIV_F64_RR,
        [MX_HOP_DIV_F64_RI] = &&L_DIV_F64_RI,
        [MX_HOP_DIV_F64_IR] = &&L_DIV_F64_IR,
        [MX_HOP_IDIV_F64_RR] = &&L_IDIV_F64_RR,
        [MX_HOP_IDIV_F64_RI] = &&L_IDIV_F64_RI,
        [MX_HOP_IDIV_F64_IR] = &&L_IDIV_F64_IR,
        [MX_HOP_MOD_F64_RR] = &&L_MOD_F64_RR,
        [MX_HOP_MOD_F64_RI] = &&L_MOD_F64_RI,
        [MX_HOP_MOD_F64_IR] = &&L_MOD_F64_IR,
        [MX_HOP_POW_F64_RR] = &&L_POW_F64_RR,
        [MX_HOP_POW_F64_RI] = &&L_POW_F64_RI,
        [MX_HOP_POW_F64_IR] = &&L_POW_F64_IR,
        [MX_HOP_TO_F64] = &&L_TO_F64,
        [MX_HOP_DIV_INT_INT] = &&L_DIV_INT_INT,
        [MX_HOP_POW_INT_INT] = &&L_POW_INT_INT,
        [MX_HOP_NEG_I64] = &&L_NEG_I64,
        [MX_HOP_NEG_F64] = &&L_NEG_F64,
        [MX_HOP_BNOT_I64] = &&L_BNOT_I64,
        [MX_HOP_LEN_STR] = &&L_LEN_STR,
        [MX_HOP_LEN_TABLE] = &&L_LEN_TABLE,
        [MX_HOP_TOSTR_I64] = &&L_TOSTR_I64,
        [MX_HOP_TOSTR_F64] = &&L_TOSTR_F64,
        [MX_HOP_CONCAT] = &&L_CONCAT,
        [MX_HOP_TABLE_NEW] = &&L_TABLE_NEW,
        [MX_HOP_TABLE_SET] = &&L_TABLE_SET,
        [MX_HOP_TABLE_SET_SK] = &&L_TABLE_SET_SK,
        [MX_HOP_CLOSURE] = &&L_CLOSURE,
        [MX_HOP_VARARGS] = &&L_VARARGS,
        [MX_HOP_UNPACK] = &&L_UNPACK,
        [MX_HOP_UPVAL_SET] = &&L_UPVAL_SET,
        [MX_HOP_UPVAL_GUARD] = &&L_UPVAL_GUARD,
        [MX_HOP_CLOSE] = &&L_CLOSE,
        [MX_HOP_HOOK] = &&L_HOOK,
        [MX_HOP_GOTO] = &&L_GOTO,
        [MX_HOP_GOTO_HOT] = &&L_GOTO_HOT,
        [MX_HOP_BR_BOOL] = &&L_BR_BOOL,
        [MX_HOP_BR_LT_I64_RR] = &&L_BR_LT_I64_RR,
        [MX_HOP_BR_LT_I64_RI] = &&L_BR_LT_I64_RI,
        [MX_HOP_BR_LT_I64_IR] = &&L_BR_LT_I64_IR,
        [MX_HOP_BR_LE_I64_RR] = &&L_BR_LE_I64_RR,
        [MX_HOP_BR_LE_I64_RI] = &&L_BR_LE_I64_RI,
        [MX_HOP_BR_LE_I64_IR] = &&L_BR_LE_I64_IR,
        [MX_HOP_BR_EQ_I64_RR] = &&L_BR_EQ_I64_RR,
        [MX_HOP_BR_EQ_I64_RI] = &&L_BR_EQ_I64_RI,
        [MX_HOP_BR_EQ_I64_IR] = &&L_BR_EQ_I64_IR,
        [MX_HOP_BR_LT_F64_RR] = &&L_BR_LT_F64_RR,
        [MX_HOP_BR_LT_F64_RI] = &&L_BR_LT_F64_RI,
        [MX_HOP_BR_LT_F64_IR] = &&L_BR_LT_F64_IR,
        [MX_HOP_BR_LE_F64_RR] = &&L_BR_LE_F64_RR,
        [MX_HOP_BR_LE_F64_RI] = &&L_BR_LE_F64_RI,
        [MX_HOP_BR_LE_F64_IR] = &&L_BR_LE_F64_IR,
        [MX_HOP_BR_EQ_F64_RR] = &&L_BR_EQ_F64_RR,
        [MX_HOP_BR_EQ_F64_RI] = &&L_BR_EQ_F64_RI,
        [MX_HOP_BR_EQ_F64_IR] = &&L_BR_EQ_F64_IR,
        [MX_HOP_BR_LT_INT_F64] = &&L_BR_LT_INT_F64,
        [MX_HOP_BR_LE_INT_F64] = &&L_BR_LE_INT_F64,
        [MX_HOP_BR_EQ_INT_F64] = &&L_BR_EQ_INT_F64,
        [MX_HOP_BR_LT_F64_INT] = &&L_BR_LT_F64_INT,
        [MX_HOP_BR_LE_F64_INT] = &&L_BR_LE_F64_INT,
        [MX_HOP_BR_LT_STR] = &&L_BR_LT_STR,
        [MX_HOP_BR_LE_STR] = &&L_BR_LE_STR,
        [MX_HOP_BR_EQ_STR] = &&L_BR_EQ_STR,
        [MX_HOP_BR_EQ_RAW] = &&L_BR_EQ_RAW,
        [MX_HOP_ADD_I64_RI_BR_LT_RR] = &&L_ADD_I64_RI_BR_LT_RR,
        [MX_HOP_ADD_I64_RI_BR_LT_RI] = &&L_ADD_I64_RI_BR_LT_RI,
        [MX_HOP_ADD_I64_RI_BR_LE_RR] = &&L_ADD_I64_RI_BR_LE_RR,
        [MX_HOP_ADD_I64_RI_BR_LE_RI] = &&L_ADD_I64_RI_BR_LE_RI,
        [MX_HOP_ADD_I64_RI_BR_EQ_RR] = &&L_ADD_I64_RI_BR_EQ_RR,
        [MX_HOP_ADD_I64_RI_BR_EQ_RI] = &&L_ADD_I64_RI_BR_EQ_RI,
        [MX_HOP_TABLE_GET] = &&L_TABLE_GET,
        [MX_HOP_TABLE_GET_SK] = &&L_TABLE_GET_SK,
        [MX_HOP_TABLE_GET_IR] = &&L_TABLE_GET_IR,
        [MX_HOP_METHOD_GET_SK] = &&L_METHOD_GET_SK,
        [MX_HOP_UPVAL_GET] = &&L_UPVAL_GET,
        [MX_HOP_CELL] = &&L_CELL,
        [MX_HOP_META_OP] = &&L_META_OP,
        [MX_HOP_META_CMP] = &&L_META_CMP,
        [MX_HOP_CALL_LUA] = &&L_CALL_LUA,
        [MX_HOP_CALL] = &&L_CALL,
        [MX_HOP_CALL_FAST] = &&L_CALL_FAST,
        [MX_HOP_TAILCALL_LUA] = &&L_TAILCALL_LUA,
        [MX_HOP_RETURN0] = &&L_RETURN0,
        [MX_HOP_RETURN1] = &&L_RETURN1,
        [MX_HOP_RETURN] = &&L_RETURN,
    };
    if (entry->start == MX_RUN_START_INIT) {
        vm->hops = labels;
        return (mx_run_result_t) { .error = mx_obj_of_nil(), .status = MX_RUN_OK };
    }
    vm->run_token += 1;
    vm->token_cur = vm->run_token;
    mx_val_t *base;
    mx_version_t *rb;
    uintptr_t *ip;
    uintptr_t *bc;
    mx_obj_t small[MX_RUN_SMALL];
    mx_kind_t kinds[MX_IR_BRANCH_MAX_ARGS + 1];
    const mx_resume_t *drec;
    const mx_obj_t *dvals;
    uint32_t dn;
    bool dresumed;
    const mx_obj_t *rvals;
    uint32_t rn;
    mx_obj_t ferr;
    const mx_resume_t *frec;
    uint32_t cw;
    uint32_t cn;
    const mx_resume_t *crec;
    mx_obj_t cobj;
    mx_link_tag_t ctag;
    mx_op_t mop;
    mx_obj_t st;
    mx_obj_t sk;
    mx_obj_t sv;
    uintptr_t *sslot;
    mx_thread_t *sw_to;
    const mx_obj_t *sw_vals;
    uint32_t sw_n;
    mx_run_result_t result;
    if (entry->start == MX_RUN_START_CALL) {
        rb = entry->rb;
        base = vm->thread->slots + entry->base;
        ip = rb->code;
        goto next;
    }
    sw_to = entry->thread;
    sw_to->resumer = vm->thread;
    sw_to->is_c_resumed = true;
    sw_to->token = vm->token_cur;
    vm->thread->status = MX_THREAD_NORMAL;
    sw_vals = entry->args;
    sw_n = entry->nargs;
    goto switch_to;

L_MOVE_R: {
    base[ip[1]] = base[ip[2]];
    ip += 3;
    MX_DISPATCH();
}
L_MOVE_I: {
    base[ip[1]].bits = ip[2];
    ip += 3;
    MX_DISPATCH();
}
L_SET_KIND: {
    base[ip[1]].bits = ip[2];
    ip += 3;
    MX_DISPATCH();
}
L_ADD_I64_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 + base[ip[3]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_ADD_I64_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 + (int64_t)ip[3];
    ip += 4;
    MX_DISPATCH();
}
L_ADD_I64_IR: {
    base[ip[1]].i64 = (int64_t)ip[3] + base[ip[2]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_SUB_I64_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 - base[ip[3]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_SUB_I64_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 - (int64_t)ip[3];
    ip += 4;
    MX_DISPATCH();
}
L_SUB_I64_IR: {
    base[ip[1]].i64 = (int64_t)ip[3] - base[ip[2]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_MUL_I64_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 * base[ip[3]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_MUL_I64_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 * (int64_t)ip[3];
    ip += 4;
    MX_DISPATCH();
}
L_MUL_I64_IR: {
    base[ip[1]].i64 = (int64_t)ip[3] * base[ip[2]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_IDIV_I64_RR: {
    int64_t d = base[ip[3]].i64;
    if (d == 0) {
        ferr = mx_error_raw(vm, "attempt to divide by zero");
        frec = (const mx_resume_t *)ip[4];
        goto fail;
    }
    base[ip[1]].i64 = mx_num_idiv(base[ip[2]].i64, d);
    ip += 5;
    MX_DISPATCH();
}
L_IDIV_I64_RI: {
    int64_t d = (int64_t)ip[3];
    if (d == 0) {
        ferr = mx_error_raw(vm, "attempt to divide by zero");
        frec = (const mx_resume_t *)ip[4];
        goto fail;
    }
    base[ip[1]].i64 = mx_num_idiv(base[ip[2]].i64, d);
    ip += 5;
    MX_DISPATCH();
}
L_IDIV_I64_IR: {
    int64_t d = base[ip[2]].i64;
    if (d == 0) {
        ferr = mx_error_raw(vm, "attempt to divide by zero");
        frec = (const mx_resume_t *)ip[4];
        goto fail;
    }
    base[ip[1]].i64 = mx_num_idiv((int64_t)ip[3], d);
    ip += 5;
    MX_DISPATCH();
}
L_MOD_I64_RR: {
    int64_t d = base[ip[3]].i64;
    if (d == 0) {
        ferr = mx_error_raw(vm, "attempt to perform 'n%%0'");
        frec = (const mx_resume_t *)ip[4];
        goto fail;
    }
    base[ip[1]].i64 = mx_num_imod(base[ip[2]].i64, d);
    ip += 5;
    MX_DISPATCH();
}
L_MOD_I64_RI: {
    int64_t d = (int64_t)ip[3];
    if (d == 0) {
        ferr = mx_error_raw(vm, "attempt to perform 'n%%0'");
        frec = (const mx_resume_t *)ip[4];
        goto fail;
    }
    base[ip[1]].i64 = mx_num_imod(base[ip[2]].i64, d);
    ip += 5;
    MX_DISPATCH();
}
L_MOD_I64_IR: {
    int64_t d = base[ip[2]].i64;
    if (d == 0) {
        ferr = mx_error_raw(vm, "attempt to perform 'n%%0'");
        frec = (const mx_resume_t *)ip[4];
        goto fail;
    }
    base[ip[1]].i64 = mx_num_imod((int64_t)ip[3], d);
    ip += 5;
    MX_DISPATCH();
}
L_BAND_I64_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 & base[ip[3]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_BAND_I64_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 & (int64_t)ip[3];
    ip += 4;
    MX_DISPATCH();
}
L_BAND_I64_IR: {
    base[ip[1]].i64 = (int64_t)ip[3] & base[ip[2]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_BOR_I64_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 | base[ip[3]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_BOR_I64_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 | (int64_t)ip[3];
    ip += 4;
    MX_DISPATCH();
}
L_BOR_I64_IR: {
    base[ip[1]].i64 = (int64_t)ip[3] | base[ip[2]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_BXOR_I64_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 ^ base[ip[3]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_BXOR_I64_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 ^ (int64_t)ip[3];
    ip += 4;
    MX_DISPATCH();
}
L_BXOR_I64_IR: {
    base[ip[1]].i64 = (int64_t)ip[3] ^ base[ip[2]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_SHL_I64_RR: {
    base[ip[1]].i64 = mx_num_shl(base[ip[2]].i64, base[ip[3]].i64);
    ip += 4;
    MX_DISPATCH();
}
L_SHL_I64_RI: {
    base[ip[1]].i64 = mx_num_shl(base[ip[2]].i64, (int64_t)ip[3]);
    ip += 4;
    MX_DISPATCH();
}
L_SHL_I64_IR: {
    base[ip[1]].i64 = mx_num_shl((int64_t)ip[3], base[ip[2]].i64);
    ip += 4;
    MX_DISPATCH();
}
L_SHR_I64_RR: {
    base[ip[1]].i64 = mx_num_shr(base[ip[2]].i64, base[ip[3]].i64);
    ip += 4;
    MX_DISPATCH();
}
L_SHR_I64_RI: {
    base[ip[1]].i64 = mx_num_shr(base[ip[2]].i64, (int64_t)ip[3]);
    ip += 4;
    MX_DISPATCH();
}
L_SHR_I64_IR: {
    base[ip[1]].i64 = mx_num_shr((int64_t)ip[3], base[ip[2]].i64);
    ip += 4;
    MX_DISPATCH();
}
L_ADD_F64_RR: {
    base[ip[1]].f64 = base[ip[2]].f64 + base[ip[3]].f64;
    ip += 4;
    MX_DISPATCH();
}
L_ADD_F64_RI: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = base[ip[2]].f64 + imm.f64;
    ip += 4;
    MX_DISPATCH();
}
L_ADD_F64_IR: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = imm.f64 + base[ip[2]].f64;
    ip += 4;
    MX_DISPATCH();
}
L_SUB_F64_RR: {
    base[ip[1]].f64 = base[ip[2]].f64 - base[ip[3]].f64;
    ip += 4;
    MX_DISPATCH();
}
L_SUB_F64_RI: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = base[ip[2]].f64 - imm.f64;
    ip += 4;
    MX_DISPATCH();
}
L_SUB_F64_IR: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = imm.f64 - base[ip[2]].f64;
    ip += 4;
    MX_DISPATCH();
}
L_MUL_F64_RR: {
    base[ip[1]].f64 = base[ip[2]].f64 * base[ip[3]].f64;
    ip += 4;
    MX_DISPATCH();
}
L_MUL_F64_RI: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = base[ip[2]].f64 * imm.f64;
    ip += 4;
    MX_DISPATCH();
}
L_MUL_F64_IR: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = imm.f64 * base[ip[2]].f64;
    ip += 4;
    MX_DISPATCH();
}
L_DIV_F64_RR: {
    base[ip[1]].f64 = base[ip[2]].f64 / base[ip[3]].f64;
    ip += 4;
    MX_DISPATCH();
}
L_DIV_F64_RI: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = base[ip[2]].f64 / imm.f64;
    ip += 4;
    MX_DISPATCH();
}
L_DIV_F64_IR: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = imm.f64 / base[ip[2]].f64;
    ip += 4;
    MX_DISPATCH();
}
L_IDIV_F64_RR: {
    base[ip[1]].f64 = floor(base[ip[2]].f64 / base[ip[3]].f64);
    ip += 4;
    MX_DISPATCH();
}
L_IDIV_F64_RI: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = floor(base[ip[2]].f64 / imm.f64);
    ip += 4;
    MX_DISPATCH();
}
L_IDIV_F64_IR: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = floor(imm.f64 / base[ip[2]].f64);
    ip += 4;
    MX_DISPATCH();
}
L_MOD_F64_RR: {
    base[ip[1]].f64 = mx_num_fmod(base[ip[2]].f64, base[ip[3]].f64);
    ip += 4;
    MX_DISPATCH();
}
L_MOD_F64_RI: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = mx_num_fmod(base[ip[2]].f64, imm.f64);
    ip += 4;
    MX_DISPATCH();
}
L_MOD_F64_IR: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = mx_num_fmod(imm.f64, base[ip[2]].f64);
    ip += 4;
    MX_DISPATCH();
}
L_POW_F64_RR: {
    base[ip[1]].f64 = pow(base[ip[2]].f64, base[ip[3]].f64);
    ip += 4;
    MX_DISPATCH();
}
L_POW_F64_RI: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = pow(base[ip[2]].f64, imm.f64);
    ip += 4;
    MX_DISPATCH();
}
L_POW_F64_IR: {
    mx_val_t imm = { .bits = ip[3] };
    base[ip[1]].f64 = pow(imm.f64, base[ip[2]].f64);
    ip += 4;
    MX_DISPATCH();
}
L_TO_F64: {
    base[ip[1]].f64 = (double)base[ip[2]].i64;
    ip += 3;
    MX_DISPATCH();
}
L_DIV_INT_INT: {
    base[ip[1]].f64 = (double)base[ip[2]].i64 / (double)base[ip[3]].i64;
    ip += 4;
    MX_DISPATCH();
}
L_POW_INT_INT: {
    base[ip[1]].f64 = pow((double)base[ip[2]].i64, (double)base[ip[3]].i64);
    ip += 4;
    MX_DISPATCH();
}
L_NEG_I64: {
    base[ip[1]].i64 = -base[ip[2]].i64;
    ip += 3;
    MX_DISPATCH();
}
L_NEG_F64: {
    base[ip[1]].f64 = -base[ip[2]].f64;
    ip += 3;
    MX_DISPATCH();
}
L_BNOT_I64: {
    base[ip[1]].i64 = ~base[ip[2]].i64;
    ip += 3;
    MX_DISPATCH();
}
L_LEN_STR: {
    base[ip[1]].i64 = (int64_t)((mx_str_t *)mx_ref_obj(base[ip[2]].ref))->len;
    ip += 3;
    MX_DISPATCH();
}
L_LEN_TABLE: {
    mx_obj_table_t *tbl = mx_ref_obj(base[ip[2]].ref);
    if (tbl->is_plain && tbl->meta == NULL) {
        base[ip[1]].i64 = (int64_t)tbl->len;
        ip += 4;
        MX_DISPATCH();
    }
    crec = (const mx_resume_t *)ip[3];
    base = mx_run_reserve(vm, base, rb->frame + 8 + MX_RUN_SLACK);
    cw = (uint32_t)(base - vm->thread->slots) + rb->frame;
    *(mx_obj_t *)(base + rb->frame + 4) = mx_obj_of_table(tbl);
    cn = 1;
    mop = MX_OP_LEN;
    goto meta;
}
L_TOSTR_I64: {
    mx_str_t *out = mx_str_new_gc(vm);
    mx_str_append_int(out, base[ip[2]].i64);
    base[ip[1]] = mx_val_ref(mx_ref_pin(out));
    ip += 3;
    MX_DISPATCH();
}
L_TOSTR_F64: {
    mx_str_t *out = mx_str_new_gc(vm);
    mx_str_float(out, base[ip[2]].f64);
    base[ip[1]] = mx_val_ref(mx_ref_pin(out));
    ip += 3;
    MX_DISPATCH();
}
L_CONCAT: {
    mx_str_t *a = mx_ref_obj(base[ip[2]].ref);
    mx_str_t *b = mx_ref_obj(base[ip[3]].ref);
    base[ip[1]] = mx_val_ref(mx_ref_pin(mx_str_new_rope_gc(vm, a, b)));
    ip += 4;
    MX_DISPATCH();
}
L_TABLE_NEW: {
    uint32_t hint = (uint32_t)ip[2];
    base[ip[1]] = mx_val_ref(mx_ref_pin(mx_table_new_sized(vm, hint & MX_IR_TABLE_HINT_MASK, hint >> MX_IR_TABLE_HINT_BITS)));
    ip += 3;
    MX_DISPATCH();
}
L_TABLE_SET: {
    mx_obj_t t = mx_run_arg(ip + 1, base);
    mx_obj_t k = mx_run_arg(ip + 3, base);
    mx_obj_t v = mx_run_arg(ip + 5, base);
    if (t.kind == vm->kinds.table) {
        mx_obj_table_t *tbl = mx_obj_get_table(t);
        if (tbl->meta == NULL && k.kind == MX_KIND_INT && v.kind != MX_KIND_NIL) {
            uint64_t at = (uint64_t)(k.val.i64 - 1);
            if (at < (uint64_t)tbl->array_len && tbl->array[at].kind != MX_KIND_NIL) {
                mx_gc_write_barrier_slot(vm, tbl, v);
                tbl->array[at] = v;
                ip += 9;
                MX_DISPATCH();
            }
        }
    }
    st = t;
    sk = k;
    sv = v;
    sslot = &ip[8];
    crec = (const mx_resume_t *)ip[7];
    ip += 9;
    goto set;
}
L_TABLE_SET_SK: {
    mx_obj_table_t *tbl = mx_ref_obj(base[ip[1]].ref);
    mx_obj_t v = mx_run_arg(ip + 3, base);
    size_t at = (size_t)ip[6];
    if (tbl->is_plain && v.kind != MX_KIND_NIL && tbl->entries != NULL && at < mx_table_hash_cap(tbl)) {
        mx_table_entry_t *e = &tbl->entries[at];
        if (e->key_kind == MX_KIND_STRING && e->key.bits == ip[2] && e->val_kind != MX_KIND_NIL) {
            mx_gc_write_barrier_slot(vm, tbl, v);
            mx_table_entry_set_val(e, v);
            ip += 7;
            MX_DISPATCH();
        }
    }
    st = mx_obj_of_table(tbl);
    sk = mx_obj_of_raw(MX_KIND_STRING, (mx_val_t) { .bits = ip[2] });
    sv = v;
    sslot = &ip[6];
    crec = (const mx_resume_t *)ip[5];
    ip += 7;
    goto set;
}
L_CLOSURE: {
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
    ip += 4 + n;
    MX_DISPATCH();
}
L_VARARGS: {
    uint32_t frame = rb->frame;
    uint32_t n = (uint32_t)base[frame].bits;
    mx_obj_table_t *pack = mx_table_new_pack(vm, (const mx_obj_t *)(base + frame + 2), n);
    base[ip[1]] = mx_val_ref(mx_ref_pin(pack));
    ip += 2;
    MX_DISPATCH();
}
L_UNPACK: {
    mx_obj_table_t *pack = mx_ref_obj(base[ip[1]].ref);
    mx_obj_t count = mx_table_get(pack, vm->char_strs[(uint8_t)'n']);
    if (!mx_obj_is_int(count) || (uint64_t)mx_obj_get_int(count) > INT32_MAX / 2) {
        ferr = mx_error_raw(vm, "vararg table has no proper 'n'");
        frec = (const mx_resume_t *)ip[2];
        goto fail;
    }
    uint32_t n = (uint32_t)mx_obj_get_int(count);
    mx_obj_t *out = mx_vm_rets_reserve(vm, n);
    for (uint32_t i = 0; i < n; i++) {
        if (i < pack->array_len) {
            out[i] = pack->array[i];
        } else {
            out[i] = mx_table_get(pack, mx_obj_of_int((int64_t)i + 1));
        }
    }
    vm->rets.len = n;
    ip += 3;
    MX_DISPATCH();
}
L_UPVAL_SET: {
    mx_obj_closure_t *cl = mx_ref_obj(base[0].ref);
    mx_upval_set(vm, cl->upvals[ip[1]], mx_run_arg(ip + 2, base));
    ip += 4;
    MX_DISPATCH();
}
L_UPVAL_GUARD: {
    mx_obj_closure_t *cl = mx_ref_obj(base[0].ref);
    mx_obj_t v = mx_upval_get(cl->upvals[ip[2]]);
    if ((uintptr_t)v.kind == ip[3]) {
        base[ip[1]] = v.val;
        ip += 5;
        MX_DISPATCH();
    }
    mx_ir_func_note_upval(rb->block->func, (uint32_t)ip[2], v.kind);
    small[0] = v;
    drec = (const mx_resume_t *)ip[4];
    dvals = small;
    dn = 1;
    dresumed = false;
    goto deliver;
}
L_CLOSE: {
    mx_thread_close_upvals(vm, vm->thread, (uint32_t)(base - vm->thread->slots) + (uint32_t)ip[1]);
    ip += 2;
    MX_DISPATCH();
}
L_HOOK: {
    const mx_resume_t *rec = (const mx_resume_t *)ip[1];
    if (!vm->is_hooked) {
        rb = mx_version_at(vm, rec, NULL, 0, false);
        ip = rb->code;
        MX_DISPATCH();
    } else if (vm->hooks.debug == NULL) {
        ip += 3;
        MX_DISPATCH();
    }
    mx_thread_t *th = vm->thread;
    base = mx_run_reserve(vm, base, rb->frame + 2 + MX_RUN_SLACK);
    uint32_t boff = (uint32_t)(base - th->slots);
    uint32_t w = boff + rb->frame;
    th->slots[w].bits = mx_link_make(boff - 2, MX_LINK_HOOK);
    th->slots[w + 1].bits = (uintptr_t)rec;
    th->frame = w;
    th->top = w + 2;
    mx_vm_rets_push(vm);
    mx_obj_t err = vm->hooks.debug(vm, th, rec, (uint32_t)ip[2]);
    mx_vm_rets_pop(vm);
    base = th->slots + boff;
    if (mx_obj_is_error(err)) {
        ferr = err;
        frec = rec;
        goto fail_c;
    }
    const mx_resume_t *cur = (const mx_resume_t *)th->slots[w + 1].bits;
    if (cur != rec || !vm->is_hooked || mx_run_cells_stale(rec, base)) {
        rb = mx_version_at(vm, cur, NULL, 0, true);
        ip = rb->code;
        if (ip[0] == (uintptr_t)labels[MX_HOP_HOOK]) {
            ip += 3;
        }
        MX_DISPATCH();
    }
    ip += 3;
    MX_DISPATCH();
}
L_GOTO: {
    mx_version_t *target = mx_version_edge(vm, rb->out, rb->nout, (mx_ir_block_t *)ip[1]);
    ip[2] = (uintptr_t)target->code;
    ip[0] = (uintptr_t)labels[MX_HOP_GOTO_HOT];
    rb = target;
    ip = target->code;
    goto next;
}
L_GOTO_HOT: {
    MX_ENTER(ip[2]);
}
L_BR_BOOL: {
    MX_BRANCH(base[ip[5]].bits != 0);
}
L_BR_LT_I64_RR: {
    MX_BRANCH(base[ip[5]].i64 < base[ip[6]].i64);
}
L_BR_LT_I64_RI: {
    MX_BRANCH(base[ip[5]].i64 < (int64_t)ip[6]);
}
L_BR_LT_I64_IR: {
    MX_BRANCH((int64_t)ip[6] < base[ip[5]].i64);
}
L_BR_LE_I64_RR: {
    MX_BRANCH(base[ip[5]].i64 <= base[ip[6]].i64);
}
L_BR_LE_I64_RI: {
    MX_BRANCH(base[ip[5]].i64 <= (int64_t)ip[6]);
}
L_BR_LE_I64_IR: {
    MX_BRANCH((int64_t)ip[6] <= base[ip[5]].i64);
}
L_BR_EQ_I64_RR: {
    MX_BRANCH(base[ip[5]].i64 == base[ip[6]].i64);
}
L_BR_EQ_I64_RI: {
    MX_BRANCH(base[ip[5]].i64 == (int64_t)ip[6]);
}
L_BR_EQ_I64_IR: {
    MX_BRANCH((int64_t)ip[6] == base[ip[5]].i64);
}
L_BR_LT_F64_RR: {
    MX_BRANCH(base[ip[5]].f64 < base[ip[6]].f64);
}
L_BR_LT_F64_RI: {
    mx_val_t imm = { .bits = ip[6] };
    MX_BRANCH(base[ip[5]].f64 < imm.f64);
}
L_BR_LT_F64_IR: {
    mx_val_t imm = { .bits = ip[6] };
    MX_BRANCH(imm.f64 < base[ip[5]].f64);
}
L_BR_LE_F64_RR: {
    MX_BRANCH(base[ip[5]].f64 <= base[ip[6]].f64);
}
L_BR_LE_F64_RI: {
    mx_val_t imm = { .bits = ip[6] };
    MX_BRANCH(base[ip[5]].f64 <= imm.f64);
}
L_BR_LE_F64_IR: {
    mx_val_t imm = { .bits = ip[6] };
    MX_BRANCH(imm.f64 <= base[ip[5]].f64);
}
L_BR_EQ_F64_RR: {
    MX_BRANCH(base[ip[5]].f64 == base[ip[6]].f64);
}
L_BR_EQ_F64_RI: {
    mx_val_t imm = { .bits = ip[6] };
    MX_BRANCH(base[ip[5]].f64 == imm.f64);
}
L_BR_EQ_F64_IR: {
    mx_val_t imm = { .bits = ip[6] };
    MX_BRANCH(imm.f64 == base[ip[5]].f64);
}
L_BR_LT_INT_F64: {
    MX_BRANCH(mx_obj_int_lt_float(base[ip[5]].i64, base[ip[6]].f64));
}
L_BR_LE_INT_F64: {
    MX_BRANCH(mx_obj_int_le_float(base[ip[5]].i64, base[ip[6]].f64));
}
L_BR_EQ_INT_F64: {
    MX_BRANCH(mx_obj_int_eq_float(base[ip[5]].i64, base[ip[6]].f64));
}
L_BR_LT_F64_INT: {
    MX_BRANCH(mx_obj_float_lt_int(base[ip[5]].f64, base[ip[6]].i64));
}
L_BR_LE_F64_INT: {
    MX_BRANCH(mx_obj_float_le_int(base[ip[5]].f64, base[ip[6]].i64));
}
L_BR_LT_STR: {
    mx_str_t *x = mx_str_flat(mx_ref_obj(base[ip[5]].ref));
    mx_str_t *y = mx_str_flat(mx_ref_obj(base[ip[6]].ref));
    MX_BRANCH(mx_obj_str_cmp(x, y) < 0);
}
L_BR_LE_STR: {
    mx_str_t *x = mx_str_flat(mx_ref_obj(base[ip[5]].ref));
    mx_str_t *y = mx_str_flat(mx_ref_obj(base[ip[6]].ref));
    MX_BRANCH(mx_obj_str_cmp(x, y) <= 0);
}
L_BR_EQ_STR: {
    mx_str_t *x = mx_ref_obj(base[ip[5]].ref);
    mx_str_t *y = mx_ref_obj(base[ip[6]].ref);
    bool is_eq;
    if (x == y) {
        is_eq = true;
    } else if (x->is_interned && y->is_interned) {
        is_eq = false;
    } else {
        x = mx_str_flat(x);
        y = mx_str_flat(y);
        is_eq = x->len == y->len && memcmp(x->buf, y->buf, x->len) == 0;
    }
    MX_BRANCH(is_eq);
}
L_BR_EQ_RAW: {
    MX_BRANCH(base[ip[5]].bits == base[ip[6]].bits);
}
L_ADD_I64_RI_BR_LT_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 + (int64_t)ip[3];
    ip += 4;
    MX_BRANCH(base[ip[5]].i64 < base[ip[6]].i64);
}
L_ADD_I64_RI_BR_LT_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 + (int64_t)ip[3];
    ip += 4;
    MX_BRANCH(base[ip[5]].i64 < (int64_t)ip[6]);
}
L_ADD_I64_RI_BR_LE_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 + (int64_t)ip[3];
    ip += 4;
    MX_BRANCH(base[ip[5]].i64 <= base[ip[6]].i64);
}
L_ADD_I64_RI_BR_LE_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 + (int64_t)ip[3];
    ip += 4;
    MX_BRANCH(base[ip[5]].i64 <= (int64_t)ip[6]);
}
L_ADD_I64_RI_BR_EQ_RR: {
    base[ip[1]].i64 = base[ip[2]].i64 + (int64_t)ip[3];
    ip += 4;
    MX_BRANCH(base[ip[5]].i64 == base[ip[6]].i64);
}
L_ADD_I64_RI_BR_EQ_RI: {
    base[ip[1]].i64 = base[ip[2]].i64 + (int64_t)ip[3];
    ip += 4;
    MX_BRANCH(base[ip[5]].i64 == (int64_t)ip[6]);
}
L_TABLE_GET: {
    mx_obj_t t = mx_run_arg(ip + 2, base);
    mx_obj_t k = mx_run_arg(ip + 4, base);
    mx_run_found_t got = mx_run_get(vm, t, k);
    if (got.is_found) {
        MX_DELIVER1((const mx_resume_t *)ip[6], got.value, NULL);
    }
    crec = (const mx_resume_t *)ip[6];
    base = mx_run_reserve(vm, base, rb->frame + 8 + MX_RUN_SLACK);
    cw = (uint32_t)(base - vm->thread->slots) + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(base + rb->frame + 4);
    args[0] = t;
    args[1] = k;
    cn = 2;
    mop = MX_OP_GET;
    goto meta;
}
L_TABLE_GET_SK: {
    mx_obj_table_t *tbl = mx_ref_obj(base[ip[2]].ref);
    mx_obj_t key = mx_obj_of_raw(MX_KIND_STRING, (mx_val_t) { .bits = ip[3] });
    if (tbl->is_plain) {
        mx_obj_t v = mx_obj_of_nil();
        size_t at = (size_t)ip[5];
        if (tbl->entries != NULL && at < mx_table_hash_cap(tbl) && tbl->entries[at].key_kind == MX_KIND_STRING && tbl->entries[at].key.bits == ip[3]) {
            MX_STAT(sk_hit);
            v = mx_table_entry_val(&tbl->entries[at]);
        } else {
            MX_STAT(sk_miss);
            mx_table_slot_t got = mx_table_get_string_slot(tbl, key);
            v = got.value;
            if (got.slot >= 0) {
                ip[5] = (uintptr_t)got.slot;
            }
        }
        if (!mx_obj_is_nil(v) || tbl->meta == NULL) {
            if ((uintptr_t)v.kind == ip[6]) {
                base[ip[1]] = v.val;
                MX_ENTER(ip[7]);
            }
            MX_DELIVER1((const mx_resume_t *)ip[4], v, ip + 6);
        }
        mx_obj_t handler = vm->kinds.ptr[tbl->head.kind].ops[MX_OP_GET];
        if (mx_obj_is_ffi(handler) && mx_obj_get_ffi(handler) == vm->meta_get.fn) {
            mx_run_found_t got = mx_run_index_chain(vm, tbl, key);
            if (got.is_found) {
                MX_DELIVER1((const mx_resume_t *)ip[4], got.value, NULL);
            }
        }
    }
    crec = (const mx_resume_t *)ip[4];
    base = mx_run_reserve(vm, base, rb->frame + 8 + MX_RUN_SLACK);
    cw = (uint32_t)(base - vm->thread->slots) + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(base + rb->frame + 4);
    args[0] = mx_obj_of_table(tbl);
    args[1] = key;
    cn = 2;
    mop = MX_OP_GET;
    goto meta;
}
L_TABLE_GET_IR: {
    mx_obj_table_t *tbl = mx_ref_obj(base[ip[2]].ref);
    int64_t k = base[ip[3]].i64;
    if (tbl->is_plain && (uint64_t)(k - 1) < (uint64_t)tbl->array_len) {
        mx_obj_t v = tbl->array[k - 1];
        if (!mx_obj_is_nil(v) || tbl->meta == NULL) {
            if ((uintptr_t)v.kind == ip[5]) {
                base[ip[1]] = v.val;
                MX_ENTER(ip[6]);
            }
            MX_DELIVER1((const mx_resume_t *)ip[4], v, ip + 5);
        }
    }
    mx_obj_t t = mx_obj_of_table(tbl);
    mx_obj_t key = mx_obj_of_int(k);
    mx_run_found_t got = mx_run_get(vm, t, key);
    if (got.is_found) {
        MX_DELIVER1((const mx_resume_t *)ip[4], got.value, NULL);
    }
    crec = (const mx_resume_t *)ip[4];
    base = mx_run_reserve(vm, base, rb->frame + 8 + MX_RUN_SLACK);
    cw = (uint32_t)(base - vm->thread->slots) + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(base + rb->frame + 4);
    args[0] = t;
    args[1] = key;
    cn = 2;
    mop = MX_OP_GET;
    goto meta;
}
L_METHOD_GET_SK: {
    mx_obj_t t = mx_obj_of_raw((mx_kind_t)ip[6], base[ip[2]]);
    mx_obj_t key = mx_obj_of_raw(MX_KIND_STRING, (mx_val_t) { .bits = ip[3] });
    mx_obj_t methods = vm->kinds.ptr[t.kind].get_methods;
    if (mx_obj_is_table(methods)) {
        mx_obj_table_t *tbl = mx_obj_get_table(methods);
        size_t at = (size_t)ip[5];
        if (tbl->entries != NULL && at < mx_table_hash_cap(tbl) && tbl->entries[at].key_kind == MX_KIND_STRING && tbl->entries[at].key.bits == ip[3]) {
            mx_obj_t v = mx_table_entry_val(&tbl->entries[at]);
            if ((uintptr_t)v.kind == ip[7]) {
                base[ip[1]] = v.val;
                MX_ENTER(ip[8]);
            }
            MX_DELIVER1((const mx_resume_t *)ip[4], v, ip + 7);
        }
        mx_table_slot_t got = mx_table_get_string_slot(tbl, key);
        if (got.slot >= 0) {
            ip[5] = (uintptr_t)got.slot;
        }
        MX_DELIVER1((const mx_resume_t *)ip[4], got.value, NULL);
    }
    mx_run_found_t got = mx_run_get(vm, t, key);
    if (got.is_found) {
        MX_DELIVER1((const mx_resume_t *)ip[4], got.value, NULL);
    }
    crec = (const mx_resume_t *)ip[4];
    base = mx_run_reserve(vm, base, rb->frame + 8 + MX_RUN_SLACK);
    cw = (uint32_t)(base - vm->thread->slots) + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(base + rb->frame + 4);
    args[0] = t;
    args[1] = key;
    cn = 2;
    mop = MX_OP_GET;
    goto meta;
}
L_UPVAL_GET: {
    mx_obj_closure_t *cl = mx_ref_obj(base[0].ref);
    mx_obj_t v = mx_upval_get(cl->upvals[ip[2]]);
    if (ip[4] != 0) {
        mx_ir_note_seen((mx_kind_t *)ip[4], v.kind);
    }
    if ((uintptr_t)v.kind == ip[5]) {
        base[ip[1]] = v.val;
        MX_ENTER(ip[6]);
    }
    MX_DELIVER1((const mx_resume_t *)ip[3], v, ip + 5);
}
L_CELL: {
    const mx_resume_t *rec = (const mx_resume_t *)ip[3];
    mx_kind_t kind = (mx_kind_t)base[ip[2]].bits;
    uintptr_t key = (uintptr_t)kind | mx_run_key_flags(vm, false);
    uintptr_t *code = mx_run_cache_find(&rec->cache, key);
    if (code == NULL) {
        rb = mx_version_at(vm, rec, &kind, 1, false);
        code = rb->code;
        mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key, (uintptr_t)code);
    }
    ip = code;
    rb = (mx_version_t *)ip[-1];
    MX_DISPATCH();
}
L_META_OP: {
    crec = (const mx_resume_t *)ip[3];
    cn = (uint32_t)ip[2];
    mop = (mx_op_t)ip[1];
    base = mx_run_reserve(vm, base, rb->frame + 8 + MX_RUN_SLACK);
    cw = (uint32_t)(base - vm->thread->slots) + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(base + rb->frame + 4);
    args[0] = mx_run_arg(ip + 4, base);
    if (cn == 2) {
        args[1] = mx_run_arg(ip + 6, base);
    } else {
        args[1] = args[0];
    }
    goto meta;
}
L_META_CMP: {
    mx_obj_t a = mx_run_arg(ip + 3, base);
    mx_obj_t b = mx_run_arg(ip + 5, base);
    mop = (mx_op_t)ip[1];
    bool is_eq;
    if (mop == MX_OP_EQ && mx_run_eq_raw(vm, a, b, &is_eq)) {
        small[0] = mx_obj_of_boolean(is_eq);
        drec = (const mx_resume_t *)ip[2];
        dvals = small;
        dn = 1;
        dresumed = false;
        goto deliver;
    }
    crec = (const mx_resume_t *)ip[2];
    base = mx_run_reserve(vm, base, rb->frame + 8 + MX_RUN_SLACK);
    cw = (uint32_t)(base - vm->thread->slots) + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(base + rb->frame + 4);
    args[0] = a;
    args[1] = b;
    cn = 2;
    goto meta;
}
L_CALL_LUA: {
    MX_STAT(call_lua);
    mx_obj_t fn = mx_obj_of_raw(MX_KIND_CLOSURE, base[ip[1]]);
    mx_ir_block_t *block = mx_obj_get_closure(fn)->block;
    uint32_t n = (uint32_t)ip[2];
    bool is_spread = ip[3] != 0;
    const mx_resume_t *rec = (const mx_resume_t *)ip[4];
    mx_resume_cache_t *cache = (mx_resume_cache_t *)(ip + 5);
    const uintptr_t *argp = ip + 5 + MX_RESUME_CACHE_WORDS;
    uint32_t total = n;
    if (is_spread) {
        total += vm->rets.len;
    }
    uintptr_t *code = NULL;
    uintptr_t key = (uintptr_t)block | (uintptr_t)vm->is_hooked;
    if (!is_spread && cache->key == key) {
        code = (uintptr_t *)cache->code;
    } else if (!is_spread) {
        code = mx_run_cache_find(cache, key);
    }
    if (code == NULL) {
        for (uint32_t i = 0; i < n; i++) {
            kinds[i] = (mx_kind_t)(argp[2 * i + 1] >> 1);
        }
        for (uint32_t i = n; i < total && i < block->func->nparams; i++) {
            kinds[i] = vm->rets.vals[i - n].kind;
        }
        code = mx_version_args(vm, block, kinds, total)->code;
        if (!is_spread) {
            mx_run_cache_put(vm, cache, key, (uintptr_t)code);
        }
    }
    mx_version_t *crb = (mx_version_t *)code[-1];
    uint32_t frame = crb->frame;
    uint32_t above = rb->frame + 2 + frame + 2 + 2 * total + MX_RUN_SLACK;
    if ((uint32_t)(base - vm->thread->slots) + above > vm->thread->limit) {
        ferr = mx_error_raw(vm, "stack overflow");
        frec = rec;
        goto fail;
    }
    base = mx_run_reserve(vm, base, above);
    mx_val_t *callee = base + rb->frame + 2;
    callee[-2].bits = mx_link_make((uint32_t)(base - vm->thread->slots) - 2, MX_LINK_LUA);
    callee[-1].bits = (uintptr_t)rec;
    callee[0] = fn.val;
    uint32_t np = crb->nparams;
    for (uint32_t i = 0; i < np; i++) {
        if (i < n) {
            callee[i + 1] = mx_run_arg(argp + 2 * i, base).val;
        } else if (i < total) {
            callee[i + 1] = vm->rets.vals[i - n].val;
        } else {
            callee[i + 1].bits = 0;
        }
    }
    if (crb->is_vararg) {
        uint32_t nextra = 0;
        if (total > np) {
            nextra = total - np;
        }
        callee[frame].bits = nextra;
        mx_obj_t *spill = (mx_obj_t *)(callee + frame + 2);
        for (uint32_t j = 0; j < nextra; j++) {
            uint32_t i = np + j;
            if (i < n) {
                spill[j] = mx_run_arg(argp + 2 * i, base);
            } else {
                spill[j] = vm->rets.vals[i - n];
            }
        }
    }
    base = callee;
    rb = crb;
    ip = code;
    goto next;
}
L_TAILCALL_LUA: {
    mx_obj_t fn = mx_obj_of_raw(MX_KIND_CLOSURE, base[ip[1]]);
    mx_ir_block_t *block = mx_obj_get_closure(fn)->block;
    uint32_t n = (uint32_t)ip[2];
    bool is_spread = ip[3] != 0;
    mx_resume_cache_t *cache = (mx_resume_cache_t *)(ip + 4);
    const uintptr_t *argp = ip + 4 + MX_RESUME_CACHE_WORDS;
    uint32_t total = n;
    if (is_spread) {
        total += vm->rets.len;
    }
    mx_obj_t *tmp = mx_vm_tmp_reserve(vm, total);
    for (uint32_t i = 0; i < n; i++) {
        tmp[i] = mx_run_arg(argp + 2 * i, base);
    }
    if (is_spread) {
        memcpy(tmp + n, vm->rets.vals, sizeof(mx_obj_t) * vm->rets.len);
    }
    uintptr_t *code = NULL;
    uintptr_t key = (uintptr_t)block | (uintptr_t)vm->is_hooked;
    if (!is_spread && cache->key == key) {
        code = (uintptr_t *)cache->code;
    } else if (!is_spread) {
        code = mx_run_cache_find(cache, key);
    }
    if (code == NULL) {
        code = mx_version_entry(vm, block, tmp, total)->code;
        if (!is_spread) {
            mx_run_cache_put(vm, cache, key, (uintptr_t)code);
        }
    }
    mx_version_t *crb = (mx_version_t *)code[-1];
    uint32_t frame = crb->frame;
    uint32_t above = frame + 2 + 2 * total + MX_RUN_SLACK;
    if ((uint32_t)(base - vm->thread->slots) + above > vm->thread->limit) {
        ferr = mx_error_raw(vm, "stack overflow");
        frec = (const mx_resume_t *)base[-1].bits;
        goto fail;
    }
    base = mx_run_reserve(vm, base, above);
    base[-2].bits = mx_link_with_extra(base[-2].bits, 0);
    base[0] = fn.val;
    uint32_t np = crb->nparams;
    for (uint32_t i = 0; i < np; i++) {
        if (i < total) {
            base[i + 1] = tmp[i].val;
        } else {
            base[i + 1].bits = 0;
        }
    }
    if (crb->is_vararg) {
        if (total > np) {
            mx_run_spill(base, frame, tmp + np, total - np);
        } else {
            mx_run_spill(base, frame, NULL, 0);
        }
    }
    rb = crb;
    ip = code;
    goto next;
}
L_CALL: {
    mx_obj_t fn = mx_run_arg(ip + 1, base);
    uint32_t n = (uint32_t)ip[3];
    bool is_spread = ip[4] != 0;
    crec = (const mx_resume_t *)ip[5];
    const uintptr_t *argp = ip + 8;
    uint32_t total = n;
    if (is_spread) {
        total += vm->rets.len;
    }
    uint32_t above = rb->frame + 6 + 2 * total + MX_RUN_SLACK;
    if ((uint32_t)(base - vm->thread->slots) + above > vm->thread->limit) {
        ferr = mx_error_raw(vm, "stack overflow");
        frec = crec;
        goto fail;
    }
    base = mx_run_reserve(vm, base, above);
    uint32_t boff = (uint32_t)(base - vm->thread->slots);
    cw = boff + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(base + rb->frame + 4);
    for (uint32_t i = 0; i < n; i++) {
        args[i] = mx_run_arg(argp + 2 * i, base);
    }
    if (is_spread) {
        memcpy(args + n, vm->rets.vals, sizeof(mx_obj_t) * vm->rets.len);
    }
    cn = total;
    if (fn.kind == MX_KIND_FFI && ip[6] != fn.val.bits) {
        mx_ffi_fast_t *fast = mx_run_fast_of(vm, mx_obj_get_ffi(fn));
        ip[6] = fn.val.bits;
        ip[7] = (uintptr_t)fast;
        if (fast != NULL && !is_spread && n <= MX_RUN_SMALL - 2 && !rb->is_hooked) {
            ip[0] = (uintptr_t)labels[MX_HOP_CALL_FAST];
        } else {
            ip[0] = (uintptr_t)labels[MX_HOP_CALL];
        }
    }
    if (fn.kind == MX_KIND_FFI || fn.kind == vm->kinds.cfn) {
        cobj = fn;
        ctag = MX_LINK_C;
        goto ffi;
    } else if (fn.kind == MX_KIND_CLOSURE) {
        uint32_t p = cw + 4 + 2 * total;
        mx_version_t *crb = mx_run_push(vm, vm->thread, p, mx_link_make(boff - 2, MX_LINK_LUA), crec, fn, args, total);
        if (crb == NULL) {
            ferr = mx_error_raw(vm, "stack overflow");
            frec = crec;
            goto fail;
        }
        rb = crb;
        base = vm->thread->slots + p + 2;
        ip = rb->code;
        goto next;
    }
    cn = total;
    mx_obj_t handler = mx_run_call_handler(vm, fn);
    while (!mx_obj_is_nil(handler) && cn - total < 15) {
        memmove(args + 1, args, sizeof(mx_obj_t) * cn);
        args[0] = fn;
        cn += 1;
        fn = handler;
        handler = mx_run_call_handler(vm, fn);
    }
    if (!mx_obj_is_nil(handler)) {
        ferr = mx_error_raw(vm, "'__call' chain too long");
        frec = crec;
        goto fail;
    } else if (cn != total && (fn.kind == MX_KIND_FFI || fn.kind == vm->kinds.cfn)) {
        cobj = fn;
        ctag = MX_LINK_C;
        goto ffi;
    } else if (cn != total && fn.kind == MX_KIND_CLOSURE) {
        uint32_t p = cw + 4 + 2 * cn;
        mx_version_t *crb = mx_run_push(vm, vm->thread, p, mx_link_with_extra(mx_link_make(boff - 2, MX_LINK_LUA), cn - total), crec, fn, args, cn);
        if (crb == NULL) {
            ferr = mx_error_raw(vm, "stack overflow");
            frec = crec;
            goto fail;
        }
        rb = crb;
        base = vm->thread->slots + p + 2;
        ip = rb->code;
        goto next;
    }
    memmove(args + 1, args, sizeof(mx_obj_t) * cn);
    args[0] = fn;
    cn += 1;
    mop = MX_OP_CALL;
    goto meta;
}
L_CALL_FAST: {
    mx_obj_t fn = mx_run_arg(ip + 1, base);
    if (fn.kind == MX_KIND_FFI && fn.val.bits == ip[6]) {
        uint32_t n = (uint32_t)ip[3];
        for (uint32_t i = 0; i < n; i++) {
            small[i] = mx_run_arg(ip + 8 + 2 * i, base);
        }
        vm->rets.len = 1;
        vm->rets.vals[0] = mx_obj_of_nil();
        if (((mx_ffi_fast_t *)ip[7])(vm, n, small)) {
            MX_STAT(call_fast);
            MX_DELIVER_RESUMED((const mx_resume_t *)ip[5], small, vm->rets.len);
        }
    }
    goto L_CALL;
}
L_RETURN0: {
    rvals = NULL;
    rn = 0;
    goto ret;
}
L_RETURN1: {
    small[0] = mx_run_arg(ip + 1, base);
    rvals = small;
    rn = 1;
    goto ret;
}
L_RETURN: {
    uint32_t n = (uint32_t)ip[1];
    bool is_spread = ip[2] != 0;
    if (!is_spread && n <= MX_RUN_SMALL) {
        for (uint32_t i = 0; i < n; i++) {
            small[i] = mx_run_arg(ip + 3 + 2 * i, base);
        }
        rvals = small;
        rn = n;
        goto ret;
    }
    uint32_t extra = 0;
    if (is_spread) {
        extra = vm->rets.len;
    }
    mx_obj_t *out = mx_vm_rets_reserve(vm, n + extra);
    memmove(out + n, out, sizeof(mx_obj_t) * extra);
    for (uint32_t i = 0; i < n; i++) {
        out[i] = mx_run_arg(ip + 3 + 2 * i, base);
    }
    vm->rets.len = n + extra;
    rvals = vm->rets.vals;
    rn = n + extra;
    goto ret;
}

next:
    if (vm->interrupt) {
        goto safepoint;
    }
    MX_DISPATCH();

safepoint: {
    vm->interrupt = false;
    mx_thread_t *th = vm->thread;
    uint32_t boff = (uint32_t)(base - th->slots);
    uint32_t top = boff + rb->frame + 2;
    const mx_ir_func_t *func = rb->block->func;
    if (func->is_vararg && rb->block->is_func && rb->start_at == 0) {
        top += 2 * (uint32_t)base[rb->frame].bits;
    }
    mx_thread_reserve(vm, th, top + 2 + MX_RUN_SLACK);
    th->slots[top].bits = mx_link_make(boff - 2, MX_LINK_SAFEPOINT);
    th->slots[top + 1].bits = (uintptr_t)rb;
    th->frame = top;
    th->top = top + 2;
    bool ran = false;
    if (vm->gc_pending != 0 && vm->no_gc == 0 && !vm->is_gc_stopped) {
        ran = mx_run_collect(vm);
    }
    base = th->slots + boff;
    if (ran || rb->is_hooked != vm->is_hooked) {
        rb = mx_version_again(vm, rb, ran);
        ip = rb->code;
    }
    MX_DISPATCH();
}

branch: {
    if (*bc != 0) {
        MX_ENTER(*bc);
    }
    rb = mx_version_edge(vm, rb->out, rb->nout, (mx_ir_block_t *)bc[-2]);
    *bc = (uintptr_t)rb->code;
    ip = rb->code;
    goto next;
}

deliver: {
    const mx_resume_t *rec = drec;
    uintptr_t key;
    uintptr_t *code;
    switch (rec->kind) {
        case MX_RESUME_VALUES: {
            uint32_t want = rec->want;
            uint32_t nout;
            if (want == MX_IR_WANT_MULTI) {
                mx_vm_rets_set(vm, dvals, dn);
                key = 1;
                nout = 0;
            } else {
                for (uint32_t i = 0; i < want; i++) {
                    mx_obj_t v = mx_obj_of_nil();
                    if (i < dn) {
                        v = dvals[i];
                    }
                    base[rec->out + i] = v.val;
                    kinds[i] = v.kind;
                }
                if (rec->has_cell_out) {
                    mx_run_cells_write(rec, base, kinds, want);
                }
                key = (uintptr_t)mx_version_kinds_key(kinds, want);
                nout = want;
            }
            if (key != 0) {
                key |= mx_run_key_flags(vm, dresumed);
                code = mx_run_cache_find(&rec->cache, key);
                if (code != NULL) {
                    MX_STAT(deliver_found);
                    ip = code;
                    rb = (mx_version_t *)ip[-1];
                    goto next;
                }
            }
            MX_STAT(deliver_new);
            rb = mx_version_values(vm, rec, kinds, nout, dresumed);
            if (key != 0) {
                mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key,(uintptr_t)rb->code);
            }
            ip = rb->code;
            goto next;
        }
        case MX_RESUME_AT: {
            mx_obj_t v = mx_obj_of_nil();
            if (dn > 0) {
                v = dvals[0];
            }
            base[rec->out] = v.val;
            kinds[0] = v.kind;
            if (rec->has_cell_out) {
                mx_run_cells_write(rec, base, kinds, 1);
            }
            key = (uintptr_t)v.kind | mx_run_key_flags(vm, dresumed);
            code = mx_run_cache_find(&rec->cache, key);
            if (code == NULL) {
                MX_STAT(deliver_new);
                rb = mx_version_at(vm, rec, kinds, 1, dresumed);
                code = rb->code;
                mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key,(uintptr_t)code);
            }
            ip = code;
            rb = (mx_version_t *)ip[-1];
            goto next;
        }
        case MX_RESUME_BRANCH: {
            bool is_true = dn > 0 && mx_obj_is_truthy(dvals[0]);
            key = ((uintptr_t)is_true + 1) | mx_run_key_flags(vm, dresumed);
            code = mx_run_cache_find(&rec->cache, key);
            if (code == NULL) {
                MX_STAT(deliver_new);
                rb = mx_version_branch(vm, rec, is_true, dresumed);
                code = rb->code;
                mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key,(uintptr_t)code);
            }
            ip = code;
            rb = (mx_version_t *)ip[-1];
            goto next;
        }
        case MX_RESUME_VOID: {
            MX_STAT(deliver_void);
            if (!rec->is_override && rec->rb->is_hooked == vm->is_hooked && !(dresumed && mx_run_cells_stale(rec, base))) {
                rb = rec->rb;
                ip = rec->next_ip;
                MX_DISPATCH();
            }
            rb = mx_version_at(vm, rec, NULL, 0, dresumed);
            ip = rb->code;
            goto next;
        }
        case MX_RESUME_RETURN: {
            rvals = dvals;
            rn = dn;
            goto ret;
        }
        default: {
            mx_fatal("run: resume record cannot take values");
        }
    }
}

ret: {
    mx_thread_t *th = vm->thread;
    mx_val_t *slots = th->slots;
    uintptr_t link = base[-2].bits;
    const mx_resume_t *rec = (const mx_resume_t *)base[-1].bits;
    uint32_t p = (uint32_t)(base - slots) - 2;
    switch (mx_link_tag(link)) {
        case MX_LINK_LUA: {
            base = slots + mx_link_caller(link) + 2;
            MX_DELIVER_RESUMED(rec, rvals, rn);
        }
        case MX_LINK_ENTRY: {
            mx_vm_rets_set(vm, rvals, rn);
            result = (mx_run_result_t) { .error = mx_obj_of_nil(), .status = MX_RUN_OK };
            goto done;
        }
        case MX_LINK_PROTECT:
        case MX_LINK_PROTECT_MSGH: {
            th->protects -= 1;
            mx_vm_rets_prepend(vm, mx_obj_of_boolean(true), rvals, rn);
            cw = mx_link_caller(link);
            rvals = vm->rets.vals;
            rn += 1;
            goto cret;
        }
        case MX_LINK_CALLCC: {
            vm->hooks.cont_done(vm, *mx_thread_objs(th, p - 2));
            cw = mx_link_caller(link);
            goto cret;
        }
        case MX_LINK_BOTTOM: {
            goto co_done;
        }
        default: {
            mx_fatal("run: bad frame link");
        }
    }
}

co_done: {
    mx_thread_t *co = vm->thread;
    co->status = MX_THREAD_DEAD;
    co->frame = MX_THREAD_NONE;
    co->top = 0;
    mx_thread_t *resumer = co->resumer;
    co->resumer = NULL;
    if (co->is_c_resumed) {
        mx_vm_rets_set(vm, rvals, rn);
        result = (mx_run_result_t) { .error = mx_obj_of_nil(), .status = MX_RUN_OK };
        goto done;
    }
    vm->thread = resumer;
    resumer->status = MX_THREAD_RUNNING;
    if (co->shape == MX_THREAD_SHAPE_STATUS) {
        mx_vm_rets_prepend(vm, mx_obj_of_boolean(true), rvals, rn);
        rvals = vm->rets.vals;
        rn += 1;
    }
    cw = resumer->frame;
    goto cret;
}

set: {
    if (mx_obj_is_table(st) && mx_run_set_is_raw(vm, mx_obj_get_table(st), sk)) {
        mx_obj_t err = mx_run_set_raw(vm, mx_obj_get_table(st), sk, sv, sslot);
        if (mx_obj_is_error(err)) {
            ferr = err;
            frec = crec;
            goto fail;
        }
        MX_DISPATCH();
    }
    base = mx_run_reserve(vm, base, rb->frame + 10 + MX_RUN_SLACK);
    cw = (uint32_t)(base - vm->thread->slots) + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(base + rb->frame + 4);
    args[0] = st;
    args[1] = sk;
    args[2] = sv;
    cn = 3;
    mop = MX_OP_SET;
    goto meta;
}

meta: {
    cobj = vm->kinds.ptr[mx_thread_objs(vm->thread, cw + 4)->kind].ops[mop];
    ctag = MX_LINK_META;
    goto ffi;
}

ffi: {
    mx_thread_t *th = vm->thread;
    uint32_t boff = (uint32_t)(base - th->slots);
    th->slots[cw].bits = mx_link_make(boff - 2, ctag);
    th->slots[cw + 1].bits = (uintptr_t)crec;
    *mx_thread_objs(th, cw + 2) = cobj;
    th->frame = cw;
    th->top = cw + 4 + 2 * cn;
    mx_ffi_t *fn;
    if (cobj.kind == MX_KIND_FFI) {
        fn = mx_obj_get_ffi(cobj);
    } else {
        fn = ((mx_obj_cfn_t *)mx_ref_obj(cobj.val.ref))->fn;
    }
    if (vm->is_hooked && ctag == MX_LINK_C && !mx_run_site_is_prim(crec)) {
        mx_obj_t herr = vm->hooks.debug(vm, th, crec, 1);
        base = th->slots + boff;
        if (mx_obj_is_error(herr)) {
            ferr = herr;
            frec = crec;
            goto fail_c;
        }
    }
    MX_STAT(call_ffi);
    vm->rets.len = 1;
    vm->rets.vals[0] = mx_obj_of_nil();
    vm->pending.is_ok = true;
    vm->no_gc += 1;
    fn(vm, cn, mx_thread_objs(th, cw + 4));
    vm->no_gc -= 1;
    vm->pending.is_ok = false;
    base = th->slots + boff;
    if (vm->pending.is_set) {
        goto pending;
    }
    mx_obj_t *res = mx_thread_objs(th, cw + 4);
    if (mx_obj_is_error(res[0])) {
        ferr = res[0];
        frec = crec;
        goto fail_c;
    }
    if (vm->is_hooked && ctag == MX_LINK_C && !mx_run_site_is_prim(crec)) {
        if (th->top < cw + 4 + 2 * vm->rets.len) {
            th->top = cw + 4 + 2 * vm->rets.len;
        }
        mx_vm_rets_push(vm);
        mx_obj_t herr = vm->hooks.debug(vm, th, crec, 2);
        mx_vm_rets_pop(vm);
        base = th->slots + boff;
        res = mx_thread_objs(th, cw + 4);
        if (mx_obj_is_error(herr)) {
            ferr = herr;
            frec = crec;
            goto fail_c;
        }
    }
    if (ctag == MX_LINK_PROTECT_C || ctag == MX_LINK_PROTECT_MSGH_C || ctag == MX_LINK_BOTTOM_C) {
        rvals = res;
        rn = vm->rets.len;
        goto cret;
    }
    MX_DELIVER_RESUMED((const mx_resume_t *)th->slots[cw + 1].bits, res, vm->rets.len);
}

cret: {
    mx_val_t *slots = vm->thread->slots;
    mx_link_tag_t tag = mx_link_tag(slots[cw].bits);
    while (tag == MX_LINK_PROTECT_C || tag == MX_LINK_PROTECT_MSGH_C) {
        vm->thread->protects -= 1;
        mx_vm_rets_prepend(vm, mx_obj_of_boolean(true), rvals, rn);
        rvals = vm->rets.vals;
        rn += 1;
        cw = mx_link_caller(slots[cw].bits);
        tag = mx_link_tag(slots[cw].bits);
    }
    if (tag == MX_LINK_BOTTOM_C) {
        goto co_done;
    }
    base = slots + mx_link_caller(slots[cw].bits) + 2;
    MX_DELIVER_RESUMED((const mx_resume_t *)slots[cw + 1].bits, rvals, rn);
}

pending: {
    mx_thread_t *th = vm->thread;
    mx_val_t *slots = th->slots;
    vm->pending.is_set = false;
    mx_run_pending_t call = vm->pending.call;
    const mx_obj_t *pargs = vm->pending.args;
    uint32_t pn = call.nargs;
    uint32_t cur = (uint32_t)(base - slots) - 2;
    crec = (const mx_resume_t *)slots[cw + 1].bits;
    switch (call.kind) {
        case MX_RUN_PENDING_KIND_CALL: {
            if (!mx_obj_is_closure(call.fn)) {
                if (cw + 4 + 2 * pn + MX_RUN_SLACK > th->limit) {
                    ferr = mx_error_raw(vm, "stack overflow");
                    frec = crec;
                    goto fail_c;
                }
                mx_thread_reserve(vm, th, cw + 4 + 2 * pn + MX_RUN_SLACK);
                memmove(mx_thread_objs(th, cw + 4), pargs, sizeof(mx_obj_t) * pn);
                base = th->slots + (base - slots);
                cobj = call.fn;
                cn = pn;
                ctag = mx_link_tag(th->slots[cw].bits);
                goto ffi;
            }
            mx_link_tag_t tag;
            if (mx_link_tag(slots[cw].bits) == MX_LINK_PROTECT_C) {
                tag = MX_LINK_PROTECT;
            } else if (mx_link_tag(slots[cw].bits) == MX_LINK_PROTECT_MSGH_C) {
                tag = MX_LINK_PROTECT_MSGH;
            } else if (mx_link_tag(slots[cw].bits) == MX_LINK_BOTTOM_C) {
                tag = MX_LINK_BOTTOM;
            } else {
                tag = MX_LINK_LUA;
            }
            mx_version_t *crb = mx_run_push(vm, th, cw, mx_link_make(cur, tag), crec, call.fn, pargs, pn);
            if (crb == NULL) {
                ferr = mx_error_raw(vm, "stack overflow");
                frec = crec;
                goto fail_c;
            }
            rb = crb;
            base = th->slots + cw + 2;
            ip = rb->code;
            goto next;
        }
        case MX_RUN_PENDING_KIND_PROTECT:
        case MX_RUN_PENDING_KIND_PROTECT_MSGH:
        case MX_RUN_PENDING_KIND_CALLCC: {
            mx_link_tag_t tag;
            if (call.kind == MX_RUN_PENDING_KIND_PROTECT) {
                tag = MX_LINK_PROTECT;
            } else if (call.kind == MX_RUN_PENDING_KIND_PROTECT_MSGH) {
                tag = MX_LINK_PROTECT_MSGH;
            } else {
                tag = MX_LINK_CALLCC;
            }
            bool is_protect = call.kind != MX_RUN_PENDING_KIND_CALLCC;
            if (is_protect && th->nest + th->protects >= 200) {
                mx_obj_t err = mx_error_value(vm, mx_obj_new_string(vm, "C stack overflow"));
                if (call.kind == MX_RUN_PENDING_KIND_PROTECT_MSGH) {
                    err = mx_run_msgh(vm, call.extra, err);
                }
                small[0] = mx_obj_of_boolean(false);
                small[1] = mx_error_payload(err);
                rvals = small;
                rn = 2;
                goto cret;
            }
            *mx_thread_objs(th, cw + 4) = call.extra;
            if (!mx_obj_is_closure(call.fn)) {
                uint32_t q = cw + 6;
                if (q + 4 + 2 * pn + MX_RUN_SLACK > th->limit) {
                    ferr = mx_error_raw(vm, "stack overflow");
                    frec = crec;
                    goto fail_c;
                }
                th->protects += 1;
                mx_thread_reserve(vm, th, q + 4 + 2 * pn + MX_RUN_SLACK);
                memcpy(mx_thread_objs(th, q + 4), pargs, sizeof(mx_obj_t) * pn);
                base = th->slots + cw + 2;
                cw = q;
                crec = NULL;
                cobj = call.fn;
                cn = pn;
                if (tag == MX_LINK_PROTECT) {
                    ctag = MX_LINK_PROTECT_C;
                } else {
                    ctag = MX_LINK_PROTECT_MSGH_C;
                }
                goto ffi;
            }
            mx_version_t *crb = mx_run_push(vm, th, cw + 6, mx_link_make(cw, tag), crec, call.fn, pargs, pn);
            if (crb == NULL) {
                ferr = mx_error_raw(vm, "stack overflow");
                frec = crec;
                goto fail_c;
            }
            if (is_protect) {
                th->protects += 1;
            }
            rb = crb;
            base = th->slots + cw + 8;
            ip = rb->code;
            goto next;
        }
        case MX_RUN_PENDING_KIND_RESUME:
        case MX_RUN_PENDING_KIND_RESUME_WRAP: {
            mx_thread_t *co = call.thread;
            uint32_t nest = th->nest + th->protects + 1;
            if (nest >= 200 && call.kind == MX_RUN_PENDING_KIND_RESUME) {
                small[0] = mx_obj_of_boolean(false);
                small[1] = mx_obj_new_string(vm, "C stack overflow");
                rvals = small;
                rn = 2;
                goto cret;
            } else if (nest >= 200) {
                ferr = mx_error_raw(vm, "C stack overflow");
                frec = crec;
                goto fail_c;
            }
            co->nest = nest;
            th->status = MX_THREAD_NORMAL;
            th->top = cw + 4;
            co->resumer = th;
            co->is_c_resumed = false;
            co->token = vm->token_cur;
            if (call.kind == MX_RUN_PENDING_KIND_RESUME) {
                co->shape = MX_THREAD_SHAPE_STATUS;
            } else {
                co->shape = MX_THREAD_SHAPE_WRAP;
            }
            sw_to = co;
            sw_vals = pargs;
            sw_n = pn;
            goto switch_to;
        }
        case MX_RUN_PENDING_KIND_YIELD: {
            mx_thread_t *co = th;
            co->status = MX_THREAD_SUSPENDED;
            co->top = cw + 4;
            if (mx_obj_is_truthy(call.extra)) {
                co->status = MX_THREAD_DEAD;
                co->frame = MX_THREAD_NONE;
                co->top = 0;
            }
            if (co->is_c_resumed) {
                co->resumer = NULL;
                mx_vm_rets_set(vm, pargs, pn);
                result = (mx_run_result_t) { .error = mx_obj_of_nil(), .status = MX_RUN_YIELD };
                goto done;
            }
            th = co->resumer;
            co->resumer = NULL;
            vm->thread = th;
            th->status = MX_THREAD_RUNNING;
            if (co->shape == MX_THREAD_SHAPE_STATUS) {
                mx_vm_rets_prepend(vm, mx_obj_of_boolean(true), pargs, pn);
                rvals = vm->rets.vals;
                rn = pn + 1;
            } else {
                rvals = pargs;
                rn = pn;
            }
            cw = th->frame;
            goto cret;
        }
        case MX_RUN_PENDING_KIND_KJUMP: {
            uint32_t p = (uint32_t)mx_obj_get_int(call.extra);
            uint32_t w = mx_link_caller(slots[p].bits);
            const mx_resume_t *rec = (const mx_resume_t *)slots[p + 1].bits;
            for (uint32_t q = th->frame; q != p; q = mx_link_caller(slots[q].bits)) {
                mx_link_tag_t t = mx_link_tag(slots[q].bits);
                if (t == MX_LINK_PROTECT || t == MX_LINK_PROTECT_MSGH || t == MX_LINK_PROTECT_C || t == MX_LINK_PROTECT_MSGH_C) {
                    th->protects -= 1;
                }
            }
            mx_thread_close_upvals(vm, th, p + 2);
            th->frame = w;
            th->top = p;
            mx_vm_rets_set(vm, pargs, pn);
            mx_vm_rets_push(vm);
            mx_obj_t err = vm->hooks.tbc_unwind(vm, th, p, mx_obj_of_nil());
            mx_vm_rets_pop(vm);
            if (mx_obj_is_error(err)) {
                ferr = err;
                frec = rec;
                goto fail_c;
            }
            cw = w;
            rvals = vm->rets.vals;
            rn = vm->rets.len;
            goto cret;
        }
        default: {
            mx_fatal("run: bad pending call");
        }
    }
}

switch_to: {
    mx_thread_t *co = sw_to;
    vm->thread = co;
    co->status = MX_THREAD_RUNNING;
    if (co->frame != MX_THREAD_NONE) {
        cw = co->frame;
        if (vm->is_hooked && mx_link_tag(co->slots[cw].bits) == MX_LINK_C) {
            mx_vm_rets_set(vm, sw_vals, sw_n);
            mx_vm_rets_push(vm);
            mx_obj_t herr = vm->hooks.debug(vm, co, (const mx_resume_t *)co->slots[cw + 1].bits, 2);
            mx_vm_rets_pop(vm);
            if (mx_obj_is_error(herr)) {
                ferr = herr;
                frec = (const mx_resume_t *)co->slots[cw + 1].bits;
                goto fail_c;
            }
            sw_vals = vm->rets.vals;
        }
        rvals = sw_vals;
        rn = sw_n;
        goto cret;
    }
    co->protects = 0;
    mx_thread_reserve(vm, co, MX_THREAD_MIN_WORDS + 2 * sw_n);
    *mx_thread_objs(co, 0) = mx_obj_of_nil();
    if (!mx_obj_is_closure(co->entry_fn)) {
        memcpy(mx_thread_objs(co, 6), sw_vals, sizeof(mx_obj_t) * sw_n);
        base = co->slots + 1;
        cw = 2;
        crec = NULL;
        cobj = co->entry_fn;
        cn = sw_n;
        ctag = MX_LINK_BOTTOM_C;
        goto ffi;
    }
    rb = mx_run_push(vm, co, 2, mx_link_make(MX_THREAD_NONE, MX_LINK_BOTTOM), NULL, co->entry_fn, sw_vals, sw_n);
    base = co->slots + 4;
    ip = rb->code;
    goto next;
}

fail: {
    mx_thread_t *th = vm->thread;
    base = mx_run_reserve(vm, base, rb->frame + 2 + MX_RUN_SLACK);
    uint32_t boff = (uint32_t)(base - th->slots);
    uint32_t w = boff + rb->frame;
    th->slots[w].bits = mx_link_make(boff - 2, MX_LINK_HOOK);
    th->slots[w + 1].bits = (uintptr_t)frec;
    th->frame = w;
    th->top = w + 2;
    goto fail_c;
}

fail_c: {
    mx_thread_t *th = vm->thread;
    mx_val_t *slots = th->slots;
    mx_error_t *e = mx_obj_get_error(ferr);
    if (e->is_raw && frec != NULL) {
        mx_location_range_t range = mx_resume_range(frec);
        mx_error_locate(vm, e, &range);
    } else if (e->is_raw) {
        mx_error_locate(vm, e, NULL);
    }
    uint32_t p = th->frame;
    mx_link_tag_t tag = mx_link_tag(slots[p].bits);
    while (tag != MX_LINK_ENTRY && tag != MX_LINK_BOTTOM && tag != MX_LINK_BOTTOM_C
        && (th->is_closing || (tag != MX_LINK_PROTECT && tag != MX_LINK_PROTECT_MSGH && tag != MX_LINK_PROTECT_C && tag != MX_LINK_PROTECT_MSGH_C))) {
        p = mx_link_caller(slots[p].bits);
        tag = mx_link_tag(slots[p].bits);
    }
    if (tag == MX_LINK_BOTTOM || tag == MX_LINK_BOTTOM_C) {
        goto co_fail;
    } else if (th->is_closing) {
        result = (mx_run_result_t) { .error = ferr, .status = MX_RUN_ERROR };
        goto done;
    } else if (tag != MX_LINK_ENTRY) {
        th->protects -= 1;
    }
    mx_obj_t handler = *mx_thread_objs(th, p - 2);
    bool has_msgh = tag == MX_LINK_PROTECT_MSGH || tag == MX_LINK_PROTECT_MSGH_C || (tag == MX_LINK_ENTRY && !mx_obj_is_nil(handler));
    if (!e->is_handled && has_msgh) {
        ferr = mx_run_msgh(vm, handler, ferr);
        slots = th->slots;
    }
    uintptr_t link = slots[p].bits;
    mx_thread_close_upvals(vm, th, p + 2);
    th->frame = mx_link_caller(link);
    th->top = p;
    mx_vm_hold(vm, ferr);
    ferr = vm->hooks.tbc_unwind(vm, th, p, ferr);
    mx_vm_release(vm);
    if (tag == MX_LINK_ENTRY) {
        result = (mx_run_result_t) { .error = ferr, .status = MX_RUN_ERROR };
        goto done;
    }
    small[0] = mx_obj_of_boolean(false);
    small[1] = mx_error_payload(ferr);
    cw = mx_link_caller(link);
    rvals = small;
    rn = 2;
    goto cret;
}

co_fail: {
    mx_thread_t *co = vm->thread;
    if (co->is_closing) {
        co->is_closing = false;
        mx_obj_t err = vm->hooks.tbc_unwind(vm, co, 0, mx_obj_of_nil());
        mx_thread_close_upvals(vm, co, 0);
        if (!mx_obj_is_error(err)) {
            rvals = small;
            rn = 0;
            goto co_done;
        }
        ferr = err;
    }
    co->status = MX_THREAD_DEAD;
    co->is_failed = true;
    co->failure = ferr;
    mx_thread_t *resumer = co->resumer;
    co->resumer = NULL;
    if (co->is_c_resumed) {
        result = (mx_run_result_t) { .error = ferr, .status = MX_RUN_ERROR };
        goto done;
    }
    vm->thread = resumer;
    resumer->status = MX_THREAD_RUNNING;
    const mx_resume_t *rec = (const mx_resume_t *)resumer->slots[resumer->frame + 1].bits;
    if (co->shape == MX_THREAD_SHAPE_STATUS) {
        small[0] = mx_obj_of_boolean(false);
        small[1] = mx_error_payload(ferr);
        cw = resumer->frame;
        rvals = small;
        rn = 2;
        goto cret;
    }
    mx_vm_hold(vm, ferr);
    mx_obj_t closed = vm->hooks.tbc_unwind(vm, co, 0, ferr);
    mx_vm_release(vm);
    ferr = mx_run_wrap_error(vm, rec, closed);
    frec = rec;
    goto fail_c;
}

done:
    return result;
}
