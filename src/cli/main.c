
#include "main.h"
#include "repl.h"
#include "../lua/front/chunk.h"
#include "../lua/rt/api/std.h"
#include "../lua/rt/lua.h"
#include "../lua/rt/meta.h"
#include "../lua/rt/trace.h"
#include "../run/run.h"
#include "../value/error.h"
#include "../value/obj.h"
#include "../value/string.h"
#include "../value/table.h"
#include "../vm/vm.h"

void mx_main_usage(const char *prog, const mx_main_cmd_t *cmd) {
    if (cmd->is_missing_arg) {
        fprintf(stderr, "%s: '%s' needs argument\n", prog, cmd->bad);
    } else {
        fprintf(stderr, "%s: unrecognized option '%s'\n", prog, cmd->bad);
    }
    fprintf(
        stderr,
        "usage: %s [options] [script [args]]\n"
        "options:\n"
        "  -e chunk   run the Lua text 'chunk'\n"
        "  -l mod     require 'mod' and store it in the global 'mod'\n"
        "  -l g=mod   require 'mod' and store it in the global 'g'\n"
        "  -i         read and run lines interactively after everything else\n"
        "  -v         print the version banner\n"
        "  -E         ignore environment variables\n"
        "  -W         turn warnings on\n"
        "  --ext      keep the MiniVM extensions (callcc, vm, ...)\n"
        "  --         treat the next argument as the script\n"
        "  -          run standard input as the script\n",
        prog
    );
}

mx_main_cmd_t mx_main_parse(int32_t argc, char **argv) {
    mx_main_cmd_t cmd = {
        .steps = mx_mem_alloc(sizeof(mx_main_step_t) * (size_t)argc),
    };
    int32_t i = 1;
    while (i < argc && cmd.bad == NULL && cmd.script == 0) {
        const char *a = argv[i];
        if (a[0] != '-') {
            cmd.script = i;
        } else if (strcmp(a, "-") == 0) {
            cmd.script = i;
            cmd.is_stdin_script = true;
        } else if (strcmp(a, "--") == 0) {
            if (i + 1 < argc) {
                cmd.script = i + 1;
            }
            i = argc;
        } else if (strcmp(a, "--ext") == 0) {
            cmd.is_ext = true;
        } else if (strcmp(a, "-i") == 0) {
            cmd.is_interactive = true;
            cmd.is_version = true;
        } else if (strcmp(a, "-v") == 0) {
            cmd.is_version = true;
        } else if (strcmp(a, "-E") == 0) {
            cmd.is_noenv = true;
        } else if (strcmp(a, "-W") == 0) {
            cmd.steps[cmd.nsteps] = (mx_main_step_t) { .kind = MX_MAIN_STEP_WARN };
            cmd.nsteps += 1;
        } else if (a[1] == 'e' || a[1] == 'l') {
            const char *text = a + 2;
            if (*text == '\0' && (i + 1 >= argc || argv[i + 1][0] == '-')) {
                cmd.bad = a;
                cmd.is_missing_arg = true;
            } else if (*text == '\0') {
                i += 1;
                text = argv[i];
            }
            mx_main_step_kind_t kind = MX_MAIN_STEP_REQUIRE;
            if (a[1] == 'e') {
                kind = MX_MAIN_STEP_EXEC;
                cmd.has_exec = true;
            }
            cmd.steps[cmd.nsteps] = (mx_main_step_t) { .text = text, .kind = kind };
            cmd.nsteps += 1;
        } else {
            cmd.bad = a;
        }
        i += 1;
    }
    return cmd;
}

void mx_main_drop_extensions(mx_t *vm) {
    mx_obj_table_t *g = mx_obj_get_table(mx_lua_of(vm)->std);
    const char *const globals[] = {"callcc", "callcci", "vm", "unpack"};
    for (size_t i = 0; i < sizeof(globals) / sizeof(globals[0]); i++) {
        mx_table_set(vm, g, mx_obj_intern_strn(vm, globals[i], (uint32_t)strlen(globals[i])), mx_obj_of_nil());
    }
    mx_obj_t tablib = mx_table_get(g, mx_obj_intern_strn(vm, "table", 5));
    mx_table_set(vm, mx_obj_get_table(tablib), mx_obj_intern_strn(vm, "keys", 4), mx_obj_of_nil());
    mx_table_set(vm, mx_obj_get_table(tablib), mx_obj_intern_strn(vm, "values", 6), mx_obj_of_nil());
}

void mx_main_report(mx_t *vm, const char *prog, mx_obj_t err) {
    mx_obj_t payload = mx_error_payload(err);
    if (mx_obj_is_string(payload)) {
        mx_str_t *msg = mx_obj_get_string(payload);
        fprintf(stderr, "%s: %.*s\n", prog, (int)msg->len, msg->buf);
    } else {
        fprintf(stderr, "%s: (error object is a %s value)\n", prog, mx_meta_typename(vm, payload));
    }
    fflush(stderr);
}

void mx_main_msgh(mx_t *vm, size_t nargs, mx_obj_t *args) {
    mx_obj_t v = mx_obj_of_nil();
    if (nargs > 0) {
        v = args[0];
    }
    mx_str_t *out = mx_str_new();
    if (mx_obj_is_string(v) || mx_obj_is_number(v)) {
        mx_meta_render(vm, out, v);
    } else {
        mx_obj_t meta = mx_meta_handler_of(vm, v, mx_lua_of(vm)->meta_keys[MX_LUA_META_KEY_TOSTRING]);
        mx_obj_t got = mx_obj_of_nil();
        if (!mx_obj_is_nil(meta)) {
            got = mx_call(vm, meta, 1, &v);
        }
        if (mx_obj_is_string(got)) {
            mx_str_free(out);
            args[0] = got;
            vm->rets.len = 1;
            return;
        }
        mx_str_format(out, "(error object is a %s value)", mx_meta_typename(vm, v));
    }
    mx_str_format(out, "\n");
    mx_trace_append(vm, out, vm->thread, 1);
    args[0] = mx_std_str_of(vm, out->buf, out->len);
    vm->rets.len = 1;
    mx_str_free(out);
}

mx_obj_t mx_main_call(mx_t *vm, mx_obj_t fn) {
    mx_vm_hold(vm, fn);
    mx_obj_t got = mx_pcall(vm, fn, 0, NULL, mx_obj_of_ffi(mx_main_msgh));
    mx_vm_release(vm);
    return got;
}

mx_main_exit_t mx_main_run_source(mx_t *vm, const char *prog, const char *src, uint32_t len, const char *chunkname) {
    mx_chunk_compiled_t got = mx_chunk_compile(vm, src, len, chunkname);
    mx_obj_t result;
    if (got.error != NULL) {
        result = mx_obj_of_error(got.error);
    } else {
        result = mx_main_call(vm, mx_std_load_closure(vm, got.entry, mx_lua_of(vm)->std));
    }
    if (mx_obj_is_error(result)) {
        mx_main_report(vm, prog, result);
        return MX_MAIN_EXIT_ERROR;
    } else {
        return MX_MAIN_EXIT_OK;
    }
}

mx_main_exit_t mx_main_run_file(mx_t *vm, const char *prog, const char *fname) {
    mx_std_text_t text;
    mx_str_t *name = mx_str_new();
    if (fname == NULL) {
        text = mx_std_read_stream(stdin);
        mx_str_format(name, "=stdin");
    } else {
        text = mx_std_read_file(fname);
        mx_str_format(name, "@%s", fname);
    }
    if (text.ptr == NULL) {
        if (fname == NULL) {
            fprintf(stderr, "%s: cannot read stdin\n", prog);
        } else {
            fprintf(stderr, "%s: cannot open %s\n", prog, fname);
        }
        mx_str_free(name);
        return MX_MAIN_EXIT_ERROR;
    }
    char *body = text.ptr;
    size_t len = text.len;
    if (len >= 3 && memcmp(body, "\xEF\xBB\xBF", 3) == 0) {
        body += 3;
        len -= 3;
    }
    if (len > 0 && body[0] == '#') {
        size_t eol = 0;
        while (eol < len && body[eol] != '\n') {
            eol += 1;
        }
        memset(body, ' ', eol);
    }
    mx_main_exit_t status = mx_main_run_source(vm, prog, body, (uint32_t)len, name->buf);
    mx_mem_free(text.ptr);
    mx_str_free(name);
    return status;
}

mx_main_exit_t mx_main_require(mx_t *vm, const char *prog, const char *spec) {
    const char *eq = strchr(spec, '=');
    const char *module = spec;
    uint32_t global_len = (uint32_t)strlen(spec);
    if (eq != NULL) {
        module = eq + 1;
        global_len = (uint32_t)(eq - spec);
    }
    mx_obj_table_t *g = mx_obj_get_table(mx_lua_of(vm)->std);
    mx_obj_t require = mx_table_get(g, mx_obj_intern_strn(vm, "require", 7));
    mx_obj_t args[1] = {mx_obj_new_string(vm, module)};
    mx_obj_t got = mx_call(vm, require, 1, args);
    if (mx_obj_is_error(got)) {
        mx_main_report(vm, prog, got);
        return MX_MAIN_EXIT_ERROR;
    } else {
        mx_table_set(vm, g, mx_obj_intern_strn(vm, spec, global_len), got);
        return MX_MAIN_EXIT_OK;
    }
}

mx_main_exit_t mx_main_run_init(mx_t *vm, const char *prog) {
    const char *chunkname = "=LUA_INIT_5_5";
    const char *init = getenv(chunkname + 1);
    if (init == NULL) {
        chunkname = "=LUA_INIT";
        init = getenv(chunkname + 1);
    }
    if (init == NULL) {
        return MX_MAIN_EXIT_OK;
    } else if (init[0] == '@') {
        return mx_main_run_file(vm, prog, init + 1);
    } else {
        return mx_main_run_source(vm, prog, init, (uint32_t)strlen(init), chunkname);
    }
}

mx_main_exit_t mx_main_run_steps(mx_t *vm, const char *prog, const mx_main_cmd_t *cmd) {
    mx_main_exit_t status = MX_MAIN_EXIT_OK;
    for (uint32_t i = 0; i < cmd->nsteps && status == MX_MAIN_EXIT_OK; i++) {
        const mx_main_step_t *step = &cmd->steps[i];
        switch (step->kind) {
            case MX_MAIN_STEP_EXEC: {
                status = mx_main_run_source(vm, prog, step->text, (uint32_t)strlen(step->text), MX_MAIN_CHUNK_COMMAND);
                break;
            }
            case MX_MAIN_STEP_REQUIRE: {
                status = mx_main_require(vm, prog, step->text);
                break;
            }
            default: {
                mx_lua_warning(vm, "@on", false);
                break;
            }
        }
    }
    return status;
}

int main(int argc, char **argv) {
    const char *prog = argv[0];
    mx_main_cmd_t cmd = mx_main_parse(argc, argv);
    if (cmd.bad != NULL) {
        mx_main_usage(prog, &cmd);
        mx_mem_free(cmd.steps);
        return MX_MAIN_EXIT_ERROR;
    }
    mx_t *vm = mx_vm_new();
    mx_lua_t *lua = mx_lua_open(vm, cmd.is_noenv);
    lua->is_ext = cmd.is_ext;
    if (!cmd.is_ext) {
        mx_main_drop_extensions(vm);
    }
    mx_lua_set_arg(vm, argc, argv, cmd.script);
    if (cmd.script == 0 && !cmd.has_exec && !cmd.is_version && isatty(STDIN_FILENO)) {
        cmd.is_interactive = true;
        cmd.is_version = true;
    }
    if (cmd.is_version) {
        printf("%s\n", MX_MAIN_BANNER);
        fflush(stdout);
    }
    mx_main_exit_t status = MX_MAIN_EXIT_OK;
    if (!cmd.is_noenv) {
        status = mx_main_run_init(vm, prog);
    }
    if (status == MX_MAIN_EXIT_OK) {
        status = mx_main_run_steps(vm, prog, &cmd);
    }
    if (status == MX_MAIN_EXIT_OK && cmd.script != 0) {
        const char *fname = argv[cmd.script];
        if (cmd.is_stdin_script) {
            fname = NULL;
        }
        status = mx_main_run_file(vm, prog, fname);
    }
    if (status == MX_MAIN_EXIT_OK && cmd.is_interactive) {
        status = mx_repl_run(vm, prog);
    } else if (status == MX_MAIN_EXIT_OK && cmd.script == 0 && !cmd.has_exec && !cmd.is_version) {
        status = mx_main_run_file(vm, prog, NULL);
    }
    mx_mem_free(cmd.steps);
    mx_lua_close(vm);
    mx_vm_free(vm);
    return status;
}
