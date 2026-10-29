
#pragma once

#if !defined(MX_HEADER_PUBLIC_LUACONF)
#define MX_HEADER_PUBLIC_LUACONF

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define LUA_INTEGER long long
#define LUA_UNSIGNED unsigned long long
#define LUA_NUMBER double
#define LUA_KCONTEXT intptr_t

#define LUA_INTEGER_FMT "%lld"
#define LUA_INTEGER_FRMLEN "ll"
#define LUA_NUMBER_FMT "%.15g"
#define LUAI_UACNUMBER double
#define LUAI_UACINT LUA_INTEGER

#define LUA_MAXINTEGER LLONG_MAX
#define LUA_MININTEGER LLONG_MIN

#define LUA_IDSIZE 60
#define LUA_EXTRASPACE (sizeof(void *))
#define LUAI_MAXALIGN long double u; void *s; long long l
#define LUAL_BUFFERSIZE ((int)(16 * sizeof(void *) * sizeof(lua_Number)))

#define LUA_API extern
#define LUALIB_API LUA_API

#if defined(__cplusplus)
#define LUAMOD_API extern "C"
#else
#define LUAMOD_API LUA_API
#endif

#define LUA_ROOT "/usr/local/"
#define LUA_LDIR LUA_ROOT "share/lua/5.5/"
#define LUA_CDIR LUA_ROOT "lib/lua/5.5/"
#define LUA_PATH_DEFAULT LUA_LDIR "?.lua;" LUA_LDIR "?/init.lua;" LUA_CDIR "?.lua;" LUA_CDIR "?/init.lua;" "./?.lua;" "./?/init.lua"
#define LUA_CPATH_DEFAULT LUA_CDIR "?.so;" LUA_CDIR "loadall.so;" "./?.so"

#define LUA_DIRSEP "/"
#define LUA_PATH_SEP ";"
#define LUA_PATH_MARK "?"
#define LUA_EXEC_DIR "!"

#if defined(__GNUC__)
#define luai_likely(x) (__builtin_expect(((x) != 0), 1))
#define luai_unlikely(x) (__builtin_expect(((x) != 0), 0))
#else
#define luai_likely(x) (x)
#define luai_unlikely(x) (x)
#endif

#endif
