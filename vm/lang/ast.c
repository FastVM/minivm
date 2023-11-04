
#include "ast.h"

#define VM_MACRO_SELECT(_0, _1, _2, NAME, ...) NAME
#define vm_ast_form(TYPE_, ...)             \
    ((vm_ast_node_t){                       \
        .type = VM_AST_NODE_FORM,           \
        .value.form.type = (TYPE_),         \
        .value.form.args = VM_MACRO_SELECT( \
            __VA_ARGS__,                    \
            vm_ast_form_args_c3,            \
            vm_ast_form_args_c2,            \
            vm_ast_form_args_c1,            \
            vm_ast_args_c0)(__VA_ARGS__),   \
    })
#define vm_ast_form0(TYPE_) \
    ((vm_ast_node_t){                       \
        .type = VM_AST_NODE_FORM,           \
        .value.form.type = (TYPE_),         \
    })

#define vm_ast_ident(STR_)         \
    ((vm_ast_node_t){              \
        .type = VM_AST_NODE_IDENT, \
        .value.ident = (STR_),     \
    })

static vm_ast_node_t *vm_ast_args_var(size_t nargs, ...) {
    va_list list;
    va_start(list, nargs);
    vm_ast_node_t *ret = vm_malloc(sizeof(vm_ast_node_t) * nargs);
    for (size_t i = 0; i < nargs; i++) {
        ret[i] = va_arg(list, vm_ast_node_t);
    }
    va_end(list);
    return ret;
}

static vm_ast_node_t *vm_ast_args_c0(void) {
    return NULL;
}

static vm_ast_node_t *vm_ast_form_args_c1(vm_ast_node_t arg0) {
    vm_ast_node_t *ret = vm_malloc(sizeof(vm_ast_node_t) * 1);
    ret[0] = arg0;
    return ret;
}

static vm_ast_node_t *vm_ast_form_args_c2(vm_ast_node_t arg0, vm_ast_node_t arg1) {
    vm_ast_node_t *ret = vm_malloc(sizeof(vm_ast_node_t) * 2);
    ret[0] = arg0;
    ret[1] = arg0;
    return ret;
}

static vm_ast_node_t *vm_ast_form_args_c3(vm_ast_node_t arg0, vm_ast_node_t arg1, vm_ast_node_t arg2) {
    vm_ast_node_t *ret = vm_malloc(sizeof(vm_ast_node_t) * 3);
    ret[0] = arg0;
    ret[1] = arg1;
    ret[2] = arg2;
    return ret;
}

// blocks
vm_ast_node_t vm_ast_do(vm_ast_node_t lhs, vm_ast_node_t rhs) {
    return vm_ast_form(VM_AST_FORM_DO, lhs, rhs);
}

// locals
vm_ast_node_t vm_ast_set_local(const char *name, vm_ast_node_t value) {
    return vm_ast_form(VM_AST_FORM_SET_LOCAL, vm_ast_ident(name), value);
}

vm_ast_node_t vm_ast_get_local(const char *name) {
    return vm_ast_form(VM_AST_FORM_GET_LOCAL, vm_ast_ident(name));
}

// globals
vm_ast_node_t vm_ast_env(void) {
    return vm_ast_form0(VM_AST_FORM_ENV);
}

// tables
vm_ast_node_t vm_ast_table_new(void) {
    return vm_ast_form0(VM_AST_FORM_TABLE_NEW);
}
vm_ast_node_t vm_ast_table_set(vm_ast_node_t table, vm_ast_node_t key, vm_ast_node_t value) {
    return vm_ast_form(VM_AST_FORM_TABLE_NEW, table, key, value);
}
vm_ast_node_t vm_ast_table_get(vm_ast_node_t table, vm_ast_node_t key) {
    return vm_ast_form(VM_AST_FORM_TABLE_NEW, table, key);
}

// math
vm_ast_node_t vm_ast_add(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_sub(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_mul(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_div(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_mod(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_pow(vm_ast_node_t lhs, vm_ast_node_t rhs);

// compare
vm_ast_node_t vm_ast_eq(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_ne(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_lt(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_gt(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_le(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_ge(vm_ast_node_t lhs, vm_ast_node_t rhs);

// control flow
vm_ast_node_t vm_ast_if(vm_ast_node_t cond, vm_ast_node_t iftrue, vm_ast_node_t iffalse);
vm_ast_node_t vm_ast_while(vm_ast_node_t cond, vm_ast_node_t whiletrue);

// functions
vm_ast_node_t vm_ast_args(size_t nargs, vm_ast_node_t *args);
vm_ast_node_t vm_ast_lambda(vm_ast_node_t args, vm_ast_node_t body);
vm_ast_node_t vm_ast_call(vm_ast_node_t func, size_t nargs, vm_ast_node_t *args);
vm_ast_node_t vm_ast_return(vm_ast_node_t value);
