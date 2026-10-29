
#include "../api/std.h"
#include "../lua.h"
#include "../../../vm/gc.h"
#include "../../../value/kind.h"
#include "../../../value/obj.h"

int32_t mx_std_vm_setop(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_TABLE);
    const char *opname = mx_std_checkstring(ls, 2).ptr;
    mx_std_checktype(ls, 3, MX_LUA_TYPE_FUNCTION);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    for (mx_op_t op = 0; op < MX_OP_COUNT; op++) {
        if (strcmp(mx_op_names[op], opname) == 0) {
            mx_kind_set_op(ls->vm, mx_obj_kind(*mx_std_at(ls, 1)), op, *mx_std_at(ls, 3));
            return 0;
        }
    }
    return mx_std_argerror(ls, 2, "unknown op");
}

int32_t mx_std_vm_typename(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_std_pushstring(ls, mx_kind_name(ls->vm, mx_obj_kind(*mx_std_at(ls, 1))));
    return 1;
}

int32_t mx_std_vm_stats(mx_std_state_t *ls) {
    mx_std_newtable(ls);
    mx_std_pushinteger(ls, (int64_t)ls->vm->stats.versions);
    mx_std_setfield(ls, -2, "versions");
    mx_std_pushinteger(ls, (int64_t)ls->vm->stats.version_bytes);
    mx_std_setfield(ls, -2, "version_bytes");
    mx_std_pushinteger(ls, (int64_t)mx_gc_bytes(ls->vm));
    mx_std_setfield(ls, -2, "heap_bytes");
    return 1;
}

int32_t mx_std_vm_gc(mx_std_state_t *ls) {
    ls->vm->gc->is_major_forced = true;
    mx_std_sync(ls);
    mx_gc_collect(ls->vm);
    return 0;
}

int32_t mx_std_vm_version(mx_std_state_t *ls) {
    mx_std_pushstring(ls, MX_VERSION);
    return 1;
}

const mx_std_reg_t mx_std_vm_regs[] = {
    {"setop", mx_std_vm_setop, NULL},
    {"typename", mx_std_vm_typename, NULL},
    {"stats", mx_std_vm_stats, NULL},
    {"gc", mx_std_vm_gc, NULL},
    {"version", mx_std_vm_version, NULL},
    {NULL, NULL, NULL},
};

void mx_std_open_vm(mx_std_state_t *ls) {
    mx_std_newlib(ls, "vm", mx_std_vm_regs);
    mx_std_pop(ls, 1);
}
