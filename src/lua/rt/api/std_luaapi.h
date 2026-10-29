
#pragma once

#if !defined(MX_HEADER_LUA_RT_API_STD_LUAAPI)
#define MX_HEADER_LUA_RT_API_STD_LUAAPI

#include "std.h"
#include "../../../../include/lauxlib.h"
#include "../../../../include/lua.h"
#include "../../../../include/lualib.h"

typedef struct mx_luaapi_frame_t mx_luaapi_frame_t;
typedef struct mx_luaapi_box_t mx_luaapi_box_t;

struct mx_luaapi_frame_t {
    void *extra;
    mx_std_state_t ls;
};

struct mx_luaapi_box_t {
    void *data;
    size_t size;
};

int32_t mx_luaapi_type_of(mx_lua_type_t t);
mx_lua_type_t mx_luaapi_type_to(int32_t t);
int32_t mx_luaapi_bool(bool b);
void mx_luaapi_check(mx_std_state_t *ls);
int32_t mx_luaapi_protect(mx_lua_t *lua, mx_std_fn_t *fn, mx_std_state_t *ls);
void mx_luaapi_call(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_luaapi_push_cfn(mx_std_state_t *ls, mx_std_fn_t *fn, uint32_t nup);
int32_t mx_luaapi_open_loaded(mx_std_state_t *ls, const char *name);
int32_t mx_luaapi_load_status(mx_std_state_t *ls, int32_t n, int32_t failed);
void mx_luaapi_add_utf8(mx_str_t *out, unsigned long x);
mx_obj_t *mx_luaapi_cfn_upval(mx_std_state_t *ls, int32_t funcindex, int32_t n);
mx_obj_upval_t **mx_luaapi_lua_upval(mx_std_state_t *ls, int32_t funcindex, int32_t n);
const char *mx_luaapi_upval_name(mx_std_state_t *ls, int32_t funcindex, int32_t n);
void *mx_luaapi_box_resize(lua_State *L, int32_t idx, size_t size);
int32_t mx_luaapi_box_gc(mx_std_state_t *ls);
void mx_luaapi_box_new(lua_State *L);
char *mx_luaapi_buf_prep(luaL_Buffer *B, size_t sz, int32_t boxidx);
mx_std_state_t *mx_luaapi_thread_state(mx_t *vm, mx_thread_t *co);
int32_t mx_luaapi_hook_adapter(mx_std_state_t *ls);
lua_Hook mx_luaapi_hook_of(mx_t *vm);

#endif
