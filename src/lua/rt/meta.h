
#pragma once

#if !defined(MX_HEADER_LUA_RT_META)
#define MX_HEADER_LUA_RT_META

#include "lua.h"
#include "../../value/kind.h"
#include "../../value/obj.h"
#include "../../value/string.h"
#include "../../vm/vm.h"

mx_lua_type_t mx_meta_data_type(mx_t *vm, mx_kind_t kind);
mx_lua_type_t mx_meta_lua_type(mx_t *vm, mx_obj_t v);
mx_lua_type_t mx_meta_basic_type(mx_t *vm, mx_obj_t v);
const char *mx_meta_lua_typename(mx_lua_type_t type);
const char *mx_meta_typename(mx_t *vm, mx_obj_t v);
void mx_meta_render(mx_t *vm, mx_str_t *buf, mx_obj_t v);
mx_obj_t mx_meta_typed_error(mx_t *vm, const char *fmt, mx_obj_t bad);
mx_obj_t mx_meta_operand_error(mx_t *vm, const char *fmt, mx_obj_t a, mx_obj_t b, bool is_lhs_fine);
mx_obj_table_t *mx_meta_table_of(mx_t *vm, mx_obj_t v);
mx_obj_t mx_meta_handler_of(mx_t *vm, mx_obj_t v, mx_obj_t key);
mx_obj_num_t mx_meta_coerce_num(mx_t *vm, mx_obj_t v);
mx_obj_t mx_meta_arith_core(mx_t *vm, mx_op_t op, mx_obj_t a, mx_obj_t b);
mx_obj_t mx_meta_arith_event(mx_t *vm, mx_op_t op, mx_obj_t a, mx_obj_t b);
mx_obj_t mx_meta_arith(mx_t *vm, mx_op_t op, mx_obj_t a, mx_obj_t b);
mx_obj_t mx_meta_cmp_fail(mx_t *vm, mx_obj_t a, mx_obj_t b);
mx_obj_t mx_meta_compare(mx_t *vm, mx_op_t op, mx_obj_t a, mx_obj_t b);
mx_obj_t mx_meta_index(mx_t *vm, mx_obj_t v, mx_obj_t key);
mx_obj_t mx_meta_newindex(mx_t *vm, mx_obj_t v, mx_obj_t key, mx_obj_t value);
mx_obj_t mx_meta_len(mx_t *vm, mx_obj_t v);
mx_obj_t mx_meta_concat(mx_t *vm, mx_obj_t a, mx_obj_t b);
void mx_meta_result(mx_t *vm, mx_obj_t v);

void mx_meta_op_add(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_sub(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_mul(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_div(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_idiv(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_mod(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_pow(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_band(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_bor(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_bxor(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_shl(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_shr(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_unm(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_bnot(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_concat(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_eq(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_lt(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_le(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_get(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_set(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_call(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_meta_op_len(mx_t *vm, size_t nargs, mx_obj_t *args);

bool mx_meta_run_finalizers(mx_t *vm);
void mx_meta_open(mx_t *vm);

#endif
