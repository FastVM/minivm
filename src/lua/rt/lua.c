
#include "lua.h"
#include "cont.h"
#include "meta.h"
#include "prim.h"
#include "api/std.h"
#include "../front/parse.h"
#include "../../vm/gc.h"
#include "../../value/obj.h"
#include "../../value/table.h"

void mx_lua_close(mx_t *vm) {
    mx_gc_finalize_on_close(vm);
    mx_mem_free(vm->lua_state);
    vm->lua_state = NULL;
}

void mx_lua_set_arg(mx_t *vm, int32_t argc, char **argv, int32_t script) {
    mx_lua_t *lua = mx_lua_of(vm);
    mx_obj_table_t *arg = mx_table_new(vm);
    if (script >= argc) {
        script = 0;
    }
    for (int32_t i = 0; i < argc; i++) {
        mx_table_set(vm, arg, mx_obj_of_int(i - script), mx_obj_new_string(vm, argv[i]));
    }
    mx_table_set(vm, mx_obj_get_table(lua->std), mx_obj_intern(vm, "arg"), mx_obj_of_table(arg));
}

void mx_lua_warning(mx_t *vm, const char *msg, bool tocont) {
    mx_lua_warn_t *w = &mx_lua_of(vm)->warn;
    if (w->fn != NULL) {
        w->fn(w->ud, msg, tocont);
    } else if (w->is_cont) {
        fputs(msg, stderr);
        if (!tocont) {
            fputs("\n", stderr);
            w->is_cont = false;
        }
        fflush(stderr);
    } else if (!tocont && msg[0] == '@') {
        if (strcmp(msg, "@on") == 0) {
            w->is_on = true;
        } else if (strcmp(msg, "@off") == 0) {
            w->is_on = false;
        }
    } else if (w->is_on) {
        fprintf(stderr, "Lua warning: %s", msg);
        if (tocont) {
            w->is_cont = true;
        } else {
            fputs("\n", stderr);
        }
        fflush(stderr);
    }
}

mx_lua_t *mx_lua_open(mx_t *vm, bool is_noenv) {
    vm->no_gc += 1;
    mx_lua_t *lua = mx_mem_calloc(sizeof(mx_lua_t));
    lua->vm = vm;
    vm->lua_state = lua;
    mx_gc_add_root(vm, &lua->std);
    mx_gc_add_root(vm, &lua->registry);
    mx_gc_add_root(vm, &lua->stringlib);
    mx_gc_add_root(vm, &lua->string_meta);
    mx_gc_add_root(vm, &lua->hook.fn);
    mx_gc_add_root(vm, &lua->file_meta);
    mx_gc_add_root(vm, &lua->io_in);
    mx_gc_add_root(vm, &lua->io_out);
    for (uint32_t i = 0; i < MX_LUA_TYPE_COUNT; i++) {
        mx_gc_add_root(vm, &lua->type_metas[i]);
        mx_gc_add_root(vm, &lua->type_names[i]);
    }
    for (uint32_t i = 0; i < MX_LUA_META_KEY_COUNT; i++) {
        mx_gc_add_root(vm, &lua->meta_keys[i]);
    }
    lua->file_kind = mx_kind_register(vm, mx_kind_base("FILE*"));
    vm->kinds.udata = mx_kind_register(vm, mx_kind_base("userdata"));
    vm->hooks.tbc_unwind = mx_prim_tbc_unwind;
    vm->hooks.gc_finalizer = mx_meta_run_finalizers;
    vm->hooks.cont_done = mx_cont_done;
    mx_meta_open(vm);
    mx_obj_table_t *registry = mx_table_new(vm);
    lua->registry = mx_obj_of_table(registry);
    lua->std = mx_obj_of_table(mx_table_new(vm));
    mx_table_set(vm, registry, mx_obj_of_int(2), lua->std);
    mx_table_set(vm, registry, mx_obj_of_int(3), mx_obj_of_thread(vm->main));
    if (is_noenv) {
        mx_table_set(vm, registry, mx_obj_intern(vm, "LUA_NOENV"), mx_obj_of_boolean(true));
    }
    mx_std_setup(vm, lua->std);
    vm->no_gc -= 1;
    return lua;
}
