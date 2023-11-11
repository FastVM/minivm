
#include "../../ast/ast.h"
#include "../../ast/build.h"
#include "../../ast/print.h"
#include "api.h"
#include "ts.h"

extern const TSLanguage *tree_sitter_lua(void);

char *vm_lang_lua_src(const char *src, TSNode node) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    uint32_t len = end - start;
    char *ident = vm_malloc(sizeof(char) * (len + 1));
    for (size_t i = 0; i < len; i++) {
        ident[i] = src[start + i];
    }
    ident[len] = '\0';
    return ident;
}

vm_ast_node_t vm_lang_lua_conv(const char *src, TSNode node) {
    const char *type = ts_node_type(node);
    size_t num_children = ts_node_child_count(node);
    if (!strcmp(type, "chunk") || !strcmp(type, "block")) {
        if (num_children == 0) {
            return vm_ast_build_nil();
        }
        vm_ast_node_t ret = vm_lang_lua_conv(src, ts_node_child(node, 0));
        for (size_t i = 1; i < num_children; i++) {
            ret = vm_ast_build_do(ret, vm_lang_lua_conv(src, ts_node_child(node, i)));
        }
        return ret;
    }
    if (!strcmp(type, "return_statement")) {
        TSNode value = ts_node_child(node, 1);
        if (ts_node_child_count(value) == 0) {
            return vm_ast_build_return(vm_ast_build_nil());
        } else {
            return vm_ast_build_return(vm_lang_lua_conv(src, ts_node_child(value, 0)));
        }
    }
    if (!strcmp(type, "variable_declaration")) {
        return vm_lang_lua_conv(src, ts_node_child(node, 1));
    }
    if (!strcmp(type, "assignment_statement")) {
        TSNode list = ts_node_child(node, 0);
        TSNode exprs = ts_node_child(node, 2);
        vm_ast_node_t ret = vm_ast_build_nil();
        for (size_t i = 0; i < ts_node_child_count(list); i++) {
            TSNode list_ent = ts_node_child(list, i);
            vm_ast_node_t cur;
            if (i < ts_node_child_count(exprs)) {
                cur = vm_ast_build_set(vm_lang_lua_conv(src, list_ent), vm_lang_lua_conv(src, ts_node_child(exprs, i)));
            } else {
                printf("local = nil\n");
                cur = vm_ast_build_set(vm_lang_lua_conv(src, list_ent), vm_ast_build_nil());
            }
            ret = vm_ast_build_do(ret, cur);
        }
        return ret;
    }
    if (!strcmp(type, "identifier")) {
        char *ident = vm_lang_lua_src(src, node);
        return vm_ast_build_ident(ident);
    }
    if (!strcmp(type, "while_statement")) {
        return vm_ast_build_while(
            vm_lang_lua_conv(src, ts_node_child(node, 1)),
            vm_lang_lua_conv(src, ts_node_child(node, 3))
        );
    }
    if (!strcmp(type, "if_statement")) {
        return vm_ast_build_if(
            vm_lang_lua_conv(src, ts_node_child(node, 1)),
            vm_lang_lua_conv(src, ts_node_child(node, 3)),
            vm_ast_build_nil()
        );
    }
    if (!strcmp(type, "binary_expression")) {
        vm_ast_node_t left = vm_lang_lua_conv(src, ts_node_child(node, 0));
        const char *op = ts_node_type(ts_node_child(node, 1));
        vm_ast_node_t right = vm_lang_lua_conv(src, ts_node_child(node, 2));
        if (!strcmp(op, "+")) {
            return vm_ast_build_add(left, right);
        }
        if (!strcmp(op, "-")) {
            return vm_ast_build_sub(left, right);
        }
        if (!strcmp(op, "*")) {
            return vm_ast_build_mul(left, right);
        }
        if (!strcmp(op, "/")) {
            return vm_ast_build_div(left, right);
        }
        if (!strcmp(op, "%")) {
            return vm_ast_build_mod(left, right);
        }
        if (!strcmp(op, "^")) {
            return vm_ast_build_pow(left, right);
        }
        if (!strcmp(op, "==")) {
            return vm_ast_build_eq(left, right);
        }
        if (!strcmp(op, "~=")) {
            return vm_ast_build_ne(left, right);
        }
        if (!strcmp(op, "<")) {
            return vm_ast_build_lt(left, right);
        }
        if (!strcmp(op, ">")) {
            return vm_ast_build_gt(left, right);
        }
        if (!strcmp(op, "<=")) {
            return vm_ast_build_le(left, right);
        }
        if (!strcmp(op, ">=")) {
            return vm_ast_build_ge(left, right);
        }
        return vm_ast_build_nil();
    }
    if (!strcmp(type, "string")) {
        TSNode content = ts_node_child(node, 1);
        return vm_ast_build_literal(str, vm_lang_lua_src(src, content));
    }
    if (!strcmp(type, "number")) {
        int64_t n;
        sscanf(vm_lang_lua_src(src, node), "%"SCNi64, &n);
        return vm_ast_build_literal(i64, n);
    }
    if (!strcmp(type, "function_call")) {
        vm_ast_node_t func = vm_lang_lua_conv(src, ts_node_child(node, 0));
        TSNode args_node = ts_node_child(node, 1);
        size_t nargs = ts_node_child_count(args_node) - 2;
        vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * nargs);
        for (size_t i = 0; i < nargs; i++) {
            args[i] = vm_lang_lua_conv(src, ts_node_child(args_node, i + 1));
        }
        return vm_ast_build_call(func, nargs, args);
    }
    printf("str = %s\n", ts_node_string(node));
    fflush(stdout);
    return vm_ast_build_nil();
}

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str) {
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_lua());
    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        str,
        strlen(str)
    );

    TSNode root_node = ts_tree_root_node(tree);

    vm_ast_node_t res = vm_lang_lua_conv(str, root_node);

    fflush(stdout);

    return vm_ast_build_do(res, vm_ast_build_return(vm_ast_build_nil()));
}
