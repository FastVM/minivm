
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static void *const luaabi_fns[] = {
    (void *)lua_newstate, (void *)lua_close, (void *)lua_newthread, (void *)lua_closethread,
    (void *)lua_atpanic, (void *)lua_version, (void *)lua_absindex, (void *)lua_gettop,
    (void *)lua_settop, (void *)lua_pushvalue, (void *)lua_rotate, (void *)lua_copy,
    (void *)lua_checkstack, (void *)lua_xmove, (void *)lua_isnumber, (void *)lua_isstring,
    (void *)lua_iscfunction, (void *)lua_isinteger, (void *)lua_isuserdata, (void *)lua_type,
    (void *)lua_typename, (void *)lua_tonumberx, (void *)lua_tointegerx, (void *)lua_toboolean,
    (void *)lua_tolstring, (void *)lua_rawlen, (void *)lua_tocfunction, (void *)lua_touserdata,
    (void *)lua_tothread, (void *)lua_topointer, (void *)lua_arith, (void *)lua_rawequal,
    (void *)lua_compare, (void *)lua_pushnil, (void *)lua_pushnumber, (void *)lua_pushinteger,
    (void *)lua_pushlstring, (void *)lua_pushexternalstring, (void *)lua_pushstring, (void *)lua_pushvfstring,
    (void *)lua_pushfstring, (void *)lua_pushcclosure, (void *)lua_pushboolean, (void *)lua_pushlightuserdata,
    (void *)lua_pushthread, (void *)lua_getglobal, (void *)lua_gettable, (void *)lua_getfield,
    (void *)lua_geti, (void *)lua_rawget, (void *)lua_rawgeti, (void *)lua_rawgetp,
    (void *)lua_createtable, (void *)lua_newuserdatauv, (void *)lua_getmetatable, (void *)lua_getiuservalue,
    (void *)lua_setglobal, (void *)lua_settable, (void *)lua_setfield, (void *)lua_seti,
    (void *)lua_rawset, (void *)lua_rawseti, (void *)lua_rawsetp, (void *)lua_setmetatable,
    (void *)lua_setiuservalue, (void *)lua_callk, (void *)lua_pcallk, (void *)lua_load,
    (void *)lua_dump, (void *)lua_yieldk, (void *)lua_resume, (void *)lua_status,
    (void *)lua_isyieldable, (void *)lua_setwarnf, (void *)lua_warning, (void *)lua_gc,
    (void *)lua_error, (void *)lua_next, (void *)lua_concat, (void *)lua_len,
    (void *)lua_numbertocstring, (void *)lua_stringtonumber, (void *)lua_getallocf, (void *)lua_setallocf,
    (void *)lua_toclose, (void *)lua_closeslot, (void *)lua_getstack, (void *)lua_getinfo,
    (void *)lua_getlocal, (void *)lua_setlocal, (void *)lua_getupvalue, (void *)lua_setupvalue,
    (void *)lua_upvalueid, (void *)lua_upvaluejoin, (void *)lua_sethook, (void *)lua_gethook,
    (void *)lua_gethookmask, (void *)lua_gethookcount,
    (void *)luaL_checkversion_, (void *)luaL_getmetafield, (void *)luaL_callmeta, (void *)luaL_tolstring,
    (void *)luaL_argerror, (void *)luaL_typeerror, (void *)luaL_checklstring, (void *)luaL_optlstring,
    (void *)luaL_checknumber, (void *)luaL_optnumber, (void *)luaL_checkinteger, (void *)luaL_optinteger,
    (void *)luaL_checkstack, (void *)luaL_checktype, (void *)luaL_checkany, (void *)luaL_newmetatable,
    (void *)luaL_setmetatable, (void *)luaL_testudata, (void *)luaL_checkudata, (void *)luaL_where,
    (void *)luaL_error, (void *)luaL_checkoption, (void *)luaL_fileresult, (void *)luaL_execresult,
    (void *)luaL_alloc, (void *)luaL_ref, (void *)luaL_unref, (void *)luaL_loadfilex,
    (void *)luaL_loadbufferx, (void *)luaL_loadstring, (void *)luaL_newstate, (void *)luaL_makeseed,
    (void *)luaL_len, (void *)luaL_addgsub, (void *)luaL_gsub, (void *)luaL_setfuncs,
    (void *)luaL_getsubtable, (void *)luaL_traceback, (void *)luaL_requiref, (void *)luaL_buffinit,
    (void *)luaL_prepbuffsize, (void *)luaL_addlstring, (void *)luaL_addstring, (void *)luaL_addvalue,
    (void *)luaL_pushresult, (void *)luaL_pushresultsize, (void *)luaL_buffinitsize,
    (void *)luaopen_base, (void *)luaopen_package, (void *)luaopen_coroutine, (void *)luaopen_debug,
    (void *)luaopen_io, (void *)luaopen_math, (void *)luaopen_os, (void *)luaopen_string,
    (void *)luaopen_table, (void *)luaopen_utf8, (void *)luaL_openselectedlibs,
};

static int luaabi_gsub(lua_State *L) {
    lua_pushstring(L, luaL_gsub(L, luaL_checkstring(L, 1), luaL_checkstring(L, 2), luaL_checkstring(L, 3)));
    return 1;
}

static int luaabi_fill(lua_State *L) {
    lua_Integer n = luaL_checkinteger(L, 1);
    int fail = lua_toboolean(L, 2);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    for (lua_Integer i = 0; i < n; i++) {
        luaL_addchar(&b, (char)('a' + i % 26));
    }
    lua_pushstring(L, "!");
    luaL_addvalue(&b);
    if (fail) {
        return luaL_error(L, "failed after %d bytes", (int)luaL_bufflen(&b));
    }
    luaL_pushresult(&b);
    lua_pushinteger(L, lua_gettop(L));
    return 2;
}

static int luaabi_drive(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_settop(L, 1);
    lua_State *co = lua_newthread(L);
    lua_pushvalue(L, 1);
    lua_xmove(L, co, 1);
    lua_createtable(L, 0, 0);
    int narg = 0;
    lua_Integer i = 1;
    int status;
    do {
        int nres = 0;
        status = lua_resume(co, L, narg, &nres);
        lua_xmove(co, L, nres);
        for (int k = 1; k <= nres; k++) {
            lua_pushvalue(L, 3 + k);
            lua_rawseti(L, 3, i);
            i += 1;
        }
        lua_settop(L, 3);
        lua_pushinteger(co, i);
        narg = 1;
    } while (status == LUA_YIELD);
    lua_pushinteger(L, status);
    lua_pushinteger(L, lua_status(co));
    return 3;
}

static int luaabi_peek(lua_State *L) {
    lua_State *co = lua_tothread(L, 1);
    if (co == NULL) {
        return 0;
    }
    int status = lua_status(co);
    int closed = lua_closethread(co, L);
    lua_pushinteger(L, status);
    lua_pushinteger(L, closed);
    return 2;
}

static int luaabi_counted;
static int luaabi_lastline;
static int luaabi_lines;

static void luaabi_count_hook(lua_State *L, lua_Debug *ar) {
    if (ar->event == LUA_HOOKCOUNT) {
        luaabi_counted += 1;
    } else if (ar->event == LUA_HOOKLINE && lua_getinfo(L, "Sl", ar) && ar->currentline > 0) {
        luaabi_lastline = ar->currentline;
        luaabi_lines += 1;
    }
}

static int luaabi_hooked(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    luaabi_counted = 0;
    luaabi_lines = 0;
    lua_sethook(L, luaabi_count_hook, LUA_MASKCOUNT | LUA_MASKLINE, 100);
    int same = lua_gethook(L) == luaabi_count_hook && lua_gethookcount(L) == 100;
    lua_pushvalue(L, 1);
    lua_call(L, 0, 0);
    lua_sethook(L, NULL, 0, 0);
    lua_pushinteger(L, luaabi_counted);
    lua_pushinteger(L, luaabi_lines);
    lua_pushinteger(L, luaabi_lastline);
    lua_pushboolean(L, same && lua_gethook(L) == NULL);
    return 4;
}

static int luaabi_fstring(lua_State *L) {
    lua_pushfstring(L, "%s|%d|%I|%c|%U|%%", "str", 42, (lua_Integer)1 << 40, 'z', 0x20AC);
    return 1;
}

static int luaabi_arith(lua_State *L) {
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);
    lua_arith(L, LUA_OPIDIV);
    lua_pushvalue(L, 1);
    lua_arith(L, LUA_OPUNM);
    lua_pushboolean(L, lua_compare(L, 1, 2, LUA_OPLT));
    return 3;
}

static int luaabi_info(lua_State *L) {
    lua_Debug ar;
    if (!lua_getstack(L, 1, &ar) || !lua_getinfo(L, "Sl", &ar)) {
        return 0;
    }
    lua_pushstring(L, ar.what);
    lua_pushinteger(L, ar.currentline);
    lua_pushstring(L, ar.short_src);
    return 3;
}

static int luaabi_globals(lua_State *L) {
    lua_pushglobaltable(L);
    lua_getfield(L, -1, "print");
    lua_pushboolean(L, lua_isfunction(L, -1));
    return 1;
}

static int luaabi_load(lua_State *L) {
    size_t len;
    const char *src = luaL_checklstring(L, 1, &len);
    int status = luaL_loadbuffer(L, src, len, "=luaabi");
    lua_pushinteger(L, status);
    lua_insert(L, -2);
    return 2;
}

static const luaL_Reg luaabi_lib[] = {
    {"gsub", luaabi_gsub},
    {"fill", luaabi_fill},
    {"drive", luaabi_drive},
    {"peek", luaabi_peek},
    {"hooked", luaabi_hooked},
    {"fstring", luaabi_fstring},
    {"arith", luaabi_arith},
    {"info", luaabi_info},
    {"globals", luaabi_globals},
    {"load", luaabi_load},
    {NULL, NULL},
};

int luaopen_luaabi(lua_State *L) {
    luaL_newlib(L, luaabi_lib);
    lua_pushboolean(L, luaabi_fns[0] != NULL);
    lua_setfield(L, -2, "fns");
    return 1;
}
