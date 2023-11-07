
#if !defined(VM_HEADER_AST_COMP)
#define VM_HEADER_AST_COMP

#include "ast.h"
#include "../ir.h"

struct vm_ast_blocks_t;
typedef struct vm_ast_blocks_t vm_ast_blocks_t;


struct vm_ast_blocks_t {
    size_t len;
    vm_block_t **blocks;
    size_t alloc;
};

vm_ast_blocks_t vm_ast_comp(vm_ast_node_t node);

#endif
