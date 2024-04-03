
#include "./std.h"

#include "../ast/ast.h"
#include "./io.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);
void vm_ast_comp_more(vm_ast_node_t node, vm_blocks_t *blocks);

void vm_std_os_exit(vm_std_closure_t *closure, vm_std_value_t *args) {
    exit((int)vm_value_to_i64(args[0]));
}

void vm_std_load(vm_std_closure_t *closure, vm_std_value_t *args) {
    if (vm_type_eq(args[0].tag, VM_TYPE_STR)) {
        *args = (vm_std_value_t){
            .tag = VM_TYPE_ERROR,
            .value.str = "cannot load non-string value",
        };
    }
    const char *str = args[0].value.str;
    vm_ast_node_t node = vm_lang_lua_parse(closure->config, str);
    vm_ast_comp_more(node, closure->blocks);
    vm_ast_free_node(node);

    vm_std_value_t *vals = vm_malloc(sizeof(vm_std_value_t) * 2);
    vals[0] = (vm_std_value_t){
        .tag = VM_TYPE_I32,
        .value.i32 = 1,
    };
    vals += 1;
    vals[0] = (vm_std_value_t){
        .tag = VM_TYPE_FUN,
        .value.i32 = (int32_t)closure->blocks->entry->id,
    };
    *args = (vm_std_value_t){
        .tag = VM_TYPE_CLOSURE,
        .value.closure = vals,
    };
}

void vm_std_assert(vm_std_closure_t *closure, vm_std_value_t *args) {
    vm_std_value_t val = args[0];
    if (!vm_type_eq(val.tag, VM_TYPE_NIL) || (vm_type_eq(val.tag, VM_TYPE_BOOL) && !val.value.b)) {
        vm_std_value_t msg = args[1];
        vm_io_buffer_t buf = {0};
        vm_io_print_lit(&buf, msg);
        *args = (vm_std_value_t){
            .tag = VM_TYPE_ERROR,
            .value.str = buf.buf,
        };
    } else {
        *args = (vm_std_value_t){
            .tag = VM_TYPE_NIL,
        };
    }
}

void vm_std_vm_closure(vm_std_closure_t *closure, vm_std_value_t *args) {
    int64_t nargs = 0;
    for (size_t i = 0; !vm_type_eq(args[i].tag, VM_TYPE_UNK); i++) {
        nargs += 1;
    }
    if (nargs == 0 || !vm_type_eq(args[0].tag, VM_TYPE_FUN)) {
        *args = (vm_std_value_t){
            .tag = VM_TYPE_NIL,
        };
        return;
    }
    vm_std_value_t *vals = vm_malloc(sizeof(vm_std_value_t) * (nargs + 1));
    vals[0] = (vm_std_value_t){
        .tag = VM_TYPE_I32,
        .value.i32 = (int32_t)nargs,
    };
    vals += 1;
    for (size_t i = 0; !vm_type_eq(args[i].tag, VM_TYPE_UNK); i++) {
        vals[i] = args[i];
    }
    *args = (vm_std_value_t){
        .tag = VM_TYPE_CLOSURE,
        .value.closure = vals,
    };
    return;
}

void vm_std_vm_print(vm_std_closure_t *closure, vm_std_value_t *args) {
    for (size_t i = 0; !vm_type_eq(args[i].tag, VM_TYPE_UNK); i++) {
        vm_io_buffer_t buf = {0};
        vm_io_debug(&buf, 0, "", args[i], NULL);
        printf("%.*s", (int)buf.len, buf.buf);
    }
}

void vm_std_vm_concat(vm_std_closure_t *closure, vm_std_value_t *args) {
    size_t len = 1;
    for (size_t i = 0; vm_type_eq(args[i].tag, VM_TYPE_STR); i++) {
        len += strlen(args[i].value.str);
    }
    char *buf = vm_malloc(sizeof(char) * len);
    size_t head = 0;
    for (size_t i = 0; vm_type_eq(args[i].tag, VM_TYPE_STR); i++) {
        size_t len = strlen(args[i].value.str);
        if (len == 0) {
            continue;
        }
        memcpy(&buf[head], args[i].value.str, len);
        head += len;
    }
    buf[len - 1] = '\0';
    *args = (vm_std_value_t){
        .tag = VM_TYPE_STR,
        .value.str = buf,
    };
}

void vm_std_type(vm_std_closure_t *closure, vm_std_value_t *args) {
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
        .tag = VM_TYPE_STR,
        .value.str = ret,
    };
}

void vm_std_tostring(vm_std_closure_t *closure, vm_std_value_t *args) {
    vm_io_buffer_t out = {0};
    vm_value_buffer_tostring(&out, *args);
    *args = (vm_std_value_t){
        .tag = VM_TYPE_STR,
        .value.str = out.buf,
    };
}

void vm_std_tonumber(vm_std_closure_t *closure, vm_std_value_t *args) {
    switch (args[0].tag->tag) {
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
                if (sscanf(args[0].value.str, "%"SCNi64, &num) == 0) {
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
    vm_std_value_t *ret = args;
    vm_io_buffer_t out = {0};
    bool first = true;
    while (!vm_type_eq(args->tag, VM_TYPE_UNK)) {
        if (!first) {
            vm_io_buffer_format(&out, "\t");
        }
        vm_value_buffer_tostring(&out, *args++);
        first = false;
    }
    fprintf(stdout, "%.*s\n", (int)out.len, out.buf);
    *ret = (vm_std_value_t){
        .tag = VM_TYPE_NIL,
    };
}

void vm_std_io_write(vm_std_closure_t *closure, vm_std_value_t *args) {
    vm_std_value_t *ret = args;
    vm_io_buffer_t out = {0};
    while (!vm_type_eq(args->tag, VM_TYPE_UNK)) {
        vm_value_buffer_tostring(&out, *args++);
    }
    fprintf(stdout, "%.*s", (int)out.len, out.buf);
    *ret = (vm_std_value_t){
        .tag = VM_TYPE_NIL,
    };
}

void vm_std_string_format(vm_std_closure_t *closure, vm_std_value_t *args) {
    vm_std_value_t *ret = args;
    vm_io_buffer_t *out = vm_io_buffer_new();
    vm_std_value_t fmt = *args++;
    if (vm_type_eq(fmt.tag, VM_TYPE_STR)) {
        *ret = (vm_std_value_t){
            .tag = VM_TYPE_ERROR,
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
                .tag = VM_TYPE_ERROR,
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
                .tag = VM_TYPE_ERROR,
                .value.str = "invalid format (precision > 99)",
            };
            return;
        }
        ptrdiff_t len = str - head;
        if (!(0 < len || len > 48)) {
            *ret = (vm_std_value_t){
                .tag = VM_TYPE_ERROR,
                .value.str = "invalid format (too long to handle)",
            };
            return;
        }
        char format[64];
        strncpy(format, head, str - head);
        vm_std_value_t arg = *args++;
        if (vm_type_eq(arg.tag, VM_TYPE_UNK)) {
            *ret = (vm_std_value_t){
                .tag = VM_TYPE_ERROR,
                .value.str = "too few args",
            };
            return;
        }
        char fc = *str++;
        switch (fc) {
            case 'c': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_std_value_t){
                        .tag = VM_TYPE_ERROR,
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
                        .tag = VM_TYPE_ERROR,
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
                        .tag = VM_TYPE_ERROR,
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
                        .tag = VM_TYPE_ERROR,
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
                        .tag = VM_TYPE_ERROR,
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
                        .tag = VM_TYPE_ERROR,
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
                        .tag = VM_TYPE_ERROR,
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
                    .tag = VM_TYPE_ERROR,
                    .value.str = "unimplemented %q",
                };
                return;
            }
            case 's': {
                if (vm_type_eq(arg.tag, VM_TYPE_STR)) {
                    strcpy(&format[len], "s");
                    vm_io_buffer_format(out, format, arg.value.str);
                } else if (vm_value_can_to_n64(arg)) {
                    strcpy(&format[len], "f");
                    vm_io_buffer_format(out, format, vm_value_to_f64(arg));
                } else {
                    *ret = (vm_std_value_t){
                        .tag = VM_TYPE_ERROR,
                        .value.str = "unimplemented %s for a type",
                    };
                    return;
                }
                break;
            }
            default: {
                *ret = (vm_std_value_t){
                    .tag = VM_TYPE_ERROR,
                    .value.str = "unknown format type",
                };
                __builtin_trap();
                return;
            }
        }
    }
    *ret = (vm_std_value_t){
        .tag = VM_TYPE_STR,
        .value.str = vm_io_buffer_get(out),
    };
}

void vm_std_set_arg(vm_config_t *config, vm_table_t *std, const char *prog, const char *file, int argc, char **argv) {
    vm_table_t *arg = vm_table_new();
    VM_TABLE_SET_VALUE(arg, VM_STD_VALUE_NUMBER(config, -1), VM_STD_VALUE_LITERAL(str, prog));
    VM_TABLE_SET_VALUE(arg, VM_STD_VALUE_NUMBER(config, 0), VM_STD_VALUE_LITERAL(str, file));
    for (int64_t i = 0; i < argc; i++) {
        VM_TABLE_SET_VALUE(arg, VM_STD_VALUE_NUMBER(config, i+1), VM_STD_VALUE_LITERAL(str, argv[i]));
    }
    VM_TABLE_SET(std, str, "arg", table, arg);
}

vm_table_t *vm_std_new(vm_config_t *config) {
    vm_table_t *std = vm_table_new();

    {
        vm_table_t *io = vm_table_new();
        VM_TABLE_SET(std, str, "io", table, io);
        VM_TABLE_SET(io, str, "write", ffi, &vm_std_io_write);
    }

    {
        vm_table_t *string = vm_table_new();
        VM_TABLE_SET(std, str, "string", table, string);
        VM_TABLE_SET(string, str, "format", ffi, &vm_std_string_format);
    }

    {
        vm_table_t *vm = vm_table_new();
        VM_TABLE_SET(vm, str, "print", ffi, &vm_std_vm_print);
        {
            vm_table_t *vm_ver = vm_table_new();
            VM_TABLE_SET(vm, str, "version", table, vm_ver);
        }
        VM_TABLE_SET(std, str, "vm", table, vm);
    }

    {
        vm_table_t *os = vm_table_new();
        VM_TABLE_SET(os, str, "exit", ffi, &vm_std_os_exit);
        VM_TABLE_SET(std, str, "os", table, os);
    }

    VM_TABLE_SET(std, str, "tostring", ffi, &vm_std_tostring);
    VM_TABLE_SET(std, str, "tonumber", ffi, &vm_std_tonumber);
    VM_TABLE_SET(std, str, "type", ffi, &vm_std_type);
    VM_TABLE_SET(std, str, "print", ffi, &vm_std_print);
    VM_TABLE_SET(std, str, "assert", ffi, &vm_std_assert);
    VM_TABLE_SET(std, str, "load", ffi, &vm_std_load);

    return std;
}
