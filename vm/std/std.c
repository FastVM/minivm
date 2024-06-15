
#include "./std.h"

#include "../ast/ast.h"
#include "./io.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);
void vm_ast_comp_more(vm_ast_node_t node, vm_blocks_t *blocks);

void vm_std_os_exit(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    exit((int)vm_value_to_i64(args[0]));
    return;
}

void vm_std_load(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    if (vm_type_eq(args[0].tag, VM_TAG_STR)) {
        *args = (vm_std_value_t){
            .tag = VM_TAG_ERROR,
            .value.str = "cannot load non-string value",
        };
    }
    const char *str = args[0].value.str;
    vm_ast_node_t node = vm_lang_lua_parse(closure->config, str);
    vm_blocks_add_src(closure->blocks, str);
    vm_ast_comp_more(node, closure->blocks);
    vm_ast_free_node(node);

    vm_std_value_t *vals = vm_malloc(sizeof(vm_std_value_t) * 2);
    vals[0] = (vm_std_value_t){
        .tag = VM_TAG_I32,
        .value.i32 = 1,
    };
    vals += 1;
    vals[0] = (vm_std_value_t){
        .tag = VM_TAG_FUN,
        .value.i32 = (int32_t)closure->blocks->entry->id,
    };
    *args = (vm_std_value_t){
        .tag = VM_TAG_CLOSURE,
        .value.closure = vals,
    };
    return;
}

void vm_std_assert(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    vm_std_value_t val = args[0];
    if (vm_type_eq(val.tag, VM_TAG_NIL) || (vm_type_eq(val.tag, VM_TAG_BOOL) && !val.value.b)) {
        vm_std_value_t msg = args[1];
        vm_io_buffer_t buf = {0};
        vm_io_debug(&buf, 0, "assert failed with mesage: ", msg, NULL);
        *args = (vm_std_value_t){
            .tag = VM_TAG_ERROR,
            .value.str = buf.buf,
        };
        return;
    } else {
        *args = (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
        return;
    }
}

void vm_std_error(vm_std_closure_t *closure, vm_std_value_t *args) {
    if (args[0].tag == VM_TAG_STR) {
        *args = (vm_std_value_t){
            .tag = VM_TAG_ERROR,
            .value.str = args[0].value.str,
        };
        return;
    }
    vm_std_value_t msg = args[0];
    vm_io_buffer_t buf = {0};
    vm_io_debug(&buf, 0, "", msg, NULL);
    *args = (vm_std_value_t){
        .tag = VM_TAG_ERROR,
        .value.str = buf.buf,
    };
    return;
}

void vm_std_vm_closure(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    int64_t nargs = 0;
    for (size_t i = 0; !vm_type_eq(args[i].tag, VM_TAG_UNK); i++) {
        nargs += 1;
    }
    if (nargs == 0 || !vm_type_eq(args[0].tag, VM_TAG_FUN)) {
        *args = (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
        return;
    }
    vm_std_value_t *vals = vm_malloc(sizeof(vm_std_value_t) * (nargs + 1));
    vals[0] = (vm_std_value_t){
        .tag = VM_TAG_I32,
        .value.i32 = (int32_t)nargs,
    };
    vals += 1;
    for (size_t i = 0; !vm_type_eq(args[i].tag, VM_TAG_UNK); i++) {
        vals[i] = args[i];
    }
    *args = (vm_std_value_t){
        .tag = VM_TAG_CLOSURE,
        .value.closure = vals,
    };
    return;
}

void vm_std_vm_print(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    for (size_t i = 0; !vm_type_eq(args[i].tag, VM_TAG_UNK); i++) {
        vm_io_buffer_t buf = {0};
        vm_io_debug(&buf, 0, "", args[i], NULL);
        printf("%.*s", (int)buf.len, buf.buf);
    }
    args[0] = (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
}

void vm_std_vm_concat(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    size_t len = 1;
    for (size_t i = 0; vm_type_eq(args[i].tag, VM_TAG_STR); i++) {
        len += strlen(args[i].value.str);
    }
    char *buf = vm_malloc(sizeof(char) * len);
    size_t head = 0;
    for (size_t i = 0; vm_type_eq(args[i].tag, VM_TAG_STR); i++) {
        size_t len = strlen(args[i].value.str);
        if (len == 0) {
            continue;
        }
        memcpy(&buf[head], args[i].value.str, len);
        head += len;
    }
    buf[len - 1] = '\0';
    *args = (vm_std_value_t){
        .tag = VM_TAG_STR,
        .value.str = buf,
    };
}

void vm_std_type(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    const char *ret = "unknown";
    switch (vm_type_tag(args[0].tag)) {
        case VM_TAG_NIL: {
            ret = "nil";
            break;
        }
        case VM_TAG_BOOL: {
            ret = "boolean";
            break;
        }
        case VM_TAG_I8: {
            ret = "number";
            break;
        }
        case VM_TAG_I16: {
            ret = "number";
            break;
        }
        case VM_TAG_I32: {
            ret = "number";
            break;
        }
        case VM_TAG_I64: {
            ret = "number";
            break;
        }
        case VM_TAG_F32: {
            ret = "number";
            break;
        }
        case VM_TAG_F64: {
            ret = "number";
            break;
        }
        case VM_TAG_STR: {
            ret = "string";
            break;
        }
        case VM_TAG_CLOSURE: {
            ret = "function";
            break;
        }
        case VM_TAG_FUN: {
            ret = "function";
            break;
        }
        case VM_TAG_TAB: {
            ret = "table";
            break;
        }
        case VM_TAG_FFI: {
            ret = "function";
            break;
        }
        case VM_TAG_ERROR: {
            ret = "string";
            break;
        }
    }
    *args = (vm_std_value_t){
        .tag = VM_TAG_STR,
        .value.str = ret,
    };
}

void vm_std_tostring(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    vm_io_buffer_t out = {0};
    vm_value_buffer_tostring(&out, *args);
    *args = (vm_std_value_t){
        .tag = VM_TAG_STR,
        .value.str = out.buf,
    };
}

void vm_std_tonumber(vm_std_closure_t *closure, vm_std_value_t *args) {
    switch (args[0].tag) {
        case VM_TAG_I8: {
            args[0] = VM_STD_VALUE_NUMBER(closure->config, args[0].value.i8);
            return;
        }
        case VM_TAG_I16: {
            args[0] = VM_STD_VALUE_NUMBER(closure->config, args[0].value.i16);
            return;
        }
        case VM_TAG_I32: {
            args[0] = VM_STD_VALUE_NUMBER(closure->config, args[0].value.i32);
            return;
        }
        case VM_TAG_I64: {
            args[0] = VM_STD_VALUE_NUMBER(closure->config, args[0].value.i64);
            return;
        }
        case VM_TAG_F32: {
            args[0] = VM_STD_VALUE_NUMBER(closure->config, args[0].value.f32);
            return;
        }
        case VM_TAG_F64: {
            args[0] = VM_STD_VALUE_NUMBER(closure->config, args[0].value.f64);
            return;
        }
        case VM_TAG_STR: {
            if (closure->config->use_num == VM_USE_NUM_F32 || closure->config->use_num == VM_USE_NUM_F64) {
                double num;
                if (sscanf(args[0].value.str, "%lf", &num) == 0) {
                    args[0] = VM_STD_VALUE_NIL;
                    return;
                }
                args[0] = VM_STD_VALUE_NUMBER(closure->config, num);
                return;
            } else {
                int64_t num;
                if (sscanf(args[0].value.str, "%" SCNi64, &num) == 0) {
                    args[0] = VM_STD_VALUE_NIL;
                    return;
                }
                args[0] = VM_STD_VALUE_NUMBER(closure->config, num);
                return;
            }
        }
        default: {
            args[0] = VM_STD_VALUE_NIL;
            return;
        }
    }
}

void vm_std_print(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    vm_std_value_t *ret = args;
    vm_io_buffer_t out = {0};
    bool first = true;
    while (!vm_type_eq(args->tag, VM_TAG_UNK)) {
        if (!first) {
            vm_io_buffer_format(&out, "\t");
        }
        vm_value_buffer_tostring(&out, *args++);
        first = false;
    }
    fprintf(stdout, "%.*s\n", (int)out.len, out.buf);
    *ret = (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
}

void vm_std_io_write(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    vm_std_value_t *ret = args;
    vm_io_buffer_t out = {0};
    while (!vm_type_eq(args->tag, VM_TAG_UNK)) {
        vm_value_buffer_tostring(&out, *args++);
    }
    fprintf(stdout, "%.*s", (int)out.len, out.buf);
    *ret = (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
}

void vm_std_string_format(vm_std_closure_t *closure, vm_std_value_t *args) {
    (void)closure;
    vm_std_value_t *ret = args;
    vm_io_buffer_t *out = vm_io_buffer_new();
    vm_std_value_t fmt = *args++;
    if (vm_type_eq(fmt.tag, VM_TAG_STR)) {
        *ret = (vm_std_value_t){
            .tag = VM_TAG_ERROR,
            .value.str = "invalid format (not a string)",
        };
        return;
    }
    const char *str = fmt.value.str;
    while (*str != '\0') {
        const char *head = str;
        char got = *str++;
        if (got != '%') {
            vm_io_buffer_format(out, "%c", got);
            continue;
        }
        if (*str == '%') {
            str++;
            vm_io_buffer_format(out, "%%");
            continue;
        }
        while (true) {
            char p = *str;
            if (p == '-' || p == '+' || p == ' ' || p == '#' || p == '0') {
                str++;
                continue;
            }
            break;
        }
        if ('0' < *str && *str <= '9') {
            str++;
        }
        if ('0' < *str && *str <= '9') {
            str++;
        }
        if ('0' < *str && *str <= '9') {
            *ret = (vm_std_value_t){
                .tag = VM_TAG_ERROR,
                .value.str = "invalid format (width > 99)",
            };
            return;
        }
        bool format_has_dot = *str == '.';
        if (format_has_dot) {
            if ('0' < *str && *str <= '9') {
                str++;
            }
            if ('0' < *str && *str <= '9') {
                str++;
            }
        }
        if ('0' < *str && *str <= '9') {
            *ret = (vm_std_value_t){
                .tag = VM_TAG_ERROR,
                .value.str = "invalid format (precision > 99)",
            };
            return;
        }
        ptrdiff_t len = str - head;
        if (!(0 < len || len > 48)) {
            *ret = (vm_std_value_t){
                .tag = VM_TAG_ERROR,
                .value.str = "invalid format (too long to handle)",
            };
            return;
        }
        char format[64];
        strncpy(format, head, str - head);
        vm_std_value_t arg = *args++;
        if (vm_type_eq(arg.tag, VM_TAG_UNK)) {
            *ret = (vm_std_value_t){
                .tag = VM_TAG_ERROR,
                .value.str = "too few args",
            };
            return;
        }
        char fc = *str++;
        switch (fc) {
            case 'c': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_std_value_t){
                        .tag = VM_TAG_ERROR,
                        .value.str = "expected a number for %c format",
                    };
                    return;
                }
                strcpy(&format[len], "c");
                vm_io_buffer_format(out, format, (int)vm_value_to_i64(arg));
                break;
            }
            case 'd':
            case 'i': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_std_value_t){
                        .tag = VM_TAG_ERROR,
                        .value.str = "expected a number for integer format",
                    };
                    return;
                }
                strcpy(&format[len], PRIi64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'o': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_std_value_t){
                        .tag = VM_TAG_ERROR,
                        .value.str = "expected a number for %o format",
                    };
                    return;
                }
                strcpy(&format[len], PRIo64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'u': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_std_value_t){
                        .tag = VM_TAG_ERROR,
                        .value.str = "expected a number for %u format",
                    };
                    return;
                }
                strcpy(&format[len], PRIu64);
                vm_io_buffer_format(out, format, (uint64_t)vm_value_to_i64(arg));
                break;
            }
            case 'x': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_std_value_t){
                        .tag = VM_TAG_ERROR,
                        .value.str = "expected a number for %x format",
                    };
                    return;
                }
                strcpy(&format[len], PRIx64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'X': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_std_value_t){
                        .tag = VM_TAG_ERROR,
                        .value.str = "expected a number for %X format",
                    };
                    return;
                }
                strcpy(&format[len], PRIX64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_std_value_t){
                        .tag = VM_TAG_ERROR,
                        .value.str = "expected a number for float format",
                    };
                    return;
                }
                format[len] = fc;
                format[len + 1] = '\0';
                vm_io_buffer_format(out, format, vm_value_to_f64(arg));
                break;
            }
            case 'q': {
                *ret = (vm_std_value_t){
                    .tag = VM_TAG_ERROR,
                    .value.str = "unimplemented %q",
                };
                return;
            }
            case 's': {
                if (vm_type_eq(arg.tag, VM_TAG_STR)) {
                    strcpy(&format[len], "s");
                    vm_io_buffer_format(out, format, arg.value.str);
                } else if (vm_value_can_to_n64(arg)) {
                    strcpy(&format[len], "f");
                    vm_io_buffer_format(out, format, vm_value_to_f64(arg));
                } else {
                    *ret = (vm_std_value_t){
                        .tag = VM_TAG_ERROR,
                        .value.str = "unimplemented %s for a type",
                    };
                    return;
                }
                break;
            }
            default: {
                *ret = (vm_std_value_t){
                    .tag = VM_TAG_ERROR,
                    .value.str = "unknown format type",
                };
                __builtin_trap();
                return;
            }
        }
    }
    *ret = (vm_std_value_t){
        .tag = VM_TAG_STR,
        .value.str = vm_io_buffer_get(out),
    };
}

void vm_std_set_arg(vm_config_t *config, vm_table_t *std, const char *prog, const char *file, int argc, char **argv) {
    vm_table_t *arg = vm_table_new();
    if (prog != NULL) {
        VM_TABLE_SET_VALUE(arg, VM_STD_VALUE_NUMBER(config, -1), VM_STD_VALUE_LITERAL(str, prog));
    }
    if (file != NULL) {
        VM_TABLE_SET_VALUE(arg, VM_STD_VALUE_NUMBER(config, 0), VM_STD_VALUE_LITERAL(str, file));
    }
    for (int64_t i = 0; i < argc; i++) {
        VM_TABLE_SET_VALUE(arg, VM_STD_VALUE_NUMBER(config, i + 1), VM_STD_VALUE_LITERAL(str, argv[i]));
    }
    VM_TABLE_SET(std, str, "arg", table, arg);
}

void vm_std_vm_typename(vm_std_closure_t *closure, vm_std_value_t *args) {
    if (args[0].tag != VM_TAG_STR) {
        *args = (vm_std_value_t){
            .tag = VM_TAG_ERROR,
            .value.str = "vm.type: expected string",
        };
        return;
    }
    const char *str = args[0].value.str;
    if (!strcmp(str, "nil")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_NIL);
        return;
    }
    if (!strcmp(str, "bool")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_BOOL);
        return;
    }
    if (!strcmp(str, "i8")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_I8);
        return;
    }
    if (!strcmp(str, "i16")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_I16);
        return;
    }
    if (!strcmp(str, "i32")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_I32);
        return;
    }
    if (!strcmp(str, "i64")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_I64);
        return;
    }
    if (!strcmp(str, "f32")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_F32);
        return;
    }
    if (!strcmp(str, "f64")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_F64);
        return;
    }
    if (!strcmp(str, "str")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_STR);
        return;
    }
    if (!strcmp(str, "tab")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_TAB);
        return;
    }
    if (!strcmp(str, "closure")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_CLOSURE);
        return;
    }
    if (!strcmp(str, "ffi")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_FFI);
        return;
    }
    if (!strcmp(str, "error")) {
        *args = VM_STD_VALUE_NUMBER(closure->config, VM_TAG_ERROR);
        return;
    }
    *args = VM_STD_VALUE_NIL;
    return;
}

void vm_std_vm_typeof(vm_std_closure_t *closure, vm_std_value_t *args) {
    args[0] = VM_STD_VALUE_NUMBER(closure->config, args[0].tag);
}

void vm_std_table_keys(vm_std_closure_t *closure, vm_std_value_t *args) {
    if (args[0].tag != VM_TAG_TAB) {
        *args = (vm_std_value_t){
            .tag = VM_TAG_ERROR,
            .value.str = "table.values: expect a table",
        };
        return;
    }
    vm_table_t *ret = vm_table_new();
    vm_table_t *tab = args[0].value.table;
    size_t len = 1 << tab->alloc;
    size_t write_head = 1;
    for (size_t i = 0; i < len; i++) {
        vm_pair_t *pair = &tab->pairs[i];
        if (pair->key_tag != VM_TAG_UNK) {
            vm_std_value_t key = VM_STD_VALUE_NUMBER(closure->config, write_head);
            vm_std_value_t value = (vm_std_value_t){
                .tag = pair->key_tag,
                .value = pair->key_val,
            };
            VM_TABLE_SET_VALUE(ret, key, value);
            write_head++;
        }
    }
    *args = (vm_std_value_t){
        .tag = VM_TAG_TAB,
        .value.table = ret,
    };
    return;
}

void vm_std_table_values(vm_std_closure_t *closure, vm_std_value_t *args) {
    if (args[0].tag != VM_TAG_TAB) {
        *args = (vm_std_value_t){
            .tag = VM_TAG_ERROR,
            .value.str = "table.values: expect a table",
        };
        return;
    }
    vm_table_t *ret = vm_table_new();
    vm_table_t *tab = args[0].value.table;
    size_t len = 1 << tab->alloc;
    size_t write_head = 1;
    for (size_t i = 0; i < len; i++) {
        vm_pair_t *pair = &tab->pairs[i];
        if (pair->key_tag != VM_TAG_UNK) {
            vm_std_value_t key = VM_STD_VALUE_NUMBER(closure->config, write_head);
            vm_std_value_t value = (vm_std_value_t){
                .tag = pair->val_tag,
                .value = pair->val_val,
            };
            VM_TABLE_SET_VALUE(ret, key, value);
            write_head++;
        }
    }
    *args = (vm_std_value_t){
        .tag = VM_TAG_TAB,
        .value.table = ret,
    };
    return;
}

void vm_lua_comp_op_std_pow(vm_std_closure_t *closure, vm_std_value_t *args);

#if VM_USE_RAYLIB

#include "../../vendor/raylib/src/raylib.h"
#include "../../vendor/cuik/c11threads/threads.h"
#include "../backend/backend.h"

static Color vm_gui_draw_color = BLACK;

int vm_std_gui_draw(void *arg) {
    vm_table_t *gui = arg;
    return 0;
}

typedef struct  {
    vm_config_t *config;
    vm_table_t *std;
    vm_blocks_t *blocks;
} vm_std_gui_repl_t;

void vm_lang_lua_repl(vm_config_t *config, vm_table_t *std, vm_blocks_t *blocks);

int vm_std_gui_repl(void *arg) {
    vm_std_gui_repl_t *repl = arg;
    vm_lang_lua_repl(repl->config, repl->std, repl->blocks);
    exit(0);
    return 0;
}

static Color vm_value_to_color(vm_std_value_t arg) {
    if (arg.tag != VM_TAG_TAB) {
        return BLACK;
    }
    vm_std_value_t *r = VM_TABLE_LOOKUP_STR(arg.value.table, "red");
    vm_std_value_t *g = VM_TABLE_LOOKUP_STR(arg.value.table, "green");
    vm_std_value_t *b = VM_TABLE_LOOKUP_STR(arg.value.table, "blue");
    vm_std_value_t *a = VM_TABLE_LOOKUP_STR(arg.value.table, "alpha");
    return (Color) {
        r ? vm_value_to_i64(*r) : 0,
        g ? vm_value_to_i64(*g) : 0,
        b ? vm_value_to_i64(*b) : 0,
        a ? vm_value_to_i64(*a) : 255,
    };
}

void vm_std_gui_rect(vm_std_closure_t *closure, vm_std_value_t *args) {
    DrawRectangle(vm_value_to_i64(args[0]), vm_value_to_i64(args[1]), vm_value_to_i64(args[2]), vm_value_to_i64(args[3]), vm_gui_draw_color);
}

void vm_std_gui_init(vm_std_closure_t *closure, vm_std_value_t *args) {
    vm_table_t *env = args[0].value.table;
    thrd_t thrd;
    vm_std_gui_repl_t *repl = vm_malloc(sizeof(vm_std_gui_repl_t));
    *repl = (vm_std_gui_repl_t) {
        .config = closure->config,
        .std = env,
        .blocks = closure->blocks,
    };
    thrd_create(&thrd, &vm_std_gui_repl, &repl);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(960, 540, "MiniVM");
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        for (vm_blocks_srcs_t *src = closure->blocks->srcs; src != NULL; src = src->last) {
            if (src->src[0] == '!') {
                if (!strncmp(&src->src[1], "draw", 4)) {
                    const char *loop_code = &src->src[5];
                    size_t n = closure->blocks->len;
                    vm_ast_node_t node = vm_lang_lua_parse(closure->config, loop_code);
                    vm_ast_comp_more(node, closure->blocks);
                    vm_ast_free_node(node);
                    vm_run_repl(closure->config, closure->blocks->blocks[n], closure->blocks, env);
                    closure->blocks->len = n;
                }
            }
        }
        EndDrawing();
    }
    CloseWindow();
    args[0] = VM_STD_VALUE_NIL;
    return;
}

#endif

vm_table_t *vm_std_new(vm_config_t *config) {
    vm_table_t *std = vm_table_new();

    {
        vm_table_t *io = vm_table_new();
        VM_TABLE_SET(std, str, "io", table, io);
        VM_TABLE_SET(io, str, "write", ffi, VM_STD_REF(config, vm_std_io_write));
    }

    {
        vm_table_t *string = vm_table_new();
        VM_TABLE_SET(std, str, "string", table, string);
        VM_TABLE_SET(string, str, "format", ffi, VM_STD_REF(config, vm_std_string_format));
    }

    {
        vm_table_t *vm = vm_table_new();
        VM_TABLE_SET(std, str, "vm", table, vm);
        VM_TABLE_SET(vm, str, "print", ffi, VM_STD_REF(config, vm_std_vm_print));
        VM_TABLE_SET(vm, str, "version", str, "0.0.4");
        VM_TABLE_SET(vm, str, "typename", ffi, VM_STD_REF(config, vm_std_vm_typename));
        VM_TABLE_SET(vm, str, "typeof", ffi, VM_STD_REF(config, vm_std_vm_typeof));
        VM_TABLE_SET(vm, str, "concat", ffi, VM_STD_REF(config, vm_std_vm_concat));
    }

    {
        vm_table_t *os = vm_table_new();
        VM_TABLE_SET(os, str, "exit", ffi, VM_STD_REF(config, vm_std_os_exit));
        VM_TABLE_SET(std, str, "os", table, os);
    }

    {
        vm_table_t *table = vm_table_new();
        VM_TABLE_SET(table, str, "keys", ffi, VM_STD_REF(config, vm_std_table_keys));
        VM_TABLE_SET(table, str, "values", ffi, VM_STD_REF(config, vm_std_table_values));
        VM_TABLE_SET(std, str, "table", table, table);
    }

    #if VM_USE_RAYLIB
    {
        vm_table_t *gui = vm_table_new();
        VM_TABLE_SET(std, str, "gui", table, gui);
        vm_table_t *state = vm_table_new();
        VM_TABLE_SET(gui, str, "init", ffi, VM_STD_REF(config, vm_std_gui_init));
        VM_TABLE_SET(gui, str, "rect", ffi, VM_STD_REF(config, vm_std_gui_rect));
    }
    #endif

    VM_TABLE_SET(std, str, "error", ffi, VM_STD_REF(config, vm_std_error));
    VM_TABLE_SET(std, str, "tostring", ffi, VM_STD_REF(config, vm_std_tostring));
    VM_TABLE_SET(std, str, "tonumber", ffi, VM_STD_REF(config, vm_std_tonumber));
    VM_TABLE_SET(std, str, "type", ffi, VM_STD_REF(config, vm_std_type));
    VM_TABLE_SET(std, str, "print", ffi, VM_STD_REF(config, vm_std_print));
    VM_TABLE_SET(std, str, "assert", ffi, VM_STD_REF(config, vm_std_assert));
    VM_TABLE_SET(std, str, "load", ffi, VM_STD_REF(config, vm_std_load));
    VM_TABLE_SET(std, str, "_G", table, std);

    vm_config_add_extern(config, &vm_lua_comp_op_std_pow);
    vm_config_add_extern(config, &vm_std_vm_closure);

    return std;
}
