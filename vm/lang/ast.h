#if !defined(VM_HEADER_LANG_AST)
#define VM_HEADER_LANG_AST

#include "../lib.h"
#include "../std/std.h"

struct vm_ast_form_t;
typedef struct vm_ast_form_t vm_ast_form_t;

union vm_ast_node_value_t;
typedef union vm_ast_node_value_t vm_ast_node_value_t;

struct vm_ast_node_t;
typedef struct vm_ast_node_t vm_ast_node_t;

typedef uint8_t vm_ast_node_type_t;
enum {
    VM_AST_NODE_FORM,
    VM_AST_NODE_IDENT,
    VM_AST_NODE_LITERAL,
};

typedef uint8_t vm_ast_form_type_t;
enum {
    VM_AST_FORM_DO,
    // locals
    VM_AST_FORM_GET_LOCAL,
    VM_AST_FORM_SET_LOCAL,
    // globals
    VM_AST_FORM_ENV,
    // tables
    VM_AST_FORM_TABLE_NEW,
    VM_AST_FORM_TABLE_SET,
    VM_AST_FORM_TABLE_GET,
    // math
    VM_AST_FORM_ADD,
    VM_AST_FORM_SUB,
    VM_AST_FORM_MUL,
    VM_AST_FORM_DIV,
    VM_AST_FORM_MOD,
    VM_AST_FORM_POW,
    // compare
    VM_AST_FORM_EQ,
    VM_AST_FORM_NE,
    VM_AST_FORM_LT,
    VM_AST_FORM_GT,
    VM_AST_FORM_LE,
    VM_AST_FORM_GE,
    // branch
    VM_AST_FORM_IF,
    VM_AST_FORM_WHILE,
    // calls
    VM_AST_FORM_ARG,
    VM_AST_FORM_LAMBDA,
    VM_AST_FORM_CALL,
    VM_AST_FORM_RETURN,
};

struct vm_ast_form_t {
    vm_ast_node_t *args;
    vm_ast_form_type_t type;
};

union vm_ast_node_value_t {
    vm_ast_form_t form;
    const char *ident;
    vm_std_value_t literal;
};

struct vm_ast_node_t {
    vm_ast_node_value_t value;
    vm_ast_node_type_t type;
};

// blocks
vm_ast_node_t vm_ast_do(vm_ast_node_t lhs, vm_ast_node_t rhs);

// locals
vm_ast_node_t vm_ast_set_local(const char *name, vm_ast_node_t value);
vm_ast_node_t vm_ast_get_local(const char *name);

// globals
vm_ast_node_t vm_ast_env(void);

// tables
vm_ast_node_t vm_ast_table_new(void);
vm_ast_node_t vm_ast_table_set(vm_ast_node_t table, vm_ast_node_t key, vm_ast_node_t value);
vm_ast_node_t vm_ast_table_get(vm_ast_node_t table, vm_ast_node_t key);

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
vm_ast_node_t vm_ast_while(vm_ast_node_t cond, vm_ast_node_t body);

// functions
vm_ast_node_t vm_ast_arg(uint32_t nth);
vm_ast_node_t vm_ast_lambda(vm_ast_node_t body);
vm_ast_node_t vm_ast_call(vm_ast_node_t func, size_t nargs, vm_ast_node_t *args);
vm_ast_node_t vm_ast_return(vm_ast_node_t value);

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
#define vm_ast_literal(TYPE_, VALUE_)                                                         \
    ((vm_ast_node_t){                                                                         \
        .type = VM_AST_NODE_LITERAL,                                                          \
        .value.literal = (vm_std_value_t){                                                    \
            .tag = VM_AST_LITERAL_TYPE_TO_TAG_CONCAT2(VM_AST_LITERAL_TYPE_TO_TAG_, TYPE_)(), \
            .value = (vm_value_t){                                                            \
                .TYPE_ = (VALUE_),                                                            \
            },                                                                                \
        },                                                                                    \
    })

#endif