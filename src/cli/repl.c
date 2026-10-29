
#include "repl.h"
#include "main.h"
#include "../../vendor/isocline/include/isocline.h"
#include "../lua/front/chunk.h"
#include "../lua/rt/api/std.h"
#include "../lua/rt/lua.h"
#include "../lua/rt/meta.h"
#include "../run/run.h"
#include "../value/error.h"
#include "../value/obj.h"
#include "../value/string.h"
#include "../value/table.h"

void mx_repl_completer(ic_completion_env_t *cenv, const char *prefix) {
    mx_t *vm = ic_completion_arg(cenv);
    const char *dot = strrchr(prefix, '.');
    mx_obj_table_t *t = mx_obj_get_table(mx_lua_of(vm)->std);
    const char *tail = prefix;
    bool ok = true;
    if (dot != NULL) {
        size_t hlen = (size_t)(dot - prefix);
        mx_obj_t head = mx_table_get(t, mx_obj_intern_strn(vm, prefix, (uint32_t)hlen));
        ok = mx_obj_is_table(head);
        if (ok) {
            t = mx_obj_get_table(head);
            tail = dot + 1;
        }
    }
    if (ok) {
        size_t plen = strlen(tail);
        mx_obj_t key = mx_obj_of_nil();
        mx_table_pair_t pair = mx_table_pairs_next(vm, t, key);
        bool room = true;
        while (room && pair.is_valid && !mx_obj_is_nil(pair.key)) {
            key = pair.key;
            if (mx_obj_is_string(pair.key)) {
                mx_str_t *buf = mx_obj_get_string(pair.key);
                if (buf->len >= plen && memcmp(buf->buf, tail, plen) == 0) {
                    room = ic_add_completion(cenv, buf->buf + plen);
                }
            }
            if (room) {
                pair = mx_table_pairs_next(vm, t, key);
            }
        }
    }
}

bool mx_repl_is_incomplete(mx_error_t *err) {
    mx_obj_t payload = mx_error_payload(mx_obj_of_error(err));
    if (!mx_obj_is_string(payload)) {
        return false;
    }
    mx_str_t *msg = mx_obj_get_string(payload);
    size_t mark = strlen(MX_REPL_INCOMPLETE_MARK);
    for (size_t i = 0; i + mark <= msg->len; i++) {
        if (memcmp(msg->buf + i, MX_REPL_INCOMPLETE_MARK, mark) == 0) {
            return true;
        }
    }
    return false;
}

void mx_repl_print_results(mx_t *vm) {
    uint32_t n = vm->rets.len;
    if (n != 0 && n <= 256) {
        mx_obj_t args[256];
        memcpy(args, vm->rets.vals, sizeof(mx_obj_t) * n);
        mx_obj_t print = mx_table_get(mx_obj_get_table(mx_lua_of(vm)->std), mx_obj_intern_strn(vm, "print", 5));
        if (!mx_obj_is_nil(print)) {
            mx_call(vm, print, n, args);
        }
    }
}

bool mx_repl_try_expression(mx_t *vm, const char *prog, const char *line) {
    mx_str_t *buf = mx_str_new();
    mx_str_append(buf, MX_REPL_RETURN_PREFIX, (uint32_t)strlen(MX_REPL_RETURN_PREFIX));
    mx_str_append(buf, line, (uint32_t)strlen(line));
    mx_chunk_compiled_t got = mx_chunk_compile(vm, buf->buf, buf->len, MX_REPL_CHUNK);
    mx_str_free(buf);
    if (got.error != NULL) {
        return false;
    }
    vm->rets.len = 0;
    mx_obj_t res = mx_main_call(vm, mx_std_load_closure(vm, got.entry, mx_lua_of(vm)->std));
    if (mx_obj_is_error(res)) {
        mx_main_report(vm, prog, res);
    } else {
        mx_repl_print_results(vm);
    }
    return true;
}

void mx_repl_run_chunk(mx_t *vm, const char *prog, mx_ir_block_t *entry) {
    mx_obj_t res = mx_main_call(vm, mx_std_load_closure(vm, entry, mx_lua_of(vm)->std));
    if (mx_obj_is_error(res)) {
        mx_main_report(vm, prog, res);
    }
}

mx_main_exit_t mx_repl_run(mx_t *vm, const char *prog) {
    ic_set_history(NULL, MX_REPL_HISTORY_MAX);
    ic_set_default_completer(mx_repl_completer, vm);
    ic_enable_auto_tab(false);
    while (true) {
        char *line = ic_readline(MX_REPL_PROMPT);
        if (line == NULL) {
            break;
        }
        mx_str_t *buf = mx_str_new();
        mx_str_append(buf, line, (uint32_t)strlen(line));
        ic_free(line);
        if (mx_repl_try_expression(vm, prog, buf->buf)) {
            ic_history_add(buf->buf);
            mx_str_free(buf);
            continue;
        }
        while (true) {
            mx_chunk_compiled_t got = mx_chunk_compile(vm, buf->buf, buf->len, MX_REPL_CHUNK);
            if (got.error != NULL && mx_repl_is_incomplete(got.error)) {
                char *more = ic_readline(MX_REPL_CONT_PROMPT);
                if (more == NULL) {
                    break;
                }
                mx_str_append_byte(buf, '\n');
                mx_str_append(buf, more, (uint32_t)strlen(more));
                ic_free(more);
            } else if (got.error != NULL) {
                mx_main_report(vm, prog, mx_obj_of_error(got.error));
                break;
            } else {
                mx_repl_run_chunk(vm, prog, got.entry);
                break;
            }
        }
        ic_history_add(buf->buf);
        mx_str_free(buf);
    }
    return MX_MAIN_EXIT_OK;
}
