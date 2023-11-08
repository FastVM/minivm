
#if !defined(VM_HEADER_LANG_BUILD)
#define VM_HEADER_LANG_BUILD

#include "ast.h"

// blocks
vm_ast_node_t vm_ast_build_do(vm_ast_node_t lhs, vm_ast_node_t rhs);

// locals
vm_ast_node_t vm_ast_build_set(const char *name, vm_ast_node_t value);

// globals
vm_ast_node_t vm_ast_build_env(void);

// tables
vm_ast_node_t vm_ast_build_new(void);
vm_ast_node_t vm_ast_build_loadt(vm_ast_node_t table, vm_ast_node_t key);

// math
vm_ast_node_t vm_ast_build_add(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_sub(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_mul(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_div(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_mod(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_pow(vm_ast_node_t lhs, vm_ast_node_t rhs);

// compare
vm_ast_node_t vm_ast_build_eq(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_ne(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_lt(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_gt(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_le(vm_ast_node_t lhs, vm_ast_node_t rhs);
vm_ast_node_t vm_ast_build_ge(vm_ast_node_t lhs, vm_ast_node_t rhs);

// control flow
vm_ast_node_t vm_ast_build_if(vm_ast_node_t cond, vm_ast_node_t iftrue, vm_ast_node_t iffalse);
vm_ast_node_t vm_ast_build_while(vm_ast_node_t cond, vm_ast_node_t body);

// functions
vm_ast_node_t vm_ast_build_arg(uint32_t nth);
vm_ast_node_t vm_ast_build_lambda(vm_ast_node_t body);
vm_ast_node_t vm_ast_build_call(vm_ast_node_t func, size_t nargs, vm_ast_node_t *args);
vm_ast_node_t vm_ast_build_return(vm_ast_node_t value);

// ugly hacks
#define VM_AST_LITERAL_TYPE_TO_TAG_i8(...) VM_TAG_I8
#define VM_AST_LITERAL_TYPE_TO_TAG_i16(...) VM_TAG_I16
#define VM_AST_LITERAL_TYPE_TO_TAG_i32(...) VM_TAG_I32
#define VM_AST_LITERAL_TYPE_TO_TAG_i64(...) VM_TAG_I64
#define VM_AST_LITERAL_TYPE_TO_TAG_f32(...) VM_TAG_F32
#define VM_AST_LITERAL_TYPE_TO_TAG_f64(...) VM_TAG_F64

#define VM_AST_LITERAL_TYPE_TO_TAG_CONCAT2_IMPL(X_, Y_) X_##Y_
#define VM_AST_LITERAL_TYPE_TO_TAG_CONCAT2(X_, Y_) VM_AST_LITERAL_TYPE_TO_TAG_CONCAT2_IMPL(X_, Y_)

// use this like follows
// vm_ast_literal(i32, 10)
// vm_ast_literal(f64, )
#define vm_ast_build_literal(TYPE_, VALUE_)                                                  \
    ((vm_ast_node_t){                                                                        \
        .type = VM_AST_NODE_LITERAL,                                                         \
        .value.literal = (vm_std_value_t){                                                   \
            .tag = VM_AST_LITERAL_TYPE_TO_TAG_CONCAT2(VM_AST_LITERAL_TYPE_TO_TAG_, TYPE_)(), \
            .value = (vm_value_t){                                                           \
                .TYPE_ = (VALUE_),                                                           \
            },                                                                               \
        },                                                                                   \
    })

#endif