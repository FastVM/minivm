
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int failures;

static void check(int cond, const char *what) {
    if (!cond) {
        printf("failed: %s\n", what);
        failures += 1;
    }
}

static int host_twice(lua_State *L) {
    lua_pushinteger(L, 2 * luaL_checkinteger(L, 1));
    return 1;
}

static void *host_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    return luaL_alloc(ud, ptr, osize, nsize);
}

static int host_panic(lua_State *L) {
    printf("panic: %s\n", lua_tostring(L, -1));
    fflush(stdout);
    exit(3);
}

static int run_panic(void) {
    lua_State *L = luaL_newstate();
    lua_atpanic(L, host_panic);
    lua_pushnil(L);
    lua_pushinteger(L, 1);
    lua_gettable(L, -2);
    return 0;
}

static int run_checks(void) {
    lua_State *L = luaL_newstate();
    check(L != NULL, "newstate");
    check(lua_getglobal(L, "print") == LUA_TNIL, "globals start empty");
    lua_settop(L, 0);
    luaL_openselectedlibs(L, LUA_GLIBK | LUA_STRLIBK | LUA_LOADLIBK, LUA_MATHLIBK);
    check(lua_gettop(L) == 0, "openselectedlibs balances the stack");
    check(lua_getglobal(L, "print") == LUA_TFUNCTION, "base opened");
    check(lua_getglobal(L, "string") == LUA_TTABLE, "string opened");
    check(lua_getglobal(L, "io") == LUA_TNIL, "io not opened");
    check(lua_getglobal(L, "math") == LUA_TNIL, "math only preloaded");
    lua_settop(L, 0);
    lua_register(L, "twice", host_twice);
    check(luaL_dostring(L, "return twice(21), ('x'):rep(3), require('math').floor(2.5)") == LUA_OK, "dostring");
    check(lua_tointeger(L, 1) == 42, "C function called from Lua");
    check(strcmp(lua_tostring(L, 2), "xxx") == 0, "string methods");
    check(lua_tointeger(L, 3) == 2, "preloaded math");
    lua_settop(L, 0);
    check(luaL_loadstring(L, "error('boom', 0)") == LUA_OK, "load");
    check(lua_pcall(L, 0, 0, 0) == LUA_ERRRUN, "error status");
    check(strcmp(lua_tostring(L, -1), "boom") == 0, "error message");
    lua_settop(L, 0);
    lua_State *other = luaL_newstate();
    luaL_openlibs(other);
    check(lua_getglobal(other, "io") == LUA_TTABLE, "second state has io");
    check(lua_getglobal(other, "twice") == LUA_TNIL, "states are separate");
    check(lua_getglobal(L, "twice") == LUA_TFUNCTION, "first state kept its global");
    lua_close(other);
    check(luaL_dostring(L, "local t = {} for i = 1, 100000 do t[i] = {i} end collectgarbage() return #t") == LUA_OK, "allocate after closing another state");
    check(lua_tointeger(L, -1) == 100000, "first state still works");
    lua_close(L);
    int tag = 0;
    lua_State *own = lua_newstate(host_alloc, &tag, 0);
    void *ud = NULL;
    check(lua_getallocf(own, &ud) == host_alloc && ud == &tag, "getallocf returns the state's allocator");
    lua_setallocf(own, luaL_alloc, NULL);
    check(lua_getallocf(own, &ud) == luaL_alloc && ud == NULL, "setallocf replaces it");
    lua_close(own);
    if (failures == 0) {
        printf("ok\n");
    }
    return failures != 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "panic") == 0) {
        return run_panic();
    } else {
        return run_checks();
    }
}
