
#pragma once

#if !defined(MX_HEADER_PUBLIC_LUACONF)
#define MX_HEADER_PUBLIC_LUACONF

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#define LUA_INT_INT 1
#define LUA_INT_LONG 2
#define LUA_INT_LONGLONG 3
#define LUA_FLOAT_FLOAT 1
#define LUA_FLOAT_DOUBLE 2
#define LUA_FLOAT_LONGDOUBLE 3
#define LUA_INT_DEFAULT LUA_INT_LONGLONG
#define LUA_FLOAT_DEFAULT LUA_FLOAT_DOUBLE
#define LUA_INT_TYPE LUA_INT_DEFAULT
#define LUA_FLOAT_TYPE LUA_FLOAT_DEFAULT

#define LUAI_IS32INT ((UINT_MAX >> 30) >= 3)

#define LUA_INTEGER long long
#define LUA_UNSIGNED unsigned long long
#define LUA_NUMBER double
#define LUA_KCONTEXT intptr_t

#define LUA_INTEGER_FMT "%lld"
#define LUA_INTEGER_FRMLEN "ll"
#define LUA_NUMBER_FMT "%.15g"
#define LUA_NUMBER_FRMLEN ""
#define LUAI_UACNUMBER double
#define LUAI_UACINT LUA_INTEGER

#define LUA_MAXINTEGER LLONG_MAX
#define LUA_MININTEGER LLONG_MIN
#define LUA_MAXUNSIGNED ULLONG_MAX

#define l_floatatt(n) (DBL_##n)
#define l_mathop(op) op
#define l_sprintf(s, sz, f, i) snprintf(s, sz, f, i)
#define lua_str2number(s, p) strtod((s), (p))
#define lua_strx2number(s, p) lua_str2number(s, p)
#define lua_number2str(s, sz, n) l_sprintf((s), sz, LUA_NUMBER_FMT, (LUAI_UACNUMBER)(n))
#define lua_integer2str(s, sz, n) l_sprintf((s), sz, LUA_INTEGER_FMT, (LUAI_UACINT)(n))
#define lua_pointer2str(buff, sz, p) l_sprintf(buff, sz, "%p", p)
#define lua_numbertointeger(n, p) ((n) >= (LUA_NUMBER)(LUA_MININTEGER) && (n) < -(LUA_NUMBER)(LUA_MININTEGER) && (*(p) = (LUA_INTEGER)(n), 1))
#define lua_getlocaledecpoint() (localeconv()->decimal_point[0])

#define LUAI_MAXSTACK 1000000
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

#define LUA_VDIR "5.5"
#define LUA_ROOT "/usr/local/"
#define LUA_LDIR LUA_ROOT "share/lua/" LUA_VDIR "/"
#define LUA_CDIR LUA_ROOT "lib/lua/" LUA_VDIR "/"
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
