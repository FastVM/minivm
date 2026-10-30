
#include "std.h"
#include "../lua.h"
#include "../../../vm/gc.h"
#include "../../../value/kind.h"
#include "../../../value/obj.h"

mx_obj_t mx_std_cfn_alloc(mx_t *vm, mx_ffi_t *fn, mx_std_fn_t *std_fn, const char *std_name, uint32_t nup, const mx_obj_t *ups) {
    mx_obj_cfn_t *cfn = mx_gc_alloc(vm, sizeof(mx_obj_cfn_t) + sizeof(mx_obj_t) * nup);
    *cfn = (mx_obj_cfn_t) {
        .head = (mx_obj_head_t) { .kind = vm->kinds.cfn },
        .len = nup,
        .fn = fn,
        .std_fn = std_fn,
        .std_name = std_name,
    };
    for (uint32_t i = 0; i < nup; i++) {
        cfn->values[i] = ups[i];
    }
    return mx_obj_of_data(cfn);
}

mx_obj_t mx_std_cfn_new(mx_t *vm, mx_ffi_t *fn, uint32_t nup, const mx_obj_t *ups) {
    return mx_std_cfn_alloc(vm, fn, NULL, NULL, nup, ups);
}

void mx_ffi_std_foreign(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_thread_t *th = vm->thread;
    mx_obj_cfn_t *cfn = (mx_obj_cfn_t *)mx_ref_obj(mx_thread_objs(th, th->frame + 2)->val.ref);
    const char *name;
    if (cfn->std_name != NULL) {
        name = cfn->std_name;
    } else {
        name = "?";
    }
    mx_std_dispatch(vm, cfn->std_fn, name, args, (int32_t)nargs);
}

mx_obj_t mx_std_cfn_new_std(mx_t *vm, mx_std_fn_t *fn, const char *name, uint32_t nup, const mx_obj_t *ups) {
    return mx_std_cfn_alloc(vm, mx_ffi_std_foreign, fn, name, nup, ups);
}

mx_obj_t mx_std_upval(mx_std_state_t *ls, uint32_t i) {
    mx_obj_cfn_t *cfn = ls->upvals;
    if (cfn == NULL || i >= cfn->len) {
        return mx_obj_of_nil();
    } else {
        return cfn->values[i];
    }
}

void mx_std_setupval(mx_std_state_t *ls, uint32_t i, mx_obj_t v) {
    mx_obj_cfn_t *cfn = ls->upvals;
    if (cfn != NULL && i < cfn->len) {
        mx_gc_write_barrier(ls->vm, cfn, v);
        cfn->values[i] = v;
    }
}

void mx_std_pushcfn_up(mx_std_state_t *ls, mx_std_fn_t *fn, const char *name, uint32_t nup) {
    if (nup > MX_STD_CFN_MAX_UPVALS) {
        mx_std_error(ls, "too many upvalues (limit is %d)", MX_STD_CFN_MAX_UPVALS);
        return;
    }
    mx_obj_t cfn = mx_std_cfn_new_std(ls->vm, fn, name, nup, mx_std_slots(ls) + ls->top - (int32_t)nup);
    ls->top -= (int32_t)nup;
    mx_std_push(ls, cfn);
}
