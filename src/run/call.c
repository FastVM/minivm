
#include "run.h"
#include "../value/error.h"
#include "../value/obj.h"

mx_obj_t *mx_call_args(mx_t *vm) {
    mx_thread_t *th = vm->thread;
    return mx_thread_objs(th, th->frame + 4);
}

mx_obj_t *mx_call_results(mx_t *vm, uint32_t n) {
    mx_thread_t *th = vm->thread;
    uint32_t end = th->frame + 4 + 2 * n;
    if (end + MX_RUN_SLACK > MX_THREAD_MAX_WORDS) {
        return NULL;
    }
    mx_thread_reserve(vm, th, end + MX_RUN_SLACK);
    if (th->top < end) {
        th->top = end;
    }
    return mx_thread_objs(th, th->frame + 4);
}

void mx_call_pend(mx_t *vm, mx_run_pending_kind_t kind, mx_obj_t fn, mx_obj_t extra, mx_thread_t *thread, uint32_t nargs, const mx_obj_t *args) {
    mx_obj_t *out = mx_vm_pending_args(vm, nargs);
    if (nargs != 0) {
        memmove(out, args, sizeof(mx_obj_t) * nargs);
    }
    vm->pending.call = (mx_run_pending_t) {
        .fn = fn,
        .extra = extra,
        .thread = thread,
        .nargs = nargs,
        .kind = kind,
    };
    vm->pending.is_set = true;
}

mx_obj_t mx_call_meta(mx_t *vm, mx_obj_t fn, uint32_t nargs, const mx_obj_t *args) {
    if (vm->pending.is_ok && mx_obj_is_closure(fn)) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_CALL, fn, mx_obj_of_nil(), NULL, nargs, args);
        return mx_obj_of_nil();
    } else {
        return mx_call(vm, fn, nargs, args);
    }
}

mx_obj_t mx_call(mx_t *vm, mx_obj_t fn, uint32_t nargs, const mx_obj_t *args) {
    return mx_pcall(vm, fn, nargs, args, mx_obj_of_nil());
}

mx_obj_t mx_pcall(mx_t *vm, mx_obj_t fn, uint32_t nargs, const mx_obj_t *args, mx_obj_t handler) {
    if (vm->call_depth >= MX_RUN_C_DEPTH) {
        return mx_error_raw(vm, "C stack overflow");
    }
    mx_thread_t *th = vm->thread;
    uint32_t saved_top = th->top;
    uint32_t saved_frame = th->frame;
    uint64_t saved_token = vm->token_cur;
    bool saved_ok = vm->pending.is_ok;
    uint32_t saved_no_gc = vm->no_gc;
    vm->call_depth += 1;
    vm->pending.is_ok = false;
    uint32_t at = th->top;
    uintptr_t argp = (uintptr_t)args;
    uintptr_t lo = (uintptr_t)th->slots;
    bool is_inside = th->slots != NULL && argp >= lo && argp < lo + sizeof(mx_val_t) * th->cap;
    mx_thread_reserve(vm, th, at + 6 + 2 * nargs + MX_RUN_SLACK);
    if (is_inside) {
        args = (const mx_obj_t *)((uintptr_t)th->slots + (argp - lo));
    }
    mx_obj_t out;
    if (mx_obj_is_closure(fn)) {
        *mx_thread_objs(th, at) = handler;
        mx_version_t *rb = mx_run_push(vm, th, at + 2, mx_link_make(th->frame, MX_LINK_ENTRY), NULL, fn, args, nargs);
        if (rb == NULL) {
            out = mx_error_raw(vm, "stack overflow");
        } else {
            vm->no_gc = 0;
            mx_run_result_t res = mx_run_body(vm, &(mx_run_entry_t) { .start = MX_RUN_START_CALL, .rb = rb, .base = at + 4 });
            if (res.status == MX_RUN_ERROR) {
                out = res.error;
            } else if (vm->rets.len > 0) {
                out = vm->rets.vals[0];
            } else {
                out = mx_obj_of_nil();
            }
        }
    } else {
        mx_ffi_t *ffi;
        uint32_t n = nargs;
        mx_obj_t *dst = mx_thread_objs(th, at + 4);
        if (fn.kind == MX_KIND_FFI) {
            ffi = mx_obj_get_ffi(fn);
            memmove(dst, args, sizeof(mx_obj_t) * nargs);
        } else if (fn.kind == vm->kinds.cfn) {
            ffi = ((mx_obj_cfn_t *)mx_ref_obj(fn.val.ref))->fn;
            memmove(dst, args, sizeof(mx_obj_t) * nargs);
        } else {
            mx_obj_t op = vm->kinds.ptr[fn.kind].ops[MX_OP_CALL];
            ffi = mx_obj_get_ffi(op);
            n = nargs + 1;
            memmove(dst + 1, args, sizeof(mx_obj_t) * nargs);
            dst[0] = fn;
        }
        th->slots[at].bits = mx_link_make(th->frame, MX_LINK_C);
        th->slots[at + 1].bits = 0;
        *mx_thread_objs(th, at + 2) = fn;
        th->frame = at;
        th->top = at + 4 + 2 * n;
        vm->rets.len = 1;
        vm->rets.vals[0] = mx_obj_of_nil();
        vm->no_gc += 1;
        ffi(vm, n, mx_thread_objs(th, at + 4));
        vm->no_gc -= 1;
        mx_obj_t *res = mx_thread_objs(th, at + 4);
        if (mx_obj_is_error(res[0])) {
            out = res[0];
            mx_error_t *e = mx_obj_get_error(out);
            if (e->is_raw) {
                mx_error_locate(vm, e, NULL);
            }
            if (!mx_obj_is_nil(handler) && !e->is_handled) {
                out = mx_run_msgh(vm, handler, out);
            }
        } else {
            mx_vm_rets_set(vm, res, vm->rets.len);
            if (vm->rets.len > 0) {
                out = vm->rets.vals[0];
            } else {
                out = mx_obj_of_nil();
            }
        }
    }
    th = vm->thread;
    th->top = saved_top;
    th->frame = saved_frame;
    vm->token_cur = saved_token;
    vm->no_gc = saved_no_gc;
    vm->pending.is_ok = saved_ok;
    vm->call_depth -= 1;
    return out;
}

mx_run_result_t mx_resume(mx_t *vm, mx_thread_t *co, uint32_t nargs, const mx_obj_t *args) {
    if (vm->call_depth >= MX_RUN_C_DEPTH) {
        return (mx_run_result_t) { .error = mx_error_raw(vm, "C stack overflow"), .status = MX_RUN_ERROR };
    }
    mx_thread_t *th = vm->thread;
    uint64_t saved_token = vm->token_cur;
    bool saved_ok = vm->pending.is_ok;
    uint32_t saved_no_gc = vm->no_gc;
    vm->call_depth += 1;
    vm->pending.is_ok = false;
    vm->no_gc = 0;
    co->nest = th->nest + th->protects + 1;
    mx_obj_t *copy = mx_mem_alloc(sizeof(mx_obj_t) * (nargs + 1));
    memcpy(copy, args, sizeof(mx_obj_t) * nargs);
    mx_run_result_t res = mx_run_body(vm, &(mx_run_entry_t) { .start = MX_RUN_START_RESUME, .thread = co, .args = copy, .nargs = nargs });
    mx_mem_free(copy);
    co->resumer = NULL;
    vm->thread = th;
    th->status = MX_THREAD_RUNNING;
    vm->token_cur = saved_token;
    vm->no_gc = saved_no_gc;
    vm->pending.is_ok = saved_ok;
    vm->call_depth -= 1;
    return res;
}
