
#pragma once

#if !defined(MX_HEADER_PUBLIC_LUA)
#define MX_HEADER_PUBLIC_LUA

#include <stdarg.h>
#include <stddef.h>

#include "luaconf.h"

#define LUA_VERSION_MAJOR_N 5
#define LUA_VERSION_MINOR_N 5
#define LUA_VERSION_RELEASE_N 1
#define LUA_VERSION_NUM (LUA_VERSION_MAJOR_N * 100 + LUA_VERSION_MINOR_N)
#define LUA_VERSION_RELEASE_NUM (LUA_VERSION_NUM * 100 + LUA_VERSION_RELEASE_N)
#define LUA_VERSION_MAJOR "5"
#define LUA_VERSION_MINOR "5"
#define LUA_VERSION_RELEASE "1"
#define LUA_VERSION "Lua 5.5"
#define LUA_RELEASE "Lua 5.5.1"
#define LUA_COPYRIGHT LUA_RELEASE "  Copyright (C) 1994-2026 Lua.org, PUC-Rio"
#define LUA_AUTHORS "R. Ierusalimschy, L. H. de Figueiredo, W. Celes"

#define LUA_SIGNATURE "\x1bLua"
#define LUA_MULTRET (-1)
#define LUA_REGISTRYINDEX (-(INT_MAX / 2 + 1000))
#define lua_upvalueindex(i) (LUA_REGISTRYINDEX - (i))

#define LUA_OK 0
#define LUA_YIELD 1
#define LUA_ERRRUN 2
#define LUA_ERRSYNTAX 3
#define LUA_ERRMEM 4
#define LUA_ERRERR 5

typedef struct lua_State lua_State;

#define LUA_TNONE (-1)
#define LUA_TNIL 0
#define LUA_TBOOLEAN 1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TNUMBER 3
#define LUA_TSTRING 4
#define LUA_TTABLE 5
#define LUA_TFUNCTION 6
#define LUA_TUSERDATA 7
#define LUA_TTHREAD 8
#define LUA_NUMTYPES 9

#define LUA_MINSTACK 20

#define LUA_RIDX_GLOBALS 2
#define LUA_RIDX_MAINTHREAD 3
#define LUA_RIDX_LAST 3

typedef LUA_NUMBER lua_Number;
typedef LUA_INTEGER lua_Integer;
typedef LUA_UNSIGNED lua_Unsigned;
typedef LUA_KCONTEXT lua_KContext;

typedef int (*lua_CFunction)(lua_State *L);
typedef int (*lua_KFunction)(lua_State *L, int status, lua_KContext ctx);
typedef const char *(*lua_Reader)(lua_State *L, void *ud, size_t *sz);
typedef int (*lua_Writer)(lua_State *L, const void *p, size_t sz, void *ud);
typedef void *(*lua_Alloc)(void *ud, void *ptr, size_t osize, size_t nsize);
typedef void (*lua_WarnFunction)(void *ud, const char *msg, int tocont);

typedef struct lua_Debug lua_Debug;
typedef void (*lua_Hook)(lua_State *L, lua_Debug *ar);

LUA_API lua_State *lua_newstate(lua_Alloc f, void *ud, unsigned seed);
LUA_API void lua_close(lua_State *L);
LUA_API lua_State *lua_newthread(lua_State *L);
LUA_API int lua_closethread(lua_State *L, lua_State *from);
LUA_API lua_CFunction lua_atpanic(lua_State *L, lua_CFunction panicf);
LUA_API lua_Number lua_version(lua_State *L);

LUA_API int lua_absindex(lua_State *L, int idx);
LUA_API int lua_gettop(lua_State *L);
LUA_API void lua_settop(lua_State *L, int idx);
LUA_API void lua_pushvalue(lua_State *L, int idx);
LUA_API void lua_rotate(lua_State *L, int idx, int n);
LUA_API void lua_copy(lua_State *L, int fromidx, int toidx);
LUA_API int lua_checkstack(lua_State *L, int n);
LUA_API void lua_xmove(lua_State *from, lua_State *to, int n);

LUA_API int lua_isnumber(lua_State *L, int idx);
LUA_API int lua_isstring(lua_State *L, int idx);
LUA_API int lua_iscfunction(lua_State *L, int idx);
LUA_API int lua_isinteger(lua_State *L, int idx);
LUA_API int lua_isuserdata(lua_State *L, int idx);
LUA_API int lua_type(lua_State *L, int idx);
LUA_API const char *lua_typename(lua_State *L, int tp);

LUA_API lua_Number lua_tonumberx(lua_State *L, int idx, int *isnum);
LUA_API lua_Integer lua_tointegerx(lua_State *L, int idx, int *isnum);
LUA_API int lua_toboolean(lua_State *L, int idx);
LUA_API const char *lua_tolstring(lua_State *L, int idx, size_t *len);
LUA_API lua_Unsigned lua_rawlen(lua_State *L, int idx);
LUA_API lua_CFunction lua_tocfunction(lua_State *L, int idx);
LUA_API void *lua_touserdata(lua_State *L, int idx);
LUA_API lua_State *lua_tothread(lua_State *L, int idx);
LUA_API const void *lua_topointer(lua_State *L, int idx);

#define LUA_OPADD 0
#define LUA_OPSUB 1
#define LUA_OPMUL 2
#define LUA_OPMOD 3
#define LUA_OPPOW 4
#define LUA_OPDIV 5
#define LUA_OPIDIV 6
#define LUA_OPBAND 7
#define LUA_OPBOR 8
#define LUA_OPBXOR 9
#define LUA_OPSHL 10
#define LUA_OPSHR 11
#define LUA_OPUNM 12
#define LUA_OPBNOT 13

LUA_API void lua_arith(lua_State *L, int op);

#define LUA_OPEQ 0
#define LUA_OPLT 1
#define LUA_OPLE 2

LUA_API int lua_rawequal(lua_State *L, int idx1, int idx2);
LUA_API int lua_compare(lua_State *L, int idx1, int idx2, int op);

LUA_API void lua_pushnil(lua_State *L);
LUA_API void lua_pushnumber(lua_State *L, lua_Number n);
LUA_API void lua_pushinteger(lua_State *L, lua_Integer n);
LUA_API const char *lua_pushlstring(lua_State *L, const char *s, size_t len);
LUA_API const char *lua_pushexternalstring(lua_State *L, const char *s, size_t len, lua_Alloc falloc, void *ud);
LUA_API const char *lua_pushstring(lua_State *L, const char *s);
LUA_API const char *lua_pushvfstring(lua_State *L, const char *fmt, va_list argp);
LUA_API const char *lua_pushfstring(lua_State *L, const char *fmt, ...);
LUA_API void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n);
LUA_API void lua_pushboolean(lua_State *L, int b);
LUA_API void lua_pushlightuserdata(lua_State *L, void *p);
LUA_API int lua_pushthread(lua_State *L);

LUA_API int lua_getglobal(lua_State *L, const char *name);
LUA_API int lua_gettable(lua_State *L, int idx);
LUA_API int lua_getfield(lua_State *L, int idx, const char *k);
LUA_API int lua_geti(lua_State *L, int idx, lua_Integer n);
LUA_API int lua_rawget(lua_State *L, int idx);
LUA_API int lua_rawgeti(lua_State *L, int idx, lua_Integer n);
LUA_API int lua_rawgetp(lua_State *L, int idx, const void *p);

LUA_API void lua_createtable(lua_State *L, int narr, int nrec);
LUA_API void *lua_newuserdatauv(lua_State *L, size_t sz, int nuvalue);
LUA_API int lua_getmetatable(lua_State *L, int objindex);
LUA_API int lua_getiuservalue(lua_State *L, int idx, int n);

LUA_API void lua_setglobal(lua_State *L, const char *name);
LUA_API void lua_settable(lua_State *L, int idx);
LUA_API void lua_setfield(lua_State *L, int idx, const char *k);
LUA_API void lua_seti(lua_State *L, int idx, lua_Integer n);
LUA_API void lua_rawset(lua_State *L, int idx);
LUA_API void lua_rawseti(lua_State *L, int idx, lua_Integer n);
LUA_API void lua_rawsetp(lua_State *L, int idx, const void *p);
LUA_API int lua_setmetatable(lua_State *L, int objindex);
LUA_API int lua_setiuservalue(lua_State *L, int idx, int n);

LUA_API void lua_callk(lua_State *L, int nargs, int nresults, lua_KContext ctx, lua_KFunction k);
#define lua_call(L, n, r) lua_callk(L, (n), (r), 0, NULL)

LUA_API int lua_pcallk(lua_State *L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k);
#define lua_pcall(L, n, r, f) lua_pcallk(L, (n), (r), (f), 0, NULL)

LUA_API int lua_load(lua_State *L, lua_Reader reader, void *dt, const char *chunkname, const char *mode);
LUA_API int lua_dump(lua_State *L, lua_Writer writer, void *data, int strip);

LUA_API int lua_yieldk(lua_State *L, int nresults, lua_KContext ctx, lua_KFunction k);
LUA_API int lua_resume(lua_State *L, lua_State *from, int narg, int *nres);
LUA_API int lua_status(lua_State *L);
LUA_API int lua_isyieldable(lua_State *L);
#define lua_yield(L, n) lua_yieldk(L, (n), 0, NULL)

LUA_API void lua_setwarnf(lua_State *L, lua_WarnFunction f, void *ud);
LUA_API void lua_warning(lua_State *L, const char *msg, int tocont);

#define LUA_GCSTOP 0
#define LUA_GCRESTART 1
#define LUA_GCCOLLECT 2
#define LUA_GCCOUNT 3
#define LUA_GCCOUNTB 4
#define LUA_GCSTEP 5
#define LUA_GCISRUNNING 6
#define LUA_GCGEN 7
#define LUA_GCINC 8
#define LUA_GCPARAM 9

#define LUA_GCPMINORMUL 0
#define LUA_GCPMAJORMINOR 1
#define LUA_GCPMINORMAJOR 2
#define LUA_GCPPAUSE 3
#define LUA_GCPSTEPMUL 4
#define LUA_GCPSTEPSIZE 5
#define LUA_GCPN 6

LUA_API int lua_gc(lua_State *L, int what, ...);

LUA_API int lua_error(lua_State *L);
LUA_API int lua_next(lua_State *L, int idx);
LUA_API void lua_concat(lua_State *L, int n);
LUA_API void lua_len(lua_State *L, int idx);

#define LUA_N2SBUFFSZ 64
LUA_API unsigned lua_numbertocstring(lua_State *L, int idx, char *buff);
LUA_API size_t lua_stringtonumber(lua_State *L, const char *s);

LUA_API lua_Alloc lua_getallocf(lua_State *L, void **ud);
LUA_API void lua_setallocf(lua_State *L, lua_Alloc f, void *ud);

LUA_API void lua_toclose(lua_State *L, int idx);
LUA_API void lua_closeslot(lua_State *L, int idx);

#define lua_getextraspace(L) ((void *)((char *)(L) - LUA_EXTRASPACE))

#define lua_tonumber(L, i) lua_tonumberx(L, (i), NULL)
#define lua_tointeger(L, i) lua_tointegerx(L, (i), NULL)

#define lua_pop(L, n) lua_settop(L, -(n) - 1)

#define lua_newtable(L) lua_createtable(L, 0, 0)

#define lua_register(L, n, f) (lua_pushcfunction(L, (f)), lua_setglobal(L, (n)))

#define lua_pushcfunction(L, f) lua_pushcclosure(L, (f), 0)

#define lua_isfunction(L, n) (lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L, n) (lua_type(L, (n)) == LUA_TTABLE)
#define lua_islightuserdata(L, n) (lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L, n) (lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L, n) (lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L, n) (lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L, n) (lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n) (lua_type(L, (n)) <= 0)

#define lua_pushliteral(L, s) lua_pushstring(L, "" s)

#define lua_pushglobaltable(L) ((void)lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS))

#define lua_tostring(L, i) lua_tolstring(L, (i), NULL)

#define lua_insert(L, idx) lua_rotate(L, (idx), 1)

#define lua_remove(L, idx) (lua_rotate(L, (idx), -1), lua_pop(L, 1))

#define lua_replace(L, idx) (lua_copy(L, -1, (idx)), lua_pop(L, 1))

#define lua_newuserdata(L, s) lua_newuserdatauv(L, s, 1)
#define lua_getuservalue(L, idx) lua_getiuservalue(L, idx, 1)
#define lua_setuservalue(L, idx) lua_setiuservalue(L, idx, 1)

#define lua_resetthread(L) lua_closethread(L, NULL)

#define LUA_HOOKCALL 0
#define LUA_HOOKRET 1
#define LUA_HOOKLINE 2
#define LUA_HOOKCOUNT 3
#define LUA_HOOKTAILCALL 4

#define LUA_MASKCALL (1 << LUA_HOOKCALL)
#define LUA_MASKRET (1 << LUA_HOOKRET)
#define LUA_MASKLINE (1 << LUA_HOOKLINE)
#define LUA_MASKCOUNT (1 << LUA_HOOKCOUNT)

LUA_API int lua_getstack(lua_State *L, int level, lua_Debug *ar);
LUA_API int lua_getinfo(lua_State *L, const char *what, lua_Debug *ar);
LUA_API const char *lua_getlocal(lua_State *L, const lua_Debug *ar, int n);
LUA_API const char *lua_setlocal(lua_State *L, const lua_Debug *ar, int n);
LUA_API const char *lua_getupvalue(lua_State *L, int funcindex, int n);
LUA_API const char *lua_setupvalue(lua_State *L, int funcindex, int n);

LUA_API void *lua_upvalueid(lua_State *L, int fidx, int n);
LUA_API void lua_upvaluejoin(lua_State *L, int fidx1, int n1, int fidx2, int n2);

LUA_API void lua_sethook(lua_State *L, lua_Hook func, int mask, int count);
LUA_API lua_Hook lua_gethook(lua_State *L);
LUA_API int lua_gethookmask(lua_State *L);
LUA_API int lua_gethookcount(lua_State *L);

struct lua_Debug {
    int event;
    const char *name;
    const char *namewhat;
    const char *what;
    const char *source;
    size_t srclen;
    int currentline;
    int linedefined;
    int lastlinedefined;
    unsigned char nups;
    unsigned char nparams;
    char isvararg;
    unsigned char extraargs;
    char istailcall;
    int ftransfer;
    int ntransfer;
    char short_src[LUA_IDSIZE];
    struct CallInfo *i_ci;
};

#endif
