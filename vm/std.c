
#include "std.h"

#include "backend/backend.h"
#include "errors.h"
#include "ir.h"
#include "obj.h"
#include "gc.h"
#include "io.h"

#include "primes.inc"

#define VM_LOCATION_RANGE_FUNC ((vm_location_range_t) { .file =  "<builtins>", .src = __func__ })

void vm_std_os_exit(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    exit((int)vm_obj_get_number(args[0]));
    return;
}

void vm_std_load(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    if (!vm_obj_is_string(args[0])) {
        *args = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "cannot load non-string value"));
        return;
    }
    const char *str = vm_obj_get_string(args[0])->buf;
    vm_ir_block_t *entry = vm_lang_lua_compile(vm, str, "__load__");

    vm_obj_closure_t *closure = vm_malloc(sizeof(vm_obj_closure_t));
    closure->header = (vm_obj_gc_header_t) {0};
    closure->block = entry;
    closure->len = 0;
    vm_obj_t ret = vm_obj_of_closure(closure);
    vm_gc_add(vm, ret);
    *args = ret;
    return;
}

void vm_std_assert(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    vm_obj_t val = args[0];
    if (vm_obj_is_nil(val) || (vm_obj_is_boolean(val) && !vm_obj_get_boolean(val))) {
        vm_obj_t msg = args[1];
        vm_io_buffer_t *buf = vm_io_buffer_new();
        vm_io_buffer_obj_debug(buf, 0, "assert failed with mesage: ", msg, NULL);
        vm_obj_t ret = vm_obj_of_buffer(buf);
        vm_gc_add(vm, ret);
        *args = ret;
        return;
    } else {
        *args = vm_obj_of_nil();
        return;
    }
}

void vm_std_error(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (vm_obj_is_string(args[0])) {
        *args = vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, vm_obj_get_string(args[0])->buf));
        return;
    }
    vm_obj_t msg = args[0];
    vm_io_buffer_t *buf = vm_io_buffer_new();
    vm_io_buffer_obj_debug(buf, 0, "", msg, NULL);
    vm_obj_t ret = vm_obj_of_buffer(buf);
    vm_gc_add(vm, ret);
    *args = ret;
    return;
}

void vm_std_vm_closure(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (nargs == 0 || !vm_obj_is_block(args[0])) {
        *args = vm_obj_of_nil();
        return;
    }
    vm_obj_closure_t *closure = vm_malloc(sizeof(vm_obj_closure_t) + sizeof(vm_obj_t) * (nargs - 1));
    closure->header = (vm_obj_gc_header_t) {0};
    closure->block = vm_obj_get_block(args[0]);
    closure->len = nargs - 1;
    for (size_t i = 0; i < nargs - 1; i++) {
        closure->values[i] = args[i + 1];
    }
    vm_obj_t ret = vm_obj_of_closure(closure);
    vm_gc_add(vm, ret);
    *args = ret;
    return;
}

void vm_std_vm_gc(vm_t *vm, size_t nargs, vm_obj_t *args) {
    vm_gc_run(vm, vm->regs);
    *args = vm_obj_of_nil();
}

void vm_std_vm_print(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    for (size_t i = 0; i < nargs; i++) {
        vm_io_buffer_t *buf = vm_io_buffer_new();
        vm_io_buffer_obj_debug(buf, 0, "", args[i], NULL);
        printf("%.*s", (int)buf->len, buf->buf);
        vm_free(buf->buf);
        vm_free(buf);
    }
    *args = vm_obj_of_nil();
    return;
}

void vm_std_vm_concat(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    size_t len = 1;
    for (size_t i = 0; i < nargs; i++) {
        len += vm_obj_get_string(args[i])->len;
    }
    char *buf = vm_malloc(sizeof(char) * len);
    size_t head = 0;
    for (size_t i = 0; vm_obj_is_string(args[i]); i++) {
        size_t len = vm_obj_get_string(args[i])->len;
        if (len == 0) {
            continue;
        }
        memcpy(&buf[head], vm_obj_get_string(args[i])->buf, len);
        head += len;
    }
    buf[len - 1] = '\0';
    *args = vm_obj_of_string(vm, buf);
}

void vm_std_math_rand_int(vm_t *vm, size_t nargs, vm_obj_t *args) {
    args[0] = vm_obj_of_number(rand());
}

void vm_std_type(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    const char *ret;
    if (vm_obj_is_nil(args[0])) {
        ret = "nil";
    } else if (vm_obj_is_boolean(args[0])) {
        ret = "booolean";
    } else if (vm_obj_is_number(args[0])) {
        ret = "number";
    } else if (vm_obj_is_string(args[0])) {
        ret = "string";
    } else if (vm_obj_is_table(args[0])) {
        ret = "table";
    } else if (vm_obj_is_ffi(args[0]) || vm_obj_is_closure(args[0])) {
        ret = "function";
    } else {
        ret = "userdata";
    }
    *args = vm_obj_of_string(vm, ret);
}

void vm_std_tostring(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    vm_io_buffer_t *buf = vm_io_buffer_new();
    vm_io_buffer_object_tostring(buf, *args);
    vm_obj_t ret = vm_obj_of_buffer(buf);
    vm_gc_add(vm->gc, ret);
    *args = ret;
    return;
}

void vm_std_tonumber(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (vm_obj_is_number(args[0])) {
    } else if (vm_obj_is_string(args[0])) {
        double num;
        if (sscanf(vm_obj_get_string(args[0])->buf, "%lf", &num) == 0) {
            args[0] = vm_obj_of_nil();
            return;
        }
        args[0] = vm_obj_of_number(num);
    } else {
        args[0] = vm_obj_of_nil();
    }
}

void vm_std_print(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    vm_io_buffer_t *buf = vm_io_buffer_new();
    bool first = true;
    for (size_t i = 0; i < nargs; i++) {
        if (!first) {
            vm_io_buffer_format(buf, "\t");
        }
        vm_io_buffer_object_tostring(buf, args[i]);
        first = false;
    }
    fprintf(stdout, "%.*s\n", (int)buf->len, buf->buf);
    vm_free(buf->buf);
    vm_free(buf);
    *args = vm_obj_of_nil();
    return;
}

void vm_std_io_write(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    vm_io_buffer_t *buf = vm_io_buffer_new();
    for (size_t i = 0; i < nargs; i++) {
        vm_io_buffer_object_tostring(buf, args[i]);
    }
    fprintf(stdout, "%.*s", (int)buf->len, buf->buf);
    vm_free(buf->buf);
    vm_free(buf);
    *args = vm_obj_of_nil();
    return;
}

void vm_std_string_format(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    vm_io_buffer_t *out = vm_io_buffer_new();
    size_t argnum = 0;
    vm_obj_t fmt = args[argnum++];
    if (!vm_obj_is_string(fmt)) {
        args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (not a string)"));
        return;
    }
    const char *str = vm_obj_get_string(fmt)->buf;
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
            args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (width > 99)"));
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
            args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (precision > 99)"));
            return;
        }
        ptrdiff_t len = str - head;
        if (!(0 < len || len > 48)) {
            args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (too long to handle)"));
            return;
        }
        char format[64];
        strncpy(format, head, str - head);
        if (argnum >= nargs) {
            args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "too few args"));
            return;
        }
        vm_obj_t arg = args[argnum++];
        char fc = *str++;
        switch (fc) {
            case 'c': {
                if (!vm_obj_is_number(arg)) {
                    args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %c format"));
                    return;
                }
                strcpy(&format[len], "c");
                vm_io_buffer_format(out, format, (int)vm_obj_get_number(arg));
                break;
            }
            case 'd':
            case 'i': {
                if (!vm_obj_is_number(arg)) {
                    args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for integer format"));
                    return;
                }
                strcpy(&format[len], PRIi64);
                vm_io_buffer_format(out, format, (int64_t) vm_obj_get_number(arg));
                break;
            }
            case 'o': {
                if (!vm_obj_is_number(arg)) {
                    args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %o format"));
                    return;
                }
                strcpy(&format[len], PRIo64);
                vm_io_buffer_format(out, format, (int64_t) vm_obj_get_number(arg));
                break;
            }
            case 'u': {
                if (!vm_obj_is_number(arg)) {
                    args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %u format"));
                    return;
                }
                strcpy(&format[len], PRIu64);
                vm_io_buffer_format(out, format, (uint64_t)vm_obj_get_number(arg));
                break;
            }
            case 'x': {
                if (!vm_obj_is_number(arg)) {
                    args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %x format"));
                    return;
                }
                strcpy(&format[len], PRIx64);
                vm_io_buffer_format(out, format, (uint64_t) vm_obj_get_number(arg));
                break;
            }
            case 'X': {
                if (!vm_obj_is_number(arg)) {
                    args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %X format"));
                    return;
                }
                strcpy(&format[len], PRIX64);
                vm_io_buffer_format(out, format, (uint64_t) vm_obj_get_number(arg));
                break;
            }
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G': {
                if (!vm_obj_is_number(arg)) {
                    args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for float format"));
                    return;
                }
                format[len] = fc;
                format[len + 1] = '\0';
                vm_io_buffer_format(out, format, vm_obj_get_number(arg));
                break;
            }
            case 'q': {
                args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "unimplemented %q"));
                return;
            }
            case 's': {
                if (vm_obj_is_string(arg)) {
                    strcpy(&format[len], "s");
                    vm_io_buffer_format(out, format, vm_obj_get_string(arg));
                } else if (vm_obj_is_number(arg)) {
                    strcpy(&format[len], "f");
                    vm_io_buffer_format(out, format, vm_obj_get_number(arg));
                } else {
                    args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "unimplemented %s for a type"));
                    return;
                }
                break;
            }
            default: {
                args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %u format"));
                return;
            }
        }
    }
    args[0] = vm_obj_of_string(vm, out->buf);
    vm_free(out->buf);
    vm_free(out);
}

void vm_std_set_arg(vm_t *vm, const char *prog, const char *file, int argc, char **argv) {
    vm_obj_table_t *arg = vm_table_new(vm);
    if (prog != NULL) {
        vm_table_set(arg, vm_obj_of_number(-1), vm_obj_of_string(vm, prog));
    }
    if (file != NULL) {
        vm_table_set(arg, vm_obj_of_number(0), vm_obj_of_string(vm, file));
    }
    for (int64_t i = 0; i < argc; i++) {
        vm_table_set(arg, vm_obj_of_number(i + 1), vm_obj_of_string(vm, argv[i]));
    }
    vm_table_set(vm_obj_get_table(vm->std), vm_obj_of_string(vm, "arg"), vm_obj_of_table(arg));
}


void vm_std_table_keys(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (!vm_obj_is_table(args[0])) {
        *args = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "table.values: expect a table"));
        return;
    }
    vm_obj_table_t *ret = vm_table_new(vm);
    vm_obj_table_t *tab = vm_obj_get_table(args[0]);
    size_t len = vm_primes_table[tab->size];
    size_t write_head = 1;
    for (size_t i = 0; i < len; i++) {
        vm_table_pair_t *pair = &tab->pairs[i];
        if (!vm_obj_is_nil(pair->key)) {
            vm_obj_t key = vm_obj_of_number(write_head);
            vm_table_set(ret, key, pair->key);
            write_head++;
        }
    }
    *args = vm_obj_of_table(ret);
    return;
}

void vm_std_table_values(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (!vm_obj_is_table(args[0])) {
        *args = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "table.values: expect a table"));
        return;
    }
    vm_obj_table_t *ret = vm_table_new(vm);
    vm_obj_table_t *tab = vm_obj_get_table(args[0]);
    size_t len = vm_primes_table[tab->size];
    size_t write_head = 1;
    for (size_t i = 0; i < len; i++) {
        vm_table_pair_t *pair = &tab->pairs[i];
        if (!vm_obj_is_nil(pair->key)) {
            vm_obj_t key = vm_obj_of_number(write_head);
            vm_table_set(ret, key, pair->value);
            write_head++;
        }
    }
    *args = vm_obj_of_table(ret);
    return;
}

void vm_std_vm_import(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (!vm_obj_is_string(args[0])) {
        args[0] = vm_obj_of_string(vm, "import() must take a string");
        return;
    }
    const char *src = vm_io_read(vm_obj_get_string(args[0])->buf);
    if (src == NULL) {
        args[0] = vm_obj_of_string(vm, "import() no such file");
        return;
    }
    vm_ir_block_t *block = vm_lang_lua_compile(vm, src, vm_obj_get_string(args[0])->buf);
    args[0] = vm_run_repl(vm, block);
    vm_free(src);
    return;
}

// void vm_std_lang_eb_if(vm_t *vm, size_t nargs, vm_obj_t *args) {
//     if (!vm_obj_is_number(args[0])) {
//         args[0] = vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "pass if a number"));
//         return;
//     }
//     vm_obj_t obj;
//     if (vm_obj_get_number(args[0]) != 0) {
//         obj = args[1];
//     } else {
//         obj = args[2];
//     }
//     if (!vm_obj_is_closure(obj)) {
//         args[0] = vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "define if like: (if c (t) (f)) ?"));
//         return;
//     }
//     vm_obj_closure_t *closure = vm_obj_get_closure(obj);
//     for (size_t i = 0; i < closure->len; i++) {
//         vm->regs[i] = closure->values[i];
//     }
//     vm_obj_t ret = vm_run_repl(vm, closure->block);
//     args[0] = ret;
//     return;
// }

void vm_std_lang_eb_error(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (nargs == 0) {
        args[0] = vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "error"));
        return;
    }
    vm_io_buffer_t *buf = vm_io_buffer_new();
    vm_io_buffer_obj_debug(buf, 0, "error: ", args[0], NULL);
    args[0] = vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, ""));
    free(buf->buf);
    free(buf);
    return;
}

void vm_std_lang_eb_putchar(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (nargs == 0 || !vm_obj_is_number(args[0])) {
        args[0] = vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "putchar: takes a number"));
        return;
    }
    printf("%c", (int) vm_obj_get_number(args[0]));
    args[0] = vm_obj_of_nil();
    return;
}

void vm_std_new(vm_t *vm) {
    vm_obj_table_t *std = vm_table_new(vm);

    srand(0);

    {
        vm_obj_table_t *lang = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "lang"), vm_obj_of_table(lang));
        
        {
            vm_obj_table_t *eb = vm_table_new(vm);
            vm_table_set(lang, vm_obj_of_string(vm, "eb"), vm_obj_of_table(eb));
            vm_table_set(eb, vm_obj_of_string(vm, "putchar"), vm_obj_of_ffi(vm_std_lang_eb_putchar));
            vm_table_set(eb, vm_obj_of_string(vm, "error"), vm_obj_of_ffi(vm_std_lang_eb_error));
            vm_table_set(eb, vm_obj_of_string(vm, "debug"), vm_obj_of_ffi(vm_std_vm_print));
        }
    }
    
    {
        vm_obj_table_t *io = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "io"), vm_obj_of_table(io));
        vm_table_set(io, vm_obj_of_string(vm, "write"), vm_obj_of_ffi(vm_std_io_write));
    }

    {
        vm_obj_table_t *string = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "string"), vm_obj_of_table(string));
        vm_table_set(string, vm_obj_of_string(vm, "format"), vm_obj_of_ffi(vm_std_string_format));
    }
    
    {
        vm_obj_table_t *tvm = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "vm"), vm_obj_of_table(tvm));
        vm_table_set(tvm, vm_obj_of_string(vm, "import"), vm_obj_of_ffi(vm_std_vm_import));
        vm_table_set(tvm, vm_obj_of_string(vm, "gc"), vm_obj_of_ffi(vm_std_vm_gc));
        vm_table_set(tvm, vm_obj_of_string(vm, "print"), vm_obj_of_ffi(vm_std_vm_print));
        vm_table_set(tvm, vm_obj_of_string(vm, "version"), vm_obj_of_string(vm, VM_VERSION));
        vm_table_set(tvm, vm_obj_of_string(vm, "conacat"), vm_obj_of_ffi(vm_std_vm_concat));
    }

    {
        vm_obj_table_t *math = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "math"), vm_obj_of_table(math));
        vm_table_set(math, vm_obj_of_string(vm, "randint"), vm_obj_of_ffi(vm_std_math_rand_int));
    }

    {
        vm_obj_table_t *os = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "os"), vm_obj_of_table(os));
        vm_table_set(os, vm_obj_of_string(vm, "exit"), vm_obj_of_ffi(vm_std_os_exit));
    }

    {
        vm_obj_table_t *table = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "table"), vm_obj_of_table(table));
        vm_table_set(table, vm_obj_of_string(vm, "keys"), vm_obj_of_ffi(vm_std_table_keys));
        vm_table_set(table, vm_obj_of_string(vm, "values"), vm_obj_of_ffi(vm_std_table_values));
    }

    vm_table_set(std, vm_obj_of_string(vm, "error"), vm_obj_of_ffi(vm_std_error));
    vm_table_set(std, vm_obj_of_string(vm, "tostring"), vm_obj_of_ffi(vm_std_tostring));
    vm_table_set(std, vm_obj_of_string(vm, "tonumber"), vm_obj_of_ffi(vm_std_tonumber));
    vm_table_set(std, vm_obj_of_string(vm, "type"), vm_obj_of_ffi(vm_std_type));
    vm_table_set(std, vm_obj_of_string(vm, "print"), vm_obj_of_ffi(vm_std_print));
    vm_table_set(std, vm_obj_of_string(vm, "assert"), vm_obj_of_ffi(vm_std_assert));
    vm_table_set(std, vm_obj_of_string(vm, "load"), vm_obj_of_ffi(vm_std_load));
    vm_table_set(std, vm_obj_of_string(vm, "_G"), vm_obj_of_table(std));

    vm->std = vm_obj_of_table(std);
}
