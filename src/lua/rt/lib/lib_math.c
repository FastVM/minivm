
#include "../api/std.h"
#include "../lua.h"
#include "../../../value/obj.h"

void mx_std_math_push_rounded(mx_std_state_t *ls, double d) {
    if (mx_obj_float_fits_int(d)) {
        mx_std_pushinteger(ls, (int64_t)d);
    } else {
        mx_std_pushfloat(ls, d);
    }
}

int32_t mx_std_math_unary(mx_std_state_t *ls, double (*fn)(double)) {
    mx_std_pushfloat(ls, fn(mx_std_checkfloat(ls, 1)));
    return 1;
}

int32_t mx_std_math_abs(mx_std_state_t *ls) {
    if (mx_std_isinteger(ls, 1)) {
        uint64_t n = (uint64_t)mx_std_checkinteger(ls, 1);
        if ((int64_t)n < 0) {
            n = ~n + 1;
        }
        mx_std_pushinteger(ls, (int64_t)n);
        return 1;
    } else {
        return mx_std_math_unary(ls, fabs);
    }
}

int32_t mx_std_math_floor(mx_std_state_t *ls) {
    if (mx_std_isinteger(ls, 1)) {
        mx_std_settop(ls, 1);
    } else {
        mx_std_math_push_rounded(ls, floor(mx_std_checkfloat(ls, 1)));
    }
    return 1;
}

int32_t mx_std_math_ceil(mx_std_state_t *ls) {
    if (mx_std_isinteger(ls, 1)) {
        mx_std_settop(ls, 1);
    } else {
        mx_std_math_push_rounded(ls, ceil(mx_std_checkfloat(ls, 1)));
    }
    return 1;
}

int32_t mx_std_math_fmod(mx_std_state_t *ls) {
    if (mx_std_isinteger(ls, 1) && mx_std_isinteger(ls, 2)) {
        int64_t a = mx_std_checkinteger(ls, 1);
        int64_t b = mx_std_checkinteger(ls, 2);
        if (b == 0) {
            return mx_std_argerror(ls, 2, "zero");
        } else if (b == -1) {
            mx_std_pushinteger(ls, 0);
        } else {
            mx_std_pushinteger(ls, a % b);
        }
    } else {
        double a = mx_std_checkfloat(ls, 1);
        mx_std_pushfloat(ls, fmod(a, mx_std_checkfloat(ls, 2)));
    }
    return 1;
}

int32_t mx_std_math_modf(mx_std_state_t *ls) {
    if (mx_std_isinteger(ls, 1)) {
        mx_std_settop(ls, 1);
        mx_std_pushfloat(ls, 0.0);
    } else {
        double x = mx_std_checkfloat(ls, 1);
        double whole = trunc(x);
        mx_std_math_push_rounded(ls, whole);
        if (isinf(x)) {
            mx_std_pushfloat(ls, 0.0);
        } else {
            mx_std_pushfloat(ls, x - whole);
        }
    }
    return 2;
}

int32_t mx_std_math_sqrt(mx_std_state_t *ls) {
    return mx_std_math_unary(ls, sqrt);
}

int32_t mx_std_math_exp(mx_std_state_t *ls) {
    return mx_std_math_unary(ls, exp);
}

int32_t mx_std_math_sin(mx_std_state_t *ls) {
    return mx_std_math_unary(ls, sin);
}

int32_t mx_std_math_cos(mx_std_state_t *ls) {
    return mx_std_math_unary(ls, cos);
}

int32_t mx_std_math_tan(mx_std_state_t *ls) {
    return mx_std_math_unary(ls, tan);
}

int32_t mx_std_math_asin(mx_std_state_t *ls) {
    return mx_std_math_unary(ls, asin);
}

int32_t mx_std_math_acos(mx_std_state_t *ls) {
    return mx_std_math_unary(ls, acos);
}

int32_t mx_std_math_ult(mx_std_state_t *ls) {
    uint64_t a = (uint64_t)mx_std_checkinteger(ls, 1);
    mx_std_pushboolean(ls, a < (uint64_t)mx_std_checkinteger(ls, 2));
    return 1;
}

int32_t mx_std_math_log(mx_std_state_t *ls) {
    double x = mx_std_checkfloat(ls, 1);
    double r;
    if (mx_std_isnoneornil(ls, 2)) {
        r = log(x);
    } else {
        double b = mx_std_checkfloat(ls, 2);
        if (b == 2.0) {
            r = log2(x);
        } else if (b == 10.0) {
            r = log10(x);
        } else {
            r = log(x) / log(b);
        }
    }
    mx_std_pushfloat(ls, r);
    return 1;
}

int32_t mx_std_math_deg(mx_std_state_t *ls) {
    mx_std_pushfloat(ls, mx_std_checkfloat(ls, 1) * 57.29577951308232);
    return 1;
}

int32_t mx_std_math_rad(mx_std_state_t *ls) {
    mx_std_pushfloat(ls, mx_std_checkfloat(ls, 1) * 0.017453292519943295);
    return 1;
}

int32_t mx_std_math_atan(mx_std_state_t *ls) {
    double y = mx_std_checkfloat(ls, 1);
    mx_std_pushfloat(ls, atan2(y, mx_std_optfloat(ls, 2, 1.0)));
    return 1;
}

int32_t mx_std_math_frexp(mx_std_state_t *ls) {
    int32_t e = 0;
    double m = frexp(mx_std_checkfloat(ls, 1), &e);
    mx_std_pushfloat(ls, m);
    mx_std_pushinteger(ls, e);
    return 2;
}

int32_t mx_std_math_ldexp(mx_std_state_t *ls) {
    double m = mx_std_checkfloat(ls, 1);
    mx_std_pushfloat(ls, ldexp(m, (int32_t)mx_std_checkinteger(ls, 2)));
    return 1;
}

int32_t mx_std_math_tointeger(mx_std_state_t *ls) {
    mx_obj_int_t got = mx_std_tointeger(ls, 1);
    if (got.is_int) {
        mx_std_pushinteger(ls, got.value);
    } else {
        mx_std_checkany(ls, 1);
        mx_std_pushnil(ls);
    }
    return 1;
}

int32_t mx_std_math_type(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    mx_obj_t *v = mx_std_at(ls, 1);
    if (v != NULL && mx_obj_is_int(*v)) {
        mx_std_pushstring(ls, "integer");
    } else if (v != NULL && mx_obj_is_float(*v)) {
        mx_std_pushstring(ls, "float");
    } else {
        mx_std_pushnil(ls);
    }
    return 1;
}

int32_t mx_std_math_pick(mx_std_state_t *ls, bool want_max) {
    int32_t n = mx_std_top(ls);
    if (n == 0) {
        return mx_std_argerror(ls, 1, "value expected");
    }
    int32_t best = 1;
    for (int32_t i = 2; i <= n && !ls->vm->std_error.is_set; i++) {
        bool better;
        if (want_max) {
            better = mx_std_compare(ls, best, i, MX_OP_LT);
        } else {
            better = mx_std_compare(ls, i, best, MX_OP_LT);
        }
        if (better) {
            best = i;
        }
    }
    mx_std_pushvalue(ls, best);
    return 1;
}

int32_t mx_std_math_min(mx_std_state_t *ls) {
    return mx_std_math_pick(ls, false);
}

int32_t mx_std_math_max(mx_std_state_t *ls) {
    return mx_std_math_pick(ls, true);
}

uint64_t mx_std_rand_rotl(uint64_t x, uint32_t n) {
    return (x << n) | (x >> (64 - n));
}

uint64_t mx_std_rand_next(mx_t *vm) {
    uint64_t *s = mx_lua_of(vm)->rand.s;
    uint64_t out = mx_std_rand_rotl(s[1] * 5, 7) * 9;
    uint64_t shifted = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= shifted;
    s[3] = mx_std_rand_rotl(s[3], 45);
    return out;
}

void mx_std_rand_seed(mx_t *vm, uint64_t n1, uint64_t n2) {
    uint64_t *s = mx_lua_of(vm)->rand.s;
    s[0] = n1;
    s[1] = 0xff;
    s[2] = n2;
    s[3] = 0;
    for (uint32_t i = 0; i < 16; i++) {
        mx_std_rand_next(vm);
    }
}

uint64_t mx_std_rand_entropy(mx_t *vm) {
    uint64_t t = (uint64_t)time(NULL);
    uint64_t c = (uint64_t)clock();
    return t ^ (c << 32) ^ (uint64_t)(uintptr_t)vm;
}

uint64_t mx_std_rand_upto(mx_t *vm, uint64_t span) {
    uint64_t x = mx_std_rand_next(vm);
    if (span == UINT64_MAX) {
        return x;
    }
    uint64_t n = span + 1;
    unsigned __int128 m = (unsigned __int128)x * n;
    if ((uint64_t)m < n) {
        uint64_t reject = (0 - n) % n;
        while ((uint64_t)m < reject) {
            m = (unsigned __int128)mx_std_rand_next(vm) * n;
        }
    }
    return (uint64_t)(m >> 64);
}

int32_t mx_std_math_random(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    int64_t lo;
    int64_t hi;
    switch (mx_std_top(ls)) {
        case 0: {
            mx_std_pushfloat(ls, (double)(mx_std_rand_next(vm) >> 11) * 0x1.0p-53);
            return 1;
        }
        case 1: {
            lo = 1;
            hi = mx_std_checkinteger(ls, 1);
            if (hi == 0) {
                mx_std_pushinteger(ls, (int64_t)mx_std_rand_next(vm));
                return 1;
            }
            break;
        }
        case 2: {
            lo = mx_std_checkinteger(ls, 1);
            hi = mx_std_checkinteger(ls, 2);
            break;
        }
        default: {
            return mx_std_error(ls, "wrong number of arguments");
        }
    }
    if (lo > hi) {
        return mx_std_argerror(ls, 1, "interval is empty");
    }
    uint64_t off = mx_std_rand_upto(vm, (uint64_t)hi - (uint64_t)lo);
    mx_std_pushinteger(ls, (int64_t)((uint64_t)lo + off));
    return 1;
}

int32_t mx_std_math_randomseed(mx_std_state_t *ls) {
    uint64_t n1;
    uint64_t n2;
    if (mx_std_isnone(ls, 1)) {
        n1 = mx_std_rand_entropy(ls->vm);
        n2 = mx_std_rand_next(ls->vm);
    } else {
        n1 = (uint64_t)mx_std_checkinteger(ls, 1);
        n2 = (uint64_t)mx_std_optinteger(ls, 2, 0);
    }
    mx_std_rand_seed(ls->vm, n1, n2);
    mx_std_pushinteger(ls, (int64_t)n1);
    mx_std_pushinteger(ls, (int64_t)n2);
    return 2;
}

void mx_ffi_math_fmod(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_fmod, "fmod", args, (int32_t)nargs);
}

void mx_ffi_math_modf(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_modf, "modf", args, (int32_t)nargs);
}

void mx_ffi_math_ult(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_ult, "ult", args, (int32_t)nargs);
}

void mx_ffi_math_log(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_log, "log", args, (int32_t)nargs);
}

void mx_ffi_math_exp(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_exp, "exp", args, (int32_t)nargs);
}

void mx_ffi_math_deg(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_deg, "deg", args, (int32_t)nargs);
}

void mx_ffi_math_rad(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_rad, "rad", args, (int32_t)nargs);
}

void mx_ffi_math_sin(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_sin, "sin", args, (int32_t)nargs);
}

void mx_ffi_math_cos(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_cos, "cos", args, (int32_t)nargs);
}

void mx_ffi_math_tan(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_tan, "tan", args, (int32_t)nargs);
}

void mx_ffi_math_asin(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_asin, "asin", args, (int32_t)nargs);
}

void mx_ffi_math_acos(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_acos, "acos", args, (int32_t)nargs);
}

void mx_ffi_math_atan(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_atan, "atan", args, (int32_t)nargs);
}

void mx_ffi_math_frexp(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_frexp, "frexp", args, (int32_t)nargs);
}

void mx_ffi_math_ldexp(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_ldexp, "ldexp", args, (int32_t)nargs);
}

void mx_ffi_math_tointeger(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_tointeger, "tointeger", args, (int32_t)nargs);
}

void mx_ffi_math_type(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_type, "type", args, (int32_t)nargs);
}

void mx_ffi_math_random(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_random, "random", args, (int32_t)nargs);
}

void mx_ffi_math_randomseed(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_std_dispatch(vm, mx_std_math_randomseed, "randomseed", args, (int32_t)nargs);
}

void mx_ffi_math_round_result(mx_t *vm, mx_obj_t *args, double d) {
    if (mx_obj_float_fits_int(d)) {
        args[0] = mx_obj_of_int((int64_t)d);
    } else {
        args[0] = mx_obj_of_number(d);
    }
    vm->rets.len = 1;
}

void mx_ffi_math_sqrt(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs >= 1 && mx_obj_is_number(args[0])) {
        args[0] = mx_obj_of_number(sqrt(mx_obj_get_number(args[0])));
        vm->rets.len = 1;
    } else {
        mx_std_dispatch(vm, mx_std_math_sqrt, "sqrt", args, (int32_t)nargs);
    }
}

void mx_ffi_math_floor(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs >= 1 && mx_obj_is_int(args[0])) {
        vm->rets.len = 1;
    } else if (nargs >= 1 && mx_obj_is_float(args[0])) {
        mx_ffi_math_round_result(vm, args, floor(mx_obj_get_float(args[0])));
    } else {
        mx_std_dispatch(vm, mx_std_math_floor, "floor", args, (int32_t)nargs);
    }
}

void mx_ffi_math_ceil(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs >= 1 && mx_obj_is_int(args[0])) {
        vm->rets.len = 1;
    } else if (nargs >= 1 && mx_obj_is_float(args[0])) {
        mx_ffi_math_round_result(vm, args, ceil(mx_obj_get_float(args[0])));
    } else {
        mx_std_dispatch(vm, mx_std_math_ceil, "ceil", args, (int32_t)nargs);
    }
}

void mx_ffi_math_abs(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs >= 1 && mx_obj_is_int(args[0])) {
        uint64_t n = (uint64_t)mx_obj_get_int(args[0]);
        if ((int64_t)n < 0) {
            n = ~n + 1;
        }
        args[0] = mx_obj_of_int((int64_t)n);
        vm->rets.len = 1;
    } else if (nargs >= 1 && mx_obj_is_float(args[0])) {
        args[0] = mx_obj_of_number(fabs(mx_obj_get_float(args[0])));
        vm->rets.len = 1;
    } else {
        mx_std_dispatch(vm, mx_std_math_abs, "abs", args, (int32_t)nargs);
    }
}

void mx_ffi_math_max(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs == 2 && mx_obj_is_number(args[0]) && mx_obj_is_number(args[1])) {
        if (mx_obj_num_lt(args[0], args[1])) {
            args[0] = args[1];
        }
        vm->rets.len = 1;
    } else {
        mx_std_dispatch(vm, mx_std_math_max, "max", args, (int32_t)nargs);
    }
}

void mx_ffi_math_min(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs == 2 && mx_obj_is_number(args[0]) && mx_obj_is_number(args[1])) {
        if (mx_obj_num_lt(args[1], args[0])) {
            args[0] = args[1];
        }
        vm->rets.len = 1;
    } else {
        mx_std_dispatch(vm, mx_std_math_min, "min", args, (int32_t)nargs);
    }
}

const mx_std_reg_t mx_std_math_regs[] = {
    {"abs", mx_std_math_abs, mx_ffi_math_abs},
    {"ceil", mx_std_math_ceil, mx_ffi_math_ceil},
    {"floor", mx_std_math_floor, mx_ffi_math_floor},
    {"fmod", mx_std_math_fmod, mx_ffi_math_fmod},
    {"modf", mx_std_math_modf, mx_ffi_math_modf},
    {"sqrt", mx_std_math_sqrt, mx_ffi_math_sqrt},
    {"ult", mx_std_math_ult, mx_ffi_math_ult},
    {"log", mx_std_math_log, mx_ffi_math_log},
    {"exp", mx_std_math_exp, mx_ffi_math_exp},
    {"deg", mx_std_math_deg, mx_ffi_math_deg},
    {"rad", mx_std_math_rad, mx_ffi_math_rad},
    {"sin", mx_std_math_sin, mx_ffi_math_sin},
    {"cos", mx_std_math_cos, mx_ffi_math_cos},
    {"tan", mx_std_math_tan, mx_ffi_math_tan},
    {"asin", mx_std_math_asin, mx_ffi_math_asin},
    {"acos", mx_std_math_acos, mx_ffi_math_acos},
    {"atan", mx_std_math_atan, mx_ffi_math_atan},
    {"frexp", mx_std_math_frexp, mx_ffi_math_frexp},
    {"ldexp", mx_std_math_ldexp, mx_ffi_math_ldexp},
    {"tointeger", mx_std_math_tointeger, mx_ffi_math_tointeger},
    {"type", mx_std_math_type, mx_ffi_math_type},
    {"min", mx_std_math_min, mx_ffi_math_min},
    {"max", mx_std_math_max, mx_ffi_math_max},
    {"random", mx_std_math_random, mx_ffi_math_random},
    {"randomseed", mx_std_math_randomseed, mx_ffi_math_randomseed},
    {NULL, NULL, NULL},
};

void mx_std_open_math(mx_std_state_t *ls) {
    mx_std_newlib(ls, "math", mx_std_math_regs);
    mx_std_pushfloat(ls, 3.141592653589793);
    mx_std_setfield(ls, -2, "pi");
    mx_std_pushfloat(ls, (double)HUGE_VAL);
    mx_std_setfield(ls, -2, "huge");
    mx_std_pushinteger(ls, INT64_MAX);
    mx_std_setfield(ls, -2, "maxinteger");
    mx_std_pushinteger(ls, INT64_MIN);
    mx_std_setfield(ls, -2, "mininteger");
    mx_std_pop(ls, 1);
    mx_std_rand_seed(ls->vm, mx_std_rand_entropy(ls->vm), 0);
}
