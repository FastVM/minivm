
#if !defined(VM_HEADER_AST_IO)
#define VM_HEADER_AST_IO

#include "../std/io.h"
#include "ast.h"

void vm_ast_print_node(vm_io_buffer_t *out, size_t indent, const char *prefix, vm_ast_node_t node);

#endif
