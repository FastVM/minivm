
#include "jit.h"
#include "../run/op.h"
#include "../value/num.h"
#include "../value/obj.h"

void mx_jit_rt_hop(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_hop_t hop) {
    mx_val_t imm = { .bits = ip[3] };
    switch (hop) {
        case MX_HOP_SHL_I64_RR: {
            base[ip[1]].i64 = mx_num_shl(base[ip[2]].i64, base[ip[3]].i64);
            break;
        }
        case MX_HOP_SHL_I64_IR: {
            base[ip[1]].i64 = mx_num_shl((int64_t)ip[3], base[ip[2]].i64);
            break;
        }
        case MX_HOP_SHR_I64_RR: {
            base[ip[1]].i64 = mx_num_shr(base[ip[2]].i64, base[ip[3]].i64);
            break;
        }
        case MX_HOP_SHR_I64_IR: {
            base[ip[1]].i64 = mx_num_shr((int64_t)ip[3], base[ip[2]].i64);
            break;
        }
        case MX_HOP_IDIV_F64_RR: {
            base[ip[1]].f64 = floor(base[ip[2]].f64 / base[ip[3]].f64);
            break;
        }
        case MX_HOP_IDIV_F64_RI: {
            base[ip[1]].f64 = floor(base[ip[2]].f64 / imm.f64);
            break;
        }
        case MX_HOP_IDIV_F64_IR: {
            base[ip[1]].f64 = floor(imm.f64 / base[ip[2]].f64);
            break;
        }
        case MX_HOP_MOD_F64_RR: {
            base[ip[1]].f64 = mx_num_fmod(base[ip[2]].f64, base[ip[3]].f64);
            break;
        }
        case MX_HOP_MOD_F64_RI: {
            base[ip[1]].f64 = mx_num_fmod(base[ip[2]].f64, imm.f64);
            break;
        }
        case MX_HOP_MOD_F64_IR: {
            base[ip[1]].f64 = mx_num_fmod(imm.f64, base[ip[2]].f64);
            break;
        }
        case MX_HOP_POW_F64_RR: {
            base[ip[1]].f64 = pow(base[ip[2]].f64, base[ip[3]].f64);
            break;
        }
        case MX_HOP_POW_F64_RI: {
            base[ip[1]].f64 = pow(base[ip[2]].f64, imm.f64);
            break;
        }
        case MX_HOP_POW_F64_IR: {
            base[ip[1]].f64 = pow(imm.f64, base[ip[2]].f64);
            break;
        }
        case MX_HOP_POW_INT_INT: {
            base[ip[1]].f64 = pow((double)base[ip[2]].i64, (double)base[ip[3]].i64);
            break;
        }
        case MX_HOP_TOSTR_I64: {
            mx_run_op_tostr_i64(vm, base, ip);
            break;
        }
        case MX_HOP_TOSTR_F64: {
            mx_run_op_tostr_f64(vm, base, ip);
            break;
        }
        case MX_HOP_CONCAT: {
            mx_run_op_concat(vm, base, ip);
            break;
        }
        case MX_HOP_CONCAT_SI: {
            mx_run_op_concat_si(vm, base, ip);
            break;
        }
        case MX_HOP_CONCAT_IS: {
            mx_run_op_concat_is(vm, base, ip);
            break;
        }
        case MX_HOP_TABLE_NEW: {
            mx_run_op_table_new(vm, base, ip);
            break;
        }
        case MX_HOP_TABLE_INIT: {
            mx_run_op_table_init(vm, base, ip);
            break;
        }
        case MX_HOP_TABLE_INIT_IK: {
            mx_run_op_table_init_ik(vm, base, ip);
            break;
        }
        case MX_HOP_TABLE_INIT_SK: {
            mx_run_op_table_init_sk(vm, base, ip);
            break;
        }
        case MX_HOP_CLOSURE: {
            mx_run_op_closure(vm, base, ip);
            break;
        }
        case MX_HOP_VARARGS: {
            mx_obj_closure_t *cl = mx_ref_obj(base[0].ref);
            mx_run_op_varargs(vm, base, ip, mx_ir_func_frame(cl->block->func));
            break;
        }
        case MX_HOP_UPVAL_SET: {
            mx_obj_closure_t *cl = mx_ref_obj(base[0].ref);
            mx_upval_set(vm, cl->upvals[ip[1]], mx_run_arg(ip + 2, base));
            break;
        }
        case MX_HOP_CLOSE: {
            mx_thread_close_upvals(vm, vm->thread, (uint32_t)(base - vm->thread->slots) + (uint32_t)ip[1]);
            break;
        }
        default: {
            mx_fatal("jit: hop has no helper");
        }
    }
}

bool mx_jit_rt_cmp(mx_t *vm, mx_val_t *base, const uintptr_t *ip, mx_hop_t hop) {
    switch (hop) {
        case MX_HOP_BR_LT_INT_F64: {
            return mx_obj_int_lt_float(base[ip[5]].i64, base[ip[6]].f64);
        }
        case MX_HOP_BR_LE_INT_F64: {
            return mx_obj_int_le_float(base[ip[5]].i64, base[ip[6]].f64);
        }
        case MX_HOP_BR_EQ_INT_F64: {
            return mx_obj_int_eq_float(base[ip[5]].i64, base[ip[6]].f64);
        }
        case MX_HOP_BR_LT_F64_INT: {
            return mx_obj_float_lt_int(base[ip[5]].f64, base[ip[6]].i64);
        }
        case MX_HOP_BR_LE_F64_INT: {
            return mx_obj_float_le_int(base[ip[5]].f64, base[ip[6]].i64);
        }
        case MX_HOP_BR_LT_STR: {
            mx_str_t *x = mx_str_flat(mx_ref_obj(base[ip[5]].ref));
            mx_str_t *y = mx_str_flat(mx_ref_obj(base[ip[6]].ref));
            return mx_obj_str_cmp(x, y) < 0;
        }
        case MX_HOP_BR_LE_STR: {
            mx_str_t *x = mx_str_flat(mx_ref_obj(base[ip[5]].ref));
            mx_str_t *y = mx_str_flat(mx_ref_obj(base[ip[6]].ref));
            return mx_obj_str_cmp(x, y) <= 0;
        }
        case MX_HOP_BR_EQ_STR: {
            mx_str_t *x = mx_ref_obj(base[ip[5]].ref);
            mx_str_t *y = mx_ref_obj(base[ip[6]].ref);
            if (x == y) {
                return true;
            } else if (x->is_interned && y->is_interned) {
                return false;
            }
            mx_str_t *fx = mx_str_flat(x);
            mx_str_t *fy = mx_str_flat(y);
            return fx->len == fy->len && memcmp(fx->buf, fy->buf, fx->len) == 0;
        }
        default: {
            mx_fatal("jit: hop has no comparison");
            return false;
        }
    }
}

bool mx_jit_rt_set(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_hop_t hop) {
    mx_obj_t t;
    mx_obj_t k;
    mx_obj_t v;
    uintptr_t *slot;
    switch (hop) {
        case MX_HOP_TABLE_SET_SK: {
            t = mx_obj_of_table(mx_ref_obj(base[ip[1]].ref));
            k = mx_obj_of_raw(MX_KIND_STRING, (mx_val_t) { .bits = ip[2] });
            v = mx_run_arg(ip + 3, base);
            slot = &ip[6];
            break;
        }
        case MX_HOP_TABLE_SET_IR: {
            t = mx_obj_of_table(mx_ref_obj(base[ip[1]].ref));
            k = mx_obj_of_int(base[ip[2]].i64);
            v = mx_run_arg(ip + 3, base);
            slot = &ip[6];
            break;
        }
        case MX_HOP_TABLE_SET_IK: {
            t = mx_obj_of_table(mx_ref_obj(base[ip[1]].ref));
            k = mx_obj_of_int((int64_t)ip[2]);
            v = mx_run_arg(ip + 3, base);
            slot = &ip[6];
            break;
        }
        default: {
            t = mx_run_arg(ip + 1, base);
            k = mx_run_arg(ip + 3, base);
            v = mx_run_arg(ip + 5, base);
            slot = &ip[8];
            break;
        }
    }
    bool is_raw = t.kind == vm->kinds.table && mx_obj_get_table(t)->is_plain && mx_obj_get_table(t)->meta == NULL;
    if (!is_raw && !(mx_obj_is_table(t) && mx_run_set_is_raw(vm, mx_obj_get_table(t), k))) {
        return false;
    }
    return !mx_obj_is_error(mx_run_set_raw(vm, mx_obj_get_table(t), k, v, slot));
}

uintptr_t *mx_jit_rt_call_fast(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    mx_obj_t small[MX_RUN_SMALL];
    const mx_resume_t *rec = (const mx_resume_t *)ip[5];
    mx_obj_t fn = mx_run_arg(ip + 1, base);
    uint32_t n = (uint32_t)ip[3];
    uint32_t nspread;
    if (ip[4] != 0) {
        nspread = vm->rets.len;
    } else {
        nspread = 0;
    }
    if (fn.kind != MX_KIND_FFI || fn.val.bits != ip[6] || n + nspread > MX_RUN_SMALL || rec->kind != MX_RESUME_VALUES || (rec->want > MX_JIT_WANT_MAX && rec->want != MX_IR_WANT_MULTI)) {
        return NULL;
    }
    for (uint32_t i = 0; i < n; i++) {
        small[i] = mx_run_arg(ip + 8 + 2 * i, base);
    }
    if (nspread != 0) {
        memcpy(small + n, vm->rets.vals, sizeof(mx_obj_t) * nspread);
    }
    vm->rets.len = 1;
    vm->rets.vals[0] = mx_obj_of_nil();
    if (!((mx_ffi_fast_t *)ip[7])(vm, n + nspread, small)) {
        if (nspread != 0) {
            memcpy(vm->rets.vals, small + n, sizeof(mx_obj_t) * nspread);
        }
        vm->rets.len = nspread;
        return NULL;
    }
    return mx_jit_rt_deliver(vm, base, rec, small);
}

uintptr_t *mx_jit_rt_multi(mx_t *vm, const mx_resume_t *rec, const mx_obj_t *vals, uint32_t n) {
    mx_vm_rets_set(vm, vals, n);
    uintptr_t key = 1 | mx_run_key_flags(vm, true);
    uintptr_t *code = mx_run_cache_find(&rec->cache, key);
    if (code == NULL) {
        code = mx_version_values(vm, rec, NULL, 0, true)->code;
        mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key, (uintptr_t)code);
    }
    return code;
}

uintptr_t *mx_jit_rt_deliver(mx_t *vm, mx_val_t *base, const mx_resume_t *rec, const mx_obj_t *vals) {
    if (rec->want == MX_IR_WANT_MULTI) {
        return mx_jit_rt_multi(vm, rec, vals, vm->rets.len);
    }
    return mx_jit_rt_deliver_n(vm, base, rec, vals, vm->rets.len, rec->want != 1 || vm->rets.len == 0);
}

mx_jit_exit_t mx_jit_rt_ffi(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *rb) {
    mx_obj_t fn = mx_run_arg(ip + 1, base);
    if ((fn.kind != MX_KIND_FFI && fn.kind != vm->kinds.cfn) || vm->is_hooked) {
        return (mx_jit_exit_t) { .ip = NULL, .base = base };
    }
    uint32_t n = (uint32_t)ip[3];
    bool is_spread = ip[4] != 0;
    uint32_t total;
    if (is_spread) {
        total = n + vm->rets.len;
    } else {
        total = n;
    }
    uint32_t above = rb->frame + 6 + 2 * total + MX_RUN_SLACK;
    mx_thread_t *th = vm->thread;
    if ((uint32_t)(base - th->slots) + above > th->limit) {
        return (mx_jit_exit_t) { .ip = NULL, .base = base };
    }
    mx_val_t *at = mx_run_reserve(vm, base, above);
    uint32_t boff = (uint32_t)(at - th->slots);
    uint32_t cw = boff + rb->frame;
    mx_obj_t *args = (mx_obj_t *)(at + rb->frame + 4);
    const uintptr_t *argp = ip + 8;
    for (uint32_t i = 0; i < n; i++) {
        args[i] = mx_run_arg(argp + 2 * i, at);
    }
    if (is_spread) {
        memcpy(args + n, vm->rets.vals, sizeof(mx_obj_t) * vm->rets.len);
    }
    mx_ffi_t *call;
    if (fn.kind == MX_KIND_FFI) {
        call = mx_obj_get_ffi(fn);
        if (ip[6] != fn.val.bits) {
            mx_ffi_fast_t *fast = mx_run_fast_of(vm, call);
            ip[6] = fn.val.bits;
            ip[7] = (uintptr_t)fast;
            if (fast != NULL && n <= MX_RUN_SMALL - 2) {
                ip[0] = (uintptr_t)vm->hops[MX_HOP_CALL_FAST];
            } else {
                ip[0] = (uintptr_t)vm->hops[MX_HOP_CALL];
            }
        }
    } else {
        call = ((mx_obj_cfn_t *)mx_ref_obj(fn.val.ref))->fn;
    }
    th->slots[cw].bits = mx_link_make(boff - 2, MX_LINK_C);
    th->slots[cw + 1].bits = ip[5];
    *mx_thread_objs(th, cw + 2) = fn;
    th->frame = cw;
    th->top = cw + 4 + 2 * total;
    vm->rets.len = 1;
    vm->rets.vals[0] = mx_obj_of_nil();
    vm->pending.is_ok = true;
    vm->no_gc += 1;
    call(vm, total, mx_thread_objs(th, cw + 4));
    vm->no_gc -= 1;
    vm->pending.is_ok = false;
    return mx_jit_rt_ffi_after(vm, boff, ip, rb);
}

mx_jit_exit_t mx_jit_rt_ffi_after(mx_t *vm, uint32_t boff, uintptr_t *ip, mx_version_t *rb) {
    mx_thread_t *th = vm->thread;
    uint32_t cw = boff + rb->frame;
    mx_val_t *after = th->slots + boff;
    if (vm->pending.is_set) {
        vm->jit.handoff = MX_JIT_HANDOFF_PENDING;
        vm->jit.hand_cw = cw;
        return (mx_jit_exit_t) { .ip = NULL, .base = after };
    }
    mx_obj_t first = *mx_thread_objs(th, cw + 4);
    if (mx_obj_is_error(first)) {
        vm->jit.handoff = MX_JIT_HANDOFF_FAIL;
        vm->jit.hand_err = first;
        vm->jit.hand_rec = (const mx_resume_t *)ip[5];
        return (mx_jit_exit_t) { .ip = NULL, .base = after };
    }
    if (vm->is_hooked && !mx_run_site_is_prim((const mx_resume_t *)ip[5])) {
        if (th->top < cw + 4 + 2 * vm->rets.len) {
            th->top = cw + 4 + 2 * vm->rets.len;
        }
        mx_vm_rets_push(vm);
        mx_obj_t herr = vm->hooks.debug(vm, th, (const mx_resume_t *)ip[5], 2);
        mx_vm_rets_pop(vm);
        after = th->slots + boff;
        if (mx_obj_is_error(herr)) {
            vm->jit.handoff = MX_JIT_HANDOFF_FAIL;
            vm->jit.hand_err = herr;
            vm->jit.hand_rec = (const mx_resume_t *)ip[5];
            return (mx_jit_exit_t) { .ip = NULL, .base = after };
        }
    }
    mx_obj_t *res = mx_thread_objs(th, cw + 4);
    const mx_resume_t *rec = (const mx_resume_t *)th->slots[cw + 1].bits;
    if (rec->kind == MX_RESUME_VALUES && rec->want <= MX_JIT_WANT_MAX) {
        return (mx_jit_exit_t) { .ip = mx_jit_rt_deliver_n(vm, after, rec, res, vm->rets.len, true), .base = after };
    } else if (rec->kind == MX_RESUME_VALUES && rec->want == MX_IR_WANT_MULTI) {
        return (mx_jit_exit_t) { .ip = mx_jit_rt_multi(vm, rec, res, vm->rets.len), .base = after };
    } else if (rec->kind == MX_RESUME_RETURN && !vm->is_hooked) {
        uintptr_t link = after[-2].bits;
        const mx_resume_t *up = (const mx_resume_t *)after[-1].bits;
        if (mx_link_tag(link) == MX_LINK_LUA && up->kind == MX_RESUME_VALUES && (up->want <= MX_JIT_WANT_MAX || up->want == MX_IR_WANT_MULTI)) {
            mx_val_t *caller = th->slots + mx_link_caller(link) + 2;
            if (up->want == MX_IR_WANT_MULTI) {
                return (mx_jit_exit_t) { .ip = mx_jit_rt_multi(vm, up, res, vm->rets.len), .base = caller };
            }
            return (mx_jit_exit_t) { .ip = mx_jit_rt_deliver_n(vm, caller, up, res, vm->rets.len, true), .base = caller };
        }
    }
    vm->jit.handoff = MX_JIT_HANDOFF_DELIVER;
    vm->jit.hand_rec = rec;
    vm->jit.hand_vals = res;
    vm->jit.hand_n = vm->rets.len;
    return (mx_jit_exit_t) { .ip = NULL, .base = after };
}

mx_jit_exit_t mx_jit_rt_return(mx_t *vm, mx_val_t *base, const uintptr_t *ip, mx_hop_t hop) {
    uintptr_t link = base[-2].bits;
    const mx_resume_t *rec = (const mx_resume_t *)base[-1].bits;
    if (mx_link_tag(link) != MX_LINK_LUA || rec->kind != MX_RESUME_VALUES || (rec->want > MX_JIT_WANT_MAX && rec->want != MX_IR_WANT_MULTI)) {
        return (mx_jit_exit_t) { .ip = NULL, .base = base };
    }
    mx_obj_t small[MX_RUN_SMALL];
    const mx_obj_t *vals;
    uint32_t n;
    if (hop == MX_HOP_RETURN0) {
        vals = small;
        n = 0;
    } else if (hop == MX_HOP_RETURN1) {
        small[0] = mx_run_arg(ip + 1, base);
        vals = small;
        n = 1;
    } else {
        uint32_t count = (uint32_t)ip[1];
        if (ip[2] == 0 && count <= MX_RUN_SMALL) {
            for (uint32_t i = 0; i < count; i++) {
                small[i] = mx_run_arg(ip + 3 + 2 * i, base);
            }
            vals = small;
            n = count;
        } else {
            uint32_t extra;
            if (ip[2] != 0) {
                extra = vm->rets.len;
            } else {
                extra = 0;
            }
            mx_obj_t *out = mx_vm_rets_reserve(vm, count + extra);
            memmove(out + count, out, sizeof(mx_obj_t) * extra);
            for (uint32_t i = 0; i < count; i++) {
                out[i] = mx_run_arg(ip + 3 + 2 * i, base);
            }
            vm->rets.len = count + extra;
            vals = vm->rets.vals;
            n = count + extra;
        }
    }
    mx_val_t *after = vm->thread->slots + mx_link_caller(link) + 2;
    if (rec->want == MX_IR_WANT_MULTI) {
        return (mx_jit_exit_t) { .ip = mx_jit_rt_multi(vm, rec, vals, n), .base = after };
    }
    return (mx_jit_exit_t) { .ip = mx_jit_rt_deliver_n(vm, after, rec, vals, n, true), .base = after };
}

mx_jit_exit_t mx_jit_rt_call_lua(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *rb) {
    mx_obj_t fn = mx_obj_of_raw(MX_KIND_CLOSURE, base[ip[1]]);
    mx_ir_block_t *block = mx_obj_get_closure(fn)->block;
    uint32_t n = (uint32_t)ip[2];
    bool is_spread = ip[3] != 0;
    const mx_resume_t *rec = (const mx_resume_t *)ip[4];
    mx_resume_cache_t *cache = (mx_resume_cache_t *)(ip + 5);
    const uintptr_t *argp = ip + 5 + MX_RESUME_CACHE_WORDS;
    uint32_t total;
    if (is_spread) {
        total = n + vm->rets.len;
    } else {
        total = n;
    }
    uintptr_t key = (uintptr_t)block | (uintptr_t)vm->is_hooked;
    bool is_keyed = !is_spread;
    if (is_spread && total == n + 1) {
        key |= ((uintptr_t)vm->rets.vals[0].kind + 1) << 48;
        is_keyed = true;
    }
    uintptr_t *code;
    if (is_keyed) {
        code = mx_run_cache_find(cache, key);
    } else {
        code = NULL;
    }
    if (code == NULL) {
        mx_kind_t kinds[MX_IR_BRANCH_MAX_ARGS + 1];
        for (uint32_t i = 0; i < n; i++) {
            kinds[i] = (mx_kind_t)(argp[2 * i + 1] >> 1);
        }
        for (uint32_t i = n; i < total && i < block->func->nparams; i++) {
            kinds[i] = vm->rets.vals[i - n].kind;
        }
        code = mx_version_args(vm, block, kinds, total)->code;
        if (is_keyed) {
            mx_run_cache_put(vm, cache, key, (uintptr_t)code);
        }
    }
    const mx_version_t *crb = (const mx_version_t *)code[-1];
    uint32_t frame = crb->frame;
    uint32_t above = rb->frame + 2 + frame + 2 + 2 * total + MX_RUN_SLACK;
    if ((uint32_t)(base - vm->thread->slots) + above > vm->thread->limit) {
        return (mx_jit_exit_t) { .ip = NULL, .base = base };
    }
    mx_val_t *at = mx_run_reserve(vm, base, above);
    mx_val_t *callee = at + rb->frame + 2;
    callee[-2].bits = mx_link_make((uint32_t)(at - vm->thread->slots) - 2, MX_LINK_LUA);
    callee[-1].bits = (uintptr_t)rec;
    callee[0] = fn.val;
    uint32_t np = crb->nparams;
    for (uint32_t i = 0; i < np; i++) {
        if (i < n) {
            callee[i + 1] = mx_run_arg(argp + 2 * i, at).val;
        } else if (i < total) {
            callee[i + 1] = vm->rets.vals[i - n].val;
        } else {
            callee[i + 1].bits = 0;
        }
    }
    if (crb->is_vararg) {
        uint32_t nextra;
        if (total > np) {
            nextra = total - np;
        } else {
            nextra = 0;
        }
        callee[frame].bits = nextra;
        mx_obj_t *spill = (mx_obj_t *)(callee + frame + 2);
        for (uint32_t j = 0; j < nextra; j++) {
            uint32_t i = np + j;
            if (i < n) {
                spill[j] = mx_run_arg(argp + 2 * i, at);
            } else {
                spill[j] = vm->rets.vals[i - n];
            }
        }
    }
    return (mx_jit_exit_t) { .ip = code, .base = callee };
}

mx_jit_exit_t mx_jit_rt_tailcall_lua(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *rb) {
    (void)rb;
    mx_obj_t fn = mx_obj_of_raw(MX_KIND_CLOSURE, base[ip[1]]);
    mx_ir_block_t *block = mx_obj_get_closure(fn)->block;
    uint32_t n = (uint32_t)ip[2];
    bool is_spread = ip[3] != 0;
    mx_resume_cache_t *cache = (mx_resume_cache_t *)(ip + 4);
    const uintptr_t *argp = ip + 4 + MX_RESUME_CACHE_WORDS;
    uint32_t total;
    if (is_spread) {
        total = n + vm->rets.len;
    } else {
        total = n;
    }
    mx_obj_t small[MX_RUN_SMALL];
    mx_obj_t *tmp;
    if (total <= MX_RUN_SMALL) {
        tmp = small;
    } else {
        tmp = mx_vm_tmp_reserve(vm, total);
    }
    for (uint32_t i = 0; i < n; i++) {
        tmp[i] = mx_run_arg(argp + 2 * i, base);
    }
    if (is_spread) {
        memcpy(tmp + n, vm->rets.vals, sizeof(mx_obj_t) * vm->rets.len);
    }
    uintptr_t key = (uintptr_t)block | (uintptr_t)vm->is_hooked;
    uintptr_t *code;
    if (is_spread) {
        code = NULL;
    } else {
        code = mx_run_cache_find(cache, key);
    }
    if (code == NULL) {
        code = mx_version_entry(vm, block, tmp, total)->code;
        if (!is_spread) {
            mx_run_cache_put(vm, cache, key, (uintptr_t)code);
        }
    }
    const mx_version_t *crb = (const mx_version_t *)code[-1];
    uint32_t frame = crb->frame;
    uint32_t above = frame + 2 + 2 * total + MX_RUN_SLACK;
    if ((uint32_t)(base - vm->thread->slots) + above > vm->thread->limit) {
        return (mx_jit_exit_t) { .ip = NULL, .base = base };
    }
    mx_val_t *at = mx_run_reserve(vm, base, above);
    at[-2].bits = mx_link_with_tail(at[-2].bits);
    at[0] = fn.val;
    uint32_t np = crb->nparams;
    for (uint32_t i = 0; i < np; i++) {
        if (i < total) {
            at[i + 1] = tmp[i].val;
        } else {
            at[i + 1].bits = 0;
        }
    }
    if (crb->is_vararg) {
        if (total > np) {
            mx_run_spill(at, frame, tmp + np, total - np);
        } else {
            mx_run_spill(at, frame, NULL, 0);
        }
    }
    return (mx_jit_exit_t) { .ip = code, .base = at };
}

uintptr_t *mx_jit_rt_meta_cmp(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    if ((mx_op_t)ip[1] != MX_OP_EQ) {
        return NULL;
    }
    mx_run_eq_t eq = mx_run_eq_raw(vm, mx_run_arg(ip + 3, base), mx_run_arg(ip + 5, base));
    if (!eq.is_known) {
        return NULL;
    }
    const mx_resume_t *rec = (const mx_resume_t *)ip[2];
    uintptr_t key = ((uintptr_t)eq.is_eq + 1) | mx_run_key_flags(vm, false);
    uintptr_t *code = mx_run_cache_find(&rec->cache, key);
    if (code == NULL) {
        code = mx_version_branch(vm, rec, eq.is_eq, false)->code;
        mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key, (uintptr_t)code);
    }
    return code;
}

uintptr_t *mx_jit_rt_cell(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    const mx_resume_t *rec = (const mx_resume_t *)ip[3];
    mx_kind_t kind = (mx_kind_t)base[ip[2]].bits;
    uintptr_t key = (uintptr_t)kind | mx_run_key_flags(vm, false);
    uintptr_t *code = mx_run_cache_find(&rec->cache, key);
    if (code == NULL) {
        code = mx_version_at(vm, rec, &kind, 1, false)->code;
        mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key, (uintptr_t)code);
    }
    return code;
}

bool mx_jit_rt_unpack(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    return mx_run_op_unpack(vm, base, ip);
}

uintptr_t *mx_jit_rt_deliver_n(mx_t *vm, mx_val_t *base, const mx_resume_t *rec, const mx_obj_t *vals, uint32_t got, bool is_resumed) {
    uint32_t want = rec->want;
    mx_kind_t kinds[MX_JIT_WANT_MAX];
    for (uint32_t i = 0; i < want; i++) {
        mx_obj_t v;
        if (i < got) {
            v = vals[i];
        } else {
            v = mx_obj_of_nil();
        }
        base[rec->out + i] = v.val;
        kinds[i] = v.kind;
    }
    if (rec->has_cell_out) {
        mx_run_cells_write(rec, base, kinds, want);
    }
    uintptr_t shape = (uintptr_t)mx_version_kinds_key(kinds, want);
    uintptr_t key = shape | mx_run_key_flags(vm, is_resumed);
    uintptr_t *code;
    if (shape != 0) {
        code = mx_run_cache_find(&rec->cache, key);
    } else {
        code = NULL;
    }
    if (code == NULL) {
        code = mx_version_values(vm, rec, kinds, want, is_resumed)->code;
        if (shape != 0) {
            mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key, (uintptr_t)code);
        }
    }
    return code;
}

uintptr_t *mx_jit_rt_found(mx_t *vm, mx_val_t *base, const mx_resume_t *rec, mx_run_found_t got, mx_version_t *first) {
    if (!got.is_found) {
        return NULL;
    }
    if (rec->has_cell_out) {
        mx_run_cells_write(rec, base, &got.value.kind, 1);
    }
    uintptr_t key = mx_run_key1(vm, got.value.kind, false);
    uintptr_t *code = mx_run_cache_find(&rec->cache, key);
    if (code == NULL) {
        code = mx_version_values(vm, rec, &got.value.kind, 1, false)->code;
        mx_run_cache_put(vm, (mx_resume_cache_t *)&rec->cache, key, (uintptr_t)code);
    }
    base[rec->out] = got.value.val;
    if (first->is_loop && first->recompiles < MX_JIT_RECOMPILE_MAX) {
        first->misses += 1;
        if (first->misses >= MX_JIT_MISS_RECOMPILE) {
            first->misses = 0;
            mx_jit_recompile(vm, first);
        }
    }
    return code;
}

uintptr_t *mx_jit_rt_get_sk(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first) {
    mx_obj_table_t *tbl = mx_ref_obj(base[ip[2]].ref);
    mx_obj_t key = mx_obj_of_raw(MX_KIND_STRING, (mx_val_t) { .bits = ip[3] });
    if (tbl->is_plain) {
        mx_table_slot_t got = mx_table_get_string_slot(tbl, key);
        if (got.slot >= 0) {
            ip[5] = (uintptr_t)got.slot;
        }
        if (!mx_obj_is_nil(got.value) || tbl->meta == NULL) {
            return mx_jit_rt_found(vm, base, (const mx_resume_t *)ip[4], (mx_run_found_t) { .value = got.value, .is_found = true }, first);
        }
    }
    return mx_jit_rt_found(vm, base, (const mx_resume_t *)ip[4], mx_run_get(vm, mx_obj_of_table(tbl), key), first);
}

uintptr_t *mx_jit_rt_get_ir(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first) {
    mx_run_found_t got = mx_run_get(vm, mx_obj_of_table(mx_ref_obj(base[ip[2]].ref)), mx_obj_of_int(base[ip[3]].i64));
    return mx_jit_rt_found(vm, base, (const mx_resume_t *)ip[4], got, first);
}

uintptr_t *mx_jit_rt_get_ik(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first) {
    mx_run_found_t got = mx_run_get(vm, mx_obj_of_table(mx_ref_obj(base[ip[2]].ref)), mx_obj_of_int((int64_t)ip[3]));
    return mx_jit_rt_found(vm, base, (const mx_resume_t *)ip[4], got, first);
}

uintptr_t *mx_jit_rt_get_var(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first) {
    mx_run_found_t got = mx_run_get(vm, mx_run_arg(ip + 2, base), mx_run_arg(ip + 4, base));
    return mx_jit_rt_found(vm, base, (const mx_resume_t *)ip[6], got, first);
}

uintptr_t *mx_jit_rt_get_method(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first) {
    mx_run_found_t got = mx_run_get(vm, mx_obj_of_raw((mx_kind_t)ip[6], base[ip[2]]), mx_obj_of_raw(MX_KIND_STRING, (mx_val_t) { .bits = ip[3] }));
    return mx_jit_rt_found(vm, base, (const mx_resume_t *)ip[4], got, first);
}

uintptr_t *mx_jit_rt_get_upval(mx_t *vm, mx_val_t *base, uintptr_t *ip, mx_version_t *first) {
    mx_obj_closure_t *cl = mx_ref_obj(base[0].ref);
    mx_obj_t v = mx_upval_get(cl->upvals[ip[2]]);
    if (ip[4] != 0) {
        mx_ir_note_seen((mx_kind_t *)ip[4], v.kind);
    }
    return mx_jit_rt_found(vm, base, (const mx_resume_t *)ip[3], (mx_run_found_t) { .value = v, .is_found = true }, first);
}

void mx_jit_rt_table_new(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    mx_run_op_table_new(vm, base, ip);
}

void mx_jit_rt_concat(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    mx_run_op_concat(vm, base, ip);
}

void mx_jit_rt_closure(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    mx_run_op_closure(vm, base, ip);
}

void mx_jit_rt_close(mx_t *vm, mx_val_t *base, uintptr_t *ip) {
    mx_thread_close_upvals(vm, vm->thread, (uint32_t)(base - vm->thread->slots) + (uint32_t)ip[1]);
}

bool mx_jit_rt_eq_str(mx_t *vm, mx_val_t *base, const uintptr_t *ip) {
    return mx_jit_rt_cmp(vm, base, ip, MX_HOP_BR_EQ_STR);
}

uintptr_t *mx_jit_rt_link(mx_t *vm, uintptr_t *word, uint32_t site, uint32_t info, mx_version_t *rb, mx_version_t *first) {
    uintptr_t *code = (uintptr_t *)*word;
    if (code == NULL) {
        mx_ir_block_t *block = (mx_ir_block_t *)*(word - ((info & ~MX_JIT_EDGE_SIDE) >> 1));
        code = mx_version_edge(vm, rb->out, rb->nout, block)->code;
        *word = (uintptr_t)code;
    }
    mx_version_t *target = (mx_version_t *)code[-1];
    if (code[1] != vm->jit.back) {
        const uint8_t *to = (const uint8_t *)code[1];
        if ((info & 1) == 0 && target->block->id > rb->block->id) {
            mx_x64_patch(vm->jit.mem + site, to + vm->jit.poll_len);
        } else {
            mx_x64_patch(vm->jit.mem + site, to);
        }
        if ((info & MX_JIT_EDGE_SIDE) != 0 && first->is_loop && first->recompiles < MX_JIT_RECOMPILE_MAX) {
            mx_jit_recompile(vm, first);
        }
    }
    return code;
}
