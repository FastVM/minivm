
#include <lua.h>
#include <lauxlib.h>

static int luamod_add(lua_State *L) {
    lua_Integer a = luaL_checkinteger(L, 1);
    lua_Integer b = luaL_checkinteger(L, 2);
    lua_pushinteger(L, a + b);
    return 1;
}

static int luamod_concat(lua_State *L) {
    const char *a = luaL_checkstring(L, 1);
    const char *b = luaL_optstring(L, 2, "!");
    lua_pushfstring(L, "%s%s", a, b);
    return 1;
}

static int luamod_counter_step(lua_State *L) {
    lua_Integer n = lua_tointeger(L, lua_upvalueindex(1)) + 1;
    lua_pushinteger(L, n);
    lua_copy(L, -1, lua_upvalueindex(1));
    return 1;
}

static int luamod_counter(lua_State *L) {
    lua_pushinteger(L, 0);
    lua_pushcclosure(L, luamod_counter_step, 1);
    return 1;
}

static int luamod_sum(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_Integer total = 0;
    lua_Integer n = (lua_Integer)lua_rawlen(L, 1);
    for (lua_Integer i = 1; i <= n; i++) {
        lua_rawgeti(L, 1, i);
        total += lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    lua_pushinteger(L, total);
    return 1;
}

static int luamod_boom(lua_State *L) {
    return luaL_error(L, "boom from %s", "C");
}

static int luamod_twice(lua_State *L) {
    lua_Integer n = luaL_checkinteger(L, 1);
    lua_pushinteger(L, n * 2);
    return lua_yield(L, 1);
}

static int luamod_call(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
    return lua_gettop(L);
}

static int luamod_try(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int status = lua_pcall(L, lua_gettop(L) - 1, 1, 0);
    lua_pushinteger(L, status);
    lua_insert(L, -2);
    return 2;
}

static int luamod_ptr(lua_State *L) {
    static int marker = 0;
    lua_pushlightuserdata(L, &marker);
    return 1;
}

typedef struct { double x; double y; } Point;

#define POINT_MT "luamod.Point"

static int luamod_point_new(lua_State *L) {
    double x = luaL_checknumber(L, 1);
    double y = luaL_checknumber(L, 2);
    Point *p = lua_newuserdatauv(L, sizeof(Point), 1);
    p->x = x;
    p->y = y;
    lua_pushstring(L, "tag");
    lua_setiuservalue(L, -2, 1);
    luaL_setmetatable(L, POINT_MT);
    return 1;
}

static int luamod_point_sum(lua_State *L) {
    Point *p = luaL_checkudata(L, 1, POINT_MT);
    lua_pushnumber(L, p->x + p->y);
    return 1;
}

static int luamod_point_uv(lua_State *L) {
    luaL_checkudata(L, 1, POINT_MT);
    lua_getiuservalue(L, 1, 1);
    return 1;
}

static int luamod_point_tostring(lua_State *L) {
    Point *p = luaL_checkudata(L, 1, POINT_MT);
    lua_pushfstring(L, "Point(%d,%d)", (int)p->x, (int)p->y);
    return 1;
}

static const luaL_Reg luamod_point_mt[] = {
    {"sum", luamod_point_sum},
    {"uv", luamod_point_uv},
    {NULL, NULL},
};

static const luaL_Reg luamod_fns[] = {
    {"point", luamod_point_new},
    {"add", luamod_add},
    {"concat", luamod_concat},
    {"counter", luamod_counter},
    {"sum", luamod_sum},
    {"boom", luamod_boom},
    {"ptr", luamod_ptr},
    {"twice", luamod_twice},
    {"call", luamod_call},
    {"try", luamod_try},
    {NULL, NULL},
};

int luaopen_luamod(lua_State *L) {
    luaL_checkversion(L);
    luaL_newmetatable(L, POINT_MT);
    luaL_newlib(L, luamod_point_mt);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, luamod_point_tostring);
    lua_setfield(L, -2, "__tostring");
    lua_pop(L, 1);
    luaL_newlib(L, luamod_fns);
    return 1;
}
