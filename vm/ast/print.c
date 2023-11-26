
#include "../std/libs/io.h"
#include "ast.h"

static void vm_indent(FILE *out, size_t indent, const char *prefix) {
    while (indent-- > 0) {
        fprintf(out, "    ");
    }
    fprintf(out, "%s", prefix);
}

void vm_ast_print_node(FILE *out, size_t indent, const char *prefix, vm_ast_node_t node) {
    switch (node.type) {
        case VM_AST_NODE_FORM: {
            vm_ast_form_t form = node.value.form;
            vm_indent(out, indent, prefix);
            switch (form.type) {
                case VM_AST_FORM_DO: {
                    fprintf(out, "do");
                    break;
                }
                case VM_AST_FORM_SET: {
                    fprintf(out, "set");
                    break;
                }
                case VM_AST_FORM_ENV: {
                    fprintf(out, "env");
                    break;
                }
                case VM_AST_FORM_NEW: {
                    fprintf(out, "new");
                    break;
                }
                case VM_AST_FORM_LOAD: {
                    fprintf(out, "load");
                    break;
                }
                case VM_AST_FORM_ADD: {
                    fprintf(out, "add");
                    break;
                }
                case VM_AST_FORM_SUB: {
                    fprintf(out, "sub");
                    break;
                }
                case VM_AST_FORM_MUL: {
                    fprintf(out, "mul");
                    break;
                }
                case VM_AST_FORM_DIV: {
                    fprintf(out, "div");
                    break;
                }
                case VM_AST_FORM_MOD: {
                    fprintf(out, "mod");
                    break;
                }
                case VM_AST_FORM_POW: {
                    fprintf(out, "pow");
                    break;
                }
                case VM_AST_FORM_EQ: {
                    fprintf(out, "eq");
                    break;
                }
                case VM_AST_FORM_NE: {
                    fprintf(out, "ne");
                    break;
                }
                case VM_AST_FORM_LT: {
                    fprintf(out, "lt");
                    break;
                }
                case VM_AST_FORM_GT: {
                    fprintf(out, "gt");
                    break;
                }
                case VM_AST_FORM_LE: {
                    fprintf(out, "le");
                    break;
                }
                case VM_AST_FORM_GE: {
                    fprintf(out, "ge");
                    break;
                }
                case VM_AST_FORM_AND: {
                    fprintf(out, "and");
                    break;
                }
                case VM_AST_FORM_OR: {
                    fprintf(out, "or");
                    break;
                }
                case VM_AST_FORM_NOT: {
                    fprintf(out, "not");
                    break;
                }
                case VM_AST_FORM_IF: {
                    fprintf(out, "if");
                    break;
                }
                case VM_AST_FORM_WHILE: {
                    fprintf(out, "while");
                    break;
                }
                case VM_AST_FORM_ARGS: {
                    fprintf(out, "args");
                    break;
                }
                case VM_AST_FORM_LAMBDA: {
                    fprintf(out, "lambda");
                    break;
                }
                case VM_AST_FORM_CALL: {
                    fprintf(out, "call");
                    break;
                }
                case VM_AST_FORM_RETURN: {
                    fprintf(out, "return");
                    break;
                }
            }
            fprintf(out, " {\n");
            for (size_t i = 0; i < form.len; i++) {
                vm_ast_print_node(out, indent + 1, "", form.args[i]);
            }
            vm_indent(out, indent, "");
            fprintf(out, "}\n");
            break;
        }
        case VM_AST_NODE_IDENT: {
            vm_indent(out, indent, prefix);
            fprintf(out, "%s\n", node.value.ident);
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_io_debug(out, indent, prefix, node.value.literal, NULL);
            break;
        }
    }
}
