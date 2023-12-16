
#include "../std/io.h"
#include "ast.h"

static void vm_indent(vm_io_buffer_t *out, size_t indent, const char *prefix) {
    while (indent-- > 0) {
        vm_io_buffer_format(out, "    ");
    }
    vm_io_buffer_format(out, "%s", prefix);
}

void vm_ast_print_node(vm_io_buffer_t *out, size_t indent, const char *prefix, vm_ast_node_t node) {
    switch (node.type) {
        case VM_AST_NODE_FORM: {
            vm_ast_form_t form = node.value.form;
            vm_indent(out, indent, prefix);
            switch (form.type) {
                case VM_AST_FORM_DO: {
                    vm_io_buffer_format(out, "do");
                    break;
                }
                case VM_AST_FORM_SET: {
                    vm_io_buffer_format(out, "set");
                    break;
                }
                case VM_AST_FORM_LOCAL: {
                    vm_io_buffer_format(out, "set");
                    break;
                }
                case VM_AST_FORM_ENV: {
                    vm_io_buffer_format(out, "env");
                    break;
                }
                case VM_AST_FORM_NEW: {
                    vm_io_buffer_format(out, "new");
                    break;
                }
                case VM_AST_FORM_LEN: {
                    vm_io_buffer_format(out, "len");
                    break;
                }
                case VM_AST_FORM_LOAD: {
                    vm_io_buffer_format(out, "load");
                    break;
                }
                case VM_AST_FORM_ADD: {
                    vm_io_buffer_format(out, "add");
                    break;
                }
                case VM_AST_FORM_SUB: {
                    vm_io_buffer_format(out, "sub");
                    break;
                }
                case VM_AST_FORM_MUL: {
                    vm_io_buffer_format(out, "mul");
                    break;
                }
                case VM_AST_FORM_DIV: {
                    vm_io_buffer_format(out, "div");
                    break;
                }
                case VM_AST_FORM_IDIV: {
                    vm_io_buffer_format(out, "idiv");
                    break;
                }
                case VM_AST_FORM_MOD: {
                    vm_io_buffer_format(out, "mod");
                    break;
                }
                case VM_AST_FORM_POW: {
                    vm_io_buffer_format(out, "pow");
                    break;
                }
                case VM_AST_FORM_CONCAT: {
                    vm_io_buffer_format(out, "concat");
                    break;
                }
                case VM_AST_FORM_EQ: {
                    vm_io_buffer_format(out, "eq");
                    break;
                }
                case VM_AST_FORM_NE: {
                    vm_io_buffer_format(out, "ne");
                    break;
                }
                case VM_AST_FORM_LT: {
                    vm_io_buffer_format(out, "lt");
                    break;
                }
                case VM_AST_FORM_GT: {
                    vm_io_buffer_format(out, "gt");
                    break;
                }
                case VM_AST_FORM_LE: {
                    vm_io_buffer_format(out, "le");
                    break;
                }
                case VM_AST_FORM_GE: {
                    vm_io_buffer_format(out, "ge");
                    break;
                }
                case VM_AST_FORM_AND: {
                    vm_io_buffer_format(out, "and");
                    break;
                }
                case VM_AST_FORM_OR: {
                    vm_io_buffer_format(out, "or");
                    break;
                }
                case VM_AST_FORM_NOT: {
                    vm_io_buffer_format(out, "not");
                    break;
                }
                case VM_AST_FORM_IF: {
                    vm_io_buffer_format(out, "if");
                    break;
                }
                case VM_AST_FORM_WHILE: {
                    vm_io_buffer_format(out, "while");
                    break;
                }
                case VM_AST_FORM_BREAK: {
                    vm_io_buffer_format(out, "break");
                    break;
                }
                case VM_AST_FORM_ARGS: {
                    vm_io_buffer_format(out, "args");
                    break;
                }
                case VM_AST_FORM_LAMBDA: {
                    vm_io_buffer_format(out, "lambda");
                    break;
                }
                case VM_AST_FORM_CALL: {
                    vm_io_buffer_format(out, "call");
                    break;
                }
                case VM_AST_FORM_RETURN: {
                    vm_io_buffer_format(out, "return");
                    break;
                }
            }
            vm_io_buffer_format(out, " {\n");
            for (size_t i = 0; i < form.len; i++) {
                vm_ast_print_node(out, indent + 1, "", form.args[i]);
            }
            vm_indent(out, indent, "");
            vm_io_buffer_format(out, "}\n");
            break;
        }
        case VM_AST_NODE_IDENT: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "%s\n", node.value.ident);
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_io_debug(out, indent, prefix, node.value.literal, NULL);
            break;
        }
    }
}
