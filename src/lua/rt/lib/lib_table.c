
#include "../api/std.h"
#include "../lua.h"
#include "../meta.h"
#include "../../../vm/gc.h"
#include "../../../value/error.h"
#include "../../../value/kind.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"
#include "../../../value/table.h"

bool mx_std_table_proxies(mx_std_state_t *ls, mx_std_slot_t arg, mx_std_table_access_t need) {
    if (!mx_std_getmetatable(ls, arg)) {
        return false;
    } else {
        mx_obj_table_t *meta = mx_obj_get_table(*mx_std_at(ls, -1));
        mx_obj_t *keys = mx_lua_of(ls->vm)->meta_keys;
        bool has_index = !mx_obj_is_nil(mx_table_get(meta, keys[MX_LUA_META_KEY_INDEX]));
        bool has_newindex = !mx_obj_is_nil(mx_table_get(meta, keys[MX_LUA_META_KEY_NEWINDEX]));
        bool has_len = !mx_obj_is_nil(mx_table_get(meta, keys[MX_LUA_META_KEY_LEN]));
        mx_std_pop(ls, 1);
        return ((need & MX_STD_TABLE_READ) == 0 || has_index)
            && ((need & MX_STD_TABLE_WRITE) == 0 || has_newindex)
            && ((need & MX_STD_TABLE_LEN) == 0 || has_len);
    }
}

void mx_std_table_check(mx_std_state_t *ls, mx_std_slot_t arg, mx_std_table_access_t need) {
    mx_std_table_access_t wanted;
    if (mx_std_type(ls, arg) == MX_LUA_TYPE_STRING) {
        wanted = need & (mx_std_table_access_t)~MX_STD_TABLE_LEN;
    } else {
        wanted = need;
    }
    if (!mx_std_istable(ls, arg) && !mx_std_table_proxies(ls, arg, wanted)) {
        mx_std_typeerror(ls, arg, "table");
    }
}

int32_t mx_std_table_insert(mx_std_state_t *ls) {
    mx_std_table_check(ls, 1, MX_STD_TABLE_READ | MX_STD_TABLE_WRITE | MX_STD_TABLE_LEN);
    int64_t e = mx_std_len_meta(ls, 1) + 1;
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    int64_t pos;
    switch (mx_std_top(ls)) {
        case 2: {
            pos = e;
            break;
        }
        case 3: {
            pos = mx_std_checkinteger(ls, 2);
            if (ls->vm->std_error.is_set) {
                return 0;
            } else if ((uint64_t)pos - (uint64_t)1 >= (uint64_t)e) {
                return mx_std_argerror(ls, 2, "position out of bounds");
            }
            for (int64_t i = e; i > pos && !ls->vm->std_error.is_set; i--) {
                mx_std_geti(ls, 1, i - 1);
                mx_std_seti(ls, 1, i);
            }
            break;
        }
        default: {
            return mx_std_error(ls, "wrong number of arguments to 'insert'");
        }
    }
    mx_std_seti(ls, 1, pos);
    return 0;
}

int32_t mx_std_table_remove(mx_std_state_t *ls) {
    mx_std_table_check(ls, 1, MX_STD_TABLE_READ | MX_STD_TABLE_WRITE | MX_STD_TABLE_LEN);
    int64_t size = mx_std_len_meta(ls, 1);
    int64_t pos = mx_std_optinteger(ls, 2, size);
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (pos != size && (uint64_t)pos - (uint64_t)1 >= (uint64_t)size + (uint64_t)1) {
        return mx_std_argerror(ls, 2, "position out of bounds");
    }
    mx_std_geti(ls, 1, pos);
    for (; pos < size && !ls->vm->std_error.is_set; pos++) {
        mx_std_geti(ls, 1, pos + 1);
        mx_std_seti(ls, 1, pos);
    }
    mx_std_pushnil(ls);
    mx_std_seti(ls, 1, pos);
    return 1;
}

int32_t mx_std_table_concat(mx_std_state_t *ls) {
    mx_std_table_check(ls, 1, MX_STD_TABLE_READ | MX_STD_TABLE_LEN);
    mx_std_str_t sep = mx_std_optstring(ls, 2, "");
    int64_t i = mx_std_optinteger(ls, 3, 1);
    int64_t last = mx_std_optinteger(ls, 4, mx_std_len_meta(ls, 1));
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_std_buf_t buf;
    mx_std_buf_init(ls, &buf);
    bool is_more = i <= last;
    while (is_more && !ls->vm->std_error.is_set) {
        mx_std_geti(ls, 1, i);
        if (!mx_std_isstring(ls, -1)) {
            mx_std_error(ls, "invalid value (%s) at index %lld in table for 'concat'", mx_std_typename(ls, mx_std_type(ls, -1)), (long long)i);
        } else {
            mx_std_buf_addvalue(&buf, -1);
        }
        mx_std_pop(ls, 1);
        is_more = i != last;
        if (is_more) {
            mx_std_buf_addlstring(&buf, sep.ptr, sep.len);
            i += 1;
        }
    }
    mx_std_buf_push(&buf);
    return 1;
}

int32_t mx_std_table_unpack(mx_std_state_t *ls) {
    mx_std_table_check(ls, 1, MX_STD_TABLE_READ | MX_STD_TABLE_LEN);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    int64_t len = mx_std_len_meta(ls, 1);
    int64_t i = mx_std_optinteger(ls, 2, 1);
    int64_t e = mx_std_optinteger(ls, 3, len);
    if (ls->vm->std_error.is_set || i > e) {
        return 0;
    }
    uint64_t n = (uint64_t)e - (uint64_t)i;
    if (n >= INT32_MAX || !mx_std_checkstack(ls, (int32_t)n + 1)) {
        return mx_std_error(ls, "too many results to unpack");
    }
    for (; i < e && !ls->vm->std_error.is_set; i++) {
        mx_std_geti(ls, 1, i);
    }
    mx_std_geti(ls, 1, e);
    return (int32_t)n + 1;
}

int32_t mx_std_table_pack(mx_std_state_t *ls) {
    int32_t n = mx_std_top(ls);
    mx_std_push(ls, mx_obj_of_table(mx_table_new_pack(ls->vm, mx_std_slots(ls), (uint32_t)n)));
    return 1;
}

int32_t mx_std_table_move(mx_std_state_t *ls) {
    int64_t f = mx_std_checkinteger(ls, 2);
    int64_t e = mx_std_checkinteger(ls, 3);
    int64_t t = mx_std_checkinteger(ls, 4);
    int32_t tt = 1;
    if (!mx_std_isnoneornil(ls, 5)) {
        tt = 5;
    }
    mx_std_table_check(ls, 1, MX_STD_TABLE_READ);
    mx_std_table_check(ls, tt, MX_STD_TABLE_WRITE);
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (e >= f) {
        if (!(f > 0 || e < INT64_MAX + f)) {
            return mx_std_argerror(ls, 3, "too many elements to move");
        } else if (!(t <= INT64_MAX - (e - f))) {
            return mx_std_argerror(ls, 4, "destination wrap around");
        } else if (t > e || t <= f || (tt != 1 && !mx_std_rawequal(ls, 1, tt))) {
            for (int64_t i = 0; i <= e - f && !ls->vm->std_error.is_set; i++) {
                mx_std_geti(ls, 1, f + i);
                mx_std_seti(ls, tt, t + i);
            }
        } else {
            for (int64_t i = e - f; i >= 0 && !ls->vm->std_error.is_set; i--) {
                mx_std_geti(ls, 1, f + i);
                mx_std_seti(ls, tt, t + i);
            }
        }
    }
    mx_std_pushvalue(ls, tt);
    return 1;
}

bool mx_std_sort_lt(mx_std_state_t *ls, mx_obj_t cmp, mx_obj_t a, mx_obj_t b) {
    mx_t *vm = ls->vm;
    mx_obj_t got;
    if (vm->std_error.is_set) {
        return false;
    } else if (!mx_obj_is_nil(cmp)) {
        mx_obj_t args[2] = {a, b};
        got = mx_call(vm, cmp, 2, args);
    } else if (mx_obj_is_number(a) && mx_obj_is_number(b)) {
        return mx_obj_num_lt(a, b);
    } else if (mx_obj_is_string(a) && mx_obj_is_string(b)) {
        return mx_obj_str_cmp(mx_obj_get_string(a), mx_obj_get_string(b)) < 0;
    } else {
        got = mx_meta_compare(vm, MX_OP_LT, a, b);
    }
    if (mx_obj_is_error(got)) {
        mx_error_locate(vm, mx_obj_get_error(got), NULL);
        ls->error = got;
        mx_std_raise(ls);
        return false;
    } else {
        return mx_obj_is_truthy(got);
    }
}

bool mx_std_sort_lt_at(mx_std_state_t *ls, mx_obj_t cmp, uint32_t base, int64_t i, int64_t j) {
    mx_obj_t *a = ls->vm->holds.ptr + base;
    return mx_std_sort_lt(ls, cmp, a[i], a[j]);
}

void mx_std_sort_swap(mx_t *vm, uint32_t base, int64_t i, int64_t j) {
    mx_obj_t *a = vm->holds.ptr + base;
    mx_obj_t tmp = a[i];
    a[i] = a[j];
    a[j] = tmp;
}

void mx_std_sort_range(mx_std_state_t *ls, mx_obj_t cmp, uint32_t base, int64_t lo, int64_t hi) {
    mx_t *vm = ls->vm;
    while (hi - lo >= 3 && !vm->std_error.is_set) {
        int64_t mid = lo + (hi - lo) / 2;
        if (mx_std_sort_lt_at(ls, cmp, base, mid, lo)) {
            mx_std_sort_swap(vm, base, mid, lo);
        }
        if (mx_std_sort_lt_at(ls, cmp, base, hi, mid)) {
            mx_std_sort_swap(vm, base, hi, mid);
            if (mx_std_sort_lt_at(ls, cmp, base, mid, lo)) {
                mx_std_sort_swap(vm, base, mid, lo);
            }
        }
        int64_t piv = hi - 1;
        mx_std_sort_swap(vm, base, mid, piv);
        int64_t i = lo;
        int64_t j = piv;
        while (!vm->std_error.is_set) {
            i += 1;
            while (mx_std_sort_lt_at(ls, cmp, base, i, piv)) {
                if (i >= piv) {
                    mx_std_error(ls, "invalid order function for sorting");
                    return;
                }
                i += 1;
            }
            j -= 1;
            while (mx_std_sort_lt_at(ls, cmp, base, piv, j)) {
                if (j <= lo) {
                    mx_std_error(ls, "invalid order function for sorting");
                    return;
                }
                j -= 1;
            }
            if (i >= j) {
                break;
            }
            mx_std_sort_swap(vm, base, i, j);
        }
        mx_std_sort_swap(vm, base, i, piv);
        if (i - lo < hi - i) {
            mx_std_sort_range(ls, cmp, base, lo, i - 1);
            lo = i + 1;
        } else {
            mx_std_sort_range(ls, cmp, base, i + 1, hi);
            hi = i - 1;
        }
    }
    for (int64_t k = lo + 1; k <= hi && !vm->std_error.is_set; k++) {
        for (int64_t m = k; m > lo && mx_std_sort_lt_at(ls, cmp, base, m, m - 1); m--) {
            mx_std_sort_swap(vm, base, m, m - 1);
        }
    }
}

int32_t mx_std_table_sort(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_std_table_check(ls, 1, MX_STD_TABLE_READ | MX_STD_TABLE_WRITE | MX_STD_TABLE_LEN);
    int64_t n = mx_std_len_meta(ls, 1);
    if (vm->std_error.is_set || n <= 1) {
        return 0;
    } else if (n >= INT32_MAX) {
        return mx_std_argerror(ls, 1, "array too big");
    }
    mx_obj_t cmp = mx_obj_of_nil();
    if (!mx_std_isnoneornil(ls, 2)) {
        mx_std_checktype(ls, 2, MX_LUA_TYPE_FUNCTION);
        if (vm->std_error.is_set) {
            return 0;
        }
        cmp = *mx_std_at(ls, 2);
    }
    mx_std_settop(ls, 2);
    uint32_t base = vm->holds.len;
    for (int64_t i = 1; i <= n && !vm->std_error.is_set; i++) {
        mx_std_geti(ls, 1, i);
        mx_vm_hold(vm, *mx_std_at(ls, -1));
        mx_std_pop(ls, 1);
    }
    if (!vm->std_error.is_set) {
        mx_std_sort_range(ls, cmp, base, 0, n - 1);
    }
    for (int64_t i = 1; i <= n && !vm->std_error.is_set; i++) {
        mx_std_push(ls, vm->holds.ptr[base + i - 1]);
        mx_std_seti(ls, 1, i);
    }
    vm->holds.len = base;
    return 0;
}

int32_t mx_std_table_create(mx_std_state_t *ls) {
    int64_t narr = mx_std_checkinteger(ls, 1);
    int64_t nhash = mx_std_optinteger(ls, 2, 0);
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (narr < 0 || narr > INT32_MAX) {
        return mx_std_argerror(ls, 1, "out of range");
    } else if (nhash < 0 || nhash > INT32_MAX) {
        return mx_std_argerror(ls, 2, "out of range");
    } else if (nhash > (int64_t)1 << 30) {
        return mx_std_error_noloc(ls, "table overflow");
    } else if (!mx_mem_fits((size_t)narr * sizeof(mx_obj_t) + (size_t)nhash * 3 * sizeof(mx_table_entry_t))) {
        return mx_std_error_noloc(ls, "not enough memory");
    } else {
        mx_std_createtable(ls, (uint32_t)narr, (uint32_t)nhash);
        return 1;
    }
}

int32_t mx_std_table_keys(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_TABLE);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_std_newtable(ls);
    int64_t n = 0;
    mx_std_pushnil(ls);
    while (mx_std_next(ls, 1)) {
        mx_std_pop(ls, 1);
        mx_std_pushvalue(ls, -1);
        n += 1;
        mx_std_seti(ls, 2, n);
    }
    return 1;
}

int32_t mx_std_table_values(mx_std_state_t *ls) {
    mx_std_checktype(ls, 1, MX_LUA_TYPE_TABLE);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_std_newtable(ls);
    int64_t n = 0;
    mx_std_pushnil(ls);
    while (mx_std_next(ls, 1)) {
        n += 1;
        mx_std_seti(ls, 2, n);
    }
    return 1;
}

bool mx_ffi_table_append_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs != 2 || mx_obj_kind(args[0]) != vm->kinds.table) {
        return false;
    }
    mx_obj_table_t *t = mx_obj_get_table(args[0]);
    if (!t->is_plain || t->meta != NULL) {
        return false;
    }
    mx_table_set(vm, t, mx_obj_of_int((int64_t)t->len + 1), args[1]);
    args[0] = mx_obj_of_nil();
    vm->rets.len = 0;
    return true;
}

bool mx_ffi_table_insert_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (mx_ffi_table_append_fast(vm, nargs, args)) {
        return true;
    } else if (nargs != 3 || mx_obj_kind(args[0]) != vm->kinds.table || !mx_obj_is_int(args[1])) {
        return false;
    }
    mx_obj_table_t *t = mx_obj_get_table(args[0]);
    int64_t pos = mx_obj_get_int(args[1]);
    if (!t->is_plain || t->meta != NULL || t->len != t->array_len || pos < 1 || pos > (int64_t)t->len + 1 || t->used != 0) {
        return false;
    }
    mx_table_array_push(vm, t, args[2]);
    t->len += 1;
    if (pos <= (int64_t)t->array_len - 1) {
        memmove(&t->array[pos], &t->array[pos - 1], sizeof(mx_obj_t) * (size_t)(t->array_len - (uint32_t)pos));
        t->array[pos - 1] = args[2];
    }
    args[0] = mx_obj_of_nil();
    vm->rets.len = 0;
    return true;
}

void mx_ffi_table_insert(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_table_insert_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_table_insert, "insert", args, (int32_t)nargs);
    }
}

bool mx_ffi_table_concat_fast(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (nargs < 1 || nargs > 2 || mx_obj_kind(args[0]) != vm->kinds.table || (nargs == 2 && !mx_obj_is_string(args[1]))) {
        return false;
    }
    mx_obj_table_t *t = mx_obj_get_table(args[0]);
    if (!t->is_plain || t->meta != NULL || t->len > t->array_len) {
        return false;
    }
    mx_str_t *sep = NULL;
    uint64_t seplen = 0;
    if (nargs == 2) {
        sep = mx_obj_get_string(args[1]);
        seplen = sep->len;
    }
    uint32_t n = t->len;
    uint64_t total = 0;
    for (uint32_t k = 0; k < n; k++) {
        mx_obj_t v = t->array[k];
        if (mx_obj_is_string(v)) {
            total += mx_obj_get_string(v)->len;
        } else if (mx_obj_is_int(v)) {
            char tmp[MX_STR_INT_DIGITS];
            total += MX_STR_INT_DIGITS - mx_str_int_digits(tmp, mx_obj_get_int(v));
        } else {
            return false;
        }
    }
    if (n > 0) {
        total += seplen * (n - 1);
    }
    if (total > MX_STD_MAX_STRING || !mx_mem_fits(sizeof(mx_str_t) + (size_t)total + 1)) {
        return false;
    }
    mx_str_t *out = mx_str_new_gc_sized(vm, (uint32_t)total);
    for (uint32_t k = 0; k < n; k++) {
        mx_obj_t v = t->array[k];
        if (mx_obj_is_string(v)) {
            mx_str_t *s = mx_obj_get_string(v);
            mx_str_append(out, s->buf, s->len);
        } else {
            mx_str_append_int(out, mx_obj_get_int(v));
        }
        if (seplen > 0 && k + 1 < n) {
            mx_str_append(out, sep->buf, (uint32_t)seplen);
        }
    }
    args[0] = mx_obj_of_string(out);
    vm->rets.len = 1;
    return true;
}

void mx_ffi_table_concat(mx_t *vm, size_t nargs, mx_obj_t *args) {
    if (!mx_ffi_table_concat_fast(vm, nargs, args)) {
        mx_std_dispatch(vm, mx_std_table_concat, "concat", args, (int32_t)nargs);
    }
}

const mx_std_reg_t mx_std_table_regs[] = {
    {"concat", mx_std_table_concat, mx_ffi_table_concat},
    {"insert", mx_std_table_insert, mx_ffi_table_insert},
    {"move", mx_std_table_move, NULL},
    {"pack", mx_std_table_pack, NULL},
    {"remove", mx_std_table_remove, NULL},
    {"sort", mx_std_table_sort, NULL},
    {"unpack", mx_std_table_unpack, NULL},
    {"create", mx_std_table_create, NULL},
    {"keys", mx_std_table_keys, NULL},
    {"values", mx_std_table_values, NULL},
    {NULL, NULL, NULL},
};

void mx_std_open_table(mx_std_state_t *ls) {
    mx_std_newlib(ls, "table", mx_std_table_regs);
    mx_std_pop(ls, 1);
}
