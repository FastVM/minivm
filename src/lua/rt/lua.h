
#pragma once

#if !defined(MX_HEADER_LUA_RT_LUA)
#define MX_HEADER_LUA_RT_LUA

#include <setjmp.h>

#include "pattern.h"
#include "../../value/kind.h"
#include "../../vm/vm.h"

typedef uint8_t mx_lua_meta_key_t;
enum {
    MX_LUA_META_KEY_INDEX,
    MX_LUA_META_KEY_NEWINDEX,
    MX_LUA_META_KEY_EQ,
    MX_LUA_META_KEY_LT,
    MX_LUA_META_KEY_LE,
    MX_LUA_META_KEY_CALL,
    MX_LUA_META_KEY_LEN,
    MX_LUA_META_KEY_TOSTRING,
    MX_LUA_META_KEY_NAME,
    MX_LUA_META_KEY_METATABLE,
    MX_LUA_META_KEY_PAIRS,
    MX_LUA_META_KEY_CLOSE,
    MX_LUA_META_KEY_MODE,
    MX_LUA_META_KEY_GC,
    MX_LUA_META_KEY_ARITH_BASE,
    MX_LUA_META_KEY_UNM = MX_LUA_META_KEY_ARITH_BASE + 13,
    MX_LUA_META_KEY_BNOT = MX_LUA_META_KEY_ARITH_BASE + 14,
    MX_LUA_META_KEY_COUNT = MX_LUA_META_KEY_ARITH_BASE + 15,
};

typedef int32_t mx_lua_type_t;
enum {
    MX_LUA_TYPE_NONE = -1,
    MX_LUA_TYPE_NIL,
    MX_LUA_TYPE_BOOLEAN,
    MX_LUA_TYPE_INTEGER,
    MX_LUA_TYPE_FLOAT,
    MX_LUA_TYPE_STRING,
    MX_LUA_TYPE_TABLE,
    MX_LUA_TYPE_FUNCTION,
    MX_LUA_TYPE_USERDATA,
    MX_LUA_TYPE_THREAD,
    MX_LUA_TYPE_COUNT,
};

#define MX_LUA_HOOK_CALL 1
#define MX_LUA_HOOK_RETURN 2
#define MX_LUA_HOOK_LINE 4
#define MX_LUA_HOOK_COUNT 8

typedef struct mx_lua_hook_t mx_lua_hook_t;
typedef struct mx_rand_t mx_rand_t;
typedef struct mx_lua_foreign_t mx_lua_foreign_t;
typedef struct mx_lua_warn_t mx_lua_warn_t;
typedef void mx_lua_warnf_t(void *ud, const char *msg, int32_t tocont);
typedef void *mx_lua_allocf_t(void *ud, void *ptr, size_t osize, size_t nsize);

struct mx_lua_warn_t {
    mx_lua_warnf_t *fn;
    void *ud;
    bool is_on: 1;
    bool is_cont: 1;
};

struct mx_lua_foreign_t {
    jmp_buf *jmp;
    mx_std_state_t *ls;
};

struct mx_rand_t {
    uint64_t s[4];
};

struct mx_lua_hook_t {
    mx_obj_t fn;
    mx_thread_t *thread;
    uint64_t last_at;
    int64_t last_line;
    uint32_t last_frame;
    uint32_t mask;
    int64_t count;
    int64_t counter;
    bool is_in_hook: 1;
    bool is_tail: 1;
    bool is_resumed: 1;
};

struct mx_lua_t {
    mx_t *vm;
    mx_obj_t std;
    mx_obj_t registry;
    mx_obj_t stringlib;
    mx_obj_t string_meta;
    mx_obj_t meta_keys[MX_LUA_META_KEY_COUNT];
    mx_obj_t type_names[MX_LUA_TYPE_COUNT];
    mx_obj_t type_metas[MX_LUA_TYPE_COUNT];
    mx_obj_t file_meta;
    mx_obj_t io_in;
    mx_obj_t io_out;
    mx_obj_t light_cfns;
    mx_obj_t libs;
    mx_std_fn_t *panic;
    mx_lua_allocf_t *allocf;
    void *alloc_ud;
    mx_lua_hook_t hook;
    mx_lua_foreign_t foreign;
    mx_lua_warn_t warn;
    mx_pattern_cache_t patterns;
    mx_kind_t file_kind;
    mx_kind_t cont_kind;
    mx_rand_t rand;
    bool is_ext: 1;
};

static inline mx_lua_t *mx_lua_of(mx_t *vm) {
    return vm->lua_state;
}

mx_lua_t *mx_lua_open(mx_t *vm, bool is_noenv);
void mx_lua_close(mx_t *vm);
void mx_lua_set_arg(mx_t *vm, int32_t argc, char **argv, int32_t script);
void mx_lua_drop_extensions(mx_t *vm);
void mx_lua_warning(mx_t *vm, const char *msg, bool tocont);

#endif
