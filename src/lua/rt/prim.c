
#include "prim.h"
#include "lua.h"
#include "meta.h"
#include "../../run/run.h"
#include "../../value/error.h"
#include "../../value/obj.h"
#include "../../value/table.h"

void mx_prim_table_append(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_table_t *t = mx_obj_get_table(args[0]);
    int64_t start = mx_obj_get_int(args[1]);
    for (size_t i = 2; i < nargs; i++) {
        mx_table_set(vm, t, mx_obj_of_int(start + (int64_t)i - 2), args[i]);
    }
    args[0] = mx_obj_of_nil();
    vm->rets.len = 0;
}

void mx_prim_pack(mx_t *vm, size_t nargs, mx_obj_t *args) {
    args[0] = mx_obj_of_table(mx_table_new_pack(vm, args, (uint32_t)nargs, true));
    vm->rets.len = 1;
}

mx_obj_t mx_prim_for_fail(mx_t *vm, const char *what, mx_obj_t bad) {
    return mx_error_raw(vm, "bad 'for' %s (number expected, got %s)", what, mx_meta_typename(vm, bad));
}

int64_t mx_prim_for_stop_float(int64_t step, double limit) {
    double f;
    if (step > 0) {
        f = floor(limit);
    } else {
        f = ceil(limit);
    }
    if (f != f && step > 0) {
        return INT64_MIN;
    } else if (f != f) {
        return INT64_MAX;
    } else if (mx_obj_float_fits_int(f)) {
        return (int64_t)f;
    } else if (f > 0) {
        return INT64_MAX;
    } else {
        return INT64_MIN;
    }
}

int64_t mx_prim_for_stop(int64_t step, mx_obj_t limit) {
    if (mx_obj_is_int(limit)) {
        return mx_obj_get_int(limit);
    } else {
        return mx_prim_for_stop_float(step, mx_obj_get_float(limit));
    }
}

int64_t mx_prim_for_last(int64_t init, int64_t step, mx_obj_t limit) {
    int64_t stop = mx_prim_for_stop(step, limit);
    if (step == 1) {
        return stop;
    } else if (step > 0) {
        if (init > stop) {
            return stop;
        } else {
            uint64_t span = (uint64_t)stop - (uint64_t)init;
            return (int64_t)((uint64_t)init + span / (uint64_t)step * (uint64_t)step);
        }
    } else if (init < stop) {
        return stop;
    } else {
        uint64_t span = (uint64_t)init - (uint64_t)stop;
        uint64_t ustep = (uint64_t)0 - (uint64_t)step;
        return (int64_t)((uint64_t)init - span / ustep * ustep);
    }
}

void mx_prim_forprep_initial(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_num_t init = mx_meta_coerce_num(vm, args[0]);
    if (!init.is_num) {
        args[0] = mx_prim_for_fail(vm, "initial value", args[0]);
    } else if (!(mx_obj_is_int(args[0]) && mx_obj_is_int(args[1]))) {
        args[0] = mx_obj_of_number(mx_obj_get_number(init.value));
    }
}

void mx_prim_forprep_limit(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_num_t limit = mx_meta_coerce_num(vm, args[0]);
    if (!limit.is_num) {
        args[0] = mx_prim_for_fail(vm, "limit", args[0]);
    } else if (mx_obj_is_int(args[1]) && mx_obj_is_int(args[2]) && mx_obj_is_float(limit.value) && !mx_obj_float_fits_int(mx_obj_get_float(limit.value)) && (mx_obj_get_float(limit.value) > 0) != (mx_obj_get_int(args[2]) > 0)) {
        args[0] = limit.value;
    } else if (mx_obj_is_int(args[1]) && mx_obj_is_int(args[2])) {
        args[0] = mx_obj_of_int(mx_prim_for_last(mx_obj_get_int(args[1]), mx_obj_get_int(args[2]), limit.value));
    } else {
        args[0] = mx_obj_of_number(mx_obj_get_number(limit.value));
    }
}

void mx_prim_forprep_step(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_num_t step = mx_meta_coerce_num(vm, args[0]);
    if (!step.is_num) {
        args[0] = mx_prim_for_fail(vm, "step", args[0]);
    } else if (mx_obj_get_number(step.value) == 0) {
        args[0] = mx_error_raw(vm, "'for' step is zero");
    } else {
        args[0] = step.value;
    }
}

bool mx_prim_forprep_initial_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_obj_is_number(args[0])) {
        return false;
    } else if (!(mx_obj_is_int(args[0]) && mx_obj_is_int(args[1]))) {
        args[0] = mx_obj_of_number(mx_obj_get_number(args[0]));
    }
    return true;
}

bool mx_prim_forprep_limit_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (mx_obj_is_int(args[0]) && mx_obj_is_int(args[1]) && mx_obj_is_int(args[2])) {
        args[0] = mx_obj_of_int(mx_prim_for_last(mx_obj_get_int(args[1]), mx_obj_get_int(args[2]), args[0]));
        return true;
    } else if (!mx_obj_is_number(args[0])) {
        return false;
    }
    mx_prim_forprep_limit(vm, nargs, args);
    return true;
}

bool mx_prim_forprep_step_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_obj_is_number(args[0]) || mx_obj_get_number(args[0]) == 0) {
        return false;
    }
    mx_prim_forprep_step(vm, nargs, args);
    return true;
}

void mx_prim_global_fresh(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_obj_is_nil(args[0])) {
        args[0] = mx_error_raw(vm, "global '%s' already defined", mx_obj_get_string(args[1])->buf);
    }
}

mx_obj_t mx_prim_close_handler(mx_t *vm, mx_obj_t v) {
    return mx_meta_handler_of(vm, v, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_CLOSE]);
}

void mx_prim_close_check(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_t v = args[0];
    if (!mx_obj_is_truthy(v)) {
        return;
    } else if (mx_obj_is_nil(mx_prim_close_handler(vm, v))) {
        args[0] = mx_error_raw(vm, "variable '%s' got a non-closable value", mx_obj_get_string(args[1])->buf);
        return;
    }
    if (vm->tbcs.len == vm->tbcs.cap) {
        vm->tbcs.cap = vm->tbcs.cap * 2 + 8;
        vm->tbcs.ptr = mx_mem_realloc(vm->tbcs.ptr, sizeof(mx_tbc_t) * vm->tbcs.cap);
    }
    vm->tbcs.ptr[vm->tbcs.len] = (mx_tbc_t) {
        .value = v,
        .thread = vm->thread,
        .at = vm->thread->frame,
    };
    vm->tbcs.len += 1;
}

void mx_prim_close_call(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_t v = args[0];
    uint32_t i = vm->tbcs.len;
    while (i > 0) {
        mx_tbc_t *tbc = &vm->tbcs.ptr[i - 1];
        if (tbc->thread == vm->thread && tbc->at == vm->thread->frame && tbc->value.kind == v.kind && tbc->value.val.bits == v.val.bits) {
            break;
        }
        i -= 1;
    }
    if (!mx_obj_is_truthy(v) || i == 0) {
        return;
    }
    memmove(&vm->tbcs.ptr[i - 1], &vm->tbcs.ptr[i], sizeof(mx_tbc_t) * (vm->tbcs.len - i));
    vm->tbcs.len -= 1;
    mx_obj_t handler = mx_prim_close_handler(vm, v);
    if (!mx_prim_is_direct(vm, handler) && mx_obj_is_nil(mx_meta_handler_of(vm, handler, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_CALL]))) {
        args[0] = mx_error_raw(vm, "attempt to call a %s value (metamethod 'close')", mx_meta_typename(vm, handler));
    } else if (vm->pending.is_ok && mx_prim_is_direct(vm, handler)) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_CALL, handler, mx_obj_of_nil(), NULL, 1, &v);
    } else {
        mx_meta_result(vm, mx_call_meta(vm, handler, 1, &v));
    }
}

mx_obj_t mx_prim_tbc_unwind(mx_t *vm, mx_thread_t *th, uint32_t from, mx_obj_t err) {
    uint32_t i = vm->tbcs.len;
    while (i > 0) {
        i -= 1;
        if (i >= vm->tbcs.len) {
            continue;
        }
        mx_tbc_t tbc = vm->tbcs.ptr[i];
        if (tbc.thread != th || tbc.at < from) {
            continue;
        }
        memmove(&vm->tbcs.ptr[i], &vm->tbcs.ptr[i + 1], sizeof(mx_tbc_t) * (vm->tbcs.len - i - 1));
        vm->tbcs.len -= 1;
        mx_obj_t handler = mx_prim_close_handler(vm, tbc.value);
        if (!mx_obj_is_nil(handler)) {
            mx_obj_t second;
            uint32_t n;
            if (mx_obj_is_error(err)) {
                second = mx_error_payload(err);
                n = 2;
            } else {
                second = mx_obj_of_nil();
                n = 1;
            }
            mx_obj_t pair[2] = {tbc.value, second};
            mx_vm_hold(vm, err);
            mx_obj_t got = mx_call(vm, handler, n, pair);
            mx_vm_release(vm);
            if (mx_obj_is_error(got)) {
                err = got;
            }
        }
        i = vm->tbcs.len;
    }
    return err;
}

void mx_prim_protect_result(mx_t *vm, mx_obj_t got) {
    if (mx_obj_is_error(got)) {
        mx_obj_t *out = mx_call_results(vm, 2);
        out[0] = mx_obj_of_boolean(false);
        out[1] = mx_error_payload(got);
        vm->rets.len = 2;
    } else {
        uint32_t n = vm->rets.len;
        mx_obj_t *out = mx_call_results(vm, n + 1);
        if (out == NULL) {
            mx_call_args(vm)[0] = mx_error_raw(vm, "too many results");
            vm->rets.len = 1;
            return;
        }
        memcpy(out + 1, vm->rets.vals, sizeof(mx_obj_t) * n);
        out[0] = mx_obj_of_boolean(true);
        vm->rets.len = n + 1;
    }
}

bool mx_prim_is_direct(mx_t *vm, mx_obj_t fn) {
    return mx_obj_is_closure(fn) || fn.kind == MX_KIND_FFI || fn.kind == vm->kinds.cfn;
}

void mx_prim_pcall(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs < 1) {
        args[0] = mx_error_raw(vm, "bad argument #1 to 'pcall' (value expected)");
    } else if (vm->pending.is_ok && mx_prim_is_direct(vm, args[0])) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_PROTECT, args[0], mx_obj_of_nil(), NULL, (uint32_t)nargs - 1, args + 1);
    } else {
        mx_prim_protect_result(vm, mx_pcall(vm, args[0], (uint32_t)nargs - 1, args + 1, mx_obj_of_nil()));
    }
}

void mx_prim_xpcall(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs < 2) {
        args[0] = mx_error_raw(vm, "bad argument #2 to 'xpcall' (function expected, got no value)");
    } else if (mx_meta_lua_type(vm, args[1]) != MX_LUA_TYPE_FUNCTION) {
        args[0] = mx_error_raw(vm, "bad argument #2 to 'xpcall' (function expected, got %s)", mx_meta_typename(vm, args[1]));
    } else if (vm->pending.is_ok && mx_prim_is_direct(vm, args[0])) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_PROTECT_MSGH, args[0], args[1], NULL, (uint32_t)nargs - 2, args + 2);
    } else {
        mx_prim_protect_result(vm, mx_pcall(vm, args[0], (uint32_t)nargs - 2, args + 2, args[1]));
    }
}
