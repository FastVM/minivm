
#pragma once

#if !defined(MX_HEADER_VALUE_KIND)
#define MX_HEADER_VALUE_KIND

#include "obj.h"

typedef uint8_t mx_op_t;
enum {
    MX_OP_ADD,
    MX_OP_SUB,
    MX_OP_MUL,
    MX_OP_DIV,
    MX_OP_IDIV,
    MX_OP_MOD,
    MX_OP_POW,
    MX_OP_CONCAT,
    MX_OP_BAND,
    MX_OP_BOR,
    MX_OP_BXOR,
    MX_OP_SHL,
    MX_OP_SHR,
    MX_OP_EQ,
    MX_OP_LT,
    MX_OP_LE,
    MX_OP_GET,
    MX_OP_SET,
    MX_OP_CALL,
    MX_OP_LEN,
    MX_OP_UNM,
    MX_OP_BNOT,
    MX_OP_COUNT,
};

struct mx_kind_info_t {
    const char *name;
    mx_obj_t ops[MX_OP_COUNT];
    mx_obj_t get_methods;
};

extern const char *const mx_op_names[MX_OP_COUNT];

mx_kind_t mx_kind_register(mx_t *vm, mx_kind_info_t info);
mx_kind_info_t mx_kind_base(const char *name);
void mx_kind_set_op(mx_t *vm, mx_kind_t kind, mx_op_t op, mx_obj_t fn);
const char *mx_kind_name(mx_t *vm, mx_kind_t kind);
void mx_kind_unsupported(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_kind_init(mx_t *vm);
void mx_kind_free(mx_t *vm);

#endif
