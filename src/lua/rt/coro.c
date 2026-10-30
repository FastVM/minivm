
#include "coro.h"
#include "prim.h"
#include "../../value/error.h"
#include "../../value/obj.h"

const char *mx_coro_status_name(mx_t *vm, mx_thread_t *co) {
    if (co == vm->thread) {
        return "running";
    }
    switch (co->status) {
        case MX_THREAD_FRESH:
        case MX_THREAD_SUSPENDED: {
            return "suspended";
        }
        case MX_THREAD_NORMAL: {
            return "normal";
        }
        case MX_THREAD_RUNNING: {
            return "running";
        }
        default: {
            return "dead";
        }
    }
}

const char *mx_coro_resume_check(mx_t *vm, mx_thread_t *co) {
    if (co->status == MX_THREAD_DEAD) {
        return "cannot resume dead coroutine";
    } else if (co == vm->thread || co->status == MX_THREAD_RUNNING || co->status == MX_THREAD_NORMAL) {
        return "cannot resume non-suspended coroutine";
    } else {
        return NULL;
    }
}

mx_thread_t *mx_coro_check(mx_std_state_t *ls, mx_std_slot_t arg) {
    mx_obj_t *at = mx_std_at(ls, arg);
    if (at == NULL || at->kind != ls->vm->kinds.thread) {
        mx_std_typeerror(ls, arg, "thread");
        return NULL;
    } else {
        return mx_obj_get_thread(*at);
    }
}

void mx_coro_push_resumed(mx_std_state_t *ls, mx_run_result_t res, bool is_wrap, mx_thread_t *co) {
    mx_t *vm = ls->vm;
    ls->top = 0;
    if (res.status != MX_RUN_ERROR && !is_wrap) {
        mx_std_pushboolean(ls, true);
        mx_std_take_rets(ls, 1, MX_STD_MULTRET);
    } else if (res.status != MX_RUN_ERROR) {
        mx_std_take_rets(ls, 0, MX_STD_MULTRET);
    } else if (!is_wrap) {
        mx_std_pushboolean(ls, false);
        mx_std_push(ls, mx_error_payload(res.error));
    } else {
        mx_vm_hold(vm, res.error);
        mx_obj_t closed = mx_prim_tbc_unwind(vm, co, 0, res.error);
        mx_vm_release(vm);
        ls->error = mx_run_wrap_error(vm, mx_std_site(ls), closed);
        mx_std_raise(ls);
    }
}

int32_t mx_coro_create(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_FUNCTION);
    mx_obj_t *fn = mx_std_at(ls, 1);
    if (fn == NULL) {
        return 0;
    }
    mx_std_push(ls, mx_obj_of_thread(mx_thread_new(ls->vm, *fn)));
    return 1;
}

int32_t mx_coro_resume(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_thread_t *co = mx_coro_check(ls, 1);
    if (co == NULL) {
        return 0;
    }
    const char *why = mx_coro_resume_check(vm, co);
    if (why != NULL) {
        mx_std_pushboolean(ls, false);
        mx_std_pushstring(ls, why);
        return 2;
    } else if (ls->can_pend) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_RESUME, mx_obj_of_nil(), mx_obj_of_nil(), co, (uint32_t)ls->top - 1, mx_std_slots(ls) + 1);
        return 0;
    }
    mx_std_sync(ls);
    mx_run_result_t res = mx_resume(vm, co, (uint32_t)ls->top - 1, mx_std_slots(ls) + 1);
    mx_coro_push_resumed(ls, res, false, co);
    return ls->top;
}

int32_t mx_coro_yield(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_thread_t *th = vm->thread;
    if (th == vm->main) {
        return mx_std_error_noloc(ls, "attempt to yield from outside a coroutine");
    } else if (!ls->can_pend || th->token != vm->token_cur) {
        return mx_std_error_noloc(ls, "attempt to yield across a C-call boundary");
    }
    mx_call_pend(vm, MX_RUN_PENDING_KIND_YIELD, mx_obj_of_nil(), mx_obj_of_nil(), NULL, (uint32_t)ls->top, mx_std_slots(ls));
    return 0;
}

void mx_ffi_coro_resume(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs >= 1 && args[0].kind == vm->kinds.thread && vm->pending.is_ok && mx_coro_resume_check(vm, mx_obj_get_thread(args[0])) == NULL) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_RESUME, mx_obj_of_nil(), mx_obj_of_nil(), mx_obj_get_thread(args[0]), (uint32_t)nargs - 1, args + 1);
    } else {
        mx_std_dispatch(vm, mx_coro_resume, "resume", args, (int32_t)nargs);
    }
}

void mx_ffi_coro_yield(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_thread_t *th = vm->thread;
    if (th != vm->main && vm->pending.is_ok && th->token == vm->token_cur) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_YIELD, mx_obj_of_nil(), mx_obj_of_nil(), NULL, (uint32_t)nargs, args);
    } else {
        mx_std_dispatch(vm, mx_coro_yield, "yield", args, (int32_t)nargs);
    }
}

int32_t mx_coro_status(mx_std_state_t *ls) {
    mx_thread_t *co = mx_coro_check(ls, 1);
    if (co == NULL) {
        return 0;
    }
    mx_std_pushstring(ls, mx_coro_status_name(ls->vm, co));
    return 1;
}

int32_t mx_coro_running(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_std_push(ls, mx_obj_of_thread(vm->thread));
    mx_std_pushboolean(ls, vm->thread == vm->main);
    return 2;
}

int32_t mx_coro_isyieldable(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_thread_t *co;
    if (mx_std_isnone(ls, 1)) {
        co = vm->thread;
    } else {
        co = mx_coro_check(ls, 1);
        if (co == NULL) {
            return 0;
        }
    }
    bool is_yieldable;
    if (co == vm->main) {
        is_yieldable = false;
    } else if (co == vm->thread) {
        is_yieldable = ls->can_pend && co->token == vm->token_cur;
    } else {
        is_yieldable = co->status == MX_THREAD_SUSPENDED || co->status == MX_THREAD_FRESH;
    }
    mx_std_pushboolean(ls, is_yieldable);
    return 1;
}

int32_t mx_coro_close(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_thread_t *co;
    if (mx_std_isnone(ls, 1)) {
        co = vm->thread;
    } else {
        co = mx_coro_check(ls, 1);
    }
    if (co == NULL) {
        return 0;
    } else if (co == vm->thread && co == vm->main) {
        return mx_std_error(ls, "cannot close main thread");
    } else if (co == vm->thread) {
        co->is_closing = true;
        ls->error = mx_error_value(vm, mx_obj_of_nil());
        mx_std_raise(ls);
        return 0;
    } else if (co->status == MX_THREAD_NORMAL || co->status == MX_THREAD_RUNNING) {
        return mx_std_error(ls, "cannot close a %s coroutine", mx_coro_status_name(vm, co));
    }
    mx_obj_t err = mx_coro_kill(vm, co);
    if (mx_obj_is_error(err)) {
        mx_std_pushboolean(ls, false);
        mx_std_push(ls, mx_error_payload(err));
        return 2;
    } else {
        mx_std_pushboolean(ls, true);
        return 1;
    }
}

mx_obj_t mx_coro_kill(mx_t *vm, mx_thread_t *co) {
    mx_obj_t failure;
    if (co->is_failed) {
        failure = co->failure;
        co->is_failed = false;
        co->failure = mx_obj_of_nil();
    } else {
        failure = mx_obj_of_nil();
    }
    mx_vm_hold(vm, failure);
    mx_obj_t err = mx_prim_tbc_unwind(vm, co, 0, failure);
    mx_vm_release(vm);
    mx_thread_close_upvals(vm, co, 0);
    co->status = MX_THREAD_DEAD;
    co->frame = MX_THREAD_NONE;
    co->top = 0;
    return err;
}

int32_t mx_coro_wrap(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_FUNCTION);
    mx_obj_t *fn = mx_std_at(ls, 1);
    if (fn == NULL) {
        return 0;
    }
    mx_std_push(ls, mx_obj_of_thread(mx_thread_new(ls->vm, *fn)));
    mx_std_pushcfn_up(ls, mx_coro_wrap_call, "wrap", 1);
    mx_obj_t *wrapped = mx_std_at(ls, -1);
    if (wrapped != NULL) {
        ((mx_obj_cfn_t *)mx_obj_data_head(*wrapped))->fn = mx_ffi_coro_wrap_call;
    }
    return 1;
}

void mx_ffi_coro_wrap_call(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_thread_t *th = vm->thread;
    mx_obj_cfn_t *cfn = (mx_obj_cfn_t *)mx_ref_obj(mx_thread_objs(th, th->frame + 2)->val.ref);
    mx_thread_t *co = mx_obj_get_thread(cfn->values[0]);
    if (vm->pending.is_ok && mx_coro_resume_check(vm, co) == NULL) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_RESUME_WRAP, mx_obj_of_nil(), mx_obj_of_nil(), co, (uint32_t)nargs, args);
    } else {
        mx_ffi_std_foreign(vm, nargs, args);
    }
}

int32_t mx_coro_wrap_call(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_thread_t *co = mx_obj_get_thread(mx_std_upval(ls, 0));
    const char *why = mx_coro_resume_check(vm, co);
    if (why != NULL) {
        return mx_std_error(ls, "%s", why);
    } else if (ls->can_pend) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_RESUME_WRAP, mx_obj_of_nil(), mx_obj_of_nil(), co, (uint32_t)ls->top, mx_std_slots(ls));
        return 0;
    }
    mx_std_sync(ls);
    mx_run_result_t res = mx_resume(vm, co, (uint32_t)ls->top, mx_std_slots(ls));
    mx_coro_push_resumed(ls, res, true, co);
    return ls->top;
}
