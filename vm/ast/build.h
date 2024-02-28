
#if !defined(VM_HEADER_LANG_BUILD)
#define VM_HEADER_LANG_BUILD

#include "ast.h"

// blocks
vm_ast_node_t vm_ast_build_do(vm_ast_node_t lhs, vm_ast_node_t rhs);

// locals
vm_ast_node_t vm_ast_build_set(vm_ast_node_t target, vm_ast_node_t value);
vm_ast_node_t vm_ast_build_local(vm_ast_node_t target, vm_ast_node_t value);

// globals
vm_ast_node_t vm_ast_build_env(void);

// tables
vm_ast_node_t vm_ast_build_new(void);
vm_ast_node_t vm_ast_build_len(vm_ast_node_t table);
vm_ast_node_t vm_ast_build_load(vm_ast_node_t table, vm_ast_node_t key);

// math
vm_ast_node_t vm_ast_build_add(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_sub(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_mul(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_div(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_idiv(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_mod(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_pow(vm_ast_node_t lhs, vm_ast_node_t rhs);

// strings
vm_ast_node_t vm_ast_build_concat(vm_ast_node_t lhs, vm_ast_node_t rhs);

// compare
vm_ast_node_t vm_ast_build_eq(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_ne(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_lt(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_gt(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_le(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_ge(vm_ast_node_t lhs, vm_ast_node_t rhs);

// booleans
vm_ast_node_t vm_ast_build_and(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_or(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_not(vm_ast_node_t value);

// control flow
vm_ast_node_t vm_ast_build_if(vm_ast_node_t cond, vm_ast_node_t iftrue, vm_ast_node_t iffalse);
vm_ast_node_t vm_ast_build_while(vm_ast_node_t cond, vm_ast_node_t body);
vm_ast_node_t vm_ast_build_break(void);

// functions
vm_ast_node_t vm_ast_build_args(size_t nargs, vm_ast_node_t *bind);
vm_ast_node_t vm_ast_build_lambda(vm_ast_node_t self, vm_ast_node_t args, vm_ast_node_t body);
vm_ast_node_t vm_ast_build_call(vm_ast_node_t func, size_t nargs, vm_ast_node_t *args);
vm_ast_node_t vm_ast_build_return(vm_ast_node_t value);

vm_ast_node_t vm_ast_build_block(size_t len, ...);

vm_ast_node_t vm_ast_build_error(const char *str);
vm_ast_node_t vm_ast_build_nil(void);

// use this like follows
// vm_ast_build_literal(i32, 10)
// vm_ast_build_literal(f64, )
#define vm_ast_build_literal(TYPE_, VALUE_)                   \
    ((vm_ast_node_t){                                         \
        .type = VM_AST_NODE_LITERAL,                          \
        .value.literal = VM_STD_VALUE_LITERAL(TYPE_, VALUE_), \
    })

#define vm_ast_build_ident(STR_)   \
    ((vm_ast_node_t){              \
        .type = VM_AST_NODE_IDENT, \
        .value.ident = (STR_),     \
    })

#endif