#include "./ast.h"

void vm_ast_free_form(vm_ast_form_t form) {
    for (size_t i = 0; i < form.len; i++) {
        vm_ast_free_node(form.args[i]);
    }
    vm_free(form.args);
}

void vm_ast_free_ident(const char *ident) {
    vm_free(ident);
}

void vm_ast_free_literal(vm_std_value_t literal) {
    switch (vm_type_tag(literal.tag)) {
        case VM_TAG_STR: {
            vm_free(literal.value.str);
            break;
        }
    }
}

void vm_ast_free_node(vm_ast_node_t node) {
    switch (node.type) {
        case VM_AST_NODE_FORM: {
            vm_ast_free_form(node.value.form);
            break;
        }
        case VM_AST_NODE_IDENT: {
            vm_ast_free_ident(node.value.ident);
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_ast_free_literal(node.value.literal);
            break;
        }
    }
}

#if !defined(NDEBUG)
const char *vm_ast_format(vm_ast_node_t *node) {
    vm_io_buffer_t *buf = vm_io_buffer_new();
    vm_ast_print_node(buf, 0, "", node);
    return vm_io_buffer_get(buf);
}
#endif
