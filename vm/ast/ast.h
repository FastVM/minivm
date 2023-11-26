#if !defined(VM_HEADER_AST_AST)
#define VM_HEADER_AST_AST

#include "../lib.h"
#include "../obj.h"

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
    VM_AST_FORM_SET,
    // globals
    VM_AST_FORM_ENV,
    // tables
    VM_AST_FORM_NEW,
    VM_AST_FORM_LOAD,
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
    // logic
    VM_AST_FORM_AND,
    VM_AST_FORM_OR,
    VM_AST_FORM_NOT,
    // branch
    VM_AST_FORM_IF,
    VM_AST_FORM_WHILE,
    // calls
    VM_AST_FORM_ARGS,
    VM_AST_FORM_LAMBDA,
    VM_AST_FORM_CALL,
    VM_AST_FORM_RETURN,
};

struct vm_ast_form_t {
    vm_ast_node_t *args;
    uint8_t len;
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

#endif