
#include "kind.h"
#include "error.h"

const char *const mx_op_names[MX_OP_COUNT] = {
    [MX_OP_ADD] = "add",
    [MX_OP_SUB] = "sub",
    [MX_OP_MUL] = "mul",
    [MX_OP_DIV] = "div",
    [MX_OP_IDIV] = "idiv",
    [MX_OP_MOD] = "mod",
    [MX_OP_POW] = "pow",
    [MX_OP_CONCAT] = "concat",
    [MX_OP_BAND] = "band",
    [MX_OP_BOR] = "bor",
    [MX_OP_BXOR] = "bxor",
    [MX_OP_SHL] = "shl",
    [MX_OP_SHR] = "shr",
    [MX_OP_EQ] = "eq",
    [MX_OP_LT] = "lt",
    [MX_OP_LE] = "le",
    [MX_OP_GET] = "index",
    [MX_OP_SET] = "newindex",
    [MX_OP_CALL] = "call",
    [MX_OP_LEN] = "len",
    [MX_OP_UNM] = "unm",
    [MX_OP_BNOT] = "bnot",
};

mx_kind_t mx_kind_register(mx_t *vm, mx_kind_info_t info) {
    if (vm->kinds.len + 1 >= vm->kinds.cap) {
        vm->kinds.cap = (vm->kinds.len + 1) * 2;
        vm->kinds.ptr = mx_mem_realloc(vm->kinds.ptr, sizeof(mx_kind_info_t) * vm->kinds.cap);
    }
    vm->kinds.ptr[vm->kinds.len] = info;
    vm->kinds.len += 1;
    return (mx_kind_t)(vm->kinds.len - 1);
}

void mx_kind_set_op(mx_t *vm, mx_kind_t kind, mx_op_t op, mx_obj_t fn) {
    vm->kinds.ptr[kind].ops[op] = fn;
}

const char *mx_kind_name(mx_t *vm, mx_kind_t kind) {
    return vm->kinds.ptr[kind].name;
}

void mx_kind_unsupported(mx_t *vm, size_t nargs, mx_obj_t *args) {
    args[0] = mx_error_raw_operand(vm, MX_ERROR_OPERAND_LHS, "attempt to use a %s value", mx_kind_name(vm, args[0].kind));
}

mx_kind_info_t mx_kind_base(const char *name) {
    mx_kind_info_t info = {
        .name = name,
        .get_methods = mx_obj_of_nil(),
    };
    for (mx_op_t op = 0; op < MX_OP_COUNT; op++) {
        info.ops[op] = mx_obj_of_ffi(mx_kind_unsupported);
    }
    return info;
}

void mx_kind_init(mx_t *vm) {
    mx_kind_register(vm, mx_kind_base("nil"));
    mx_kind_register(vm, mx_kind_base("boolean"));
    mx_kind_register(vm, mx_kind_base("number"));
    mx_kind_register(vm, mx_kind_base("number"));
    mx_kind_register(vm, mx_kind_base("string"));
    mx_kind_register(vm, mx_kind_base("function"));
    mx_kind_register(vm, mx_kind_base("function"));
    mx_kind_register(vm, mx_kind_base("code"));
    mx_kind_register(vm, mx_kind_base("error"));
    mx_kind_register(vm, mx_kind_base("userdata"));
    vm->kinds.table = mx_kind_register(vm, mx_kind_base("table"));
    vm->kinds.cfn = mx_kind_register(vm, mx_kind_base("function"));
    vm->kinds.thread = mx_kind_register(vm, mx_kind_base("thread"));
    vm->kinds.upval = mx_kind_register(vm, mx_kind_base("upvalue"));
}

void mx_kind_free(mx_t *vm) {
    mx_mem_free(vm->kinds.ptr);
    vm->kinds.ptr = NULL;
    vm->kinds.len = 0;
    vm->kinds.cap = 0;
}
