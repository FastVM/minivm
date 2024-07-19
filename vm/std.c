
#include "./std.h"

#include "./backend/backend.h"
#include "./errors.h"
#include "./ir.h"
#include "./obj.h"
#include "./gc.h"
#include "./io.h"

#define VM_LOCATION_RANGE_FUNC ((vm_location_range_t) { .file =  "<builtins>", .src = __func__ })

static inline void vm_config_add_extern(vm_t *vm, void *value) {
    vm_externs_t *last = vm->externs;
    for (vm_externs_t *cur = last; cur; cur = cur->last) {
        if (cur->value == value) {
            return;
        }
    }
    vm_externs_t *next = vm_malloc(sizeof(vm_externs_t));
    next->id = last == NULL ? 0 : last->id + 1;
    next->value = value;
    next->last = last;
    vm->externs = next;
}

void vm_std_os_exit(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    exit((int)vm_value_to_i64(args[0]));
    return;
}

void vm_std_load(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    if (args[0].tag != VM_TAG_STR) {
        *args = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "cannot load non-string value"));
        return;
    }
    const char *str = args[0].value.str->buf;
    vm_block_t *entry = vm_compile(vm, str, "__load__");

    vm_closure_t *closure = vm_malloc(sizeof(vm_closure_t));
    closure->block = entry;
    closure->len = 0;
    vm_obj_t ret = vm_obj_of_closure(closure);
    vm_gc_run(vm, vm->regs);
    vm_gc_add(vm, ret);
    *args = ret;
    return;
}

void vm_std_assert(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    vm_obj_t val = args[0];
    if (val.tag == VM_TAG_NIL || (val.tag == VM_TAG_BOOL && !val.value.boolean)) {
        vm_obj_t msg = args[1];
        vm_io_buffer_t buf = {0};
        vm_io_debug(&buf, 0, "assert failed with mesage: ", msg, NULL);
        *args = vm_str(vm, buf.buf);
        vm_free(buf.buf);
        return;
    } else {
        *args = vm_obj_of_nil();
        return;
    }
}

void vm_std_error(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag == VM_TAG_STR) {
        *args = vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, args[0].value.str->buf));
        return;
    }
    vm_obj_t msg = args[0];
    vm_io_buffer_t buf = {0};
    vm_io_debug(&buf, 0, "", msg, NULL);
    *args = vm_str(vm, buf.buf);
    vm_free(buf.buf);
    return;
}

void vm_std_vm_closure(vm_t *vm, vm_obj_t *args) {
    int64_t nargs = 0;
    for (size_t i = 0; args[i].tag != VM_TAG_UNK; i++) {
        nargs += 1;
    }
    if (nargs == 0 || args[0].tag != VM_TAG_FUN) {
        *args = vm_obj_of_nil();
        return;
    }
    vm_closure_t *closure = vm_malloc(sizeof(vm_closure_t) + sizeof(vm_obj_t) * (nargs - 1));
    closure->block = args[0].value.fun;
    closure->len = nargs - 1;
    for (size_t i = 1; args[i].tag != VM_TAG_UNK; i++) {
        closure->values[i - 1] = args[i];
    }
    vm_obj_t ret = vm_obj_of_closure(closure);
    vm_gc_run(vm, vm->regs);
    vm_gc_add(vm, ret);
    *args = ret;
    return;
}

void vm_std_vm_gc(vm_t *vm, vm_obj_t *args) {
    vm_gc_run(vm, vm->regs);
    *args = vm_obj_of_nil();
}

void vm_std_vm_print(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    for (size_t i = 0; args[i].tag != VM_TAG_UNK; i++) {
        vm_io_buffer_t buf = {0};
        vm_io_debug(&buf, 0, "", args[i], NULL);
        printf("%.*s", (int)buf.len, buf.buf);
    }
    *args = vm_obj_of_nil();
    return;
}

void vm_std_vm_concat(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    size_t len = 1;
    for (size_t i = 0; args[i].tag == VM_TAG_STR; i++) {
        len += args[i].value.str->len;
    }
    char *buf = vm_malloc(sizeof(char) * len);
    size_t head = 0;
    for (size_t i = 0; args[i].tag == VM_TAG_STR; i++) {
        size_t len = args[i].value.str->len;
        if (len == 0) {
            continue;
        }
        memcpy(&buf[head], args[i].value.str->buf, len);
        head += len;
    }
    buf[len - 1] = '\0';
    *args = vm_str(vm, buf);
}

void vm_std_math_rand_int(vm_t *vm, vm_obj_t *args) {
    args[0] = vm_obj_of_number(rand());
}

void vm_std_type(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    const char *ret = "unknown";
    switch (args[0].tag) {
        case VM_TAG_NIL: {
            ret = "nil";
            break;
        }
        case VM_TAG_BOOL: {
            ret = "boolean";
            break;
        }
        case VM_TAG_NUMBER: {
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
    *args = vm_str(vm, ret);
}

void vm_std_tostring(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    vm_io_buffer_t out = {0};
    vm_value_buffer_tostring(&out, *args);
    *args = vm_str(vm, out.buf);
    vm_free(out.buf);
}

void vm_std_tonumber(vm_t *vm, vm_obj_t *args) {
    switch (args[0].tag) {
        case VM_TAG_NUMBER: {
            args[0] = vm_obj_of_number(args[0].value.f64);
            return;
        }
        case VM_TAG_STR: {
            double num;
            if (sscanf(args[0].value.str->buf, "%lf", &num) == 0) {
                args[0] = vm_obj_of_nil();
                return;
            }
            args[0] = vm_obj_of_number(num);
            return;
        }
        default: {
            args[0] = vm_obj_of_nil();
            return;
        }
    }
}

void vm_std_print(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    vm_obj_t *ret = args;
    vm_io_buffer_t out = {0};
    bool first = true;
    while (args->tag != VM_TAG_UNK) {
        if (!first) {
            vm_io_buffer_format(&out, "\t");
        }
        vm_value_buffer_tostring(&out, *args++);
        first = false;
    }
    fprintf(stdout, "%.*s\n", (int)out.len, out.buf);
    vm_free(out.buf);
    *ret = vm_obj_of_nil();
    return;
}

void vm_std_io_write(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    vm_obj_t *ret = args;
    vm_io_buffer_t out = {0};
    while (args->tag != VM_TAG_UNK) {
        vm_value_buffer_tostring(&out, *args++);
    }
    fprintf(stdout, "%.*s", (int)out.len, out.buf);
    *ret = vm_obj_of_nil();
    return;
}

void vm_std_string_format(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    vm_obj_t *ret = args;
    vm_io_buffer_t *out = vm_io_buffer_new();
    vm_obj_t fmt = *args++;
    if (fmt.tag == VM_TAG_STR) {
        *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (not a string)"));
        return;
    }
    const char *str = fmt.value.str->buf;
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
            *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (width > 99)"));
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
            *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (precision > 99)"));
            return;
        }
        ptrdiff_t len = str - head;
        if (!(0 < len || len > 48)) {
            *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (too long to handle)"));
            return;
        }
        char format[64];
        strncpy(format, head, str - head);
        vm_obj_t arg = *args++;
        if (arg.tag == VM_TAG_UNK) {
            *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "too few args"));
            return;
        }
        char fc = *str++;
        switch (fc) {
            case 'c': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %c format"));
                    return;
                }
                strcpy(&format[len], "c");
                vm_io_buffer_format(out, format, (int)vm_value_to_i64(arg));
                break;
            }
            case 'd':
            case 'i': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for integer format"));
                    return;
                }
                strcpy(&format[len], PRIi64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'o': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %o format"));
                    return;
                }
                strcpy(&format[len], PRIo64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'u': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %u format"));
                    return;
                }
                strcpy(&format[len], PRIu64);
                vm_io_buffer_format(out, format, (uint64_t)vm_value_to_i64(arg));
                break;
            }
            case 'x': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %x format"));
                    return;
                }
                strcpy(&format[len], PRIx64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'X': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %X format"));
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
                    *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for float format"));
                    return;
                }
                format[len] = fc;
                format[len + 1] = '\0';
                vm_io_buffer_format(out, format, vm_value_to_f64(arg));
                break;
            }
            case 'q': {
                *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "unimplemented %q"));
                return;
            }
            case 's': {
                if (arg.tag == VM_TAG_STR) {
                    strcpy(&format[len], "s");
                    vm_io_buffer_format(out, format, arg.value.str);
                } else if (vm_value_can_to_n64(arg)) {
                    strcpy(&format[len], "f");
                    vm_io_buffer_format(out, format, vm_value_to_f64(arg));
                } else {
                    *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "unimplemented %s for a type"));
                    return;
                }
                break;
            }
            default: {
                *ret = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %u format"));
                __builtin_trap();
                return;
            }
        }
    }
    *ret = vm_str(vm, out->buf);
    vm_free(out->buf);
    vm_free(out);
}

void vm_std_set_arg(vm_t *vm, const char *prog, const char *file, int argc, char **argv) {
    vm_table_t *arg = vm_table_new(vm);
    if (prog != NULL) {
        vm_table_set(arg, vm_obj_of_number(-1), vm_str(vm, prog));
    }
    if (file != NULL) {
        vm_table_set(arg, vm_obj_of_number(0), vm_str(vm, file));
    }
    for (int64_t i = 0; i < argc; i++) {
        vm_table_set(arg, vm_obj_of_number(i + 1), vm_str(vm, argv[i]));
    }
    vm_table_set(vm->std.value.table, vm_obj_of_str(vm_str(vm, "arg").value.str), vm_obj_of_table(arg));
}

void vm_std_vm_typename(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag != VM_TAG_STR) {
        *args = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "vm.type: expected string"));
        return;
    }
    const char *str = args[0].value.str->buf;
    if (!strcmp(str, "nil")) {
        *args = vm_obj_of_number(VM_TAG_NIL);
        return;
    }
    if (!strcmp(str, "bool")) {
        *args = vm_obj_of_number(VM_TAG_BOOL);
        return;
    }
    if (!strcmp(str, "f64")) {
        *args = vm_obj_of_number(VM_TAG_NUMBER);
        return;
    }
    if (!strcmp(str, "str")) {
        *args = vm_obj_of_number(VM_TAG_STR);
        return;
    }
    if (!strcmp(str, "tab")) {
        *args = vm_obj_of_number(VM_TAG_TAB);
        return;
    }
    if (!strcmp(str, "vm")) {
        *args = vm_obj_of_number(VM_TAG_CLOSURE);
        return;
    }
    if (!strcmp(str, "ffi")) {
        *args = vm_obj_of_number(VM_TAG_FFI);
        return;
    }
    if (!strcmp(str, "error")) {
        *args = vm_obj_of_number(VM_TAG_ERROR);
        return;
    }
    *args = vm_obj_of_nil();
    return;
}

void vm_std_vm_typeof(vm_t *vm, vm_obj_t *args) {
    args[0] = vm_obj_of_number(args[0].tag);
}

void vm_std_table_keys(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag != VM_TAG_TAB) {
        *args = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "table.values: expect a table"));
        return;
    }
    vm_table_t *ret = vm_table_new(vm);
    vm_table_t *tab = args[0].value.table;
    size_t len = 1 << tab->alloc;
    size_t write_head = 1;
    for (size_t i = 0; i < len; i++) {
        vm_table_pair_t *pair = &tab->pairs[i];
        if (pair->key.tag != VM_TAG_UNK) {
            vm_obj_t key = vm_obj_of_number(write_head);
            vm_table_set(ret, key, pair->key);
            write_head++;
        }
    }
    *args = vm_obj_of_table(ret);
    return;
}

void vm_std_table_values(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag != VM_TAG_TAB) {
        *args = vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "table.values: expect a table"));
        return;
    }
    vm_table_t *ret = vm_table_new(vm);
    vm_table_t *tab = args[0].value.table;
    size_t len = 1 << tab->alloc;
    size_t write_head = 1;
    for (size_t i = 0; i < len; i++) {
        vm_table_pair_t *pair = &tab->pairs[i];
        if (pair->key.tag != VM_TAG_UNK) {
            vm_obj_t key = vm_obj_of_number(write_head);
            vm_table_set(ret, key, pair->value);
            write_head++;
        }
    }
    *args = vm_obj_of_table(ret);
    return;
}

void vm_lua_comp_op_std_pow(vm_t *vm, vm_obj_t *args);

void vm_std_vm_import(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag != VM_TAG_STR) {
        args[0] = vm_str(vm, "import() must take a string");
        return;
    }
    const char *src = vm_io_read(args[0].value.str->buf);
    if (src == NULL) {
        args[0] = vm_str(vm, "import() no such file");
        return;
    }
    vm_block_t *block = vm_compile(vm, src, args[0].value.str->buf);
    args[0] = vm_run_repl(vm, block);
    vm_free(src);
    return;
}

void vm_std_new(vm_t *vm) {
    vm_table_t *std = vm_table_new(vm);

    srand(0);

    {
        vm_table_t *io = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_str(vm_str(vm, "io").value.str), vm_obj_of_table(io));
        vm_table_set(io, vm_obj_of_str(vm_str(vm, "write").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_io_write)));
    }

    {
        vm_table_t *string = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_str(vm_str(vm, "string").value.str), vm_obj_of_table(string));
        vm_table_set(string, vm_obj_of_str(vm_str(vm, "format").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_string_format)));
    }
    
    {
        vm_table_t *tvm = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_str(vm_str(vm, "vm").value.str), vm_obj_of_table(tvm));
        vm_table_set(tvm, vm_obj_of_str(vm_str(vm, "import").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_vm_import)));
        vm_table_set(tvm, vm_obj_of_str(vm_str(vm, "gc").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_vm_gc)));
        vm_table_set(tvm, vm_obj_of_str(vm_str(vm, "print").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_vm_print)));
        vm_table_set(tvm, vm_obj_of_str(vm_str(vm, "version").value.str), vm_obj_of_str(vm_str(vm, VM_VERSION).value.str));
        vm_table_set(tvm, vm_obj_of_str(vm_str(vm, "typename").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_vm_typename)));
        vm_table_set(tvm, vm_obj_of_str(vm_str(vm, "typeof").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_vm_typeof)));
        vm_table_set(tvm, vm_obj_of_str(vm_str(vm, "concat").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_vm_concat)));
    }

    {
        vm_table_t *math = vm_table_new(vm);
        vm_table_set(std, vm_obj_of_str(vm_str(vm, "math").value.str), vm_obj_of_table(math));
        vm_table_set(math, vm_obj_of_str(vm_str(vm, "randint").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_math_rand_int)));
    }

    {
        vm_table_t *os = vm_table_new(vm);
        vm_table_set(os, vm_obj_of_str(vm_str(vm, "exit").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_os_exit)));
        vm_table_set(std, vm_obj_of_str(vm_str(vm, "os").value.str), vm_obj_of_table(os));
    }

    {
        vm_table_t *table = vm_table_new(vm);
        vm_table_set(table, vm_obj_of_str(vm_str(vm, "keys").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_table_keys)));
        vm_table_set(table, vm_obj_of_str(vm_str(vm, "values").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_table_values)));
        vm_table_set(std, vm_obj_of_str(vm_str(vm, "table").value.str), vm_obj_of_table(table));
    }

    vm_table_set(std, vm_obj_of_str(vm_str(vm, "error").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_error)));
    vm_table_set(std, vm_obj_of_str(vm_str(vm, "tostring").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_tostring)));
    vm_table_set(std, vm_obj_of_str(vm_str(vm, "tonumber").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_tonumber)));
    vm_table_set(std, vm_obj_of_str(vm_str(vm, "type").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_type)));
    vm_table_set(std, vm_obj_of_str(vm_str(vm, "print").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_print)));
    vm_table_set(std, vm_obj_of_str(vm_str(vm, "assert").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_assert)));
    vm_table_set(std, vm_obj_of_str(vm_str(vm, "load").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_load)));
    vm_table_set(std, vm_obj_of_str(vm_str(vm, "_G").value.str), vm_obj_of_table(std));

    vm_config_add_extern(vm, &vm_lua_comp_op_std_pow);
    vm_config_add_extern(vm, &vm_std_vm_closure);

#if VM_USE_RAYLIB
    {
        vm_table_set(std, vm_obj_of_str(vm_str(vm, "app").value.str), vm_obj_of_ffi(VM_STD_REF(vm, vm_std_app_init)));
        vm_table_set(std, vm_obj_of_str(vm_str(vm, "draw").value.str), vm_obj_of_table(vm_table_new(vm)));
    }
#endif

    vm->std = vm_obj_of_table(std);
}
