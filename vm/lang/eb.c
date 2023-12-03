
#include "eb.h"
#include "../ast/build.h"
#include "../ast/print.h"

typedef struct {
    size_t index;
    const char *str;
    size_t line;
    size_t col;
    vm_config_t *config;
} vm_lang_eb_state_t;

static char vm_lang_eb_peek_char(vm_lang_eb_state_t *state) {
    return state->str[state->index];
}

static void vm_lang_eb_skip_char(vm_lang_eb_state_t *state) {
    char got = vm_lang_eb_peek_char(state);
    if (got == '\0') {
        __builtin_trap();
    }
    if (got == '\n') {
        state->line += 1;
        state->col = 1;
    } else {
        state->col += 1;
    }
    state->index += 1;
}

static char vm_lang_eb_read_char(vm_lang_eb_state_t *state) {
    char ret = vm_lang_eb_peek_char(state);
    vm_lang_eb_skip_char(state);
    return ret;
}

static bool vm_lang_eb_match_keyword(vm_lang_eb_state_t *state, const char *name) {
    size_t len = strlen(name);
    if (!strncmp(&state->str[state->index], name, len)) {
        char next = state->str[state->index + len];
        if (next == ' ' || next == '\t' || next == '\n' || next == '(' || next == ')' || next == '[' || next == ']' || next == '#') {
            for (size_t i = 0; i < len; i++) {
                vm_lang_eb_skip_char(state);
            }
            return true;
        }
    }
    return false;
}

static bool vm_lang_eb_match_char(vm_lang_eb_state_t *state, char c) {
    if (vm_lang_eb_peek_char(state) == c) {
        vm_lang_eb_skip_char(state);
        return true;
    }
    return false;
}

static void vm_lang_eb_strip(vm_lang_eb_state_t *state) {
redo:;
    if (vm_lang_eb_peek_char(state) == '\0') {
        return;
    }
    if (vm_lang_eb_match_char(state, ' ')) {
        goto redo;
    }
    if (vm_lang_eb_match_char(state, '\t')) {
        goto redo;
    }
    if (vm_lang_eb_match_char(state, '\r')) {
        goto redo;
    }
    if (vm_lang_eb_match_char(state, '\n')) {
        goto redo;
    }
}

static vm_ast_node_t vm_lang_eb_expr(vm_lang_eb_state_t *state) {
    vm_lang_eb_strip(state);
    if (vm_lang_eb_match_keyword(state, "do")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_do(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "set")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_set(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "local")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_local(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "add")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_add(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "sub")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_sub(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "mul")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_mul(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "div")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_div(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "mod")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_mod(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "pow")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_pow(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "eq")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_eq(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "ne")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_ne(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "lt")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_lt(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "gt")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_gt(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "le")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_le(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "ge")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_ge(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "if")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        vm_ast_node_t arg3 = vm_lang_eb_expr(state);
        return vm_ast_build_if(arg1, arg2, arg3);
    }
    if (vm_lang_eb_match_keyword(state, "when")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        vm_ast_node_t arg3 = vm_ast_build_nil();
        return vm_ast_build_if(arg1, arg2, arg3);
    }
    if (vm_lang_eb_match_keyword(state, "while")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        vm_ast_node_t arg2 = vm_lang_eb_expr(state);
        return vm_ast_build_while(arg1, arg2);
    }
    if (vm_lang_eb_match_keyword(state, "return")) {
        vm_ast_node_t arg1 = vm_lang_eb_expr(state);
        return vm_ast_build_return(arg1);
    }
    if (vm_lang_eb_match_keyword(state, "print")) {
        vm_ast_node_t args[1] = {
            vm_lang_eb_expr(state),
        };
        return vm_ast_build_call(
            vm_ast_build_load(
                vm_ast_build_load(
                    vm_ast_build_env(),
                    vm_ast_build_literal(str, "io")
                ),
                vm_ast_build_literal(str, "debug")
            ),
            1,
            args
        );
    }
    if (vm_lang_eb_match_char(state, '[')) {
        size_t alloc = 16;
        char *buf = vm_malloc(sizeof(char) * alloc);
        size_t len = 0;
        while (!vm_lang_eb_match_char(state, ']')) {
            if (len + 2 >= alloc) {
                alloc = (len + 2) * 2;
                buf = vm_realloc(buf, sizeof(char) * alloc);
            }
            buf[len++] = vm_lang_eb_read_char(state);
        }
        buf[len] = '\0';
        return vm_ast_build_literal(str, buf);
    }
    char first = vm_lang_eb_peek_char(state);
    if (('0' <= first && first <= '9') || first == '-') {
        if (vm_lang_eb_match_keyword(state, "-9223372036854775808i64")) {
            return vm_ast_build_literal(i64, INT64_MIN);
        }
        int64_t n = 0;
        int64_t sign = 1;
        if (vm_lang_eb_match_char(state, '-')) {
            sign = -1;
        } else if (vm_lang_eb_match_char(state, '+')) {
            sign = 1;
        }
        while (true) {
            char got = vm_lang_eb_peek_char(state);
            if ('0' <= got && got <= '9') {
                n *= 10;
                n += got - '0';
                vm_lang_eb_skip_char(state);
            } else {
                if (vm_lang_eb_match_keyword(state, "i8")) {
                    return vm_ast_build_literal(i8, (int8_t)(n * sign));
                }
                if (vm_lang_eb_match_keyword(state, "i16")) {
                    return vm_ast_build_literal(i16, (int16_t)(n * sign));
                }
                if (vm_lang_eb_match_keyword(state, "i32")) {
                    return vm_ast_build_literal(i32, (int32_t)(n * sign));
                }
                if (vm_lang_eb_match_keyword(state, "i64")) {
                    return vm_ast_build_literal(i64, (int64_t)(n * sign));
                }
                fprintf(stderr, "expected a number suffix at Line %zu, Col %zu", state->line, state->col);
                exit(1);
            }
        }
    }
    size_t alloc = 16;
    char *buf = vm_malloc(sizeof(char) * alloc);
    size_t len = 0;
    while (true) {
        if (vm_lang_eb_peek_char(state) == '\0') {
            break;
        }
        if (vm_lang_eb_match_char(state, ' ')) {
            break;
        }
        if (vm_lang_eb_match_char(state, '\t')) {
            break;
        }
        if (vm_lang_eb_match_char(state, '\r')) {
            break;
        }
        if (vm_lang_eb_match_char(state, '\n')) {
            break;
        }
        if (len + 2 >= alloc) {
            alloc = (len + 2) * 2;
            buf = vm_realloc(buf, sizeof(char) * alloc);
        }
        buf[len++] = vm_lang_eb_read_char(state);
    }
    buf[len] = '\0';
    return vm_ast_build_ident(buf);
}

vm_ast_node_t vm_lang_eb_parse(vm_config_t *config, const char *str) {
    vm_lang_eb_state_t state = (vm_lang_eb_state_t){
        .str = str,
        .line = 1,
        .col = 1,
        .config = config,
    };
    return vm_lang_eb_expr(&state);
}
