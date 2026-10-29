
#include "cont.h"
#include "lua.h"
#include "meta.h"
#include "api/std.h"
#include "../../run/run.h"
#include "../../vm/gc.h"
#include "../../value/error.h"
#include "../../value/obj.h"

void mx_cont_start(mx_t *vm, size_t nargs, mx_obj_t *args, bool allow_table) {
    const char *name = "callcci";
    if (allow_table) {
        name = "callcc";
    }
    mx_obj_t fn = mx_obj_of_nil();
    if (nargs >= 1) {
        fn = args[0];
    }
    mx_obj_t target = fn;
    if (allow_table && mx_obj_is_table(fn)) {
        target = mx_meta_handler_of(vm, fn, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_CALL]);
    }
    if (!mx_obj_is_closure(target)) {
        args[0] = mx_error_raw(vm, "bad argument #1 to '%s' (function expected)", name);
        return;
    } else if (!vm->pending.is_ok) {
        args[0] = mx_error_raw(vm, "attempt to use '%s' across a C-call boundary", name);
        return;
    }
    mx_thread_t *th = vm->thread;
    mx_obj_t ups[3] = {
        mx_obj_of_thread(th),
        mx_obj_of_int((int64_t)th->frame + 6),
        mx_obj_of_int(0),
    };
    mx_obj_t k = mx_std_cfn_new(vm, mx_cont_invoke, 3, ups);
    uint32_t n = (uint32_t)nargs;
    mx_obj_t *call = mx_vm_tmp_reserve(vm, n + 1);
    if (target.val.bits == fn.val.bits) {
        call[0] = k;
        memcpy(call + 1, args + 1, sizeof(mx_obj_t) * (n - 1));
    } else {
        call[0] = fn;
        call[1] = k;
        memcpy(call + 2, args + 1, sizeof(mx_obj_t) * (n - 1));
        n += 1;
    }
    mx_call_pend(vm, MX_RUN_PENDING_KIND_CALLCC, target, k, NULL, n, call);
}

void mx_cont_callcc(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_cont_start(vm, nargs, args, true);
}

void mx_cont_callcci(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_cont_start(vm, nargs, args, false);
}

void mx_cont_invoke(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_thread_t *th = vm->thread;
    mx_obj_cfn_t *k = (mx_obj_cfn_t *)mx_ref_obj(mx_thread_objs(th, th->frame + 2)->val.ref);
    mx_thread_t *target = mx_obj_get_thread(k->values[0]);
    uint32_t p = (uint32_t)mx_obj_get_int(k->values[1]);
    int64_t state = mx_obj_get_int(k->values[2]);
    const char *fail = NULL;
    if (state == 1) {
        fail = "attempt to invoke a continuation twice";
    } else if (state == 2) {
        fail = "attempt to invoke a continuation after its callcc returned";
    } else if (target != th || !vm->pending.is_ok) {
        fail = "attempt to invoke a continuation across a C-call boundary";
    } else {
        uint32_t at = th->frame;
        while (at != p && at != MX_THREAD_NONE && fail == NULL) {
            if (mx_link_tag(th->slots[at].bits) == MX_LINK_ENTRY) {
                fail = "attempt to invoke a continuation across a C-call boundary";
            }
            at = mx_link_caller(th->slots[at].bits);
        }
        if (fail == NULL && at != p) {
            fail = "attempt to invoke a continuation after its callcc returned";
        }
    }
    if (fail != NULL) {
        args[0] = mx_error_raw(vm, "%s", fail);
        return;
    }
    mx_gc_write_barrier(vm, k, mx_obj_of_int(1));
    k->values[2] = mx_obj_of_int(1);
    mx_call_pend(vm, MX_RUN_PENDING_KIND_KJUMP, mx_obj_of_nil(), mx_obj_of_int(p), NULL, (uint32_t)nargs, args);
}

void mx_cont_done(mx_t *vm, mx_obj_t kobj) {
    mx_obj_cfn_t *k = (mx_obj_cfn_t *)mx_ref_obj(kobj.val.ref);
    if (mx_obj_get_int(k->values[2]) == 0) {
        k->values[2] = mx_obj_of_int(2);
    }
}
