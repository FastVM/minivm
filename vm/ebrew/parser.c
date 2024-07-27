#include "../lib.h"
#include "../ir.h"

#include "../ast/ast.h"
#include "../ast/comp.h"
#include "../ast/build.h"
#include "../ast/print.h"

struct vm_lang_eb_binding_t;
struct vm_lang_eb_bindings_t;
struct vm_lang_eb_parser_t;

typedef struct vm_lang_eb_binding_t vm_lang_eb_binding_t;
typedef struct vm_lang_eb_bindings_t vm_lang_eb_bindings_t;
typedef struct vm_lang_eb_parser_t vm_lang_eb_parser_t;

struct vm_lang_eb_bindings_t {
    size_t len;
    vm_lang_eb_binding_t *ptr;
    size_t alloc;
};

struct vm_lang_eb_binding_t {
    const char *name;
    vm_lang_eb_bindings_t args;
    bool is_func;
};

struct vm_lang_eb_parser_t {
    vm_lang_eb_bindings_t defs;
    vm_t *vm;
    const char *src;
    const char *file;
    size_t head;
};

vm_lang_eb_binding_t vm_lang_eb_binding_of_none() {
    return (vm_lang_eb_binding_t) {0};
}

vm_lang_eb_binding_t vm_lang_eb_binding_of_name(const char *name) {
    return (vm_lang_eb_binding_t) {
        .is_func = false,
        .name = name,
    };
}

vm_lang_eb_binding_t vm_lang_eb_binding_of_func(const char *name, vm_lang_eb_bindings_t args) {
    return (vm_lang_eb_binding_t) {
        .is_func = true,
        .name = name,
        .args = args,
    };
}

void vm_lang_eb_bindings_push(vm_lang_eb_bindings_t *bindings, vm_lang_eb_binding_t elem) {
    if (bindings->len + 1 >= bindings->alloc) {
        bindings->alloc = (bindings->len + 1) * 2;
        bindings->ptr = vm_realloc(bindings->ptr, sizeof(vm_lang_eb_binding_t) * bindings->alloc);
    }
    bindings->ptr[bindings->len++] = elem;
}

vm_lang_eb_binding_t *vm_lang_eb_bindings_find(vm_lang_eb_bindings_t *bindings, const char *name) {
    for (size_t i = 0; i < bindings->len; i++) {
       vm_lang_eb_binding_t *binding = &bindings->ptr[i];
        if (!strcmp(binding->name, name)) {
            return binding;
        }
    }
    return NULL;
}

vm_ast_node_t vm_lang_eb_wrap0(vm_ast_node_t (*inner)(vm_lang_eb_parser_t *ctx), vm_lang_eb_parser_t *ctx) {
    vm_location_t start = (vm_location_t) {
        .byte = ctx->head,
    };
    vm_ast_node_t node = inner(ctx);
    vm_location_t stop = (vm_location_t) {
        .byte = ctx->head,
    };
    node.info.range.start = start;
    node.info.range.stop = stop;
    node.info.range.src = ctx->src;
    node.info.range.file = ctx->src;
    return node;
}

vm_ast_node_t vm_lang_eb_wrap1(vm_ast_node_t (*inner)(vm_lang_eb_parser_t *ctx, void *data), vm_lang_eb_parser_t *ctx, void *data) {
    vm_location_t start = (vm_location_t) {
        .byte = ctx->head,
    };
    vm_ast_node_t node = inner(ctx, data);
    vm_location_t stop = (vm_location_t) {
        .byte = ctx->head,
    };
    node.info.range.start = start;
    node.info.range.stop = stop;
    node.info.range.src = ctx->src;
    node.info.range.file = ctx->src;
    return node;
}

bool vm_lang_eb_parser_done(vm_lang_eb_parser_t *env) {
    return env->src[env->head] == '\0';
}

void vm_lang_eb_parser_skip(vm_lang_eb_parser_t *env) {
    if (!vm_lang_eb_parser_done(env)) {
        env->head += 1;
    }
}

char vm_lang_eb_parser_first(vm_lang_eb_parser_t *env) {
    if (vm_lang_eb_parser_done(env)) {
        return '\0';
    }
    return env->src[env->head];
}

char vm_lang_eb_parser_read(vm_lang_eb_parser_t *env) {
    char ret = vm_lang_eb_parser_first(env);
    vm_lang_eb_parser_skip(env);
    return ret;
}

void vm_lang_eb_parser_skip_space(vm_lang_eb_parser_t *env) {
    while (true) {
        char first = vm_lang_eb_parser_first(env);
        if (first == ' ' || first == '\n' || first == '\r' || first == '\t') {
            vm_lang_eb_parser_skip(env);
        } else if (first == '#') {
            vm_lang_eb_parser_skip(env);
            while (true) {
                char first = vm_lang_eb_parser_first(env);
                if (first == '\0') {
                    break;
                }
                if (first == '#') {
                    vm_lang_eb_parser_skip(env);
                    break;
                }
            }
        } else {
            break;
        }
    }
}

char *vm_lang_eb_parser_read_name(vm_lang_eb_parser_t *env) {
    vm_lang_eb_parser_skip_space(env);
    vm_io_buffer_t *buf = vm_io_buffer_new();
    while (true) {
        char first = vm_lang_eb_parser_first(env);
        if (!('0' <= first && first <= '9') && !('a' <= first && first <= 'z') && !('A' <= first && first <= 'Z') && first != '-' && first != '_') {
            break;
        }
        vm_io_buffer_format(buf, "%c", (int) first);
        vm_lang_eb_parser_skip(env);
    }
    char *str = buf->buf;
    vm_free(buf);
    printf("name = %s\n", str);
    return str;
}

vm_lang_eb_bindings_t vm_lang_eb_parser_read_args(vm_lang_eb_parser_t *env) {
    vm_lang_eb_bindings_t ret = (vm_lang_eb_bindings_t) {0};
    while (true) {
        vm_lang_eb_parser_skip_space(env);
        char first = vm_lang_eb_parser_first(env);
        if (first == '\0') {
            return ret;
        }
        if (first == ')') {
            vm_lang_eb_parser_skip(env);
            break;
        }
        if (first == '(') {
            vm_lang_eb_parser_skip(env);
            vm_lang_eb_parser_skip_space(env);
            const char *name = vm_lang_eb_parser_read_name(env);
            vm_lang_eb_bindings_t args = vm_lang_eb_parser_read_args(env);
            vm_lang_eb_bindings_push(&ret, vm_lang_eb_binding_of_func(name, args));
        } else {
            const char *name = vm_lang_eb_parser_read_name(env);
            vm_lang_eb_bindings_push(&ret, vm_lang_eb_binding_of_name(name));
        }
    }
    return ret;
}

vm_ast_node_t vm_lang_eb_parser_read_match(vm_lang_eb_parser_t *env, void *data);
vm_ast_node_t vm_lang_eb_parser_read_call(vm_lang_eb_parser_t *env, void *data);

vm_ast_node_t vm_lang_eb_parser_read_match(vm_lang_eb_parser_t *env, void *data) {
    vm_lang_eb_binding_t *type = data;
    vm_lang_eb_parser_skip_space(env);
    char first = vm_lang_eb_parser_first(env);
    if (first == '\"') {
        __builtin_trap();
    } else if (first == '\'') {
        __builtin_trap();
    } else {
        // bool starts_paren = false;
        // while (first == '(') {
        //     starts_paren = true;
        //     vm_lang_eb_parser_skip(env);
        //     vm_lang_eb_parser_skip_space(env);
        //     first = vm_lang_eb_parser_first(env);
        // }
        char *name = vm_lang_eb_parser_read_name(env);
        // if (starts_paren) {
        //     vm_lang_eb_parser_skip_space(env);
        //     if (vm_lang_eb_parser_first(env) == ')') {
        //         type->is_func = true;
        //     }
        // }
        if (name[0] == '\0') {
            __builtin_trap();
        }
        if (type->is_func) {
            size_t nargs = type->args.len;
            vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * nargs);

            size_t len0 = env->defs.len;
            for (size_t i = 0; i < nargs; i++) {
                args[i] = vm_ast_build_ident(type->args.ptr[i].name);
                vm_lang_eb_bindings_push(&env->defs, type->args.ptr[i]);
            }
            vm_lang_eb_binding_t none_bind = vm_lang_eb_binding_of_none();
            vm_ast_node_t body = vm_lang_eb_parser_read_call(env, name);
            env->defs.len = len0;

            return vm_ast_build_lambda(
                vm_ast_build_nil(),
                vm_ast_build_args(nargs, args),
                body
            );
        } else {
            return vm_lang_eb_parser_read_call(env, name);
        }
    }
}

vm_ast_node_t vm_lang_eb_parser_read_call(vm_lang_eb_parser_t *env, void *data) {
    const char *name = data;
    vm_lang_eb_binding_t *arg_types = vm_lang_eb_bindings_find(&env->defs, name);
    if (arg_types != NULL) {
        if (arg_types->is_func) {
            size_t nargs = 0;
            vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * arg_types->args.len);
            for (size_t i = 0; i < arg_types->args.len; i++) {
                args[nargs++] = vm_lang_eb_parser_read_match(env, &arg_types->args.ptr[i]);
            }
            return vm_ast_build_call(vm_ast_build_ident(name), nargs, args);
        } else {
            return vm_ast_build_ident(name);
        }
    }
    if ('0' <= name[0] && name[0] <= '9') {
        double n;
        sscanf(name, "%lf", &n);
        return vm_ast_build_literal(vm_obj_of_number(n));
    }
    return vm_ast_build_ident(name);
}

vm_ast_node_t vm_lang_eb_parser_read_def(vm_lang_eb_parser_t *env) {
    char first = vm_lang_eb_parser_first(env);
    if (first == '(') {
        vm_lang_eb_parser_skip(env);
        vm_lang_eb_parser_skip_space(env);
        char *name = vm_lang_eb_parser_read_name(env);
        vm_lang_eb_bindings_t args = vm_lang_eb_parser_read_args(env);
        vm_lang_eb_binding_t bind = vm_lang_eb_binding_of_func(name, args);
        vm_lang_eb_bindings_push(&env->defs, bind);
        vm_lang_eb_parser_skip_space(env);
        if (vm_lang_eb_parser_first(env) == '?') {
            return vm_ast_build_load(
                vm_ast_build_literal(env->vm->std),
                vm_ast_build_literal(vm_obj_of_string(env->vm, name))
            );
        } else {
            size_t nargs = bind.args.len;
            vm_ast_node_t *args = vm_malloc(sizeof(vm_ast_node_t) * nargs);

            size_t len0 = env->defs.len;
            for (size_t i = 0; i < nargs; i++) {
                args[i] = vm_ast_build_ident(bind.args.ptr[i].name);
                vm_lang_eb_bindings_push(&env->defs, bind.args.ptr[i]);
            }
            vm_lang_eb_binding_t none_bind = vm_lang_eb_binding_of_none();
            vm_ast_node_t body = vm_lang_eb_parser_read_match(env, &none_bind);
            env->defs.len = len0;

            return vm_ast_build_local(
                vm_ast_build_ident(name),
                vm_ast_build_lambda(
                    vm_ast_build_ident(name),
                    vm_ast_build_args(nargs, args),
                    vm_ast_build_return(body)
                )
            );
        }
    } else {
        vm_lang_eb_binding_t none_bind = vm_lang_eb_binding_of_none();
        return vm_lang_eb_parser_read_match(env, &none_bind);
    }
}

vm_ast_node_t vm_lang_eb_parser_read_defs(vm_lang_eb_parser_t *env) {
    vm_ast_node_t all = vm_ast_build_literal(vm_obj_of_nil());
    while (true) {
        vm_lang_eb_parser_skip_space(env);
        if (vm_lang_eb_parser_done(env)) {
            break;
        }
        vm_ast_node_t def = vm_lang_eb_parser_read_def(env);
        all = vm_ast_build_do(all, def);
    }
    return all;
}

vm_ir_block_t *vm_lang_eb_compile(vm_t *vm, const char *src, const char *file) {
    vm_lang_eb_parser_t state = (vm_lang_eb_parser_t) {
        .vm = vm,
        .src = vm_strdup(src),
        .file = vm_strdup(file),
    };
    vm_ast_node_t ast = vm_ast_build_return(vm_lang_eb_parser_read_defs(&state));
    vm_io_buffer_t *out = vm_io_buffer_new();
    vm_ast_print_node(out, 0, "", ast);
    printf("%.*s\n", (int) out->len, out->buf);
    vm_ir_block_t *block = vm_ast_comp_more(vm, ast);
    // vm_ast_free_node(ast);
    return block;
}
