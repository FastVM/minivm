
#include "std.h"

#include "backend/backend.h"
#include "errors.h"
#include "ir.h"
#include "obj.h"
#include "gc.h"
#include "io.h"

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
    closure->mark = 0;
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
        *args = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, vm_obj_get_string(args[0])->buf));
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
    closure->mark = 0;
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
    vm_io_buffer_t *buf = vm_io_buffer_new();
    for (size_t i = 0; i < nargs; i++) {
        vm_io_buffer_object_tostring(buf, args[i]);
    }
    vm_obj_t ret = vm_obj_of_buffer(buf);
    vm_gc_add(vm, ret);
    *args = ret;
    return;
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
    vm_gc_add(vm, ret);
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

void vm_std_string_len(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (nargs == 0 || !vm_obj_is_string(args[0])) {
        args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "string.sub: first argument not a string"));
        return;
    }
    args[0] = vm_obj_of_number(vm_obj_get_string(args[0])->len);
    return;
}

void vm_std_string_sub(vm_t *vm, size_t nargs, vm_obj_t *args) {
    if (nargs == 0 || !vm_obj_is_string(args[0])) {
        args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "string.sub: first argument not a string"));
        return;
    }
    vm_io_buffer_t *buf = vm_obj_get_string(args[0]);
    if (nargs == 1 || !vm_obj_is_number(args[1])) {
        args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "string.sub: second argument not a number"));
        return;
    }
    int32_t start = (int32_t) vm_obj_get_number(args[1]);
    if (nargs == 2) {
        vm_io_buffer_t *out = vm_io_buffer_new();
        for (int32_t i = start-1; i <= out->len-1; i++) {
            vm_io_buffer_format(out, "%c", (int) buf->buf[i]);
        }
        vm_gc_add(vm, args[0] = vm_obj_of_buffer(out));
        return;
    } else {
        if (!vm_obj_is_number(args[2])) {
            args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "string.sub: third argument not a number"));
            return;
        }
        int32_t stop = (int32_t) vm_obj_get_number(args[2]);
        vm_io_buffer_t *out = vm_io_buffer_new();
        for (int32_t i = start-1; i <= stop-1; i++) {
            vm_io_buffer_format(out, "%c", (int) buf->buf[i]);
        }
        vm_gc_add(vm, args[0] = vm_obj_of_buffer(out));
        return;
    }
}

void vm_std_string_format(vm_t *vm, size_t nargs, vm_obj_t *args) {
    (void)vm;
    size_t argnum = 0;
    vm_obj_t fmt = args[argnum++];
    if (!vm_obj_is_string(fmt)) {
        args[0] = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (not a string)"));
        return;
    }
    vm_io_buffer_t *out = vm_io_buffer_new();
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
                    vm_io_buffer_format(out, format, vm_obj_get_string(arg)->buf);
                } else if (vm_obj_is_number(arg)) {
                    strcpy(&format[len], &VM_FORMAT_FLOAT[1]);
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
    vm_gc_add(vm, args[0] = vm_obj_of_buffer(out));
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
        vm_obj_t key = tab->entries[i];
        if (!vm_obj_is_nil(key)) {
            vm_obj_t nth = vm_obj_of_number(write_head);
            vm_table_set(ret, nth, key);
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
        if (!vm_obj_is_nil(tab->entries[i])) {
            vm_obj_t nth = vm_obj_of_number(write_head);
            vm_table_set(ret, nth, tab->entries[vm_primes_table[tab->size] + i]);
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

void vm_std_new(vm_t *vm) {
    vm_obj_table_t *std = vm_table_new(vm);

    srand(0);
    
    {
        vm_obj_table_t *io = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "io"), vm_obj_of_table(io));
        vm_table_set(io, vm_obj_of_string(vm, "write"), vm_obj_of_ffi(vm_std_io_write));
    }

    {
        vm_obj_table_t *string = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "string"), vm_obj_of_table(string));
        vm_table_set(string, vm_obj_of_string(vm, "format"), vm_obj_of_ffi(vm_std_string_format));
        vm_table_set(string, vm_obj_of_string(vm, "sub"), vm_obj_of_ffi(vm_std_string_sub));
        vm_table_set(string, vm_obj_of_string(vm, "len"), vm_obj_of_ffi(vm_std_string_len));
    }
    
    {
        vm_obj_table_t *tvm = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_string(vm, "vm"), vm_obj_of_table(tvm));
        vm_table_set(tvm, vm_obj_of_string(vm, "import"), vm_obj_of_ffi(vm_std_vm_import));
        vm_table_set(tvm, vm_obj_of_string(vm, "gc"), vm_obj_of_ffi(vm_std_vm_gc));
        vm_table_set(tvm, vm_obj_of_string(vm, "print"), vm_obj_of_ffi(vm_std_vm_print));
        vm_table_set(tvm, vm_obj_of_string(vm, "version"), vm_obj_of_string(vm, VM_VERSION));
        vm_table_set(tvm, vm_obj_of_string(vm, "concat"), vm_obj_of_ffi(vm_std_vm_concat));
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
