
#include "../ast/ast.h"
#include "../../vendor/tree-sitter/lib/include/tree_sitter/api.h"
#include "../ast/build.h"
#include "../ast/comp.h"
#include "../io.h"
#include "../ir.h"

const TSLanguage *tree_sitter_lua(void);

typedef struct {
    const char *src;
    vm_t *vm;
    size_t *nsyms;
} vm_lang_lua_t;

char *vm_lang_lua_src(vm_lang_lua_t src, TSNode node) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    uint32_t len = end - start;
    char *str = vm_malloc(sizeof(char) * (len + 1));
    for (size_t i = 0; i < len; i++) {
        str[i] = src.src[start + i];
    }
    str[len] = '\0';
    return str;
}

char *vm_lang_lua_gensym(vm_lang_lua_t src) {
    char *buf = vm_malloc(sizeof(char) * 32);
    *src.nsyms += 1;
    snprintf(buf, 31, "gensym.%zu", *src.nsyms);
    return buf;
}

vm_ast_node_t vm_lang_lua_conv_raw(vm_lang_lua_t src, TSNode node);

vm_ast_node_t vm_lang_lua_conv(vm_lang_lua_t src, TSNode node) {
    vm_location_t start = (vm_location_t) {
        .byte = ts_node_start_byte(node),
    };
    vm_ast_node_t ret = vm_lang_lua_conv_raw(src, node);
    vm_location_t stop = (vm_location_t) {
        .byte = ts_node_end_byte(node),
    };
    ret.info.type = ts_node_type(node);
    ret.info.range.start = start;
    ret.info.range.stop = stop;
    return ret;
}

vm_ast_node_t vm_lang_lua_conv_raw(vm_lang_lua_t src, TSNode node) {
    if (ts_node_is_error(node) || ts_node_is_missing(node)) {
        vm_io_buffer_t *buf = vm_io_buffer_new();
        vm_io_buffer_format(buf, "parsing %s node failed", ts_node_type(ts_node_parent(node)));
        const char *msg = buf->buf;
        vm_free(buf);
        return (vm_ast_node_t){
            .type = VM_AST_NODE_LITERAL,
            .value.literal = (vm_obj_t){
                .tag = VM_TAG_ERROR,
                .value.str = msg
            },
        };
    }
    const char *type = ts_node_type(node);
    size_t num_children = ts_node_child_count(node);
    if (!strcmp(type, "comment")) {
        return vm_ast_build_nil();
    }
    if (!strcmp(type, "chunk") || !strcmp(type, "block")) {
        if (num_children == 0) {
            return vm_ast_build_nil();
        }
        vm_ast_node_t ret = vm_lang_lua_conv(src, ts_node_child(node, 0));
        for (size_t i = 1; i < num_children; i++) {
            ret = vm_ast_build_do(ret, vm_lang_lua_conv(src, ts_node_child(node, i)));
        }
        return vm_ast_build_scope(ret);
    }
    if (!strcmp(type, "break_statement")) {
        return vm_ast_build_break();
    }
    if (!strcmp(type, "function_definition")) {
        TSNode params = ts_node_child(node, 1);
        size_t nargs = 0;
        for (size_t i = 0; i < ts_node_child_count(params); i++) {
            TSNode arg = ts_node_child(params, i);
            const char *argtype = ts_node_type(arg);
            if (!strcmp(argtype, "identifier")) {
                nargs += 1;
            }
        }
        vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * nargs);
        size_t write = 0;
        for (size_t i = 0; i < ts_node_child_count(params); i++) {
            TSNode arg = ts_node_child(params, i);
            const char *argtype = ts_node_type(arg);
            if (!strcmp(argtype, "identifier")) {
                args[write++] = vm_lang_lua_conv(src, arg);
            }
        }
        return vm_ast_build_lambda(
            vm_ast_build_nil(),
            vm_ast_build_args(nargs, args),
            vm_lang_lua_conv(src, ts_node_child(node, 2))
        );
    }
    if (!strcmp(type, "function_declaration")) {
        if (!strcmp("local", ts_node_type(ts_node_child(node, 0)))) {
            TSNode self = ts_node_child(node, 2);
            TSNode params = ts_node_child(node, 3);
            TSNode body = ts_node_child(node, 4);
            vm_ast_node_t body_node;
            if (!!strcmp("end", ts_node_type(body))) {
                body_node = vm_lang_lua_conv(src, body);
            }
            size_t nargs = 0;
            for (size_t i = 0; i < ts_node_child_count(params); i++) {
                TSNode arg = ts_node_child(params, i);
                const char *argtype = ts_node_type(arg);
                if (!strcmp(argtype, "identifier")) {
                    nargs += 1;
                }
            }
            vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * nargs);
            size_t write = 0;
            for (size_t i = 0; i < ts_node_child_count(params); i++) {
                TSNode arg = ts_node_child(params, i);
                const char *argtype = ts_node_type(arg);
                if (!strcmp(argtype, "identifier")) {
                    args[write++] = vm_lang_lua_conv(src, arg);
                }
            }
            return vm_ast_build_local(
                vm_lang_lua_conv(src, self),
                vm_ast_build_lambda(
                    vm_lang_lua_conv(src, self),
                    vm_ast_build_args(nargs, args),
                    body_node
                )
            );
        } else if (!strcmp("function", ts_node_type(ts_node_child(node, 0)))) {
            TSNode target = ts_node_child(node, 1);
            if (!strcmp("method_index_expression", ts_node_type(target))) {
                TSNode params = ts_node_child(node, 2);
                vm_ast_node_t body_node = vm_ast_build_nil();
                if (num_children > 3) {
                    TSNode body = ts_node_child(node, 3);
                    if (!!strcmp("end", ts_node_type(body))) {
                        body_node = vm_lang_lua_conv(src, body);
                    }
                }
                size_t nargs = 1;
                for (size_t i = 0; i < ts_node_child_count(params); i++) {
                    TSNode arg = ts_node_child(params, i);
                    const char *argtype = ts_node_type(arg);
                    if (!strcmp(argtype, "identifier")) {
                        nargs += 1;
                    }
                }
                vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * nargs);
                args[0] = vm_ast_build_ident(vm_strdup("self"));
                size_t write = 1;
                for (size_t i = 0; i < ts_node_child_count(params); i++) {
                    TSNode arg = ts_node_child(params, i);
                    const char *argtype = ts_node_type(arg);
                    if (!strcmp(argtype, "identifier")) {
                        args[write++] = vm_lang_lua_conv(src, arg);
                    }
                }
                return vm_ast_build_set(
                    vm_ast_build_load(
                        vm_lang_lua_conv(src, ts_node_child(target, 0)),
                        vm_ast_build_literal(str, vm_lang_lua_src(src, ts_node_child(target, 2)))
                    ),
                    vm_ast_build_lambda(
                        vm_ast_build_nil(),
                        vm_ast_build_args(nargs, args),
                        body_node
                    )
                );
            } else {
                TSNode params = ts_node_child(node, 2);
                vm_ast_node_t body_node = vm_ast_build_nil();
                if (num_children > 3) {
                    TSNode body = ts_node_child(node, 3);
                    if (!!strcmp("end", ts_node_type(body))) {
                        body_node = vm_lang_lua_conv(src, body);
                    }
                }
                size_t nargs = 0;
                for (size_t i = 0; i < ts_node_child_count(params); i++) {
                    TSNode arg = ts_node_child(params, i);
                    const char *argtype = ts_node_type(arg);
                    if (!strcmp(argtype, "identifier")) {
                        nargs += 1;
                    }
                }
                vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * nargs);
                size_t write = 0;
                for (size_t i = 0; i < ts_node_child_count(params); i++) {
                    TSNode arg = ts_node_child(params, i);
                    const char *argtype = ts_node_type(arg);
                    if (!strcmp(argtype, "identifier")) {
                        args[write++] = vm_lang_lua_conv(src, arg);
                    }
                }
                return vm_ast_build_set(
                    vm_lang_lua_conv(src, target),
                    vm_ast_build_lambda(
                        vm_ast_build_nil(),
                        vm_ast_build_args(nargs, args),
                        body_node
                    )
                );
            }
        }
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
        TSNode as = ts_node_child(node, 1);
        TSNode list = ts_node_child(as, 0);
        TSNode exprs = ts_node_child(as, 2);
        vm_ast_node_t ret = vm_ast_build_nil();
        for (size_t i = 0; i < ts_node_child_count(list); i += 2) {
            TSNode list_ent = ts_node_child(list, i);
            vm_ast_node_t cur;
            if (i < ts_node_child_count(exprs)) {
                cur = vm_ast_build_local(vm_lang_lua_conv(src, list_ent), vm_lang_lua_conv(src, ts_node_child(exprs, i)));
            } else {
                cur = vm_ast_build_local(vm_lang_lua_conv(src, list_ent), vm_ast_build_nil());
            }
            ret = vm_ast_build_do(ret, cur);
        }
        return ret;
    }
    if (!strcmp(type, "assignment_statement")) {
        TSNode list = ts_node_child(node, 0);
        TSNode exprs = ts_node_child(node, 2);
        vm_ast_node_t ret = vm_ast_build_nil();
        for (size_t i = 0; i < ts_node_child_count(list); i += 2) {
            TSNode list_ent = ts_node_child(list, i);
            vm_ast_node_t cur;
            if (i < ts_node_child_count(exprs)) {
                cur = vm_ast_build_set(vm_lang_lua_conv(src, list_ent), vm_lang_lua_conv(src, ts_node_child(exprs, i)));
            } else {
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
    if (!strcmp(type, "for_statement")) {
        TSNode clause = ts_node_child(node, 1);
        const char *clause_type = ts_node_type(clause);
        if (!strcmp(clause_type, "for_numeric_clause")) {
            size_t len = ts_node_child_count(clause);
            vm_ast_node_t stop_expr = vm_lang_lua_conv(src, ts_node_child(clause, 4));
            vm_ast_node_t step_expr;
            if (len == 5) {
                switch (src.vm->use_num) {
                    case VM_USE_NUM_I8: {
                        step_expr = vm_ast_build_literal(i8, 1);
                        break;
                    }
                    case VM_USE_NUM_I16: {
                        step_expr = vm_ast_build_literal(i16, 1);
                        break;
                    }
                    case VM_USE_NUM_I32: {
                        step_expr = vm_ast_build_literal(i32, 1);
                        break;
                    }
                    case VM_USE_NUM_I64: {
                        step_expr = vm_ast_build_literal(i64, 1);
                        break;
                    }
                    case VM_USE_NUM_F32: {
                        step_expr = vm_ast_build_literal(f32, 1);
                        break;
                    }
                    case VM_USE_NUM_F64: {
                        step_expr = vm_ast_build_literal(f64, 1);
                        break;
                    }
                }
            } else {
                step_expr = vm_lang_lua_conv(src, ts_node_child(clause, 6));
            }
            const char *step_var = vm_lang_lua_gensym(src);
            const char *stop_var = vm_lang_lua_gensym(src);
            return vm_ast_build_block(
                4,
                vm_ast_build_local(
                    vm_lang_lua_conv(src, ts_node_child(clause, 0)),
                    vm_lang_lua_conv(src, ts_node_child(clause, 2))
                ),
                vm_ast_build_local(vm_ast_build_ident(vm_strdup(step_var)), step_expr),
                vm_ast_build_local(vm_ast_build_ident(vm_strdup(stop_var)), stop_expr),
                vm_ast_build_while(
                    vm_ast_build_le(
                        vm_lang_lua_conv(src, ts_node_child(clause, 0)),
                        vm_ast_build_ident(stop_var)
                    ),
                    vm_ast_build_do(
                        vm_lang_lua_conv(src, ts_node_child(node, 3)),
                        vm_ast_build_set(
                            vm_lang_lua_conv(src, ts_node_child(clause, 0)),
                            vm_ast_build_add(
                                vm_lang_lua_conv(src, ts_node_child(clause, 0)),
                                vm_ast_build_ident(step_var)
                            )
                        )
                    )
                )
            );
        }
    }
    if (!strcmp(type, "while_statement")) {
        return vm_ast_build_while(
            vm_lang_lua_conv(src, ts_node_child(node, 1)),
            vm_lang_lua_conv(src, ts_node_child(node, 3))
        );
    }
    if (!strcmp(type, "do_expression")) {
        return vm_lang_lua_conv(src, ts_node_child(node, 1));
    }
    if (!strcmp(type, "if_expression")) {
        vm_ast_node_t els = vm_ast_build_nil();
        for (size_t i = num_children - 2; i >= 4; i--) {
            TSNode child = ts_node_child(node, i);
            const char *child_type = ts_node_type(child);
            if (!strcmp(child_type, "else_expression")) {
                els = vm_lang_lua_conv(src, ts_node_child(child, 1));
            } else if (!strcmp(child_type, "elseif_expression")) {
                els = vm_ast_build_if(
                    vm_lang_lua_conv(src, ts_node_child(child, 1)),
                    vm_lang_lua_conv(src, ts_node_child(child, 3)),
                    els
                );
            } else {
                __builtin_trap();
            }
        }
        return vm_ast_build_if(
            vm_lang_lua_conv(src, ts_node_child(node, 1)),
            vm_lang_lua_conv(src, ts_node_child(node, 3)),
            els
        );
    }
    if (!strcmp(type, "unary_expression")) {
        const char *op = ts_node_type(ts_node_child(node, 0));
        vm_ast_node_t right = vm_lang_lua_conv(src, ts_node_child(node, 1));
        if (!strcmp(op, "not")) {
            return vm_ast_build_not(right);
        }
        if (!strcmp(op, "#")) {
            return vm_ast_build_len(right);
        }
        if (!strcmp(op, "-")) {
            switch (src.vm->use_num) {
                case VM_USE_NUM_I8: {
                    return vm_ast_build_sub(vm_ast_build_literal(i8, 0), right);
                }
                case VM_USE_NUM_I16: {
                    return vm_ast_build_sub(vm_ast_build_literal(i16, 0), right);
                }
                case VM_USE_NUM_I32: {
                    return vm_ast_build_sub(vm_ast_build_literal(i32, 0), right);
                }
                case VM_USE_NUM_I64: {
                    return vm_ast_build_sub(vm_ast_build_literal(i64, 0), right);
                }
                case VM_USE_NUM_F32: {
                    return vm_ast_build_sub(vm_ast_build_literal(f32, 0), right);
                }
                case VM_USE_NUM_F64: {
                    return vm_ast_build_sub(vm_ast_build_literal(f64, 0), right);
                }
            }
        }
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
        if (!strcmp(op, "//")) {
            return vm_ast_build_idiv(left, right);
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
        if (!strcmp(op, "or")) {
            return vm_ast_build_or(left, right);
        }
        if (!strcmp(op, "and")) {
            return vm_ast_build_and(left, right);
        }
        if (!strcmp(op, "..")) {
            return vm_ast_build_concat(left, right);
        }
        vm_ast_free_node(left);
        vm_ast_free_node(right);
        return vm_ast_build_error(vm_io_format("unknown operator: %s", op));
    }
    if (!strcmp(type, "string")) {
        if (ts_node_child_count(node) == 2) {
            char *ret = vm_malloc(sizeof(char) * 1);
            ret[0] = '\0';
            return vm_ast_build_literal(str, ret);
        }
        TSNode content = ts_node_child(node, 1);
        char *val = vm_lang_lua_src(src, content);
        size_t alloc = strlen(val);
        char *buf = vm_malloc(sizeof(char) * (alloc + 1));
        const char *ret = buf;
        while (*val != '\0') {
            char got = *val++;
            if (got == '\\') {
                switch (*val++) {
                    case '\\': {
                        *buf++ = '\\';
                        break;
                    }
                    case 'n': {
                        *buf++ = '\n';
                        break;
                    }
                    case 't': {
                        *buf++ = '\t';
                        break;
                    }
                    case 'r': {
                        *buf++ = '\r';
                        break;
                    }
                    default: {
                        break;
                    }
                }
            } else {
                *buf++ = got;
            }
        }
        *buf++ = '\0';
        vm_free(val);
        return vm_ast_build_literal(str, ret);
    }
    if (!strcmp(type, "number")) {
        const char *str = vm_lang_lua_src(src, node);
        vm_ast_node_t ret;
        switch (src.vm->use_num) {
            case VM_USE_NUM_F32: {
                float n;
                sscanf(str, "%f", &n);
                ret = vm_ast_build_literal(f32, n);
                break;
            }
            case VM_USE_NUM_F64: {
                double n;
                sscanf(str, "%lf", &n);
                ret = vm_ast_build_literal(f64, n);
                break;
            }
            case VM_USE_NUM_I8: {
                int8_t n;
                sscanf(str, "%" SCNi8, &n);
                ret = vm_ast_build_literal(i8, n);
                break;
            }
            case VM_USE_NUM_I16: {
                int16_t n;
                sscanf(str, "%" SCNi16, &n);
                ret = vm_ast_build_literal(i16, n);
                break;
            }
            case VM_USE_NUM_I32: {
                int32_t n;
                sscanf(str, "%" SCNi32, &n);
                ret = vm_ast_build_literal(i32, n);
                break;
            }
            case VM_USE_NUM_I64: {
                int64_t n;
                sscanf(str, "%" SCNi64, &n);
                ret = vm_ast_build_literal(i64, n);
                break;
            }
            default: {
                __builtin_trap();
            }
        }
        vm_free(str);
        return ret;
    }
    if (!strcmp(type, "function_call")) {
        TSNode func_node = ts_node_child(node, 0);
        if (!strcmp(ts_node_type(func_node), "method_index_expression")) {
            const char *obj = vm_lang_lua_gensym(src);
            vm_ast_node_t set_obj = vm_ast_build_local(vm_ast_build_ident(obj), vm_lang_lua_conv(src, ts_node_child(func_node, 0)));
            vm_ast_node_t index = vm_ast_build_literal(str, vm_lang_lua_src(src, ts_node_child(func_node, 2)));
            vm_ast_node_t func = vm_ast_build_load(vm_ast_build_ident(vm_strdup(obj)), index);
            TSNode args_node = ts_node_child(node, 1);
            size_t nargs = ts_node_child_count(args_node);
            vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * (nargs + 1));
            size_t real_nargs = 0;
            args[real_nargs++] = vm_ast_build_ident(vm_strdup(obj));
            for (size_t i = 0; i < ts_node_child_count(args_node); i += 1) {
                TSNode arg = ts_node_child(args_node, i);
                const char *name = ts_node_type(arg);
                if (!strcmp(name, "(") || !strcmp(name, ",") || !strcmp(name, ")")) {
                    continue;
                }
                args[real_nargs++] = vm_lang_lua_conv(src, arg);
            }
            return vm_ast_build_do(set_obj, vm_ast_build_call(func, real_nargs, args));
        } else {
            vm_ast_node_t func = vm_lang_lua_conv(src, func_node);
            TSNode args_node = ts_node_child(node, 1);
            size_t nargs = ts_node_child_count(args_node);
            vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * nargs);
            size_t real_nargs = 0;
            for (size_t i = 0; i < ts_node_child_count(args_node); i += 1) {
                TSNode arg = ts_node_child(args_node, i);
                const char *name = ts_node_type(arg);
                if (!strcmp(name, "(") || !strcmp(name, ",") || !strcmp(name, ")")) {
                    continue;
                }
                args[real_nargs++] = vm_lang_lua_conv(src, arg);
            }
            return vm_ast_build_call(func, real_nargs, args);
        }
    }
    if (!strcmp(type, "parenthesized_expression")) {
        return vm_lang_lua_conv(src, ts_node_child(node, 1));
    }
    if (!strcmp(type, "table_constructor")) {
        const char *var = vm_lang_lua_gensym(src);
        size_t nfields = 1;
        vm_ast_node_t built = vm_ast_build_local(vm_ast_build_ident(var), vm_ast_build_new());
        for (size_t i = 0; i < num_children; i++) {
            TSNode sub = ts_node_child(node, i);
            const char *name = ts_node_type(sub);
            if (!strcmp(name, "{") || !strcmp(name, ",") || !strcmp(name, ";") || !strcmp(name, "}")) {
                continue;
            }
            vm_ast_node_t cur = vm_ast_build_nil();
            if (!strcmp(name, "field")) {
                size_t sub_children = ts_node_child_count(sub);
                if (sub_children == 1) {
                    vm_ast_node_t target = vm_ast_build_load(vm_ast_build_ident(vm_strdup(var)), vm_ast_build_literal(i32, nfields));
                    vm_ast_node_t value = vm_lang_lua_conv(src, ts_node_child(sub, 0));
                    cur = vm_ast_build_set(target, value);
                    nfields += 1;
                } else if (sub_children == 3) {
                    char *key_field = vm_lang_lua_src(src, ts_node_child(sub, 0));
                    vm_ast_node_t key = vm_ast_build_literal(str, key_field);
                    vm_ast_node_t value = vm_lang_lua_conv(src, ts_node_child(sub, 2));
                    cur = vm_ast_build_set(vm_ast_build_load(vm_ast_build_ident(vm_strdup(var)), key), value);
                } else if (sub_children == 5) {
                    vm_ast_node_t key = vm_lang_lua_conv(src, ts_node_child(sub, 1));
                    vm_ast_node_t value = vm_lang_lua_conv(src, ts_node_child(sub, 4));
                    cur = vm_ast_build_set(vm_ast_build_load(vm_ast_build_ident(vm_strdup(var)), key), value);
                } else {
                    printf("%s\n", ts_node_string(sub));
                }
            } else {
                return vm_ast_build_error(vm_io_format("unknown field type: %s", name));
            }
            built = vm_ast_build_do(built, cur);
        }
        return vm_ast_build_do(built, vm_ast_build_ident(vm_strdup(var)));
    }
    if (!strcmp(type, "bracket_index_expression")) {
        vm_ast_node_t table = vm_lang_lua_conv(src, ts_node_child(node, 0));
        vm_ast_node_t index = vm_lang_lua_conv(src, ts_node_child(node, 2));
        return vm_ast_build_load(table, index);
    }
    if (!strcmp(type, "dot_index_expression")) {
        vm_ast_node_t table = vm_lang_lua_conv(src, ts_node_child(node, 0));
        char *field = vm_lang_lua_src(src, ts_node_child(node, 2));
        return vm_ast_build_load(table, vm_ast_build_literal(str, field));
    }
    if (!strcmp(type, "nil")) {
        return vm_ast_build_nil();
    }
    if (!strcmp(type, "true")) {
        return vm_ast_build_literal(b, true);
    }
    if (!strcmp(type, "false")) {
        return vm_ast_build_literal(b, false);
    }
    return vm_ast_build_error(vm_io_format("unknown node type: %s", type));
}

vm_ast_node_t vm_lang_lua_parse(vm_t *vm, const char *str) {
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_lua());
    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        str,
        strlen(str)
    );

    TSNode root_node = ts_tree_root_node(tree);

    size_t nsyms = 0;

    vm_lang_lua_t src = (vm_lang_lua_t){
        .src = str,
        .vm = vm,
        .nsyms = &nsyms,
    };

    vm_ast_node_t res = vm_lang_lua_conv(src, root_node);

    ts_tree_delete(tree);

    ts_parser_delete(parser);

    // return vm_ast_build_do(res, vm_ast_build_return(vm_ast_build_nil()));
    return vm_ast_build_return(res);
}

vm_block_t *vm_compile(vm_t *vm, const char *src) {
    vm_ast_node_t ast = vm_lang_lua_parse(vm, src);
    vm_blocks_srcs_t *next = vm_malloc(sizeof(vm_blocks_srcs_t));
    *next = (vm_blocks_srcs_t){
        .last = vm->blocks->srcs,
        .src = src,
    };
    vm->blocks->srcs = next;
    size_t len = vm->blocks->len;
    vm_block_t *block = vm_ast_comp_more(vm, ast);
    vm_ast_free_node(ast);
    if (vm->dump_ir) {
        vm_io_buffer_t *buf = vm_io_buffer_new();
        for (size_t i = len; i < vm->blocks->len; i++) {
            vm_io_format_block(buf, vm->blocks->blocks[i]);
            vm_io_buffer_format(buf, "\n");
        }
        fprintf(stdout, "%s\n", buf->buf);
        vm_free(buf->buf);
        vm_free(buf);
    }
    return block;
}
