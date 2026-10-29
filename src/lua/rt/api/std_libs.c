
#include "std.h"
#include "../cont.h"
#include "../lua.h"
#include "../meta.h"
#include "../../../vm/gc.h"
#include "../../../value/kind.h"
#include "../../../value/obj.h"
#include "../../../value/table.h"

const mx_ffi_fast_reg_t mx_std_fasts[] = {
    {mx_ffi_base_type, mx_ffi_base_type_fast},
    {mx_ffi_base_getmetatable, mx_ffi_base_getmetatable_fast},
    {mx_ffi_base_next, mx_ffi_base_next_fast},
    {mx_ffi_base_inext, mx_ffi_base_inext_fast},
    {mx_ffi_string_byte, mx_ffi_string_byte_fast},
    {mx_ffi_string_char, mx_ffi_string_char_fast},
    {mx_ffi_string_sub, mx_ffi_string_sub_fast},
    {mx_ffi_string_rep, mx_ffi_string_rep_fast},
    {mx_ffi_table_insert, mx_ffi_table_insert_fast},
    {mx_ffi_table_concat, mx_ffi_table_concat_fast},
    {NULL, NULL},
};

void mx_std_lib_open_into(mx_std_state_t *ls, const char *name) {
    mx_std_getglobal(ls, name);
    if (mx_std_isnil(ls, -1)) {
        mx_std_pop(ls, 1);
        mx_std_newtable(ls);
        mx_std_pushvalue(ls, -1);
        mx_std_setglobal(ls, name);
    }
}

void mx_std_open_libs(mx_std_state_t *ls) {
    mx_std_open_base(ls);
    mx_std_open_string(ls);
    mx_std_open_utf8(ls);
    mx_std_open_table(ls);
    mx_std_open_math(ls);
    mx_std_open_os(ls);
    mx_std_open_coroutine(ls);
    mx_std_open_io(ls);
    mx_std_open_debug(ls);
    mx_std_open_vm(ls);
    mx_std_open_package(ls);
}

void mx_std_open_extras(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_obj_table_t *g = mx_obj_get_table(mx_lua_of(vm)->std);
    mx_obj_t tablib = mx_table_get(g, mx_obj_intern(vm, "table"));
    mx_obj_t unpack = mx_table_get(mx_obj_get_table(tablib), mx_obj_intern(vm, "unpack"));
    mx_table_set(vm, g, mx_obj_intern(vm, "unpack"), unpack);
    mx_table_set(vm, g, mx_obj_intern(vm, "callcc"), mx_obj_of_ffi(mx_cont_callcc));
    mx_table_set(vm, g, mx_obj_intern(vm, "callcci"), mx_obj_of_ffi(mx_cont_callcci));
}

void mx_std_setup(mx_t *vm, mx_obj_t globals) {
    mx_lua_t *lua = mx_lua_of(vm);
    vm->fasts = mx_std_fasts;
    mx_thread_t *th = vm->thread;
    uint32_t at = th->top;
    uint32_t saved_frame = th->frame;
    mx_thread_reserve(vm, th, at + 4 + MX_RUN_SLACK);
    th->slots[at].bits = mx_link_make(th->frame, MX_LINK_C);
    th->slots[at + 1].bits = 0;
    *mx_thread_objs(th, at + 2) = mx_obj_of_nil();
    th->frame = at;
    th->top = at + 4;
    mx_std_state_t ls;
    mx_std_state_enter(&ls, vm, mx_thread_objs(th, at + 4), 0);
    mx_std_open_libs(&ls);
    mx_std_open_extras(&ls);
    th->frame = saved_frame;
    th->top = at;
    mx_obj_t stringlib = mx_table_get(mx_obj_get_table(globals), mx_obj_intern(vm, "string"));
    lua->stringlib = stringlib;
    vm->kinds.ptr[MX_KIND_STRING].get_methods = stringlib;
    mx_obj_table_t *smeta = mx_table_new(vm);
    mx_table_set(vm, smeta, lua->meta_keys[MX_LUA_META_KEY_INDEX], stringlib);
    mx_ffi_t *const arith[] = {
        mx_meta_op_add,
        mx_meta_op_sub,
        mx_meta_op_mul,
        mx_meta_op_div,
        mx_meta_op_idiv,
        mx_meta_op_mod,
        mx_meta_op_pow,
    };
    for (uint32_t i = 0; i < sizeof(arith) / sizeof(arith[0]); i++) {
        mx_table_set(vm, smeta, lua->meta_keys[MX_LUA_META_KEY_ARITH_BASE + i], mx_obj_of_ffi(arith[i]));
    }
    mx_table_set(vm, smeta, lua->meta_keys[MX_LUA_META_KEY_UNM], mx_obj_of_ffi(mx_meta_op_unm));
    lua->string_meta = mx_obj_of_table(smeta);
    lua->type_metas[MX_LUA_TYPE_STRING] = lua->string_meta;
}
