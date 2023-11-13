
#include "build.h"

#define VM_MACRO_SELECT(_0, _1, _2, NAME, ...) NAME
#define vm_ast_form(TYPE_, ...)        \
    ((vm_ast_node_t){                  \
        .type = VM_AST_NODE_FORM,      \
        .value.form = VM_MACRO_SELECT( \
            __VA_ARGS__,               \
            vm_ast_form_args_c3,       \
            vm_ast_form_args_c2,       \
            vm_ast_form_args_c1,       \
            "use vm_ast_form0 instead" \
        )((TYPE_), __VA_ARGS__),       \
    })
#define vm_ast_form0(TYPE_)         \
    ((vm_ast_node_t){               \
        .type = VM_AST_NODE_FORM,   \
        .value.form.type = (TYPE_), \
    })

#define vm_ast_ident(STR_)         \
    ((vm_ast_node_t){              \
        .type = VM_AST_NODE_IDENT, \
        .value.ident = (STR_),     \
    })

#define vm_ast_literal_std(LIT_)     \
    ((vm_ast_node_t){                \
        .type = VM_AST_NODE_LITERAL, \
        .value.literal = (LIT_),     \
    })

static vm_ast_form_t vm_ast_form_args_c1(vm_ast_form_type_t type, vm_ast_node_t arg0) {
    vm_ast_node_t *ret = vm_malloc(sizeof(vm_ast_node_t) * 1);
    ret[0] = arg0;
    return (vm_ast_form_t){
        .type = type,
        .len = 1,
        .args = ret,
    };
}

static vm_ast_form_t vm_ast_form_args_c2(vm_ast_form_type_t type, vm_ast_node_t arg0, vm_ast_node_t arg1) {
    vm_ast_node_t *ret = vm_malloc(sizeof(vm_ast_node_t) * 2);
    ret[0] = arg0;
    ret[1] = arg1;
    return (vm_ast_form_t){
        .type = type,
        .len = 2,
        .args = ret,
    };
}

static vm_ast_form_t vm_ast_form_args_c3(vm_ast_form_type_t type, vm_ast_node_t arg0, vm_ast_node_t arg1, vm_ast_node_t arg2) {
    vm_ast_node_t *ret = vm_malloc(sizeof(vm_ast_node_t) * 3);
    ret[0] = arg0;
    ret[1] = arg1;
    ret[2] = arg2;
    return (vm_ast_form_t){
        .type = type,
        .len = 3,
        .args = ret,
    };
}

// blocks
vm_ast_node_t vm_ast_build_do(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_DO, lhs, rhs);
}

// locals
vm_ast_node_t vm_ast_build_set(vm_ast_node_t target, vm_ast_node_t value) {
    return vm_ast_form(VM_AST_FORM_SET, target, value);
}

// globals
vm_ast_node_t vm_ast_build_env(void) {
    return vm_ast_form0(VM_AST_FORM_ENV);
}

// tables
vm_ast_node_t vm_ast_build_new(void) {
    return vm_ast_form0(VM_AST_FORM_NEW);
}
vm_ast_node_t vm_ast_build_load(vm_ast_node_t table, vm_ast_node_t key) {
    return vm_ast_form(VM_AST_FORM_LOAD, table, key);
}

// math
vm_ast_node_t vm_ast_build_add(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_ADD, lhs, rhs);
}
vm_ast_node_t vm_ast_build_sub(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_SUB, lhs, rhs);
}
vm_ast_node_t vm_ast_build_mul(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_MUL, lhs, rhs);
}
vm_ast_node_t vm_ast_build_div(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_DIV, lhs, rhs);
}
vm_ast_node_t vm_ast_build_mod(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_MOD, lhs, rhs);
}
vm_ast_node_t vm_ast_build_pow(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_POW, lhs, rhs);
}

// compare
vm_ast_node_t vm_ast_build_eq(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_EQ, lhs, rhs);
}
vm_ast_node_t vm_ast_build_ne(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_NE, lhs, rhs);
}
vm_ast_node_t vm_ast_build_lt(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_LT, lhs, rhs);
}
vm_ast_node_t vm_ast_build_gt(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_GT, lhs, rhs);
}
vm_ast_node_t vm_ast_build_le(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_LE, lhs, rhs);
}
vm_ast_node_t vm_ast_build_ge(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_GE, lhs, rhs);
}

// control flow
vm_ast_node_t vm_ast_build_if(vm_ast_node_t cond, vm_ast_node_t iftrue, vm_ast_node_t iffalse) {
    return vm_ast_form(VM_AST_FORM_IF, cond, iftrue, iffalse);
}
vm_ast_node_t vm_ast_build_while(vm_ast_node_t cond, vm_ast_node_t body) {
    return vm_ast_form(VM_AST_FORM_WHILE, cond, body);
}

// functions
vm_ast_node_t vm_ast_build_arg(uint32_t nth) {
    return vm_ast_form(VM_AST_FORM_ARG, vm_ast_build_literal(i32, nth));
}
vm_ast_node_t vm_ast_build_lambda(vm_ast_node_t body) {
    return vm_ast_form(VM_AST_FORM_LAMBDA, body);
}
vm_ast_node_t vm_ast_build_call(vm_ast_node_t func, size_t nargs, vm_ast_node_t *args) {
    vm_ast_node_t *ret = vm_malloc(sizeof(vm_ast_node_t) * (nargs + 1));
    ret[0] = func;
    for (size_t i = 0; i < nargs; i++) {
        ret[i + 1] = args[i];
    }
    return (vm_ast_node_t){
        .type = VM_AST_NODE_FORM,
        .value.form = (vm_ast_form_t){
            .type = VM_AST_FORM_CALL,
            .len = nargs + 1,
            .args = ret,
        },
    };
}
vm_ast_node_t vm_ast_build_return(vm_ast_node_t value) {
    return vm_ast_form(VM_AST_FORM_RETURN, value);
}
vm_ast_node_t vm_ast_build_block(size_t len, ...) {
    va_list ap;
    va_start(ap, len);
    vm_ast_node_t ret;
    if (len == 0) {
        ret = vm_ast_build_nil();
    } else if (len == 1) {
        ret = va_arg(ap, vm_ast_node_t);
    } else {
        ret = va_arg(ap, vm_ast_node_t);
        for (size_t i = 1; i < len; i++) {
            ret = vm_ast_build_do(ret, va_arg(ap, vm_ast_node_t));
        }
    }
    va_end(ap);
    return ret;
}

vm_ast_node_t vm_ast_build_nil(void) {
    return (vm_ast_node_t){
        .type = VM_AST_NODE_LITERAL,
        .value.literal = (vm_std_value_t){
            .tag = VM_TAG_NIL,
        },
    };
}
