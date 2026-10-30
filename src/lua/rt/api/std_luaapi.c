
#include "std_luaapi.h"
#include "../coro.h"
#include "../lua.h"
#include "../meta.h"
#include "../prim.h"
#include "../trace.h"
#include "../../front/dump.h"
#include "../../../vm/gc.h"
#include "../../../value/error.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"
#include "../../../value/table.h"

_Static_assert(offsetof(mx_luaapi_frame_t, ls) == LUA_EXTRASPACE, "lua_getextraspace layout");
_Static_assert(MX_STD_REGISTRYINDEX == LUA_REGISTRYINDEX, "registry index");

int32_t mx_luaapi_type_of(mx_lua_type_t t) {
    switch (t) {
        case MX_LUA_TYPE_NIL: {
            return LUA_TNIL;
        }
        case MX_LUA_TYPE_BOOLEAN: {
            return LUA_TBOOLEAN;
        }
        case MX_LUA_TYPE_INTEGER:
        case MX_LUA_TYPE_FLOAT: {
            return LUA_TNUMBER;
        }
        case MX_LUA_TYPE_STRING: {
            return LUA_TSTRING;
        }
        case MX_LUA_TYPE_TABLE: {
            return LUA_TTABLE;
        }
        case MX_LUA_TYPE_FUNCTION: {
            return LUA_TFUNCTION;
        }
        case MX_LUA_TYPE_THREAD: {
            return LUA_TTHREAD;
        }
        case MX_LUA_TYPE_USERDATA: {
            return LUA_TUSERDATA;
        }
        default: {
            return LUA_TNONE;
        }
    }
}

mx_lua_type_t mx_luaapi_type_to(int32_t t) {
    switch (t) {
        case LUA_TNONE: {
            return MX_LUA_TYPE_NONE;
        }
        case LUA_TNIL: {
            return MX_LUA_TYPE_NIL;
        }
        case LUA_TBOOLEAN: {
            return MX_LUA_TYPE_BOOLEAN;
        }
        case LUA_TNUMBER: {
            return MX_LUA_TYPE_INTEGER;
        }
        case LUA_TSTRING: {
            return MX_LUA_TYPE_STRING;
        }
        case LUA_TTABLE: {
            return MX_LUA_TYPE_TABLE;
        }
        case LUA_TFUNCTION: {
            return MX_LUA_TYPE_FUNCTION;
        }
        case LUA_TTHREAD: {
            return MX_LUA_TYPE_THREAD;
        }
        default: {
            return MX_LUA_TYPE_USERDATA;
        }
    }
}

int32_t mx_luaapi_bool(bool b) {
    if (b) {
        return 1;
    } else {
        return 0;
    }
}

void mx_luaapi_check(mx_std_state_t *ls) {
    mx_lua_foreign_t *foreign = &mx_lua_of(ls->vm)->foreign;
    if (ls->vm->std_error.is_set && foreign->ls == ls) {
        longjmp(*foreign->jmp, 1);
    }
}

int32_t mx_luaapi_protect(mx_lua_t *lua, mx_std_fn_t *fn, mx_std_state_t *ls) {
    jmp_buf here;
    mx_lua_foreign_t outer = lua->foreign;
    lua->foreign.jmp = &here;
    lua->foreign.ls = ls;
    volatile int32_t nret = 0;
    if (setjmp(here) == 0) {
        nret = fn(ls);
    }
    lua->foreign.jmp = outer.jmp;
    lua->foreign.ls = outer.ls;
    return nret;
}

void mx_luaapi_call(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_lua_t *lua = mx_lua_of(vm);
    mx_thread_t *th = vm->thread;
    uint32_t at = th->frame;
    mx_obj_cfn_t *cfn = (mx_obj_cfn_t *)mx_ref_obj(mx_thread_objs(th, at + 2)->val.ref);
    mx_luaapi_frame_t frame = {
        .extra = lua->foreign.extra,
        .ls = mx_std_state_enter(vm, args, (int32_t)nargs, "?", cfn->std_fn, vm->pending.is_ok),
    };
    vm->pending.is_ok = false;
    int32_t nret = mx_luaapi_protect(lua, cfn->std_fn, &frame.ls);
    lua->foreign.extra = frame.extra;
    if (vm->tbcs.len != 0 && !vm->pending.is_set && !vm->std_error.is_set) {
        mx_obj_t err = mx_prim_tbc_unwind(vm, th, at, mx_obj_of_nil());
        if (mx_obj_is_error(err)) {
            vm->std_error = (mx_std_error_t) { .value = err, .is_set = true };
        }
    }
    mx_std_dispatch_finish(&frame.ls, nret);
}

void mx_luaapi_push_cfn(mx_std_state_t *ls, mx_std_fn_t *fn, uint32_t nup) {
    if (nup > MX_STD_CFN_MAX_UPVALS) {
        mx_std_error(ls, "too many upvalues (limit is %d)", MX_STD_CFN_MAX_UPVALS);
        mx_luaapi_check(ls);
        return;
    }
    mx_obj_t obj = mx_std_cfn_alloc(ls->vm, mx_luaapi_call, fn, NULL, nup, mx_std_slots(ls) + ls->top - (int32_t)nup);
    ls->top -= (int32_t)nup;
    mx_std_push(ls, obj);
}

int32_t mx_luaapi_open_loaded(mx_std_state_t *ls, const char *name) {
    mx_std_getfield(ls, MX_STD_REGISTRYINDEX, LUA_LOADED_TABLE);
    mx_std_getfield(ls, -1, name);
    mx_std_remove(ls, -2);
    return 1;
}

int32_t mx_luaapi_load_status(mx_std_state_t *ls, int32_t n, int32_t failed) {
    if (n == 1) {
        return LUA_OK;
    } else {
        mx_std_remove(ls, -2);
        return failed;
    }
}

lua_State *lua_newstate(lua_Alloc f, void *ud, unsigned seed) {
    return NULL;
}

void lua_close(lua_State *L) {
}

mx_std_state_t *mx_luaapi_thread_state(mx_t *vm, mx_thread_t *co) {
    mx_luaapi_frame_t *frame;
    if (co->api == NULL) {
        frame = mx_mem_calloc(sizeof(mx_luaapi_frame_t));
        frame->extra = mx_lua_of(vm)->foreign.extra;
        co->api = frame;
    } else {
        frame = co->api;
    }
    mx_std_state_t *ls = &frame->ls;
    uint32_t base;
    if (co->frame == MX_THREAD_NONE) {
        base = 0;
    } else {
        base = co->top;
    }
    bool is_valid = ls->th == co && ls->base + 2 * (uint32_t)ls->top == co->top && (co->frame != MX_THREAD_NONE || ls->base == 0);
    if (!is_valid) {
        *ls = (mx_std_state_t) {
            .vm = vm,
            .th = co,
            .base = base,
            .top = 0,
            .error = mx_obj_of_nil(),
            .fnname = "?",
        };
    }
    return ls;
}

lua_State *lua_newthread(lua_State *L) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_thread_t *co = mx_thread_new(ls->vm, mx_obj_of_nil());
    mx_std_push(ls, mx_obj_of_thread(co));
    mx_luaapi_check(ls);
    return (lua_State *)mx_luaapi_thread_state(ls->vm, co);
}

int lua_closethread(lua_State *L, lua_State *from) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_thread_t *co = ls->th;
    if (co == ls->vm->thread || co->status == MX_THREAD_NORMAL) {
        mx_std_error(ls, "cannot close a running coroutine");
        mx_luaapi_check(ls);
        return LUA_ERRRUN;
    }
    mx_obj_t err = mx_coro_kill(ls->vm, co);
    mx_std_state_t *fresh = mx_luaapi_thread_state(ls->vm, co);
    if (mx_obj_is_error(err)) {
        mx_std_push(fresh, mx_error_payload(err));
        return LUA_ERRRUN;
    } else {
        return LUA_OK;
    }
}

lua_CFunction lua_atpanic(lua_State *L, lua_CFunction panicf) {
    return NULL;
}

lua_Number lua_version(lua_State *L) {
    return LUA_VERSION_NUM;
}

int lua_absindex(lua_State *L, int idx) {
    return mx_std_absindex((mx_std_state_t *)L, idx);
}

int lua_gettop(lua_State *L) {
    return mx_std_top((mx_std_state_t *)L);
}

void lua_settop(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_settop(ls, idx);
    mx_luaapi_check(ls);
}

void lua_pushvalue(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_pushvalue(ls, idx);
    mx_luaapi_check(ls);
}

void lua_rotate(lua_State *L, int idx, int n) {
    mx_std_rotate((mx_std_state_t *)L, idx, n);
}

void lua_copy(lua_State *L, int fromidx, int toidx) {
    mx_std_copy((mx_std_state_t *)L, fromidx, toidx);
}

int lua_checkstack(lua_State *L, int n) {
    return mx_luaapi_bool(mx_std_checkstack((mx_std_state_t *)L, n));
}

void lua_xmove(lua_State *from, lua_State *to, int n) {
    mx_std_state_t *src = (mx_std_state_t *)from;
    mx_std_state_t *dst = (mx_std_state_t *)to;
    if (src == dst || n <= 0) {
        return;
    }
    mx_obj_t *tmp = mx_mem_alloc(sizeof(mx_obj_t) * (size_t)n);
    memcpy(tmp, mx_std_slots(src) + src->top - n, sizeof(mx_obj_t) * (size_t)n);
    mx_std_pop(src, n);
    for (int32_t i = 0; i < n; i++) {
        mx_std_push(dst, tmp[i]);
    }
    mx_mem_free(tmp);
    mx_luaapi_check(dst);
}

int lua_isnumber(lua_State *L, int idx) {
    return mx_luaapi_bool(mx_std_isnumber((mx_std_state_t *)L, idx));
}

int lua_isstring(lua_State *L, int idx) {
    return mx_luaapi_bool(mx_std_isstring((mx_std_state_t *)L, idx));
}

int lua_iscfunction(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    return mx_luaapi_bool(at != NULL && (at->kind == MX_KIND_FFI || at->kind == ls->vm->kinds.cfn));
}

int lua_isinteger(lua_State *L, int idx) {
    return mx_luaapi_bool(mx_std_isinteger((mx_std_state_t *)L, idx));
}

int lua_isuserdata(lua_State *L, int idx) {
    int32_t t = lua_type(L, idx);
    return mx_luaapi_bool(t == LUA_TUSERDATA || t == LUA_TLIGHTUSERDATA);
}

int lua_type(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    if (at == NULL) {
        return LUA_TNONE;
    } else if (mx_obj_is_lightuserdata(*at)) {
        return LUA_TLIGHTUSERDATA;
    } else {
        return mx_luaapi_type_of(mx_std_type(ls, idx));
    }
}

const char *lua_typename(lua_State *L, int tp) {
    if (tp == LUA_TLIGHTUSERDATA) {
        return "userdata";
    } else {
        return mx_std_typename((mx_std_state_t *)L, mx_luaapi_type_to(tp));
    }
}

lua_Number lua_tonumberx(lua_State *L, int idx, int *isnum) {
    mx_obj_float_t r = mx_std_tofloat((mx_std_state_t *)L, idx);
    if (isnum != NULL) {
        *isnum = mx_luaapi_bool(r.is_num);
    }
    return r.value;
}

lua_Integer lua_tointegerx(lua_State *L, int idx, int *isnum) {
    mx_obj_int_t r = mx_std_tointeger((mx_std_state_t *)L, idx);
    if (isnum != NULL) {
        *isnum = mx_luaapi_bool(r.is_int);
    }
    return r.value;
}

int lua_toboolean(lua_State *L, int idx) {
    return mx_luaapi_bool(mx_std_toboolean((mx_std_state_t *)L, idx));
}

const char *lua_tolstring(lua_State *L, int idx, size_t *len) {
    mx_std_str_t s = mx_std_tostring((mx_std_state_t *)L, idx);
    if (len != NULL) {
        *len = s.len;
    }
    return s.ptr;
}

lua_Unsigned lua_rawlen(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    if (at != NULL && mx_std_udata_is(ls->vm, *at)) {
        return ((const mx_std_udata_header_t *)(uintptr_t)mx_obj_get_table(*at)->hidden)->size;
    } else {
        return mx_std_rawlen(ls, idx);
    }
}

lua_CFunction lua_tocfunction(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    if (at != NULL && at->kind == ls->vm->kinds.cfn) {
        return (lua_CFunction)((mx_obj_cfn_t *)mx_obj_data_head(*at))->std_fn;
    } else {
        return NULL;
    }
}

bool mx_std_udata_is(mx_t *vm, mx_obj_t v) {
    return mx_obj_kind(v) == vm->kinds.udata && mx_obj_is_table(v);
}

void *mx_std_udata_payload(mx_obj_t v) {
    mx_obj_table_t *t = mx_obj_get_table(v);
    if (t->hidden == 0) {
        return NULL;
    } else {
        return (uint8_t *)(uintptr_t)t->hidden + sizeof(mx_std_udata_header_t);
    }
}

uint64_t mx_std_udata_nuv(mx_obj_t v) {
    mx_obj_table_t *t = mx_obj_get_table(v);
    if (t->hidden == 0) {
        return 0;
    } else {
        return ((const mx_std_udata_header_t *)(uintptr_t)t->hidden)->nuv;
    }
}

void *lua_touserdata(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    if (at == NULL) {
        return NULL;
    } else if (mx_obj_is_lightuserdata(*at)) {
        return mx_obj_get_lightuserdata(*at);
    } else if (mx_std_udata_is(ls->vm, *at)) {
        return mx_std_udata_payload(*at);
    } else {
        return NULL;
    }
}

lua_State *lua_tothread(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    if (at == NULL || at->kind != ls->vm->kinds.thread) {
        return NULL;
    } else if (mx_obj_get_thread(*at) == ls->th) {
        return L;
    } else {
        return (lua_State *)mx_luaapi_thread_state(ls->vm, mx_obj_get_thread(*at));
    }
}

const void *lua_topointer(lua_State *L, int idx) {
    mx_obj_t *at = mx_std_at((mx_std_state_t *)L, idx);
    if (at == NULL) {
        return NULL;
    } else if (mx_obj_is_lightuserdata(*at)) {
        return mx_obj_get_lightuserdata(*at);
    } else if (mx_obj_kind(*at) < MX_KIND_FIRST_DATA && !mx_obj_is_string(*at) && !mx_obj_is_closure(*at) && !mx_obj_is_ffi(*at)) {
        return NULL;
    } else {
        return mx_ref_obj(at->val.ref);
    }
}

void lua_arith(lua_State *L, int op) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    const mx_op_t ops[] = {
        MX_OP_ADD, MX_OP_SUB, MX_OP_MUL, MX_OP_MOD, MX_OP_POW, MX_OP_DIV, MX_OP_IDIV,
        MX_OP_BAND, MX_OP_BOR, MX_OP_BXOR, MX_OP_SHL, MX_OP_SHR, MX_OP_UNM, MX_OP_BNOT,
    };
    mx_obj_t *slots = mx_std_slots(ls);
    mx_obj_t pair[2];
    if (op == LUA_OPUNM || op == LUA_OPBNOT) {
        pair[0] = slots[ls->top - 1];
        pair[1] = pair[0];
        mx_std_pop(ls, 1);
    } else {
        pair[0] = slots[ls->top - 2];
        pair[1] = slots[ls->top - 1];
        mx_std_pop(ls, 2);
    }
    mx_obj_t got = mx_call(ls->vm, ls->vm->kinds.ptr[pair[0].kind].ops[ops[op]], 2, pair);
    if (mx_obj_is_error(got)) {
        mx_error_locate(ls->vm, mx_obj_get_error(got), NULL);
        ls->error = got;
        mx_std_raise(ls);
        mx_std_pushnil(ls);
    } else {
        mx_std_push(ls, got);
    }
    mx_luaapi_check(ls);
}

int lua_rawequal(lua_State *L, int idx1, int idx2) {
    return mx_luaapi_bool(mx_std_rawequal((mx_std_state_t *)L, idx1, idx2));
}

int lua_compare(lua_State *L, int idx1, int idx2, int op) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    const mx_op_t ops[] = {MX_OP_EQ, MX_OP_LT, MX_OP_LE};
    bool got = mx_std_compare(ls, idx1, idx2, ops[op]);
    mx_luaapi_check(ls);
    return mx_luaapi_bool(got);
}

void lua_pushnil(lua_State *L) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_pushnil(ls);
    mx_luaapi_check(ls);
}

void lua_pushnumber(lua_State *L, lua_Number n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_pushfloat(ls, n);
    mx_luaapi_check(ls);
}

void lua_pushinteger(lua_State *L, lua_Integer n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_pushinteger(ls, n);
    mx_luaapi_check(ls);
}

const char *lua_pushlstring(lua_State *L, const char *s, size_t len) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_pushlstring(ls, s, len);
    mx_luaapi_check(ls);
    return mx_std_tostring(ls, -1).ptr;
}

const char *lua_pushexternalstring(lua_State *L, const char *s, size_t len, lua_Alloc falloc, void *ud) {
    const char *got = lua_pushlstring(L, s, len);
    if (falloc != NULL) {
        falloc(ud, (void *)s, len + 1, 0);
    }
    return got;
}

const char *lua_pushstring(lua_State *L, const char *s) {
    if (s == NULL) {
        lua_pushnil(L);
        return NULL;
    } else {
        return lua_pushlstring(L, s, strlen(s));
    }
}

void mx_luaapi_add_utf8(mx_str_t *out, unsigned long x) {
    char buf[8];
    int32_t n = 1;
    if (x < 0x80) {
        buf[7] = (char)x;
    } else {
        unsigned long mfb = 0x3f;
        do {
            buf[8 - n] = (char)(0x80 | (x & 0x3f));
            n += 1;
            x >>= 6;
            mfb >>= 1;
        } while (x > mfb);
        buf[8 - n] = (char)((~mfb << 1) | x);
    }
    mx_str_append(out, &buf[8 - n], (uint32_t)n);
}

const char *lua_pushvfstring(lua_State *L, const char *fmt, va_list argp) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_str_t *out = mx_str_new();
    const char *e = strchr(fmt, '%');
    while (e != NULL) {
        mx_str_append(out, fmt, (uint32_t)(e - fmt));
        switch (e[1]) {
            case 's': {
                const char *arg = va_arg(argp, char *);
                const char *s;
                if (arg == NULL) {
                    s = "(null)";
                } else {
                    s = arg;
                }
                mx_str_append(out, s, (uint32_t)strlen(s));
                break;
            }
            case 'c': {
                mx_str_append_byte(out, (char)va_arg(argp, int));
                break;
            }
            case 'd': {
                mx_str_append_int(out, va_arg(argp, int));
                break;
            }
            case 'I': {
                mx_str_append_int(out, (int64_t)va_arg(argp, LUAI_UACINT));
                break;
            }
            case 'f': {
                mx_str_float(out, (double)va_arg(argp, LUAI_UACNUMBER));
                break;
            }
            case 'p': {
                mx_str_format(out, "%p", va_arg(argp, void *));
                break;
            }
            case 'U': {
                mx_luaapi_add_utf8(out, (unsigned long)va_arg(argp, long));
                break;
            }
            case '%': {
                mx_str_append_byte(out, '%');
                break;
            }
            default: {
                char bad = e[1];
                mx_str_free(out);
                mx_std_error(ls, "invalid conversion '%%%c' to 'lua_pushfstring'", bad);
                mx_luaapi_check(ls);
                return NULL;
            }
        }
        fmt = e + 2;
        e = strchr(fmt, '%');
    }
    mx_str_append(out, fmt, (uint32_t)strlen(fmt));
    const char *got = lua_pushlstring(L, out->buf, out->len);
    mx_str_free(out);
    return got;
}

const char *lua_pushfstring(lua_State *L, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const char *got = lua_pushvfstring(L, fmt, ap);
    va_end(ap);
    return got;
}

void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n) {
    mx_luaapi_push_cfn((mx_std_state_t *)L, (mx_std_fn_t *)fn, (uint32_t)n);
}

void lua_pushboolean(lua_State *L, int b) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_pushboolean(ls, b != 0);
    mx_luaapi_check(ls);
}

void lua_pushlightuserdata(lua_State *L, void *p) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_push(ls, mx_obj_of_lightuserdata(p));
    mx_luaapi_check(ls);
}

int lua_pushthread(lua_State *L) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_push(ls, mx_obj_of_thread(ls->th));
    mx_luaapi_check(ls);
    return mx_luaapi_bool(ls->th == ls->vm->main);
}

int lua_getglobal(lua_State *L, const char *name) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_lua_type_t t = mx_std_getglobal(ls, name);
    mx_luaapi_check(ls);
    return mx_luaapi_type_of(t);
}

int lua_gettable(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_lua_type_t t = mx_std_gettable(ls, idx);
    mx_luaapi_check(ls);
    return mx_luaapi_type_of(t);
}

int lua_getfield(lua_State *L, int idx, const char *k) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_lua_type_t t = mx_std_getfield(ls, idx, k);
    mx_luaapi_check(ls);
    return mx_luaapi_type_of(t);
}

int lua_geti(lua_State *L, int idx, lua_Integer n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_lua_type_t t = mx_std_geti(ls, idx, n);
    mx_luaapi_check(ls);
    return mx_luaapi_type_of(t);
}

int lua_rawget(lua_State *L, int idx) {
    return mx_luaapi_type_of(mx_std_rawget((mx_std_state_t *)L, idx));
}

int lua_rawgeti(lua_State *L, int idx, lua_Integer n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_lua_type_t t = mx_std_rawgeti(ls, idx, n);
    mx_luaapi_check(ls);
    return mx_luaapi_type_of(t);
}

int lua_rawgetp(lua_State *L, int idx, const void *p) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_slot_t abs = mx_std_absindex(ls, idx);
    mx_std_push(ls, mx_obj_of_lightuserdata((void *)p));
    mx_luaapi_check(ls);
    return mx_luaapi_type_of(mx_std_rawget(ls, abs));
}

void lua_createtable(lua_State *L, int narr, int nrec) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    uint32_t na;
    if (narr > 0) {
        na = (uint32_t)narr;
    } else {
        na = 0;
    }
    uint32_t nh;
    if (nrec > 0) {
        nh = (uint32_t)nrec;
    } else {
        nh = 0;
    }
    mx_std_createtable(ls, na, nh);
    mx_luaapi_check(ls);
}

void *lua_newuserdatauv(lua_State *L, size_t sz, int nuvalue) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_t *vm = ls->vm;
    mx_obj_table_t *t = mx_table_new(vm);
    t->head.kind = vm->kinds.udata;
    t->is_plain = false;
    uint64_t nuv;
    if (nuvalue > 0) {
        nuv = (uint64_t)nuvalue;
    } else {
        nuv = 0;
    }
    mx_std_udata_header_t *header = mx_mem_alloc(sizeof(mx_std_udata_header_t) + sz);
    *header = (mx_std_udata_header_t) { .size = sz, .nuv = nuv };
    uint8_t *payload = (uint8_t *)(header + 1);
    memset(payload, 0, sz);
    t->hidden = (uint64_t)(uintptr_t)header;
    mx_gc_note_final(vm, &t->head);
    mx_std_push(ls, mx_obj_of_table(t));
    mx_luaapi_check(ls);
    return payload;
}

int lua_getmetatable(lua_State *L, int objindex) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    bool got = mx_std_getmetatable(ls, objindex);
    mx_luaapi_check(ls);
    return mx_luaapi_bool(got);
}

int lua_getiuservalue(lua_State *L, int idx, int n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    if (at == NULL || !mx_std_udata_is(ls->vm, *at) || n < 1 || (uint64_t)n > mx_std_udata_nuv(*at)) {
        lua_pushnil(L);
        return LUA_TNONE;
    } else {
        mx_std_push(ls, mx_table_get(mx_obj_get_table(*at), mx_obj_of_int(n)));
        mx_luaapi_check(ls);
        return lua_type(L, -1);
    }
}

void lua_setglobal(lua_State *L, const char *name) {
    mx_std_setglobal((mx_std_state_t *)L, name);
}

void lua_settable(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_settable(ls, idx);
    mx_luaapi_check(ls);
}

void lua_setfield(lua_State *L, int idx, const char *k) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_setfield(ls, idx, k);
    mx_luaapi_check(ls);
}

void lua_seti(lua_State *L, int idx, lua_Integer n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_seti(ls, idx, n);
    mx_luaapi_check(ls);
}

void lua_rawset(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_rawset(ls, idx);
    mx_luaapi_check(ls);
}

void lua_rawseti(lua_State *L, int idx, lua_Integer n) {
    mx_std_rawseti((mx_std_state_t *)L, idx, n);
}

void lua_rawsetp(lua_State *L, int idx, const void *p) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_slot_t abs = mx_std_absindex(ls, idx);
    mx_std_push(ls, mx_obj_of_lightuserdata((void *)p));
    mx_std_insert(ls, -2);
    mx_std_rawset(ls, abs);
    mx_luaapi_check(ls);
}

int lua_setmetatable(lua_State *L, int objindex) {
    mx_std_setmetatable((mx_std_state_t *)L, objindex);
    return 1;
}

int lua_setiuservalue(lua_State *L, int idx, int n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    if (at == NULL || !mx_std_udata_is(ls->vm, *at) || n < 1 || (uint64_t)n > mx_std_udata_nuv(*at)) {
        mx_std_pop(ls, 1);
        return 0;
    } else {
        mx_obj_table_t *t = mx_obj_get_table(*at);
        mx_obj_t v = *mx_std_at(ls, -1);
        mx_gc_write_barrier(ls->vm, t, v);
        mx_table_set(ls->vm, t, mx_obj_of_int(n), v);
        mx_std_pop(ls, 1);
        return 1;
    }
}

void lua_callk(lua_State *L, int nargs, int nresults, lua_KContext ctx, lua_KFunction k) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_call(ls, nargs, nresults);
    mx_luaapi_check(ls);
}

int lua_pcallk(lua_State *L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_status_t st = mx_std_pcall(ls, nargs, nresults, errfunc);
    mx_luaapi_check(ls);
    if (st == MX_STD_OK) {
        return LUA_OK;
    } else {
        return LUA_ERRRUN;
    }
}

int lua_load(lua_State *L, lua_Reader reader, void *dt, const char *chunkname, const char *mode) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_str_t *buf = mx_str_new();
    bool is_more = true;
    while (is_more) {
        size_t size = 0;
        const char *piece = reader(L, dt, &size);
        is_more = piece != NULL && size > 0;
        if (is_more) {
            mx_str_append(buf, piece, (uint32_t)size);
        }
    }
    const char *name;
    if (chunkname == NULL) {
        name = "?";
    } else {
        name = chunkname;
    }
    const char *how;
    if (mode == NULL) {
        how = "bt";
    } else {
        how = mode;
    }
    int32_t n = mx_std_load_chunk(ls, buf->buf, buf->len, name, how, mx_lua_of(ls->vm)->std);
    mx_str_free(buf);
    return mx_luaapi_load_status(ls, n, LUA_ERRSYNTAX);
}

int lua_dump(lua_State *L, lua_Writer writer, void *data, int strip) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, -1);
    if (at == NULL || !mx_obj_is_closure(*at)) {
        return 1;
    }
    mx_str_t *out = mx_dump_function(ls->vm, mx_obj_get_closure(*at)->block, strip != 0);
    if (out == NULL) {
        return 1;
    }
    int32_t st = writer(L, out->buf, out->len, data);
    mx_str_free(out);
    return st;
}

int lua_yieldk(lua_State *L, int nresults, lua_KContext ctx, lua_KFunction k) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_t *vm = ls->vm;
    if (k != NULL) {
        mx_std_error_noloc(ls, "C continuations (lua_yieldk) are not supported by MiniVM");
    } else if (ls->th == vm->main) {
        mx_std_error_noloc(ls, "attempt to yield from outside a coroutine");
    } else if (!ls->can_pend || ls->th->token != vm->token_cur) {
        mx_std_error_noloc(ls, "attempt to yield across a C-call boundary");
    }
    mx_luaapi_check(ls);
    if (!vm->std_error.is_set) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_YIELD, mx_obj_of_nil(), mx_obj_of_nil(), NULL, (uint32_t)nresults, mx_std_slots(ls) + ls->top - nresults);
    }
    return 0;
}

int lua_resume(lua_State *L, lua_State *from, int narg, int *nres) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_t *vm = ls->vm;
    mx_thread_t *co = ls->th;
    const char *checked = mx_coro_resume_check(vm, co);
    bool is_fresh = co->status == MX_THREAD_FRESH;
    const char *why;
    if (checked == NULL && is_fresh && ls->top < narg + 1) {
        why = "cannot resume dead coroutine";
    } else {
        why = checked;
    }
    if (why != NULL) {
        mx_std_pushstring(ls, why);
        *nres = 1;
        return LUA_ERRRUN;
    }
    mx_obj_t *args = mx_std_slots(ls) + ls->top - narg;
    if (is_fresh) {
        co->entry_fn = args[-1];
        mx_gc_write_barrier(vm, co, co->entry_fn);
        ls->top -= 1;
    }
    ls->top -= narg;
    mx_std_sync(ls);
    mx_run_result_t res = mx_resume(vm, co, (uint32_t)narg, args);
    mx_std_state_t *fresh = mx_luaapi_thread_state(vm, co);
    if (res.status == MX_RUN_ERROR) {
        mx_std_push(fresh, mx_error_payload(res.error));
        *nres = 1;
        return LUA_ERRRUN;
    }
    for (uint32_t i = 0; i < vm->rets.len; i++) {
        mx_std_push(fresh, vm->rets.vals[i]);
    }
    *nres = (int)vm->rets.len;
    if (res.status == MX_RUN_YIELD) {
        return LUA_YIELD;
    } else {
        return LUA_OK;
    }
}

int lua_status(lua_State *L) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_thread_t *co = ls->th;
    if (co != ls->vm->thread && co->status == MX_THREAD_SUSPENDED) {
        return LUA_YIELD;
    } else if (co->is_failed) {
        return LUA_ERRRUN;
    } else {
        return LUA_OK;
    }
}

int lua_isyieldable(lua_State *L) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    return mx_luaapi_bool(ls->th != ls->vm->main && ls->can_pend && ls->th->token == ls->vm->token_cur);
}

void lua_setwarnf(lua_State *L, lua_WarnFunction f, void *ud) {
    mx_lua_warn_t *w = &mx_lua_of(((mx_std_state_t *)L)->vm)->warn;
    w->fn = (mx_lua_warnf_t *)f;
    w->ud = ud;
}

void lua_warning(lua_State *L, const char *msg, int tocont) {
    mx_lua_warning(((mx_std_state_t *)L)->vm, msg, tocont != 0);
}

int lua_gc(lua_State *L, int what, ...) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_t *vm = ls->vm;
    switch (what) {
        case LUA_GCCOUNT: {
            return (int)(mx_gc_bytes(vm) >> 10);
        }
        case LUA_GCCOUNTB: {
            return (int)(mx_gc_bytes(vm) & 0x3ff);
        }
        case LUA_GCISRUNNING: {
            return mx_luaapi_bool(!vm->is_gc_stopped);
        }
        case LUA_GCSTOP: {
            vm->is_gc_stopped = true;
            return 0;
        }
        case LUA_GCRESTART: {
            vm->is_gc_stopped = false;
            return 0;
        }
        case LUA_GCGEN:
        case LUA_GCINC: {
            return LUA_GCGEN;
        }
        case LUA_GCCOLLECT:
        case LUA_GCSTEP: {
            vm->gc_pending = 1;
            vm->interrupt = 1;
            return 0;
        }
        default: {
            return 0;
        }
    }
}

int lua_error(lua_State *L) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, -1);
    mx_obj_t v;
    if (at != NULL) {
        v = *at;
    } else {
        v = mx_obj_of_nil();
    }
    ls->error = mx_error_value(ls->vm, v);
    mx_std_raise(ls);
    mx_luaapi_check(ls);
    return 0;
}

int lua_next(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    bool got = mx_std_next(ls, idx);
    mx_luaapi_check(ls);
    return mx_luaapi_bool(got);
}

void lua_concat(lua_State *L, int n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    if (n == 0) {
        mx_std_pushlstring(ls, "", 0);
    }
    for (int32_t i = 1; i < n && !ls->vm->std_error.is_set; i++) {
        mx_obj_t a = *mx_std_at(ls, -2);
        mx_obj_t b = *mx_std_at(ls, -1);
        mx_std_pop(ls, 2);
        mx_obj_t r = mx_meta_concat(ls->vm, a, b);
        if (mx_obj_is_error(r)) {
            mx_error_locate(ls->vm, mx_obj_get_error(r), NULL);
            ls->error = r;
            mx_std_raise(ls);
        } else {
            mx_std_push(ls, r);
        }
    }
    mx_luaapi_check(ls);
}

void lua_len(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    mx_obj_t v;
    if (at != NULL) {
        v = *at;
    } else {
        v = mx_obj_of_nil();
    }
    mx_obj_t got = mx_meta_len(ls->vm, v);
    if (mx_obj_is_error(got)) {
        mx_error_locate(ls->vm, mx_obj_get_error(got), NULL);
        ls->error = got;
        mx_std_raise(ls);
        mx_std_pushnil(ls);
    } else {
        mx_std_push(ls, got);
    }
    mx_luaapi_check(ls);
}

unsigned lua_numbertocstring(lua_State *L, int idx, char *buff) {
    mx_obj_t *at = mx_std_at((mx_std_state_t *)L, idx);
    if (at == NULL || !mx_obj_is_number(*at)) {
        return 0;
    }
    mx_str_t *out = mx_str_new();
    if (mx_obj_is_int(*at)) {
        mx_str_append_int(out, mx_obj_get_int(*at));
    } else {
        mx_str_float(out, mx_obj_get_float(*at));
    }
    uint32_t len;
    if (out->len >= LUA_N2SBUFFSZ) {
        len = LUA_N2SBUFFSZ - 1;
    } else {
        len = out->len;
    }
    memcpy(buff, out->buf, len);
    buff[len] = '\0';
    mx_str_free(out);
    return len + 1;
}

size_t lua_stringtonumber(lua_State *L, const char *s) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_num_t num = mx_std_str2num(s, strlen(s));
    if (!num.is_num) {
        return 0;
    }
    mx_std_push(ls, num.value);
    mx_luaapi_check(ls);
    return strlen(s) + 1;
}

lua_Alloc lua_getallocf(lua_State *L, void **ud) {
    if (ud != NULL) {
        *ud = NULL;
    }
    return luaL_alloc;
}

void lua_setallocf(lua_State *L, lua_Alloc f, void *ud) {
}

void lua_toclose(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, idx);
    mx_obj_t v;
    if (at != NULL) {
        v = *at;
    } else {
        v = mx_obj_of_nil();
    }
    mx_obj_t args[2] = {v, mx_obj_intern(ls->vm, "?")};
    mx_prim_close_check(ls->vm, 2, args);
    if (mx_obj_is_error(args[0])) {
        mx_error_locate(ls->vm, mx_obj_get_error(args[0]), NULL);
        ls->error = args[0];
        mx_std_raise(ls);
    }
    mx_luaapi_check(ls);
}

void lua_closeslot(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_t *vm = ls->vm;
    mx_obj_t *at = mx_std_at(ls, idx);
    if (at == NULL) {
        return;
    }
    mx_obj_t v = *at;
    for (uint32_t i = vm->tbcs.len; i > 0; i--) {
        mx_tbc_t *tbc = &vm->tbcs.ptr[i - 1];
        if (tbc->thread == vm->thread && tbc->value.kind == v.kind && tbc->value.val.bits == v.val.bits) {
            memmove(tbc, tbc + 1, sizeof(mx_tbc_t) * (vm->tbcs.len - i));
            vm->tbcs.len -= 1;
            break;
        }
    }
    *at = mx_obj_of_nil();
    mx_obj_t handler = mx_prim_close_handler(vm, v);
    if (mx_obj_is_truthy(v) && !mx_obj_is_nil(handler)) {
        mx_obj_t got = mx_call(vm, handler, 1, &v);
        if (mx_obj_is_error(got)) {
            ls->error = got;
            mx_std_raise(ls);
        }
    }
    mx_luaapi_check(ls);
}

int lua_getstack(lua_State *L, int level, lua_Debug *ar) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    int32_t at;
    if (ls->is_hook && level >= 0) {
        at = level + 1;
    } else {
        at = level;
    }
    if (at < 0 || !mx_frame_at_level(ls->th, (uint32_t)at).is_valid) {
        return 0;
    }
    ar->i_ci = (struct CallInfo *)(uintptr_t)((uint32_t)at + 1);
    return 1;
}

int lua_getinfo(lua_State *L, const char *what, lua_Debug *ar) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    bool is_fn = what[0] == '>';
    mx_frame_t f;
    if (is_fn) {
        f = (mx_frame_t) { .is_valid = false };
    } else {
        f = mx_frame_at_level(ls->th, (uint32_t)((uintptr_t)ar->i_ci - 1));
    }
    if (!is_fn && !f.is_valid) {
        return 0;
    }
    mx_obj_t fn;
    const char *opts;
    if (is_fn) {
        fn = *mx_std_at(ls, -1);
        mx_std_pop(ls, 1);
        opts = what + 1;
    } else {
        fn = mx_frame_fn(&f);
        opts = what;
    }
    const mx_ir_block_t *head;
    if (mx_obj_is_closure(fn)) {
        head = mx_obj_get_closure(fn)->block;
    } else {
        head = NULL;
    }
    for (const char *c = opts; *c != '\0'; c++) {
        switch (*c) {
            case 'S': {
                if (head == NULL) {
                    ar->source = "=[C]";
                    ar->what = "C";
                    ar->linedefined = -1;
                    ar->lastlinedefined = -1;
                    snprintf(ar->short_src, sizeof(ar->short_src), "[C]");
                } else {
                    const mx_ir_func_t *func = head->func;
                    ar->source = func->source->file;
                    snprintf(ar->short_src, sizeof(ar->short_src), "%s", mx_error_chunkid(func->source->file).id);
                    if (head->is_main) {
                        ar->what = "main";
                        ar->linedefined = 0;
                        ar->lastlinedefined = 0;
                    } else {
                        ar->what = "Lua";
                        ar->linedefined = (int)mx_source_line(func->source, func->start);
                        ar->lastlinedefined = (int)mx_source_line(func->source, func->stop - 1);
                    }
                }
                ar->srclen = strlen(ar->source);
                break;
            }
            case 'l': {
                if (f.is_valid && f.is_lua) {
                    ar->currentline = (int)mx_frame_line(&f);
                } else {
                    ar->currentline = -1;
                }
                break;
            }
            case 'u': {
                if (head != NULL) {
                    ar->nups = (unsigned char)head->func->nupvals;
                    ar->nparams = (unsigned char)head->func->nparams;
                    ar->isvararg = (char)head->func->is_vararg;
                } else if (fn.kind == ls->vm->kinds.cfn) {
                    ar->nups = (unsigned char)((mx_obj_cfn_t *)mx_obj_data_head(fn))->len;
                    ar->nparams = 0;
                    ar->isvararg = 1;
                } else {
                    ar->nups = 0;
                    ar->nparams = 0;
                    ar->isvararg = 1;
                }
                ar->extraargs = 0;
                break;
            }
            case 'n': {
                mx_trace_name_t named;
                if (f.is_valid) {
                    named = mx_trace_namewhat(&f);
                } else {
                    named = (mx_trace_name_t) { .what = NULL, .name = NULL };
                }
                ar->name = named.name;
                if (named.what == NULL) {
                    ar->namewhat = "";
                } else {
                    ar->namewhat = named.what;
                }
                break;
            }
            case 't': {
                ar->istailcall = 0;
                break;
            }
            case 'r': {
                ar->ftransfer = 0;
                ar->ntransfer = 0;
                break;
            }
            case 'f': {
                mx_std_push(ls, fn);
                break;
            }
            case 'L': {
                mx_std_pushnil(ls);
                break;
            }
            default: {
                return 0;
            }
        }
    }
    mx_luaapi_check(ls);
    return 1;
}

const char *lua_getlocal(lua_State *L, const lua_Debug *ar, int n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    if (ar == NULL) {
        mx_obj_t *at = mx_std_at(ls, -1);
        if (at == NULL || !mx_obj_is_closure(*at)) {
            return NULL;
        }
        const mx_ir_local_t *param = mx_std_debug_param(mx_obj_get_closure(*at)->block->func, n);
        if (param == NULL) {
            return NULL;
        } else {
            return param->name;
        }
    }
    mx_frame_t f = mx_frame_at_level(ls->th, (uint32_t)((uintptr_t)ar->i_ci - 1));
    const mx_ir_local_t *local;
    if (f.is_valid) {
        local = mx_frame_local(&f, n);
    } else {
        local = NULL;
    }
    if (local == NULL) {
        return NULL;
    }
    mx_std_push(ls, mx_frame_reg(&f, local->reg));
    mx_luaapi_check(ls);
    return local->name;
}

const char *lua_setlocal(lua_State *L, const lua_Debug *ar, int n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_frame_t f = mx_frame_at_level(ls->th, (uint32_t)((uintptr_t)ar->i_ci - 1));
    const mx_ir_local_t *local;
    if (f.is_valid) {
        local = mx_frame_local(&f, n);
    } else {
        local = NULL;
    }
    if (local != NULL && !local->is_pack) {
        mx_frame_set_reg(ls->vm, &f, local->reg, *mx_std_at(ls, -1));
    }
    mx_std_pop(ls, 1);
    if (local == NULL) {
        return NULL;
    } else {
        return local->name;
    }
}

mx_obj_t *mx_luaapi_cfn_upval(mx_std_state_t *ls, int32_t funcindex, int32_t n) {
    mx_obj_t *at = mx_std_at(ls, funcindex);
    if (at == NULL || at->kind != ls->vm->kinds.cfn) {
        return NULL;
    }
    mx_obj_cfn_t *cfn = (mx_obj_cfn_t *)mx_obj_data_head(*at);
    if (n < 1 || (uint32_t)n > cfn->len) {
        return NULL;
    } else {
        return &cfn->values[n - 1];
    }
}

mx_obj_upval_t **mx_luaapi_lua_upval(mx_std_state_t *ls, int32_t funcindex, int32_t n) {
    mx_obj_t *at = mx_std_at(ls, funcindex);
    if (at == NULL || !mx_obj_is_closure(*at)) {
        return NULL;
    }
    mx_obj_closure_t *cl = mx_obj_get_closure(*at);
    if (n < 1 || (uint32_t)n > cl->len) {
        return NULL;
    } else {
        return &cl->upvals[n - 1];
    }
}

const char *mx_luaapi_upval_name(mx_std_state_t *ls, int32_t funcindex, int32_t n) {
    const char *name = mx_obj_get_closure(*mx_std_at(ls, funcindex))->block->func->upvals[n - 1].name;
    if (name == NULL) {
        return "(no name)";
    } else {
        return name;
    }
}

const char *lua_getupvalue(lua_State *L, int funcindex, int n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *cv = mx_luaapi_cfn_upval(ls, funcindex, n);
    mx_obj_upval_t **lv = mx_luaapi_lua_upval(ls, funcindex, n);
    if (cv != NULL) {
        mx_std_push(ls, *cv);
        mx_luaapi_check(ls);
        return "";
    } else if (lv != NULL) {
        const char *name = mx_luaapi_upval_name(ls, funcindex, n);
        mx_std_push(ls, mx_upval_get(*lv));
        mx_luaapi_check(ls);
        return name;
    } else {
        return NULL;
    }
}

const char *lua_setupvalue(lua_State *L, int funcindex, int n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t v = *mx_std_at(ls, -1);
    mx_obj_t *cv = mx_luaapi_cfn_upval(ls, funcindex, n);
    mx_obj_upval_t **lv = mx_luaapi_lua_upval(ls, funcindex, n);
    if (cv != NULL) {
        mx_gc_write_barrier(ls->vm, mx_obj_data_head(*mx_std_at(ls, funcindex)), v);
        *cv = v;
        mx_std_pop(ls, 1);
        return "";
    } else if (lv != NULL) {
        mx_upval_set(ls->vm, *lv, v);
        const char *name = mx_luaapi_upval_name(ls, funcindex, n);
        mx_std_pop(ls, 1);
        return name;
    } else {
        return NULL;
    }
}

void *lua_upvalueid(lua_State *L, int fidx, int n) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *cv = mx_luaapi_cfn_upval(ls, fidx, n);
    mx_obj_upval_t **lv = mx_luaapi_lua_upval(ls, fidx, n);
    if (cv != NULL) {
        return cv;
    } else if (lv != NULL) {
        return *lv;
    } else {
        return NULL;
    }
}

void lua_upvaluejoin(lua_State *L, int fidx1, int n1, int fidx2, int n2) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_upval_t **to = mx_luaapi_lua_upval(ls, fidx1, n1);
    mx_obj_upval_t **from = mx_luaapi_lua_upval(ls, fidx2, n2);
    if (to != NULL && from != NULL) {
        mx_gc_write_barrier(ls->vm, mx_obj_data_head(*mx_std_at(ls, fidx1)), mx_obj_of_data(*from));
        *to = *from;
    }
}

int32_t mx_luaapi_hook_adapter(mx_std_state_t *ls) {
    const char *const events[] = {"call", "return", "line", "count", "tail call", NULL};
    const char *name = mx_std_tostring(ls, 1).ptr;
    int event = 0;
    while (events[event] != NULL && strcmp(events[event], name) != 0) {
        event += 1;
    }
    lua_Debug ar = {
        .event = event,
        .currentline = (int)mx_std_optinteger(ls, 2, -1),
        .i_ci = (struct CallInfo *)(uintptr_t)2,
    };
    lua_Hook hook = (lua_Hook)(uintptr_t)mx_obj_get_lightuserdata(mx_std_upval(ls, 0));
    ls->is_hook = true;
    hook((lua_State *)ls, &ar);
    return 0;
}

lua_Hook mx_luaapi_hook_of(mx_t *vm) {
    mx_obj_t fn = mx_lua_of(vm)->hook.fn;
    if (fn.kind == vm->kinds.cfn && ((mx_obj_cfn_t *)mx_obj_data_head(fn))->std_fn == mx_luaapi_hook_adapter) {
        return (lua_Hook)(uintptr_t)mx_obj_get_lightuserdata(((mx_obj_cfn_t *)mx_obj_data_head(fn))->values[0]);
    } else {
        return NULL;
    }
}

void lua_sethook(lua_State *L, lua_Hook func, int mask, int count) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_t *vm = ls->vm;
    mx_lua_hook_t *hook = &mx_lua_of(vm)->hook;
    int counted;
    if (count <= 0) {
        counted = mask & ~LUA_MASKCOUNT;
    } else {
        counted = mask;
    }
    int active;
    if (func == NULL || counted == 0) {
        hook->fn = mx_obj_of_nil();
        active = 0;
    } else {
        mx_std_push(ls, mx_obj_of_lightuserdata((void *)(uintptr_t)func));
        mx_luaapi_push_cfn(ls, mx_luaapi_hook_adapter, 1);
        hook->fn = *mx_std_at(ls, -1);
        mx_std_pop(ls, 1);
        active = counted;
    }
    ls->th->has_hook = active != 0;
    hook->mask = (uint32_t)active;
    hook->count = count;
    hook->counter = count;
    hook->is_tail = false;
    hook->thread = NULL;
    hook->last_frame = MX_THREAD_NONE;
    vm->hooks.debug = mx_std_debug_hook;
    vm->is_hooked = active != 0;
    vm->interrupt = 1;
}

lua_Hook lua_gethook(lua_State *L) {
    return mx_luaapi_hook_of(((mx_std_state_t *)L)->vm);
}

int lua_gethookmask(lua_State *L) {
    mx_t *vm = ((mx_std_state_t *)L)->vm;
    if (mx_luaapi_hook_of(vm) == NULL) {
        return 0;
    } else {
        return (int)mx_lua_of(vm)->hook.mask;
    }
}

int lua_gethookcount(lua_State *L) {
    mx_t *vm = ((mx_std_state_t *)L)->vm;
    if (mx_luaapi_hook_of(vm) == NULL) {
        return 0;
    } else {
        return (int)mx_lua_of(vm)->hook.count;
    }
}

void luaL_checkversion_(lua_State *L, lua_Number ver, size_t sz) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    if ((int32_t)ver != LUA_VERSION_NUM) {
        mx_std_error(ls, "version mismatch: app. needs %d, MiniVM provides %d", (int32_t)ver, LUA_VERSION_NUM);
    } else if (sz != LUAL_NUMSIZES) {
        mx_std_error(ls, "core and library have incompatible numeric types");
    }
    mx_luaapi_check(ls);
}

int luaL_getmetafield(lua_State *L, int obj, const char *e) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    if (!mx_std_getmetatable(ls, obj)) {
        return LUA_TNIL;
    }
    mx_std_push(ls, mx_obj_intern(ls->vm, e));
    int32_t t = mx_luaapi_type_of(mx_std_rawget(ls, -2));
    if (t == LUA_TNIL) {
        mx_std_pop(ls, 2);
    } else {
        mx_std_remove(ls, -2);
    }
    return t;
}

int luaL_callmeta(lua_State *L, int obj, const char *e) {
    int abs = lua_absindex(L, obj);
    if (luaL_getmetafield(L, abs, e) == LUA_TNIL) {
        return 0;
    }
    lua_pushvalue(L, abs);
    lua_callk(L, 1, 1, 0, NULL);
    return 1;
}

const char *luaL_tolstring(lua_State *L, int idx, size_t *len) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_tostring_meta(ls, idx);
    mx_luaapi_check(ls);
    return lua_tolstring(L, -1, len);
}

int luaL_argerror(lua_State *L, int arg, const char *extramsg) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_argerror(ls, arg, extramsg);
    mx_luaapi_check(ls);
    return 0;
}

int luaL_typeerror(lua_State *L, int arg, const char *tname) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_typeerror(ls, arg, tname);
    mx_luaapi_check(ls);
    return 0;
}

const char *luaL_checklstring(lua_State *L, int arg, size_t *l) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_str_t s = mx_std_checkstring(ls, arg);
    mx_luaapi_check(ls);
    if (l != NULL) {
        *l = s.len;
    }
    return s.ptr;
}

const char *luaL_optlstring(lua_State *L, int arg, const char *def, size_t *l) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_str_t s = mx_std_optstring(ls, arg, def);
    mx_luaapi_check(ls);
    if (l != NULL) {
        *l = s.len;
    }
    return s.ptr;
}

lua_Number luaL_checknumber(lua_State *L, int arg) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    double v = mx_std_checkfloat(ls, arg);
    mx_luaapi_check(ls);
    return v;
}

lua_Number luaL_optnumber(lua_State *L, int arg, lua_Number def) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    double v = mx_std_optfloat(ls, arg, def);
    mx_luaapi_check(ls);
    return v;
}

lua_Integer luaL_checkinteger(lua_State *L, int arg) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    int64_t v = mx_std_checkinteger(ls, arg);
    mx_luaapi_check(ls);
    return v;
}

lua_Integer luaL_optinteger(lua_State *L, int arg, lua_Integer def) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    int64_t v = mx_std_optinteger(ls, arg, def);
    mx_luaapi_check(ls);
    return v;
}

void luaL_checkstack(lua_State *L, int sz, const char *msg) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    if (mx_std_checkstack(ls, sz)) {
        return;
    } else if (msg == NULL) {
        mx_std_error(ls, "stack overflow");
    } else {
        mx_std_error(ls, "stack overflow (%s)", msg);
    }
    mx_luaapi_check(ls);
}

void luaL_checktype(lua_State *L, int arg, int t) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    if (lua_type(L, arg) != t) {
        mx_std_typeerror(ls, arg, lua_typename(L, t));
    }
    mx_luaapi_check(ls);
}

void luaL_checkany(lua_State *L, int arg) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_checkany(ls, arg);
    mx_luaapi_check(ls);
}

int luaL_newmetatable(lua_State *L, const char *tname) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    if (mx_std_getfield(ls, MX_STD_REGISTRYINDEX, tname) != MX_LUA_TYPE_NIL) {
        return 0;
    }
    mx_std_pop(ls, 1);
    mx_std_createtable(ls, 0, 2);
    mx_std_push(ls, mx_lua_of(ls->vm)->meta_keys[MX_LUA_META_KEY_NAME]);
    mx_std_pushstring(ls, tname);
    mx_std_rawset(ls, -3);
    mx_std_pushvalue(ls, -1);
    mx_std_setfield(ls, MX_STD_REGISTRYINDEX, tname);
    mx_luaapi_check(ls);
    return 1;
}

void luaL_setmetatable(lua_State *L, const char *tname) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_std_getfield(ls, MX_STD_REGISTRYINDEX, tname);
    mx_std_setmetatable(ls, -2);
    mx_luaapi_check(ls);
}

void *luaL_testudata(lua_State *L, int ud, const char *tname) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_obj_t *at = mx_std_at(ls, ud);
    if (at == NULL || !mx_std_udata_is(ls->vm, *at) || !mx_std_getmetatable(ls, ud)) {
        return NULL;
    }
    mx_std_getfield(ls, MX_STD_REGISTRYINDEX, tname);
    bool same = mx_std_rawequal(ls, -1, -2);
    mx_std_pop(ls, 2);
    if (same) {
        return mx_std_udata_payload(*at);
    } else {
        return NULL;
    }
}

void *luaL_checkudata(lua_State *L, int ud, const char *tname) {
    void *p = luaL_testudata(L, ud, tname);
    if (p == NULL) {
        luaL_typeerror(L, ud, tname);
    }
    return p;
}

void luaL_where(lua_State *L, int lvl) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_str_t *out = mx_str_new();
    if (lvl >= 0) {
        mx_std_where(ls, (uint32_t)lvl, out);
    }
    mx_std_pushlstring(ls, out->buf, out->len);
    mx_str_free(out);
    mx_luaapi_check(ls);
}

int luaL_error(lua_State *L, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    luaL_where(L, 1);
    lua_pushvfstring(L, fmt, ap);
    va_end(ap);
    lua_concat(L, 2);
    return lua_error(L);
}

int luaL_checkoption(lua_State *L, int arg, const char *def, const char *const lst[]) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    int32_t got = mx_std_checkoption(ls, arg, def, lst);
    mx_luaapi_check(ls);
    return got;
}

int luaL_fileresult(lua_State *L, int stat, const char *fname) {
    return mx_std_io_fileresult((mx_std_state_t *)L, stat != 0, fname);
}

int luaL_execresult(lua_State *L, int stat) {
    return mx_std_os_execresult((mx_std_state_t *)L, stat);
}

void *luaL_alloc(void *ud, void *ptr, size_t osize, size_t nsize) {
    if (nsize == 0) {
        free(ptr);
        return NULL;
    } else {
        return realloc(ptr, nsize);
    }
}

int luaL_ref(lua_State *L, int t) {
    if (lua_type(L, -1) == LUA_TNIL) {
        lua_settop(L, -2);
        return LUA_REFNIL;
    }
    int tab = lua_absindex(L, t);
    int32_t free_ref;
    if (lua_rawgeti(L, tab, 1) == LUA_TNUMBER) {
        free_ref = (int32_t)lua_tointegerx(L, -1, NULL);
    } else {
        free_ref = 0;
        lua_pushinteger(L, 0);
        lua_rawseti(L, tab, 1);
    }
    lua_settop(L, -2);
    int32_t ref;
    if (free_ref != 0) {
        lua_rawgeti(L, tab, free_ref);
        lua_rawseti(L, tab, 1);
        ref = free_ref;
    } else {
        ref = (int32_t)lua_rawlen(L, tab) + 1;
    }
    lua_rawseti(L, tab, ref);
    return ref;
}

void luaL_unref(lua_State *L, int t, int ref) {
    if (ref >= 0) {
        int tab = lua_absindex(L, t);
        lua_rawgeti(L, tab, 1);
        lua_rawseti(L, tab, ref);
        lua_pushinteger(L, ref);
        lua_rawseti(L, tab, 1);
    }
}

int luaL_loadfilex(lua_State *L, const char *filename, const char *mode) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    const char *how;
    if (mode == NULL) {
        how = "bt";
    } else {
        how = mode;
    }
    int32_t failed;
    if (filename != NULL && !mx_std_package_readable(filename)) {
        failed = LUA_ERRFILE;
    } else {
        failed = LUA_ERRSYNTAX;
    }
    int32_t n = mx_std_load_file(ls, filename, how, mx_lua_of(ls->vm)->std);
    mx_luaapi_check(ls);
    return mx_luaapi_load_status(ls, n, failed);
}

int luaL_loadbufferx(lua_State *L, const char *buff, size_t sz, const char *name, const char *mode) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    const char *chunkname;
    if (name == NULL) {
        chunkname = "?";
    } else {
        chunkname = name;
    }
    const char *how;
    if (mode == NULL) {
        how = "bt";
    } else {
        how = mode;
    }
    int32_t n = mx_std_load_chunk(ls, buff, sz, chunkname, how, mx_lua_of(ls->vm)->std);
    mx_luaapi_check(ls);
    return mx_luaapi_load_status(ls, n, LUA_ERRSYNTAX);
}

int luaL_loadstring(lua_State *L, const char *s) {
    return luaL_loadbufferx(L, s, strlen(s), s, NULL);
}

lua_State *luaL_newstate(void) {
    return NULL;
}

unsigned luaL_makeseed(lua_State *L) {
    return (unsigned)mx_std_rand_entropy(((mx_std_state_t *)L)->vm);
}

lua_Integer luaL_len(lua_State *L, int idx) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    int64_t n = mx_std_len_meta(ls, idx);
    mx_luaapi_check(ls);
    return n;
}

void luaL_addgsub(luaL_Buffer *b, const char *s, const char *p, const char *r) {
    size_t plen = strlen(p);
    const char *wild = strstr(s, p);
    while (wild != NULL) {
        luaL_addlstring(b, s, (size_t)(wild - s));
        luaL_addstring(b, r);
        s = wild + plen;
        wild = strstr(s, p);
    }
    luaL_addstring(b, s);
}

const char *luaL_gsub(lua_State *L, const char *s, const char *p, const char *r) {
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    luaL_addgsub(&b, s, p, r);
    luaL_pushresult(&b);
    return lua_tolstring(L, -1, NULL);
}

void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
    luaL_checkstack(L, nup, "too many upvalues");
    for (; l->name != NULL; l++) {
        if (l->func == NULL) {
            lua_pushboolean(L, 0);
        } else {
            for (int32_t i = 0; i < nup; i++) {
                lua_pushvalue(L, -nup);
            }
            lua_pushcclosure(L, l->func, nup);
        }
        lua_setfield(L, -(nup + 2), l->name);
    }
    lua_settop(L, -nup - 1);
}

int luaL_getsubtable(lua_State *L, int idx, const char *fname) {
    if (lua_getfield(L, idx, fname) == LUA_TTABLE) {
        return 1;
    }
    lua_settop(L, -2);
    int abs = lua_absindex(L, idx);
    lua_createtable(L, 0, 0);
    lua_pushvalue(L, -1);
    lua_setfield(L, abs, fname);
    return 0;
}

void luaL_traceback(lua_State *L, lua_State *L1, const char *msg, int level) {
    mx_std_state_t *ls = (mx_std_state_t *)L;
    mx_str_t *out = mx_str_new();
    if (msg != NULL) {
        mx_str_format(out, "%s\n", msg);
    }
    uint32_t from;
    if (level < 0) {
        from = 0;
    } else {
        from = (uint32_t)level;
    }
    mx_trace_append(ls->vm, out, ((mx_std_state_t *)L1)->th, from);
    mx_std_pushlstring(ls, out->buf, out->len);
    mx_str_free(out);
    mx_luaapi_check(ls);
}

void luaL_requiref(lua_State *L, const char *modname, lua_CFunction openf, int glb) {
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    lua_getfield(L, -1, modname);
    if (!lua_toboolean(L, -1)) {
        lua_settop(L, -2);
        lua_pushcclosure(L, openf, 0);
        lua_pushstring(L, modname);
        lua_callk(L, 1, 1, 0, NULL);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, modname);
    }
    lua_rotate(L, -2, -1);
    lua_settop(L, -2);
    if (glb != 0) {
        lua_pushvalue(L, -1);
        lua_setglobal(L, modname);
    }
}

void *mx_luaapi_box_resize(lua_State *L, int32_t idx, size_t size) {
    mx_luaapi_box_t *box = lua_touserdata(L, idx);
    if (box->size == size) {
        return box->data;
    }
    void *got;
    if (size == 0) {
        mx_mem_free(box->data);
        got = NULL;
    } else if (mx_mem_fits(size)) {
        got = mx_mem_realloc(box->data, size);
    } else {
        lua_pushstring(L, "not enough memory");
        lua_error(L);
        return NULL;
    }
    box->data = got;
    box->size = size;
    return got;
}

int32_t mx_luaapi_box_gc(mx_std_state_t *ls) {
    mx_luaapi_box_resize((lua_State *)ls, 1, 0);
    return 0;
}

void mx_luaapi_box_new(lua_State *L) {
    mx_luaapi_box_t *box = lua_newuserdatauv(L, sizeof(mx_luaapi_box_t), 0);
    *box = (mx_luaapi_box_t) { .data = NULL, .size = 0 };
    if (lua_getfield(L, LUA_REGISTRYINDEX, "_UBOX*") == LUA_TNIL) {
        lua_settop(L, -2);
        lua_createtable(L, 0, 2);
        mx_luaapi_push_cfn((mx_std_state_t *)L, mx_luaapi_box_gc, 0);
        lua_setfield(L, -2, "__gc");
        mx_luaapi_push_cfn((mx_std_state_t *)L, mx_luaapi_box_gc, 0);
        lua_setfield(L, -2, "__close");
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, "_UBOX*");
    }
    lua_setmetatable(L, -2);
}

char *mx_luaapi_buf_prep(luaL_Buffer *B, size_t sz, int32_t boxidx) {
    if (B->size - B->n >= sz) {
        return B->b + B->n;
    }
    lua_State *L = B->L;
    if (sz >= MX_STD_MAX_STRING - B->n) {
        luaL_error(L, "resulting string too large");
        return NULL;
    }
    size_t size;
    if (B->size + (B->size >> 1) < B->n + sz + 1) {
        size = B->n + sz + 1;
    } else {
        size = B->size + (B->size >> 1);
    }
    char *data;
    if (B->b != B->init.b) {
        data = mx_luaapi_box_resize(L, boxidx, size);
    } else {
        lua_rotate(L, boxidx, -1);
        lua_settop(L, -2);
        mx_luaapi_box_new(L);
        lua_rotate(L, boxidx, 1);
        lua_toclose(L, boxidx);
        data = mx_luaapi_box_resize(L, boxidx, size);
        if (data != NULL) {
            memcpy(data, B->b, B->n);
        }
    }
    if (data != NULL) {
        B->b = data;
        B->size = size;
        return data + B->n;
    } else {
        return NULL;
    }
}

void luaL_buffinit(lua_State *L, luaL_Buffer *B) {
    B->L = L;
    B->b = B->init.b;
    B->n = 0;
    B->size = LUAL_BUFFERSIZE;
    lua_pushlightuserdata(L, B);
}

char *luaL_prepbuffsize(luaL_Buffer *B, size_t sz) {
    return mx_luaapi_buf_prep(B, sz, -1);
}

void luaL_addlstring(luaL_Buffer *B, const char *s, size_t l) {
    if (l > 0) {
        char *dst = mx_luaapi_buf_prep(B, l, -1);
        if (dst != NULL) {
            memcpy(dst, s, l);
            B->n += l;
        }
    }
}

void luaL_addstring(luaL_Buffer *B, const char *s) {
    luaL_addlstring(B, s, strlen(s));
}

void luaL_addvalue(luaL_Buffer *B) {
    size_t len;
    const char *s = lua_tolstring(B->L, -1, &len);
    char *dst = mx_luaapi_buf_prep(B, len, -2);
    if (dst != NULL) {
        memcpy(dst, s, len);
        B->n += len;
    }
    lua_settop(B->L, -2);
}

void luaL_pushresult(luaL_Buffer *B) {
    lua_State *L = B->L;
    lua_pushlstring(L, B->b, B->n);
    if (B->b != B->init.b) {
        lua_closeslot(L, -2);
    }
    lua_rotate(L, -2, -1);
    lua_settop(L, -2);
}

void luaL_pushresultsize(luaL_Buffer *B, size_t sz) {
    B->n += sz;
    luaL_pushresult(B);
}

char *luaL_buffinitsize(lua_State *L, luaL_Buffer *B, size_t sz) {
    luaL_buffinit(L, B);
    return mx_luaapi_buf_prep(B, sz, -1);
}

int luaopen_base(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_GNAME);
}

int luaopen_package(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_LOADLIBNAME);
}

int luaopen_coroutine(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_COLIBNAME);
}

int luaopen_debug(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_DBLIBNAME);
}

int luaopen_io(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_IOLIBNAME);
}

int luaopen_math(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_MATHLIBNAME);
}

int luaopen_os(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_OSLIBNAME);
}

int luaopen_string(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_STRLIBNAME);
}

int luaopen_table(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_TABLIBNAME);
}

int luaopen_utf8(lua_State *L) {
    return mx_luaapi_open_loaded((mx_std_state_t *)L, LUA_UTF8LIBNAME);
}

void luaL_openselectedlibs(lua_State *L, int load, int preload) {
}
