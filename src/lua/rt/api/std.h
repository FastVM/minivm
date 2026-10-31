
#pragma once

#if !defined(MX_HEADER_LUA_RT_API_STD)
#define MX_HEADER_LUA_RT_API_STD

#include "../lua.h"
#include "../pattern.h"
#include "../../../ir/ir.h"
#include "../../../run/frame.h"
#include "../../../run/run.h"
#include "../../../run/thread.h"
#include "../../../value/kind.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"
#include "../../../value/table.h"
#include "../../../vm/vm.h"

#define MX_STD_BUF_INIT 256
#define MX_STD_CALL_MARGIN 8
#define MX_STD_CFN_MAX_UPVALS 255
#define MX_STD_MIN_RADIX 2
#define MX_STD_MAX_RADIX 36
#define MX_STD_FMT_OUT_CAP 256
#define MX_STD_MAX_STRING INT32_MAX
#define MX_STD_QUALNAME_CAP 64
#define MX_STD_IO_NUMBUF 201

typedef int32_t mx_std_status_t;
enum {
    MX_STD_OK = 0,
    MX_STD_ERR_RUNTIME = 1,
    MX_STD_ERR_MEMORY = 2,
    MX_STD_ERR_TYPE = 3,
    MX_STD_ERR_HANDLER = 4,
};

enum {
    MX_STD_MULTRET = -1,
    MX_STD_REGISTRYINDEX = -(INT32_MAX / 2 + 1000),
};

typedef uint8_t mx_std_pack_opt_t;
enum {
    MX_STD_PACK_INT,
    MX_STD_PACK_UINT,
    MX_STD_PACK_FLOAT,
    MX_STD_PACK_NUMBER,
    MX_STD_PACK_DOUBLE,
    MX_STD_PACK_CHAR,
    MX_STD_PACK_STRING,
    MX_STD_PACK_ZSTR,
    MX_STD_PACK_PADDING,
    MX_STD_PACK_PADDALIGN,
    MX_STD_PACK_NOP,
};

typedef int32_t mx_std_slot_t;

typedef uint8_t mx_std_table_access_t;
enum {
    MX_STD_TABLE_READ = 1,
    MX_STD_TABLE_WRITE = 2,
    MX_STD_TABLE_LEN = 4,
};

typedef uint8_t mx_std_libstat_t;
enum {
    MX_STD_LIBSTAT_OK,
    MX_STD_LIBSTAT_LIB,
    MX_STD_LIBSTAT_FUNC,
};

typedef struct mx_std_reg_t mx_std_reg_t;
typedef struct mx_std_text_t mx_std_text_t;
typedef struct mx_std_debug_target_t mx_std_debug_target_t;
typedef struct mx_std_buf_t mx_std_buf_t;
typedef struct mx_std_pack_t mx_std_pack_t;
typedef struct mx_std_pack_res_t mx_std_pack_res_t;
typedef struct mx_std_pack_det_t mx_std_pack_det_t;
typedef union mx_std_pack_f32_t mx_std_pack_f32_t;
typedef struct mx_std_utf8_decode_t mx_std_utf8_decode_t;
typedef struct mx_std_io_num_t mx_std_io_num_t;
typedef struct mx_std_fmt_step_t mx_std_fmt_step_t;
typedef struct mx_std_str_t mx_std_str_t;
typedef struct mx_std_qualname_t mx_std_qualname_t;
typedef struct mx_std_udata_header_t mx_std_udata_header_t;

struct mx_std_udata_header_t {
    uint64_t size;
    uint64_t nuv;
};

struct mx_std_state_t {
    mx_t *vm;
    mx_thread_t *th;
    uint32_t base;
    int32_t top;
    mx_obj_t error;
    const char *fnname;
    mx_obj_cfn_t *upvals;
    mx_std_fn_t *fn;
    bool can_pend: 1;
    bool is_hook: 1;
};

struct mx_std_text_t {
    char *ptr;
    size_t len;
};

struct mx_std_debug_target_t {
    mx_thread_t *th;
    int32_t arg;
};

struct mx_std_reg_t {
    const char *name;
    mx_std_fn_t *fn;
    mx_ffi_t *ffi;
};

struct mx_std_buf_t {
    mx_std_state_t *ls;
    char *data;
    size_t len;
    size_t cap;
    char init[MX_STD_BUF_INIT];
};

struct mx_std_pack_t {
    mx_std_state_t *ls;
    const char *fmt;
    int32_t max_align;
    bool is_little: 1;
};

struct mx_std_pack_res_t {
    size_t size;
    mx_std_pack_opt_t opt;
};

struct mx_std_pack_det_t {
    size_t size;
    int32_t pad;
    mx_std_pack_opt_t opt;
};

union mx_std_pack_f32_t {
    uint32_t bits;
    float f32;
};

struct mx_std_utf8_decode_t {
    uint32_t code;
    uint32_t len;
};

struct mx_std_io_num_t {
    FILE *fp;
    int32_t c;
    size_t n;
    char buf[MX_STD_IO_NUMBUF];
};

struct mx_std_str_t {
    const char *ptr;
    size_t len;
};

struct mx_std_qualname_t {
    char name[MX_STD_QUALNAME_CAP];
    bool is_method: 1;
};

struct mx_std_fmt_step_t {
    const char *next;
    int32_t arg;
    int32_t ret_count;
};

void mx_std_raise(mx_std_state_t *ls);

extern const mx_std_reg_t mx_std_base_regs[];
extern const mx_std_reg_t mx_std_string_regs[];
extern const mx_std_reg_t mx_std_utf8_regs[];
extern const mx_std_reg_t mx_std_table_regs[];
extern const mx_std_reg_t mx_std_math_regs[];
extern const mx_std_reg_t mx_std_io_regs[];
extern const mx_std_reg_t mx_std_file_regs[];
extern const mx_std_reg_t mx_std_os_regs[];
extern const mx_std_reg_t mx_std_coroutine_regs[];
extern const mx_std_reg_t mx_std_debug_regs[];
extern const mx_std_reg_t mx_std_package_regs[];
extern const mx_std_reg_t mx_std_load_regs[];
extern const mx_std_reg_t mx_std_vm_regs[];
extern const mx_ffi_fast_reg_t mx_std_fasts[];
extern const char *const mx_std_lib_names[];

void mx_ffi_base_assert(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_error(mx_t *vm, size_t nargs, mx_obj_t *args);
mx_obj_t mx_std_base_visible_meta(mx_t *vm, mx_obj_table_t *meta);
void mx_ffi_base_getmetatable(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_getmetatable_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_inext(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_inext_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_ipairs(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_ipairs_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_next(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_next_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_pairs(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_pairs_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_print(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_rawequal(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_rawget(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_rawget_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_rawlen(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_rawset(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_select(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_select_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_setmetatable(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_setmetatable_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_tonumber(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_tonumber_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_tostring(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_base_type(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_base_type_fast(mx_t *vm, size_t nargs, mx_obj_t *args);

void mx_ffi_string_byte(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_char(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_find(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_format(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_gmatch(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_gsub(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_len(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_lower(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_match(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_rep(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_reverse(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_sub(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_string_sub_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_string_match_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
mx_obj_t mx_std_str_of(mx_t *vm, const char *ptr, size_t len);
bool mx_ffi_string_byte_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_string_char_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_string_rep_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_upper(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_pack(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_unpack(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_string_packsize(mx_t *vm, size_t nargs, mx_obj_t *args);

void mx_ffi_table_concat(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_table_concat_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_table_append_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_table_insert_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_table_insert(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_table_unpack_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_table_unpack(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_ffi_table_remove_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_table_remove(mx_t *vm, size_t nargs, mx_obj_t *args);

void mx_ffi_math_abs(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_acos(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_asin(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_atan(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_ceil(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_cos(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_deg(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_exp(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_floor(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_fmod(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_log(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_max(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_min(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_modf(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_frexp(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_ldexp(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_rad(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_random(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_randomseed(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_sin(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_sqrt(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_tan(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_tointeger(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_type(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_ult(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_math_round_result(mx_t *vm, mx_obj_t *args, double d);

void mx_ffi_coroutine_wrap_call(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_ffi_coroutine_wrap_fn(mx_t *vm, size_t nargs, mx_obj_t *args);
int32_t mx_std_lib_index(mx_obj_t key);
void mx_std_libs_snapshot(mx_t *vm);
void mx_std_lib_open_into(mx_std_state_t *ls, const char *name);

mx_obj_t *mx_std_at(mx_std_state_t *ls, mx_std_slot_t slot);
mx_obj_t *mx_std_pseudo_at(mx_std_state_t *ls, mx_std_slot_t slot);

static inline mx_std_slot_t mx_std_upvalueindex(int32_t i) {
    return MX_STD_REGISTRYINDEX - i;
}

mx_std_slot_t mx_std_absindex(mx_std_state_t *ls, mx_std_slot_t slot);
mx_obj_t *mx_std_slots(mx_std_state_t *ls);
void mx_std_sync(mx_std_state_t *ls);
bool mx_std_grow(mx_std_state_t *ls, int32_t extra);
void mx_std_push(mx_std_state_t *ls, mx_obj_t value);
mx_std_state_t mx_std_state_enter(mx_t *vm, mx_obj_t *args, int32_t nargs, const char *fnname, mx_std_fn_t *fn, bool can_pend);
void mx_std_dispatch(mx_t *vm, mx_std_fn_t *fn, const char *fnname, mx_obj_t *args, int32_t nargs);
void mx_std_dispatch_finish(mx_std_state_t *ls, int32_t nret);
void mx_std_take_rets(mx_std_state_t *ls, int32_t from, mx_std_slot_t nresults);
const mx_resume_t *mx_std_site(mx_std_state_t *ls);
mx_varname_t mx_std_site_name(mx_std_state_t *ls);
void mx_std_where(mx_std_state_t *ls, uint32_t level, mx_str_t *out);

mx_std_slot_t mx_std_top(mx_std_state_t *ls);
void mx_std_settop(mx_std_state_t *ls, mx_std_slot_t n);
void mx_std_pop(mx_std_state_t *ls, mx_std_slot_t n);
void mx_std_pushvalue(mx_std_state_t *ls, mx_std_slot_t slot);
void mx_std_remove(mx_std_state_t *ls, mx_std_slot_t slot);
void mx_std_insert(mx_std_state_t *ls, mx_std_slot_t slot);
void mx_std_replace(mx_std_state_t *ls, mx_std_slot_t slot);
void mx_std_rotate(mx_std_state_t *ls, mx_std_slot_t slot, mx_std_slot_t n);
void mx_std_copy(mx_std_state_t *ls, mx_std_slot_t from, mx_std_slot_t to);
bool mx_std_checkstack(mx_std_state_t *ls, mx_std_slot_t extra);

mx_lua_type_t mx_std_type(mx_std_state_t *ls, mx_std_slot_t slot);
const char *mx_std_typename(mx_std_state_t *ls, mx_lua_type_t type);
bool mx_std_isnil(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_isboolean(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_isinteger(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_isfloat(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_isnumber(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_isstring(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_istable(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_isfunction(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_isnone(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_isnoneornil(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_rawequal(mx_std_state_t *ls, mx_std_slot_t a, mx_std_slot_t b);
bool mx_std_compare(mx_std_state_t *ls, mx_std_slot_t a, mx_std_slot_t b, mx_op_t op);

bool mx_std_toboolean(mx_std_state_t *ls, mx_std_slot_t slot);
mx_obj_int_t mx_std_tointeger(mx_std_state_t *ls, mx_std_slot_t slot);
mx_obj_float_t mx_std_tofloat(mx_std_state_t *ls, mx_std_slot_t slot);
mx_obj_strtod_t mx_std_str2d_raw(const char *str);
mx_obj_strtod_t mx_std_str2d(const char *str);
mx_obj_num_t mx_std_str2int(const char *str);
mx_obj_num_t mx_std_str2float(const char *str);
mx_obj_num_t mx_std_str2num(const char *str, size_t len);
mx_std_str_t mx_std_tostring(mx_std_state_t *ls, mx_std_slot_t slot);
void mx_std_tostring_meta(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_tostring_meta_handler(mx_std_state_t *ls, mx_std_slot_t slot);
uint64_t mx_std_rawlen(mx_std_state_t *ls, mx_std_slot_t slot);
int64_t mx_std_len_meta(mx_std_state_t *ls, mx_std_slot_t slot);

void mx_std_pushnil(mx_std_state_t *ls);
void mx_std_pushboolean(mx_std_state_t *ls, bool b);
void mx_std_pushinteger(mx_std_state_t *ls, int64_t n);
void mx_std_pushfloat(mx_std_state_t *ls, double n);
void mx_std_pushstring(mx_std_state_t *ls, const char *str);
void mx_std_pushlstring(mx_std_state_t *ls, const char *str, size_t len);
void mx_std_pushcfn(mx_std_state_t *ls, mx_std_fn_t *fn, const char *name);
void mx_std_pushfstring(mx_std_state_t *ls, const char *fmt, ...);

void mx_std_newtable(mx_std_state_t *ls);
void mx_std_createtable(mx_std_state_t *ls, uint32_t narr, uint32_t nhash);
mx_lua_type_t mx_std_gettable(mx_std_state_t *ls, mx_std_slot_t table);
void mx_std_settable(mx_std_state_t *ls, mx_std_slot_t table);
mx_lua_type_t mx_std_getfield(mx_std_state_t *ls, mx_std_slot_t table, const char *key);
mx_lua_type_t mx_std_getmetakey(mx_std_state_t *ls, mx_std_slot_t table, mx_lua_meta_key_t key);
void mx_std_setfield(mx_std_state_t *ls, mx_std_slot_t table, const char *key);
mx_lua_type_t mx_std_geti(mx_std_state_t *ls, mx_std_slot_t table, int64_t i);
void mx_std_seti(mx_std_state_t *ls, mx_std_slot_t table, int64_t i);
mx_lua_type_t mx_std_rawget(mx_std_state_t *ls, mx_std_slot_t table);
void mx_std_rawset(mx_std_state_t *ls, mx_std_slot_t table);
mx_lua_type_t mx_std_rawgeti(mx_std_state_t *ls, mx_std_slot_t table, int64_t i);
void mx_std_rawseti(mx_std_state_t *ls, mx_std_slot_t table, int64_t i);
bool mx_std_next(mx_std_state_t *ls, mx_std_slot_t table);
bool mx_std_getmetatable(mx_std_state_t *ls, mx_std_slot_t slot);
void mx_std_setmetatable(mx_std_state_t *ls, mx_std_slot_t slot);

void mx_std_call(mx_std_state_t *ls, mx_std_slot_t nargs, mx_std_slot_t nresults);
mx_std_status_t mx_std_pcall(mx_std_state_t *ls, mx_std_slot_t nargs, mx_std_slot_t nresults, mx_std_slot_t msgh);

int64_t mx_std_checkinteger(mx_std_state_t *ls, mx_std_slot_t arg);
double mx_std_checkfloat(mx_std_state_t *ls, mx_std_slot_t arg);
mx_std_str_t mx_std_checkstring(mx_std_state_t *ls, mx_std_slot_t arg);
void mx_std_checktype(mx_std_state_t *ls, mx_std_slot_t arg, mx_lua_type_t type);
void mx_std_checkany(mx_std_state_t *ls, mx_std_slot_t arg);
int64_t mx_std_optinteger(mx_std_state_t *ls, mx_std_slot_t arg, int64_t def);
double mx_std_optfloat(mx_std_state_t *ls, mx_std_slot_t arg, double def);
mx_std_str_t mx_std_optstring(mx_std_state_t *ls, mx_std_slot_t arg, const char *def);
int32_t mx_std_checkoption(mx_std_state_t *ls, mx_std_slot_t arg, const char *def, const char *const opts[]);
int32_t mx_std_checkoption_at(mx_std_state_t *ls, mx_std_slot_t slot, mx_std_slot_t argnum, const char *def, const char *const opts[]);

const char *mx_std_libname(mx_std_fn_t *fn);
mx_std_qualname_t mx_std_qualname(mx_std_state_t *ls);
int32_t mx_std_error(mx_std_state_t *ls, const char *fmt, ...);
int32_t mx_std_error_noloc(mx_std_state_t *ls, const char *fmt, ...);
int32_t mx_std_argerror(mx_std_state_t *ls, mx_std_slot_t arg, const char *msg);
int32_t mx_std_typeerror(mx_std_state_t *ls, mx_std_slot_t arg, const char *expected);

void mx_std_buf_init(mx_std_state_t *ls, mx_std_buf_t *buf);
char *mx_std_buf_prep(mx_std_buf_t *buf, size_t need);
void mx_std_buf_addlstring(mx_std_buf_t *buf, const char *data, size_t len);
void mx_std_buf_addstring(mx_std_buf_t *buf, const char *str);
void mx_std_buf_addchar_slow(mx_std_buf_t *buf, char c);

static inline void mx_std_buf_addchar(mx_std_buf_t *buf, char c) {
    if (buf->len < buf->cap) {
        buf->data[buf->len] = c;
        buf->len += 1;
    } else {
        mx_std_buf_addchar_slow(buf, c);
    }
}
void mx_std_buf_addint(mx_std_buf_t *buf, int64_t v);
void mx_std_buf_addvalue(mx_std_buf_t *buf, mx_std_slot_t slot);
void mx_std_buf_push(mx_std_buf_t *buf);
void mx_std_buf_addfmt(mx_std_buf_t *buf, const char *spec, ...);

void mx_std_register(mx_std_state_t *ls, const mx_std_reg_t *fns);
void mx_std_newlib(mx_std_state_t *ls, const char *name, const mx_std_reg_t *fns);
void mx_std_setglobal(mx_std_state_t *ls, const char *name);
mx_lua_type_t mx_std_getglobal(mx_std_state_t *ls, const char *name);

void mx_std_open_base(mx_std_state_t *ls);
void mx_std_open_string(mx_std_state_t *ls);
void mx_std_open_utf8(mx_std_state_t *ls);
void mx_std_open_table(mx_std_state_t *ls);
void mx_std_open_math(mx_std_state_t *ls);
void mx_std_open_io(mx_std_state_t *ls);
void mx_std_open_os(mx_std_state_t *ls);
void mx_std_open_coroutine(mx_std_state_t *ls);
void mx_std_open_debug(mx_std_state_t *ls);
void mx_std_open_package(mx_std_state_t *ls);
void mx_std_open_libs(mx_std_state_t *ls);
void mx_std_open_extras(mx_std_state_t *ls);
void mx_std_open_vm(mx_std_state_t *ls);
void mx_std_setup(mx_t *vm, mx_obj_t globals);

void mx_std_cfn_init(mx_t *vm);
mx_obj_t mx_std_cfn_alloc(mx_t *vm, mx_ffi_t *fn, mx_std_fn_t *std_fn, const char *std_name, uint32_t nup, const mx_obj_t *ups);
mx_obj_t mx_std_cfn_new(mx_t *vm, mx_ffi_t *fn, uint32_t nup, const mx_obj_t *ups);
mx_obj_t mx_std_cfn_new_std(mx_t *vm, mx_std_fn_t *fn, const char *name, uint32_t nup, const mx_obj_t *ups);
void mx_ffi_std_foreign(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_std_udata_is(mx_t *vm, mx_obj_t v);
void *mx_std_udata_payload(mx_obj_t v);
uint64_t mx_std_udata_nuv(mx_obj_t v);
mx_obj_t mx_std_upval(mx_std_state_t *ls, uint32_t i);
void mx_std_setupval(mx_std_state_t *ls, uint32_t i, mx_obj_t v);
void mx_std_pushcfn_up(mx_std_state_t *ls, mx_std_fn_t *fn, const char *name, uint32_t nup);
void mx_std_cfn_call_op(mx_t *vm, size_t nargs, mx_obj_t *args);

mx_obj_t mx_std_char_str_new(mx_t *vm, char c);

static inline mx_obj_t mx_std_char_str(mx_t *vm, char c) {
    mx_obj_t s = vm->char_strs[(uint8_t)c];
    if (s.kind == MX_KIND_NIL) {
        return mx_std_char_str_new(vm, c);
    } else {
        return s;
    }
}

int32_t mx_std_base_assert(mx_std_state_t *ls);
int32_t mx_std_base_collectgarbage(mx_std_state_t *ls);
int32_t mx_std_base_dofile(mx_std_state_t *ls);
int32_t mx_std_base_error(mx_std_state_t *ls);
int32_t mx_std_base_getmetatable(mx_std_state_t *ls);
int32_t mx_std_base_ipairs(mx_std_state_t *ls);
int32_t mx_std_base_inext(mx_std_state_t *ls);
int32_t mx_std_base_load(mx_std_state_t *ls);
int32_t mx_std_base_loadfile(mx_std_state_t *ls);
int32_t mx_std_base_next(mx_std_state_t *ls);
int32_t mx_std_base_pairs(mx_std_state_t *ls);
int32_t mx_std_base_pcall_args(mx_std_state_t *ls);
int32_t mx_std_base_print(mx_std_state_t *ls);
int32_t mx_std_base_rawequal(mx_std_state_t *ls);
int32_t mx_std_base_rawget(mx_std_state_t *ls);
int32_t mx_std_base_rawlen(mx_std_state_t *ls);
int32_t mx_std_base_rawset(mx_std_state_t *ls);
int32_t mx_std_base_select(mx_std_state_t *ls);
int32_t mx_std_base_setmetatable(mx_std_state_t *ls);
int32_t mx_std_base_tonumber(mx_std_state_t *ls);
int32_t mx_std_base_tostring(mx_std_state_t *ls);
int32_t mx_std_base_type(mx_std_state_t *ls);
int32_t mx_std_base_warn(mx_std_state_t *ls);
int32_t mx_std_base_xpcall_args(mx_std_state_t *ls);

void mx_std_tostr_into(mx_t *vm, mx_str_t *buf, mx_obj_t value);
const char *mx_std_skip_space(const char *s);
int32_t mx_std_base_digit(char c);
mx_error_t *mx_std_base_error_of(mx_obj_t val, int64_t level);
void mx_std_unwrap_error(mx_std_state_t *ls, mx_std_slot_t slot);

int32_t mx_std_vm_setop(mx_std_state_t *ls);
int32_t mx_std_vm_typename(mx_std_state_t *ls);
int32_t mx_std_vm_stats(mx_std_state_t *ls);
int32_t mx_std_vm_gc(mx_std_state_t *ls);
int32_t mx_std_vm_version(mx_std_state_t *ls);

int64_t mx_std_str_relat(int64_t pos, size_t len);
int64_t mx_std_str_start(int64_t pos, size_t len);
int64_t mx_std_str_end(int64_t pos, size_t len);
bool mx_std_pat_has_special(const char *p, size_t plen);
const char *mx_std_pat_lmemfind(const char *s1, size_t l1, const char *s2, size_t l2);
void mx_std_pat_push_one(mx_std_state_t *ls, const char *s, const mx_pattern_match_t *m, size_t idx);
int32_t mx_std_pat_push_captures(mx_std_state_t *ls, const char *s, const mx_pattern_match_t *m, bool whole);
mx_pattern_result_t mx_std_pat_match(mx_std_state_t *ls, mx_pattern_captures_t *scratch, const char *s, size_t len, const char *p, size_t lp, size_t init);
void mx_std_gsub_append_rep(mx_std_state_t *ls, mx_std_buf_t *buf, const char *s, const mx_pattern_match_t *m);
bool mx_std_gsub_add_value(mx_std_state_t *ls, mx_std_buf_t *buf, const char *s, const mx_pattern_match_t *m, mx_lua_type_t tr);
void mx_std_str_addquoted(mx_std_buf_t *buf, const char *s, size_t len);
void mx_std_str_addliteral(mx_std_state_t *ls, mx_std_buf_t *buf, int32_t arg);
int32_t mx_std_str_find_aux(mx_std_state_t *ls, bool find);
int32_t mx_std_string_byte(mx_std_state_t *ls);
int32_t mx_std_string_char(mx_std_state_t *ls);
int32_t mx_std_string_pack(mx_std_state_t *ls);
int32_t mx_std_string_unpack(mx_std_state_t *ls);
int32_t mx_std_string_packsize(mx_std_state_t *ls);
int32_t mx_std_string_find(mx_std_state_t *ls);
int32_t mx_std_string_format(mx_std_state_t *ls);
mx_std_fmt_step_t mx_std_fmt_fail(mx_std_state_t *ls, int32_t nret);
const char *mx_std_fmt_digits(const char *s);
bool mx_std_fmt_valid(const char *spec, const char *flags, bool has_precision);
mx_std_fmt_step_t mx_std_fmt_badspec(mx_std_state_t *ls, const char *spec);
void mx_std_fmt_addlenmod(char *spec, const char *mod);
bool mx_std_fmt_int_g(mx_std_buf_t *buf, const char *spec, size_t speclen, double d);
mx_std_fmt_step_t mx_std_fmt_one(mx_std_state_t *ls, mx_std_buf_t *buf, const char *p, const char *end, int32_t arg);
int32_t mx_std_string_gmatch(mx_std_state_t *ls);
int32_t mx_std_string_gmatch_iter(mx_std_state_t *ls);
int32_t mx_std_string_gsub(mx_std_state_t *ls);
int32_t mx_std_string_len(mx_std_state_t *ls);
int32_t mx_std_string_lower(mx_std_state_t *ls);
int32_t mx_std_string_match(mx_std_state_t *ls);
int32_t mx_std_string_rep(mx_std_state_t *ls);
int32_t mx_std_string_reverse(mx_std_state_t *ls);
int32_t mx_std_string_sub(mx_std_state_t *ls);
int32_t mx_std_string_upper(mx_std_state_t *ls);
int32_t mx_std_string_dump(mx_std_state_t *ls);

bool mx_std_pack_digit(char c);
size_t mx_std_pack_getnum(mx_std_pack_t *h, size_t df);
int32_t mx_std_pack_getnumlimit(mx_std_pack_t *h, int32_t df);
mx_std_pack_res_t mx_std_pack_getoption(mx_std_pack_t *h);
mx_std_pack_det_t mx_std_pack_getdetails(mx_std_pack_t *h, size_t totalsize);
int32_t mx_std_pack_index(int32_t i, int32_t size, bool is_little);
void mx_std_pack_packint(mx_std_buf_t *b, uint64_t n, bool is_little, int32_t size, bool neg);
int64_t mx_std_pack_unpackint(mx_std_state_t *ls, const char *str, bool is_little, int32_t size, bool issigned);

int32_t mx_std_utf8_char(mx_std_state_t *ls);
int32_t mx_std_utf8_codes(mx_std_state_t *ls);
int32_t mx_std_utf8_codes_iter(mx_std_state_t *ls);
int32_t mx_std_utf8_codepoint(mx_std_state_t *ls);
int32_t mx_std_utf8_len(mx_std_state_t *ls);
int32_t mx_std_utf8_offset(mx_std_state_t *ls);

extern const char mx_std_utf8_charpattern[];
extern const uint32_t mx_std_utf8_limits[];

mx_std_utf8_decode_t mx_std_utf8_decode(const char *s, size_t avail, bool strict);
bool mx_std_utf8_iscont(uint8_t c);

int32_t mx_std_table_concat(mx_std_state_t *ls);
int32_t mx_std_table_create(mx_std_state_t *ls);
int32_t mx_std_table_insert(mx_std_state_t *ls);
int32_t mx_std_table_keys(mx_std_state_t *ls);
int32_t mx_std_table_values(mx_std_state_t *ls);
int32_t mx_std_table_move(mx_std_state_t *ls);
int32_t mx_std_table_pack(mx_std_state_t *ls);
int32_t mx_std_table_remove(mx_std_state_t *ls);
int32_t mx_std_table_sort(mx_std_state_t *ls);
int32_t mx_std_table_unpack(mx_std_state_t *ls);

bool mx_std_table_proxies(mx_std_state_t *ls, mx_std_slot_t arg, mx_std_table_access_t need);
void mx_std_table_check(mx_std_state_t *ls, mx_std_slot_t arg, mx_std_table_access_t need);
bool mx_std_sort_lt(mx_std_state_t *ls, mx_obj_t cmp, mx_obj_t a, mx_obj_t b);
bool mx_std_sort_lt_at(mx_std_state_t *ls, mx_obj_t cmp, uint32_t base, int64_t i, int64_t j);
void mx_std_sort_swap(mx_t *vm, uint32_t base, int64_t i, int64_t j);
void mx_std_sort_range(mx_std_state_t *ls, mx_obj_t cmp, uint32_t base, int64_t lo, int64_t hi);

int32_t mx_std_math_abs(mx_std_state_t *ls);
int32_t mx_std_math_acos(mx_std_state_t *ls);
int32_t mx_std_math_asin(mx_std_state_t *ls);
int32_t mx_std_math_atan(mx_std_state_t *ls);
int32_t mx_std_math_ceil(mx_std_state_t *ls);
int32_t mx_std_math_cos(mx_std_state_t *ls);
int32_t mx_std_math_deg(mx_std_state_t *ls);
int32_t mx_std_math_exp(mx_std_state_t *ls);
int32_t mx_std_math_floor(mx_std_state_t *ls);
int32_t mx_std_math_fmod(mx_std_state_t *ls);
int32_t mx_std_math_log(mx_std_state_t *ls);
int32_t mx_std_math_max(mx_std_state_t *ls);
int32_t mx_std_math_min(mx_std_state_t *ls);
int32_t mx_std_math_modf(mx_std_state_t *ls);
int32_t mx_std_math_frexp(mx_std_state_t *ls);
int32_t mx_std_math_ldexp(mx_std_state_t *ls);
int32_t mx_std_math_rad(mx_std_state_t *ls);
int32_t mx_std_math_random(mx_std_state_t *ls);
int32_t mx_std_math_randomseed(mx_std_state_t *ls);
int32_t mx_std_math_sin(mx_std_state_t *ls);
int32_t mx_std_math_sqrt(mx_std_state_t *ls);
int32_t mx_std_math_tan(mx_std_state_t *ls);
int32_t mx_std_math_tointeger(mx_std_state_t *ls);
int32_t mx_std_math_type(mx_std_state_t *ls);
int32_t mx_std_math_ult(mx_std_state_t *ls);

void mx_std_math_push_rounded(mx_std_state_t *ls, double d);
int32_t mx_std_math_unary(mx_std_state_t *ls, double (*fn)(double));
int32_t mx_std_math_pick(mx_std_state_t *ls, bool want_max);
uint64_t mx_std_rand_rotl(uint64_t x, uint32_t n);
uint64_t mx_std_rand_next(mx_t *vm);
void mx_std_rand_seed(mx_t *vm, uint64_t n1, uint64_t n2);
uint64_t mx_std_rand_entropy(mx_t *vm);
uint64_t mx_std_rand_upto(mx_t *vm, uint64_t span);

int32_t mx_std_io_close(mx_std_state_t *ls);
int32_t mx_std_io_flush(mx_std_state_t *ls);
int32_t mx_std_io_input(mx_std_state_t *ls);
int32_t mx_std_io_lines(mx_std_state_t *ls);
int32_t mx_std_io_open(mx_std_state_t *ls);
int32_t mx_std_io_output(mx_std_state_t *ls);
int32_t mx_std_io_popen(mx_std_state_t *ls);
int32_t mx_std_io_read(mx_std_state_t *ls);
int32_t mx_std_io_tmpfile(mx_std_state_t *ls);
int32_t mx_std_io_type(mx_std_state_t *ls);
int32_t mx_std_io_write(mx_std_state_t *ls);
int32_t mx_std_io_lines_iter(mx_std_state_t *ls);

mx_obj_table_t *mx_std_io_table_of(mx_std_state_t *ls, mx_std_slot_t slot);
mx_obj_table_t *mx_std_io_check_file(mx_std_state_t *ls, mx_std_slot_t slot);
bool mx_std_io_isopen_table(const mx_obj_table_t *t);
bool mx_std_io_ispipe_table(const mx_obj_table_t *t);
FILE *mx_std_io_file_of_table(const mx_obj_table_t *t);
FILE *mx_std_io_tofile(mx_std_state_t *ls, mx_std_slot_t slot);
void mx_std_io_new_handle(mx_std_state_t *ls, FILE *fp, bool is_pipe);
bool mx_std_io_checkmode(const char *mode);
int32_t mx_std_io_fileresult(mx_std_state_t *ls, bool ok, const char *fname);
int32_t mx_std_io_close_table(mx_std_state_t *ls, mx_obj_table_t *t);
bool mx_std_io_num_take(mx_std_io_num_t *r, bool ok);
bool mx_std_io_read_number(mx_std_state_t *ls, FILE *fp);
bool mx_std_io_read_line(mx_std_state_t *ls, FILE *fp, bool chop);
void mx_std_io_read_all(mx_std_state_t *ls, FILE *fp);
bool mx_std_io_read_chars(mx_std_state_t *ls, FILE *fp, size_t n);
bool mx_std_io_test_eof(mx_std_state_t *ls, FILE *fp);
int32_t mx_std_io_read_aux(mx_std_state_t *ls, FILE *fp, int32_t first);
int32_t mx_std_io_write_aux(mx_std_state_t *ls, FILE *fp, int32_t arg);
int32_t mx_std_io_iofile(mx_std_state_t *ls, mx_obj_t *slot, const char *mode);
void mx_std_io_lines_aux(mx_std_state_t *ls, bool toclose);

int32_t mx_std_file_close(mx_std_state_t *ls);
int32_t mx_std_file_gc(mx_std_state_t *ls);
int32_t mx_std_file_flush(mx_std_state_t *ls);
int32_t mx_std_file_lines(mx_std_state_t *ls);
int32_t mx_std_file_read(mx_std_state_t *ls);
int32_t mx_std_file_seek(mx_std_state_t *ls);
int32_t mx_std_file_setvbuf(mx_std_state_t *ls);
int32_t mx_std_file_write(mx_std_state_t *ls);
int32_t mx_std_file_tostring(mx_std_state_t *ls);

int32_t mx_std_os_clock(mx_std_state_t *ls);
int32_t mx_std_os_date(mx_std_state_t *ls);
int32_t mx_std_os_difftime(mx_std_state_t *ls);
int32_t mx_std_os_execute(mx_std_state_t *ls);
int32_t mx_std_os_exit(mx_std_state_t *ls);
int32_t mx_std_os_getenv(mx_std_state_t *ls);
int32_t mx_std_os_setlocale(mx_std_state_t *ls);
int32_t mx_std_os_remove(mx_std_state_t *ls);
int32_t mx_std_os_rename(mx_std_state_t *ls);
int32_t mx_std_os_time(mx_std_state_t *ls);
int32_t mx_std_os_tmpname(mx_std_state_t *ls);

int32_t mx_std_os_getfield(mx_std_state_t *ls, const char *key, int32_t d, int32_t delta);
void mx_std_os_setfield(mx_std_state_t *ls, const char *key, int32_t value, int32_t delta);
void mx_std_os_setboolfield(mx_std_state_t *ls, const char *key, int32_t value);
void mx_std_os_setallfields(mx_std_state_t *ls, const struct tm *stm);
const char *mx_std_os_checkoption(mx_std_state_t *ls, const char *conv, ptrdiff_t convlen, char *buff);
int32_t mx_std_os_execresult(mx_std_state_t *ls, int32_t stat);

int32_t mx_std_coroutine_wrap(mx_std_state_t *ls);
int32_t mx_std_coroutine_wrap_call(mx_std_state_t *ls);
int32_t mx_std_coroutine_resume_raw(mx_std_state_t *ls);

mx_std_text_t mx_std_read_stream(FILE *file);
mx_std_text_t mx_std_read_file(const char *filename);
mx_obj_t mx_std_load_closure(mx_t *vm, mx_ir_block_t *entry, mx_obj_t env);
int32_t mx_std_load_chunk(mx_std_state_t *ls, const char *src, size_t len, const char *name, const char *mode, mx_obj_t env);
int32_t mx_std_load_file(mx_std_state_t *ls, const char *fname, const char *mode, mx_obj_t env);
bool mx_std_package_readable(const char *filename);
int32_t mx_std_package_searchpath_aux(mx_std_state_t *ls, const char *name, const char *path, const char *sep, const char *dirsep);
int32_t mx_std_package_searchpath(mx_std_state_t *ls);
mx_std_libstat_t mx_std_package_lookforfunc(mx_std_state_t *ls, const char *path, const char *sym);
mx_std_libstat_t mx_std_package_loadfunc(mx_std_state_t *ls, const char *filename, const char *modname);
int32_t mx_std_package_loadlib(mx_std_state_t *ls);
const char *mx_std_package_path(mx_std_state_t *ls, const char *field);
void mx_std_package_setpath(mx_std_state_t *ls, const char *field, const char *env, const char *dft);
int32_t mx_std_package_searcher_preload(mx_std_state_t *ls);
int32_t mx_std_package_searcher_lua(mx_std_state_t *ls);
int32_t mx_std_package_searcher_c(mx_std_state_t *ls);
int32_t mx_std_package_searcher_croot(mx_std_state_t *ls);
int32_t mx_std_package_require(mx_std_state_t *ls);

mx_std_debug_target_t mx_std_debug_target(mx_std_state_t *ls);
mx_frame_t mx_std_debug_frame(mx_thread_t *th, int64_t level);
bool mx_std_debug_validopt(const char *what);
void mx_std_debug_info_source(mx_std_state_t *ls, const mx_ir_block_t *head);
void mx_std_debug_info_c(mx_std_state_t *ls);
void mx_std_debug_activelines(mx_std_state_t *ls, mx_ir_block_t *head);
uint32_t mx_std_debug_nups(mx_t *vm, mx_obj_t fn);
int32_t mx_std_debug_getinfo(mx_std_state_t *ls);
const mx_ir_local_t *mx_std_debug_param(const mx_ir_func_t *func, int64_t n);
mx_obj_table_t *mx_std_debug_varargs(mx_t *vm, const mx_frame_t *f, int64_t i);
bool mx_std_debug_is_self(mx_std_state_t *ls, const mx_frame_t *f);
int32_t mx_std_debug_getlocal(mx_std_state_t *ls);
int32_t mx_std_debug_setlocal(mx_std_state_t *ls);
const char *mx_std_debug_upname(mx_t *vm, mx_obj_t fn, int64_t n);
int32_t mx_std_debug_getupvalue(mx_std_state_t *ls);
int32_t mx_std_debug_setupvalue(mx_std_state_t *ls);
void *mx_std_debug_upid(mx_std_state_t *ls, mx_std_slot_t fslot, mx_std_slot_t nslot);
int32_t mx_std_debug_upvalueid(mx_std_state_t *ls);
int32_t mx_std_debug_upvaluejoin(mx_std_state_t *ls);
int32_t mx_std_debug_traceback(mx_std_state_t *ls);
int32_t mx_std_debug_debug(mx_std_state_t *ls);
mx_obj_t mx_std_debug_hook_fire(mx_t *vm, const char *event, int64_t line);
void mx_std_debug_hook_returned(mx_lua_hook_t *hook, mx_thread_t *th);
mx_obj_t mx_std_debug_hook_step(mx_t *vm, mx_thread_t *th, const mx_resume_t *at);
mx_obj_t mx_std_debug_hook(mx_t *vm, mx_thread_t *th, const mx_resume_t *at, uint32_t event);
uint32_t mx_std_debug_flag(char c);
int32_t mx_std_debug_sethook(mx_std_state_t *ls);
int32_t mx_std_debug_gethook(mx_std_state_t *ls);
int32_t mx_std_debug_getuservalue(mx_std_state_t *ls);
int32_t mx_std_debug_setuservalue(mx_std_state_t *ls);
int32_t mx_std_debug_getmetatable(mx_std_state_t *ls);
int32_t mx_std_debug_setmetatable(mx_std_state_t *ls);
int32_t mx_std_debug_getregistry(mx_std_state_t *ls);

#endif
