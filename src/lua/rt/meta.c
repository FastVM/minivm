
#include "meta.h"
#include "lua.h"
#include "api/std.h"
#include "../../run/run.h"
#include "../../vm/gc.h"
#include "../../value/error.h"
#include "../../value/kind.h"
#include "../../value/num.h"
#include "../../value/obj.h"
#include "../../value/string.h"
#include "../../value/table.h"

mx_lua_type_t mx_meta_data_type(mx_t *vm, mx_kind_t kind) {
    if (kind == vm->kinds.table) {
        return MX_LUA_TYPE_TABLE;
    } else if (kind == vm->kinds.cfn) {
        return MX_LUA_TYPE_FUNCTION;
    } else if (kind == vm->kinds.thread) {
        return MX_LUA_TYPE_THREAD;
    } else {
        return MX_LUA_TYPE_USERDATA;
    }
}

mx_lua_type_t mx_meta_basic_type(mx_t *vm, mx_obj_t v) {
    mx_lua_type_t type = mx_meta_lua_type(vm, v);
    if (type == MX_LUA_TYPE_FLOAT) {
        return MX_LUA_TYPE_INTEGER;
    } else {
        return type;
    }
}

const char *mx_meta_lua_typename(mx_lua_type_t type) {
    switch (type) {
        case MX_LUA_TYPE_NONE: {
            return "no value";
        }
        case MX_LUA_TYPE_NIL: {
            return "nil";
        }
        case MX_LUA_TYPE_BOOLEAN: {
            return "boolean";
        }
        case MX_LUA_TYPE_INTEGER:
        case MX_LUA_TYPE_FLOAT: {
            return "number";
        }
        case MX_LUA_TYPE_STRING: {
            return "string";
        }
        case MX_LUA_TYPE_TABLE: {
            return "table";
        }
        case MX_LUA_TYPE_FUNCTION: {
            return "function";
        }
        case MX_LUA_TYPE_THREAD: {
            return "thread";
        }
        default: {
            return "userdata";
        }
    }
}

const char *mx_meta_typename(mx_t *vm, mx_obj_t v) {
    mx_lua_type_t type = mx_meta_lua_type(vm, v);
    bool is_full = type == MX_LUA_TYPE_TABLE || (type == MX_LUA_TYPE_USERDATA && !mx_obj_is_lightuserdata(v));
    mx_obj_t name;
    if (is_full) {
        name = mx_meta_handler_of(vm, v, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_NAME]);
    } else {
        name = mx_obj_of_nil();
    }
    if (mx_obj_is_string(name)) {
        return mx_obj_get_string(name)->buf;
    } else if (type == MX_LUA_TYPE_USERDATA && !mx_obj_is_lightuserdata(v)) {
        return mx_kind_name(vm, v.kind);
    } else {
        return mx_meta_lua_typename(type);
    }
}

void mx_meta_render(mx_t *vm, mx_str_t *buf, mx_obj_t v) {
    if (mx_obj_is_int(v)) {
        mx_str_append_int(buf, mx_obj_get_int(v));
    } else if (mx_obj_is_float(v)) {
        mx_str_float(buf, mx_obj_get_float(v));
    } else if (mx_obj_is_string(v)) {
        mx_str_t *src = mx_obj_get_string(v);
        mx_str_append(buf, src->buf, src->len);
    } else if (mx_obj_is_nil(v)) {
        mx_str_append(buf, "nil", 3);
    } else if (mx_obj_is_boolean(v)) {
        if (mx_obj_get_boolean(v)) {
            mx_str_append(buf, "true", 4);
        } else {
            mx_str_append(buf, "false", 5);
        }
    } else if (mx_obj_is_ffi(v)) {
        mx_str_format(buf, "builtin: %p", mx_ref_obj(v.val.ref));
    } else {
        mx_str_format(buf, "%s: %p", mx_meta_typename(vm, v), mx_ref_obj(v.val.ref));
    }
}

mx_obj_t mx_meta_typed_error(mx_t *vm, const char *fmt, mx_obj_t bad) {
    return mx_error_raw_operand(vm, MX_ERROR_OPERAND_LHS, fmt, mx_meta_typename(vm, bad));
}

mx_obj_t mx_meta_operand_error(mx_t *vm, const char *fmt, mx_obj_t a, mx_obj_t b, bool is_lhs_fine) {
    if (is_lhs_fine) {
        return mx_error_raw_operand(vm, MX_ERROR_OPERAND_RHS, fmt, mx_meta_typename(vm, b));
    } else {
        return mx_error_raw_operand(vm, MX_ERROR_OPERAND_LHS, fmt, mx_meta_typename(vm, a));
    }
}

mx_obj_table_t *mx_meta_table_of(mx_t *vm, mx_obj_t v) {
    mx_lua_t *lua = mx_lua_of(vm);
    if (mx_obj_is_table(v)) {
        return mx_obj_get_table(v)->meta;
    } else if (mx_obj_is_error(v) || mx_obj_is_block(v)) {
        return NULL;
    } else {
        mx_obj_t meta = lua->type_metas[mx_meta_basic_type(vm, v)];
        if (mx_obj_is_table(meta)) {
            return mx_obj_get_table(meta);
        } else {
            return NULL;
        }
    }
}

mx_obj_t mx_meta_handler_of(mx_t *vm, mx_obj_t v, mx_obj_t key) {
    mx_obj_table_t *meta = mx_meta_table_of(vm, v);
    if (meta != NULL) {
        return mx_table_get(meta, key);
    } else {
        return mx_obj_of_nil();
    }
}

mx_obj_num_t mx_meta_coerce_num(mx_t *vm, mx_obj_t v) {
    if (mx_obj_is_number(v)) {
        return (mx_obj_num_t) { .value = v, .is_num = true };
    } else if (mx_obj_is_string(v)) {
        mx_str_t *s = mx_obj_get_string(v);
        return mx_std_str2num(s->buf, s->len);
    } else {
        return (mx_obj_num_t) { .value = mx_obj_of_nil(), .is_num = false };
    }
}

mx_obj_t mx_meta_arith_core(mx_t *vm, mx_op_t op, mx_obj_t a, mx_obj_t b) {
    if (op >= MX_OP_BAND && op <= MX_OP_SHR) {
        int64_t x = mx_obj_get_int(a);
        int64_t y = mx_obj_get_int(b);
        switch (op) {
            case MX_OP_BAND: {
                return mx_obj_of_int(x & y);
            }
            case MX_OP_BOR: {
                return mx_obj_of_int(x | y);
            }
            case MX_OP_BXOR: {
                return mx_obj_of_int(x ^ y);
            }
            case MX_OP_SHL: {
                return mx_obj_of_int(mx_num_shl(x, y));
            }
            default: {
                return mx_obj_of_int(mx_num_shr(x, y));
            }
        }
    }
    bool ints = mx_obj_is_int(a) && mx_obj_is_int(b);
    switch (op) {
        case MX_OP_ADD: {
            if (ints) {
                return mx_obj_of_int(mx_obj_get_int(a) + mx_obj_get_int(b));
            } else {
                return mx_obj_of_number(mx_obj_get_number(a) + mx_obj_get_number(b));
            }
        }
        case MX_OP_SUB: {
            if (ints) {
                return mx_obj_of_int(mx_obj_get_int(a) - mx_obj_get_int(b));
            } else {
                return mx_obj_of_number(mx_obj_get_number(a) - mx_obj_get_number(b));
            }
        }
        case MX_OP_MUL: {
            if (ints) {
                return mx_obj_of_int(mx_obj_get_int(a) * mx_obj_get_int(b));
            } else {
                return mx_obj_of_number(mx_obj_get_number(a) * mx_obj_get_number(b));
            }
        }
        case MX_OP_DIV: {
            return mx_obj_of_number(mx_obj_get_number(a) / mx_obj_get_number(b));
        }
        case MX_OP_IDIV: {
            if (!ints) {
                return mx_obj_of_number(floor(mx_obj_get_number(a) / mx_obj_get_number(b)));
            } else if (mx_obj_get_int(b) == 0) {
                return mx_error_raw(vm, "attempt to divide by zero");
            } else {
                return mx_obj_of_int(mx_num_idiv(mx_obj_get_int(a), mx_obj_get_int(b)));
            }
        }
        case MX_OP_MOD: {
            if (!ints) {
                return mx_obj_of_number(mx_num_fmod(mx_obj_get_number(a), mx_obj_get_number(b)));
            } else if (mx_obj_get_int(b) == 0) {
                return mx_error_raw(vm, "attempt to perform 'n%%0'");
            } else {
                return mx_obj_of_int(mx_num_imod(mx_obj_get_int(a), mx_obj_get_int(b)));
            }
        }
        default: {
            return mx_obj_of_number(pow(mx_obj_get_number(a), mx_obj_get_number(b)));
        }
    }
}

bool mx_meta_is_default_op(mx_obj_t handler, mx_op_t op) {
    mx_ffi_t *const defaults[] = {
        [MX_OP_ADD] = mx_meta_op_add,
        [MX_OP_SUB] = mx_meta_op_sub,
        [MX_OP_MUL] = mx_meta_op_mul,
        [MX_OP_DIV] = mx_meta_op_div,
        [MX_OP_IDIV] = mx_meta_op_idiv,
        [MX_OP_MOD] = mx_meta_op_mod,
        [MX_OP_POW] = mx_meta_op_pow,
        [MX_OP_CONCAT] = mx_meta_op_concat,
        [MX_OP_BAND] = mx_meta_op_band,
        [MX_OP_BOR] = mx_meta_op_bor,
        [MX_OP_BXOR] = mx_meta_op_bxor,
        [MX_OP_SHL] = mx_meta_op_shl,
        [MX_OP_SHR] = mx_meta_op_shr,
    };
    return mx_obj_is_ffi(handler) && op <= MX_OP_SHR && mx_obj_get_ffi(handler) == defaults[op];
}

mx_obj_t mx_meta_arith_event(mx_t *vm, mx_op_t op, mx_obj_t a, mx_obj_t b) {
    mx_lua_t *lua = mx_lua_of(vm);
    mx_obj_t key = lua->meta_keys[MX_LUA_META_KEY_ARITH_BASE + (op - MX_OP_ADD)];
    mx_obj_t handler = mx_meta_handler_of(vm, a, key);
    if (mx_obj_is_nil(handler) || mx_meta_is_default_op(handler, op)) {
        handler = mx_meta_handler_of(vm, b, key);
    }
    if (!mx_obj_is_nil(handler) && !mx_meta_is_default_op(handler, op) && mx_meta_lua_type(vm, handler) != MX_LUA_TYPE_FUNCTION && mx_obj_is_nil(mx_meta_handler_of(vm, handler, lua->meta_keys[MX_LUA_META_KEY_CALL]))) {
        return mx_error_raw(vm, "attempt to call a %s value (metamethod '%s')", mx_meta_typename(vm, handler), mx_op_names[op]);
    } else if (!mx_obj_is_nil(handler) && !mx_meta_is_default_op(handler, op)) {
        mx_obj_t pair[2] = {a, b};
        return mx_call_meta(vm, handler, 2, pair);
    } else if (op == MX_OP_CONCAT) {
        return mx_meta_operand_error(vm, "attempt to concatenate a %s value", a, b, mx_obj_is_string(a) || mx_obj_is_number(a));
    } else if (op >= MX_OP_BAND && op <= MX_OP_SHR) {
        if (mx_obj_is_number(a) && mx_obj_is_number(b)) {
            mx_error_operand_t operand;
            if (mx_obj_to_int(a).is_int) {
                operand = MX_ERROR_OPERAND_RHS;
            } else {
                operand = MX_ERROR_OPERAND_LHS;
            }
            mx_obj_t err = mx_error_raw_operand(vm, operand, "number has no integer representation");
            mx_obj_get_error(err)->is_hint_inline = true;
            return err;
        } else {
            return mx_meta_operand_error(vm, "attempt to perform bitwise operation on a %s value", a, b, mx_obj_is_number(a));
        }
    } else if (mx_obj_is_string(a) || mx_obj_is_string(b)) {
        return mx_error_raw(vm, "attempt to %s a '%s' with a '%s'", mx_op_names[op], mx_meta_typename(vm, a), mx_meta_typename(vm, b));
    } else {
        return mx_meta_operand_error(vm, "attempt to perform arithmetic on a %s value", a, b, mx_obj_is_number(a));
    }
}

mx_obj_t mx_meta_arith(mx_t *vm, mx_op_t op, mx_obj_t a, mx_obj_t b) {
    if (op >= MX_OP_BAND && op <= MX_OP_SHR) {
        mx_obj_int_t ia = mx_obj_to_int(a);
        mx_obj_int_t ib = mx_obj_to_int(b);
        if (ia.is_int && ib.is_int) {
            return mx_meta_arith_core(vm, op, mx_obj_of_int(ia.value), mx_obj_of_int(ib.value));
        } else {
            return mx_meta_arith_event(vm, op, a, b);
        }
    }
    mx_obj_num_t na = mx_meta_coerce_num(vm, a);
    mx_obj_num_t nb = mx_meta_coerce_num(vm, b);
    if (na.is_num && nb.is_num) {
        return mx_meta_arith_core(vm, op, na.value, nb.value);
    } else {
        return mx_meta_arith_event(vm, op, a, b);
    }
}

mx_obj_t mx_meta_cmp_fail(mx_t *vm, mx_obj_t a, mx_obj_t b) {
    const char *ta = mx_meta_typename(vm, a);
    const char *tb = mx_meta_typename(vm, b);
    if (strcmp(ta, tb) == 0) {
        return mx_error_raw(vm, "attempt to compare two %s values", ta);
    } else {
        return mx_error_raw(vm, "attempt to compare %s with %s", ta, tb);
    }
}

mx_obj_t mx_meta_compare(mx_t *vm, mx_op_t op, mx_obj_t a, mx_obj_t b) {
    mx_lua_t *lua = mx_lua_of(vm);
    if (op == MX_OP_EQ) {
        if (mx_table_key_eq(a, b)) {
            return mx_obj_of_boolean(true);
        }
        bool is_same_class = a.kind == b.kind && mx_obj_is_table(a) && mx_obj_is_table(b);
        mx_obj_t handler = mx_obj_of_nil();
        if (is_same_class) {
            mx_obj_t key = lua->meta_keys[MX_LUA_META_KEY_EQ];
            handler = mx_meta_handler_of(vm, a, key);
            if (mx_obj_is_nil(handler)) {
                handler = mx_meta_handler_of(vm, b, key);
            }
        }
        if (mx_obj_is_nil(handler)) {
            return mx_obj_of_boolean(false);
        } else {
            mx_obj_t pair[2] = {a, b};
            return mx_call_meta(vm, handler, 2, pair);
        }
    }
    bool is_lt = op == MX_OP_LT;
    if (mx_obj_is_number(a) && mx_obj_is_number(b)) {
        if (is_lt) {
            return mx_obj_of_boolean(mx_obj_num_lt(a, b));
        } else {
            return mx_obj_of_boolean(mx_obj_num_le(a, b));
        }
    } else if (mx_obj_is_string(a) && mx_obj_is_string(b)) {
        int32_t c = mx_obj_str_cmp(mx_obj_get_string(a), mx_obj_get_string(b));
        if (is_lt) {
            return mx_obj_of_boolean(c < 0);
        } else {
            return mx_obj_of_boolean(c <= 0);
        }
    }
    mx_obj_t key;
    if (is_lt) {
        key = lua->meta_keys[MX_LUA_META_KEY_LT];
    } else {
        key = lua->meta_keys[MX_LUA_META_KEY_LE];
    }
    mx_obj_t handler = mx_meta_handler_of(vm, a, key);
    if (mx_obj_is_nil(handler)) {
        handler = mx_meta_handler_of(vm, b, key);
    }
    if (mx_obj_is_nil(handler)) {
        return mx_meta_cmp_fail(vm, a, b);
    } else if (mx_meta_lua_type(vm, handler) != MX_LUA_TYPE_FUNCTION && mx_obj_is_nil(mx_meta_handler_of(vm, handler, lua->meta_keys[MX_LUA_META_KEY_CALL]))) {
        return mx_error_raw(vm, "attempt to call a %s value (metamethod '%s')", mx_meta_typename(vm, handler), mx_op_names[op]);
    } else {
        mx_obj_t pair[2] = {a, b};
        return mx_call_meta(vm, handler, 2, pair);
    }
}

mx_obj_t mx_meta_index(mx_t *vm, mx_obj_t v, mx_obj_t key) {
    mx_obj_t idx_key = mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_INDEX];
    mx_obj_t cur = v;
    for (uint32_t steps = 0; steps < MX_RUN_MAX_CHAIN; steps++) {
        if (mx_obj_is_table(cur) && mx_obj_get_table(cur)->is_plain) {
            mx_obj_t raw = mx_table_get(mx_obj_get_table(cur), key);
            if (!mx_obj_is_nil(raw)) {
                return raw;
            }
        }
        mx_obj_t handler = mx_meta_handler_of(vm, cur, idx_key);
        if (mx_obj_is_nil(handler)) {
            if (mx_obj_is_table(cur) && mx_obj_get_table(cur)->is_plain) {
                return mx_obj_of_nil();
            } else {
                return mx_meta_typed_error(vm, "attempt to index a %s value", cur);
            }
        } else if (mx_meta_lua_type(vm, handler) == MX_LUA_TYPE_FUNCTION) {
            mx_obj_t pair[2] = {cur, key};
            return mx_call_meta(vm, handler, 2, pair);
        }
        cur = handler;
    }
    return mx_error_raw(vm, "'__index' chain too long; possible loop");
}

mx_obj_t mx_meta_newindex(mx_t *vm, mx_obj_t v, mx_obj_t key, mx_obj_t value) {
    mx_obj_t nidx_key = mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_NEWINDEX];
    mx_obj_t cur = v;
    for (uint32_t steps = 0; steps < MX_RUN_MAX_CHAIN; steps++) {
        mx_obj_t handler = mx_meta_handler_of(vm, cur, nidx_key);
        bool is_plain = mx_obj_is_table(cur) && mx_obj_get_table(cur)->is_plain;
        if (is_plain && (mx_obj_is_nil(handler) || !mx_obj_is_nil(mx_table_get(mx_obj_get_table(cur), key)))) {
            if (mx_obj_is_nil(key)) {
                return mx_error_raw(vm, "table index is nil");
            } else if (mx_obj_is_float(key) && mx_obj_get_float(key) != mx_obj_get_float(key)) {
                return mx_error_raw(vm, "table index is NaN");
            }
            mx_table_set(vm, mx_obj_get_table(cur), key, value);
            return mx_obj_of_nil();
        } else if (mx_obj_is_nil(handler)) {
            return mx_meta_typed_error(vm, "attempt to index a %s value", cur);
        } else if (mx_meta_lua_type(vm, handler) == MX_LUA_TYPE_FUNCTION) {
            mx_obj_t triple[3] = {cur, key, value};
            return mx_call_meta(vm, handler, 3, triple);
        }
        cur = handler;
    }
    return mx_error_raw(vm, "'__newindex' chain too long; possible loop");
}

mx_obj_t mx_meta_len(mx_t *vm, mx_obj_t v) {
    if (mx_obj_is_string(v)) {
        return mx_obj_of_int((int64_t)mx_obj_get_string(v)->len);
    }
    mx_obj_t handler = mx_meta_handler_of(vm, v, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_LEN]);
    if (!mx_obj_is_nil(handler)) {
        mx_obj_t pair[2] = {v, v};
        return mx_call_meta(vm, handler, 2, pair);
    } else if (v.kind == vm->kinds.table) {
        return mx_obj_of_int((int64_t)mx_obj_get_table(v)->len);
    } else {
        return mx_meta_typed_error(vm, "attempt to get length of a %s value", v);
    }
}

mx_obj_t mx_meta_concat(mx_t *vm, mx_obj_t a, mx_obj_t b) {
    bool ok_a = mx_obj_is_string(a) || mx_obj_is_number(a);
    bool ok_b = mx_obj_is_string(b) || mx_obj_is_number(b);
    if (ok_a && ok_b) {
        mx_str_t *buf = mx_str_new_gc(vm);
        mx_meta_render(vm, buf, a);
        mx_meta_render(vm, buf, b);
        return mx_obj_of_string(buf);
    } else {
        return mx_meta_arith_event(vm, MX_OP_CONCAT, a, b);
    }
}

void mx_meta_result(mx_t *vm, mx_obj_t v) {
    if (!vm->pending.is_set) {
        mx_call_args(vm)[0] = v;
        vm->rets.len = 1;
    }
}

void mx_meta_op_add(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_ADD, args[0], args[1]));
}

void mx_meta_op_sub(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_SUB, args[0], args[1]));
}

void mx_meta_op_mul(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_MUL, args[0], args[1]));
}

void mx_meta_op_div(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_DIV, args[0], args[1]));
}

void mx_meta_op_idiv(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_IDIV, args[0], args[1]));
}

void mx_meta_op_mod(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_MOD, args[0], args[1]));
}

void mx_meta_op_pow(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_POW, args[0], args[1]));
}

void mx_meta_op_band(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_BAND, args[0], args[1]));
}

void mx_meta_op_bor(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_BOR, args[0], args[1]));
}

void mx_meta_op_bxor(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_BXOR, args[0], args[1]));
}

void mx_meta_op_shl(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_SHL, args[0], args[1]));
}

void mx_meta_op_shr(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_arith(vm, MX_OP_SHR, args[0], args[1]));
}

void mx_meta_op_unm(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_t v = args[0];
    mx_obj_num_t num = mx_meta_coerce_num(vm, v);
    if (num.is_num && mx_obj_is_int(num.value)) {
        mx_meta_result(vm, mx_obj_of_int(-mx_obj_get_int(num.value)));
        return;
    } else if (num.is_num) {
        mx_meta_result(vm, mx_obj_of_number(-mx_obj_get_float(num.value)));
        return;
    }
    mx_obj_t handler = mx_meta_handler_of(vm, v, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_UNM]);
    if (mx_obj_is_nil(handler) || (mx_obj_is_ffi(handler) && mx_obj_get_ffi(handler) == mx_meta_op_unm)) {
        mx_meta_result(vm, mx_meta_typed_error(vm, "attempt to perform arithmetic on a %s value", v));
    } else {
        mx_obj_t pair[2] = {v, v};
        mx_meta_result(vm, mx_call_meta(vm, handler, 2, pair));
    }
}

void mx_meta_op_bnot(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_t v = args[0];
    mx_obj_int_t iv = mx_obj_to_int(v);
    if (iv.is_int) {
        mx_meta_result(vm, mx_obj_of_int(~iv.value));
        return;
    }
    mx_obj_t handler = mx_meta_handler_of(vm, v, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_BNOT]);
    if (!mx_obj_is_nil(handler)) {
        mx_obj_t pair[2] = {v, v};
        mx_meta_result(vm, mx_call_meta(vm, handler, 2, pair));
    } else if (mx_obj_is_float(v)) {
        mx_obj_t err = mx_error_raw_operand(vm, MX_ERROR_OPERAND_LHS, "number has no integer representation");
        mx_obj_get_error(err)->is_hint_inline = true;
        mx_meta_result(vm, err);
    } else {
        mx_meta_result(vm, mx_meta_typed_error(vm, "attempt to perform bitwise operation on a %s value", v));
    }
}

void mx_meta_op_concat(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_concat(vm, args[0], args[1]));
}

void mx_meta_op_eq(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_compare(vm, MX_OP_EQ, args[0], args[1]));
}

void mx_meta_op_lt(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_compare(vm, MX_OP_LT, args[0], args[1]));
}

void mx_meta_op_le(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_compare(vm, MX_OP_LE, args[0], args[1]));
}

void mx_meta_op_get(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_index(vm, args[0], args[1]));
}

void mx_meta_op_set(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_newindex(vm, args[0], args[1], args[2]));
}

void mx_meta_op_len(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_meta_result(vm, mx_meta_len(vm, args[0]));
}

void mx_meta_op_call(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_t v = args[0];
    mx_obj_t handler = mx_meta_handler_of(vm, v, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_CALL]);
    if (mx_obj_is_nil(handler)) {
        args[0] = mx_meta_typed_error(vm, "attempt to call a %s value", v);
        vm->rets.len = 1;
        return;
    }
    mx_obj_t *cargs = args;
    uint32_t cn = (uint32_t)nargs;
    mx_obj_t next = mx_run_call_handler(vm, handler);
    if (!mx_obj_is_nil(next)) {
        mx_obj_t *tmp = mx_vm_tmp_reserve(vm, cn + 15);
        memmove(tmp + 15, args, sizeof(mx_obj_t) * cn);
        uint32_t at = 15;
        while (!mx_obj_is_nil(next) && at > 1) {
            at -= 1;
            tmp[at] = handler;
            handler = next;
            next = mx_run_call_handler(vm, handler);
        }
        if (!mx_obj_is_nil(next)) {
            args[0] = mx_error_raw(vm, "'__call' chain too long");
            vm->rets.len = 1;
            return;
        }
        cargs = tmp + at;
        cn += 15 - at;
    }
    if (vm->pending.is_ok && mx_obj_is_closure(handler)) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_CALL, handler, mx_obj_of_nil(), NULL, cn, cargs);
        return;
    }
    mx_obj_t got = mx_call(vm, handler, cn, cargs);
    if (mx_obj_is_error(got)) {
        mx_meta_result(vm, got);
        return;
    }
    uint32_t n = vm->rets.len;
    mx_obj_t *out = mx_call_results(vm, n + 1);
    if (out == NULL) {
        mx_meta_result(vm, mx_error_raw(vm, "too many results"));
        return;
    }
    memcpy(out, vm->rets.vals, sizeof(mx_obj_t) * n);
    if (n == 0) {
        out[0] = mx_obj_of_nil();
    }
    vm->rets.len = n;
}

bool mx_meta_run_finalizers(mx_t *vm) {
    mx_gc_t *gc = vm->gc;
    mx_lua_t *lua = mx_lua_of(vm);
    bool ran = false;
    while (gc->tobefnz.len > 0) {
        gc->tobefnz.len -= 1;
        mx_obj_t obj = gc->tobefnz.ptr[gc->tobefnz.len];
        mx_obj_t handler = mx_meta_handler_of(vm, obj, lua->meta_keys[MX_LUA_META_KEY_GC]);
        if (mx_meta_lua_type(vm, handler) == MX_LUA_TYPE_FUNCTION) {
            mx_call(vm, handler, 1, &obj);
            ran = true;
        }
    }
    return ran;
}

void mx_meta_open(mx_t *vm) {
    mx_lua_t *lua = mx_lua_of(vm);
    const char *const names[] = {
        [MX_LUA_META_KEY_INDEX] = "__index",
        [MX_LUA_META_KEY_NEWINDEX] = "__newindex",
        [MX_LUA_META_KEY_EQ] = "__eq",
        [MX_LUA_META_KEY_LT] = "__lt",
        [MX_LUA_META_KEY_LE] = "__le",
        [MX_LUA_META_KEY_CALL] = "__call",
        [MX_LUA_META_KEY_LEN] = "__len",
        [MX_LUA_META_KEY_TOSTRING] = "__tostring",
        [MX_LUA_META_KEY_NAME] = "__name",
        [MX_LUA_META_KEY_METATABLE] = "__metatable",
        [MX_LUA_META_KEY_PAIRS] = "__pairs",
        [MX_LUA_META_KEY_CLOSE] = "__close",
        [MX_LUA_META_KEY_MODE] = "__mode",
        [MX_LUA_META_KEY_GC] = "__gc",
        [MX_LUA_META_KEY_UNM] = "__unm",
        [MX_LUA_META_KEY_BNOT] = "__bnot",
    };
    for (uint32_t i = 0; i < MX_LUA_META_KEY_COUNT; i++) {
        if (i >= MX_LUA_META_KEY_ARITH_BASE && i < MX_LUA_META_KEY_UNM) {
            char name[16];
            snprintf(name, sizeof(name), "__%s", mx_op_names[MX_OP_ADD + (i - MX_LUA_META_KEY_ARITH_BASE)]);
            lua->meta_keys[i] = mx_obj_intern(vm, name);
        } else {
            lua->meta_keys[i] = mx_obj_intern(vm, names[i]);
        }
    }
    vm->meta_call = (mx_meta_std_t) { .fn = mx_meta_op_call, .key = lua->meta_keys[MX_LUA_META_KEY_CALL] };
    vm->meta_get = (mx_meta_std_t) { .fn = mx_meta_op_get, .key = lua->meta_keys[MX_LUA_META_KEY_INDEX] };
    vm->meta_set = (mx_meta_std_t) { .fn = mx_meta_op_set, .key = lua->meta_keys[MX_LUA_META_KEY_NEWINDEX] };
    vm->meta_eq = (mx_meta_std_t) { .fn = mx_meta_op_eq, .key = lua->meta_keys[MX_LUA_META_KEY_EQ] };
    vm->weak_mode_key = lua->meta_keys[MX_LUA_META_KEY_MODE];
    for (mx_lua_type_t t = MX_LUA_TYPE_NIL; t < MX_LUA_TYPE_COUNT; t++) {
        lua->type_names[t] = mx_obj_intern(vm, mx_meta_lua_typename(t));
    }
    mx_ffi_t *const ops[MX_OP_COUNT] = {
        [MX_OP_ADD] = mx_meta_op_add,
        [MX_OP_SUB] = mx_meta_op_sub,
        [MX_OP_MUL] = mx_meta_op_mul,
        [MX_OP_DIV] = mx_meta_op_div,
        [MX_OP_IDIV] = mx_meta_op_idiv,
        [MX_OP_MOD] = mx_meta_op_mod,
        [MX_OP_POW] = mx_meta_op_pow,
        [MX_OP_CONCAT] = mx_meta_op_concat,
        [MX_OP_BAND] = mx_meta_op_band,
        [MX_OP_BOR] = mx_meta_op_bor,
        [MX_OP_BXOR] = mx_meta_op_bxor,
        [MX_OP_SHL] = mx_meta_op_shl,
        [MX_OP_SHR] = mx_meta_op_shr,
        [MX_OP_EQ] = mx_meta_op_eq,
        [MX_OP_LT] = mx_meta_op_lt,
        [MX_OP_LE] = mx_meta_op_le,
        [MX_OP_GET] = mx_meta_op_get,
        [MX_OP_SET] = mx_meta_op_set,
        [MX_OP_CALL] = mx_meta_op_call,
        [MX_OP_LEN] = mx_meta_op_len,
        [MX_OP_UNM] = mx_meta_op_unm,
        [MX_OP_BNOT] = mx_meta_op_bnot,
    };
    for (mx_kind_t k = 0; k < vm->kinds.len; k++) {
        if (k != vm->kinds.upval) {
            for (mx_op_t op = 0; op < MX_OP_COUNT; op++) {
                mx_kind_set_op(vm, k, op, mx_obj_of_ffi(ops[op]));
            }
        }
    }
}
