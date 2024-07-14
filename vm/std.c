
#include "./std.h"

#include "./backend/backend.h"
#include "./errors.h"
#include "./ir.h"
#include "./obj.h"
#include "./gc.h"
#include "./io.h"

#define VM_LOCATION_RANGE_FUNC ((vm_location_range_t) { .file =  "<builtins>", .src = __func__ })

#define VM_PAIR_VALUE(PAIR_) ({      \
    vm_table_pair_t pair_ = (PAIR_); \
    (vm_obj_t){                      \
        .tag = pair_.val_tag,        \
        .value = pair_.val_val,      \
    };                               \
})

#define VM_PAIR_PTR_VALUE(PPAIR_) ({   \
    vm_table_pair_t *pair_ = (PPAIR_); \
    pair_ == NULL                      \
        ? VM_OBJ_NIL                   \
        : (vm_obj_t){                  \
              .tag = pair_->val_tag,   \
              .value = pair_->val_val, \
          };                           \
})

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
    if (args[0].tag == VM_TAG_STR) {
        *args = (vm_obj_t){
            .tag = VM_TAG_ERROR,
            .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "cannot load non-string value"),
        };
    }
    const char *str = args[0].value.str->buf;
    vm_block_t *entry = vm_compile(vm, str, "__load__");

    vm_closure_t *closure = vm_malloc(sizeof(vm_closure_t) + sizeof(vm_obj_t) * 1);
    closure->len = 1;
    closure->values[0] = (vm_obj_t){
        .tag = VM_TAG_FUN,
        .value.i32 = (int32_t)entry->id,
    };
    vm_obj_t ret =  (vm_obj_t) {
        .tag = VM_TAG_CLOSURE,
        .value.closure = closure,
    };
    vm_gc_run(vm, vm->regs);
    vm_gc_add(vm, ret);
    *args = ret;
    return;
}

void vm_std_assert(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    vm_obj_t val = args[0];
    if (val.tag == VM_TAG_NIL || (val.tag == VM_TAG_BOOL && !val.value.b)) {
        vm_obj_t msg = args[1];
        vm_io_buffer_t buf = {0};
        vm_io_debug(&buf, 0, "assert failed with mesage: ", msg, NULL);
        *args = vm_str(vm, buf.buf);
        vm_free(buf.buf);
        return;
    } else {
        *args = (vm_obj_t){
            .tag = VM_TAG_NIL,
        };
        return;
    }
}

void vm_std_error(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag == VM_TAG_STR) {
        *args = (vm_obj_t){
            .tag = VM_TAG_ERROR,
            .value.error = vm_error_from_msg(vm_location_range_unknown, args[0].value.str->buf),
        };
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
    (void)vm;
    int64_t nargs = 0;
    for (size_t i = 0; args[i].tag != VM_TAG_UNK; i++) {
        nargs += 1;
    }
    if (nargs == 0 || args[0].tag != VM_TAG_FUN) {
        *args = (vm_obj_t){
            .tag = VM_TAG_NIL,
        };
        return;
    }
    vm_closure_t *closure = vm_malloc(sizeof(vm_closure_t) + sizeof(vm_obj_t) * nargs);
    closure->len = nargs;
    for (size_t i = 0; args[i].tag != VM_TAG_UNK; i++) {
        closure->values[i] = args[i];
    }
    vm_obj_t ret =  (vm_obj_t) {
        .tag = VM_TAG_CLOSURE,
        .value.closure = closure,
    };
    vm_gc_run(vm, vm->regs);
    vm_gc_add(vm, ret);
    *args = ret;
    return;
}

void vm_std_vm_gc(vm_t *vm, vm_obj_t *args) {
    vm_gc_run(vm, vm->regs);
    *args = VM_OBJ_NIL;
}

void vm_std_vm_print(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    for (size_t i = 0; args[i].tag != VM_TAG_UNK; i++) {
        vm_io_buffer_t buf = {0};
        vm_io_debug(&buf, 0, "", args[i], NULL);
        printf("%.*s", (int)buf.len, buf.buf);
    }
    args[0] = (vm_obj_t){
        .tag = VM_TAG_NIL,
    };
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
    args[0] = VM_OBJ_NUMBER(vm, rand());
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
        case VM_TAG_I8: {
            args[0] = VM_OBJ_NUMBER(vm, args[0].value.i8);
            return;
        }
        case VM_TAG_I16: {
            args[0] = VM_OBJ_NUMBER(vm, args[0].value.i16);
            return;
        }
        case VM_TAG_I32: {
            args[0] = VM_OBJ_NUMBER(vm, args[0].value.i32);
            return;
        }
        case VM_TAG_I64: {
            args[0] = VM_OBJ_NUMBER(vm, args[0].value.i64);
            return;
        }
        case VM_TAG_F32: {
            args[0] = VM_OBJ_NUMBER(vm, args[0].value.f32);
            return;
        }
        case VM_TAG_F64: {
            args[0] = VM_OBJ_NUMBER(vm, args[0].value.f64);
            return;
        }
        case VM_TAG_STR: {
            if (vm->use_num == VM_USE_NUM_F32 || vm->use_num == VM_USE_NUM_F64) {
                double num;
                if (sscanf(args[0].value.str->buf, "%lf", &num) == 0) {
                    args[0] = VM_OBJ_NIL;
                    return;
                }
                args[0] = VM_OBJ_NUMBER(vm, num);
                return;
            } else {
                int64_t num;
                if (sscanf(args[0].value.str->buf, "%" SCNi64, &num) == 0) {
                    args[0] = VM_OBJ_NIL;
                    return;
                }
                args[0] = VM_OBJ_NUMBER(vm, num);
                return;
            }
        }
        default: {
            args[0] = VM_OBJ_NIL;
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
    *ret = (vm_obj_t){
        .tag = VM_TAG_NIL,
    };
}

void vm_std_io_write(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    vm_obj_t *ret = args;
    vm_io_buffer_t out = {0};
    while (args->tag != VM_TAG_UNK) {
        vm_value_buffer_tostring(&out, *args++);
    }
    fprintf(stdout, "%.*s", (int)out.len, out.buf);
    *ret = (vm_obj_t){
        .tag = VM_TAG_NIL,
    };
}

void vm_std_string_format(vm_t *vm, vm_obj_t *args) {
    (void)vm;
    vm_obj_t *ret = args;
    vm_io_buffer_t *out = vm_io_buffer_new();
    vm_obj_t fmt = *args++;
    if (fmt.tag == VM_TAG_STR) {
        *ret = (vm_obj_t){
            .tag = VM_TAG_ERROR,
            .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (not a string)"),
        };
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
            *ret = (vm_obj_t){
                .tag = VM_TAG_ERROR,
                .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (width > 99)"),
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
            *ret = (vm_obj_t){
                .tag = VM_TAG_ERROR,
                .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (precision > 99)"),
            };
            return;
        }
        ptrdiff_t len = str - head;
        if (!(0 < len || len > 48)) {
            *ret = (vm_obj_t){
                .tag = VM_TAG_ERROR,
                .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "invalid format (too long to handle)"),
            };
            return;
        }
        char format[64];
        strncpy(format, head, str - head);
        vm_obj_t arg = *args++;
        if (arg.tag == VM_TAG_UNK) {
            *ret = (vm_obj_t){
                .tag = VM_TAG_ERROR,
                .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "too few args"),
            };
            return;
        }
        char fc = *str++;
        switch (fc) {
            case 'c': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_obj_t){
                        .tag = VM_TAG_ERROR,
                        .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %c format"),
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
                    *ret = (vm_obj_t){
                        .tag = VM_TAG_ERROR,
                        .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for integer format"),
                    };
                    return;
                }
                strcpy(&format[len], PRIi64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'o': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_obj_t){
                        .tag = VM_TAG_ERROR,
                        .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %o format"),
                    };
                    return;
                }
                strcpy(&format[len], PRIo64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'u': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_obj_t){
                        .tag = VM_TAG_ERROR,
                        .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %u format"),
                    };
                    return;
                }
                strcpy(&format[len], PRIu64);
                vm_io_buffer_format(out, format, (uint64_t)vm_value_to_i64(arg));
                break;
            }
            case 'x': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_obj_t){
                        .tag = VM_TAG_ERROR,
                        .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %x format"),
                    };
                    return;
                }
                strcpy(&format[len], PRIx64);
                vm_io_buffer_format(out, format, vm_value_to_i64(arg));
                break;
            }
            case 'X': {
                if (!vm_value_can_to_n64(arg)) {
                    *ret = (vm_obj_t){
                        .tag = VM_TAG_ERROR,
                        .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for %X format"),
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
                    *ret = (vm_obj_t){
                        .tag = VM_TAG_ERROR,
                        .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "expected a number for float format"),
                    };
                    return;
                }
                format[len] = fc;
                format[len + 1] = '\0';
                vm_io_buffer_format(out, format, vm_value_to_f64(arg));
                break;
            }
            case 'q': {
                *ret = (vm_obj_t){
                    .tag = VM_TAG_ERROR,
                    .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "unimplemented %q"),
                };
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
                    *ret = (vm_obj_t){
                        .tag = VM_TAG_ERROR,
                        .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "unimplemented %s for a type"),
                    };
                    return;
                }
                break;
            }
            default: {
                *ret = (vm_obj_t){
                    .tag = VM_TAG_ERROR,
                    .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "unknown format type"),
                };
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
        VM_TABLE_SET_VALUE(arg, VM_OBJ_NUMBER(vm, -1), vm_str(vm, prog));
    }
    if (file != NULL) {
        VM_TABLE_SET_VALUE(arg, VM_OBJ_NUMBER(vm, 0), vm_str(vm, file));
    }
    for (int64_t i = 0; i < argc; i++) {
        VM_TABLE_SET_VALUE(arg, VM_OBJ_NUMBER(vm, i + 1), vm_str(vm, argv[i]));
    }
    VM_TABLE_SET(vm->std.value.table, str, vm_str(vm, "arg").value.str, table, arg);
}

void vm_std_vm_typename(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag != VM_TAG_STR) {
        *args = (vm_obj_t){
            .tag = VM_TAG_ERROR,
            .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "vm.type: expected string"),
        };
        return;
    }
    const char *str = args[0].value.str->buf;
    if (!strcmp(str, "nil")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_NIL);
        return;
    }
    if (!strcmp(str, "bool")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_BOOL);
        return;
    }
    if (!strcmp(str, "i8")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_I8);
        return;
    }
    if (!strcmp(str, "i16")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_I16);
        return;
    }
    if (!strcmp(str, "i32")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_I32);
        return;
    }
    if (!strcmp(str, "i64")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_I64);
        return;
    }
    if (!strcmp(str, "f32")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_F32);
        return;
    }
    if (!strcmp(str, "f64")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_F64);
        return;
    }
    if (!strcmp(str, "str")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_STR);
        return;
    }
    if (!strcmp(str, "tab")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_TAB);
        return;
    }
    if (!strcmp(str, "vm")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_CLOSURE);
        return;
    }
    if (!strcmp(str, "ffi")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_FFI);
        return;
    }
    if (!strcmp(str, "error")) {
        *args = VM_OBJ_NUMBER(vm, VM_TAG_ERROR);
        return;
    }
    *args = VM_OBJ_NIL;
    return;
}

void vm_std_vm_typeof(vm_t *vm, vm_obj_t *args) {
    args[0] = VM_OBJ_NUMBER(vm, args[0].tag);
}

void vm_std_table_keys(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag != VM_TAG_TAB) {
        *args = (vm_obj_t){
            .tag = VM_TAG_ERROR,
            .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "table.values: expect a table"),
        };
        return;
    }
    vm_table_t *ret = vm_table_new(vm);
    vm_table_t *tab = args[0].value.table;
    size_t len = 1 << tab->alloc;
    size_t write_head = 1;
    for (size_t i = 0; i < len; i++) {
        vm_table_pair_t *pair = &tab->pairs[i];
        if (pair->key_tag != VM_TAG_UNK) {
            vm_obj_t key = VM_OBJ_NUMBER(vm, write_head);
            vm_obj_t value = (vm_obj_t){
                .tag = pair->key_tag,
                .value = pair->key_val,
            };
            VM_TABLE_SET_VALUE(ret, key, value);
            write_head++;
        }
    }
    *args = (vm_obj_t){
        .tag = VM_TAG_TAB,
        .value.table = ret,
    };
    return;
}

void vm_std_table_values(vm_t *vm, vm_obj_t *args) {
    if (args[0].tag != VM_TAG_TAB) {
        *args = (vm_obj_t){
            .tag = VM_TAG_ERROR,
            .value.error = vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "table.values: expect a table"),
        };
        return;
    }
    vm_table_t *ret = vm_table_new(vm);
    vm_table_t *tab = args[0].value.table;
    size_t len = 1 << tab->alloc;
    size_t write_head = 1;
    for (size_t i = 0; i < len; i++) {
        vm_table_pair_t *pair = &tab->pairs[i];
        if (pair->key_tag != VM_TAG_UNK) {
            vm_obj_t key = VM_OBJ_NUMBER(vm, write_head);
            vm_obj_t value = (vm_obj_t){
                .tag = pair->val_tag,
                .value = pair->val_val,
            };
            VM_TABLE_SET_VALUE(ret, key, value);
            write_head++;
        }
    }
    *args = (vm_obj_t){
        .tag = VM_TAG_TAB,
        .value.table = ret,
    };
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

#if VM_USE_RAYLIB
#include "./backend/backend.h"
#include "./canvas.h"
#include "./save/value.h"

static int vm_std_app_repl(void *arg) {
    vm_repl(arg);
    vm_t *vm = arg;
    if (vm->save_file != NULL) {
        vm_save_t save = vm_save_value(vm);
        FILE *f = fopen(vm->save_file, "wb");
        if (f != NULL) {
            fwrite(save.buf, 1, save.len, f);
            fclose(f);
        }
    }
    exit(0);
    return 0;
}

static Color vm_value_to_color(vm_obj_t arg) {
    if (arg.tag == VM_TAG_I8 || arg.tag == VM_TAG_I16 || arg.tag == VM_TAG_I32 || arg.tag == VM_TAG_I64 || arg.tag == VM_TAG_F32 || arg.tag == VM_TAG_F64) {
        uint8_t c = vm_value_to_i64(arg);
        return (Color){
            c,
            c,
            c,
            255,
        };
    } else if (arg.tag == VM_TAG_TAB) {
        vm_table_pair_t *r = VM_TABLE_LOOKUP_STR(arg.value.table, "red");
        vm_table_pair_t *g = VM_TABLE_LOOKUP_STR(arg.value.table, "green");
        vm_table_pair_t *b = VM_TABLE_LOOKUP_STR(arg.value.table, "blue");
        vm_table_pair_t *a = VM_TABLE_LOOKUP_STR(arg.value.table, "alpha");
        return (Color){
            r ? vm_value_to_i64(VM_PAIR_VALUE(*r)) : 0,
            g ? vm_value_to_i64(VM_PAIR_VALUE(*g)) : 0,
            b ? vm_value_to_i64(VM_PAIR_VALUE(*b)) : 0,
            a ? vm_value_to_i64(VM_PAIR_VALUE(*a)) : 255,
        };
    } else {
        return BLACK;
    }
}

static Color vm_value_field_to_color(vm_obj_t arg, const char *field) {
    if (arg.tag != VM_TAG_TAB) {
        return BLACK;
    }
    return vm_value_to_color(VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, field)));
}

static Vector2 vm_value_to_vector2(vm_obj_t arg) {
    if (arg.tag != VM_TAG_TAB) {
        return (Vector2){0, 0};
    }
    vm_table_pair_t *x = VM_TABLE_LOOKUP_STR(arg.value.table, "x");
    vm_table_pair_t *y = VM_TABLE_LOOKUP_STR(arg.value.table, "y");
    return (Vector2){
        x ? vm_value_to_i64(VM_PAIR_VALUE(*x)) : 0,
        y ? vm_value_to_i64(VM_PAIR_VALUE(*y)) : 0,
    };
}

static Vector2 vm_value_field_to_vector2(vm_obj_t arg, const char *field) {
    if (arg.tag != VM_TAG_TAB) {
        return (Vector2){0, 0};
    }
    return vm_value_to_vector2(VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, field)));
}

static void vm_std_app_draw_tree(vm_t *vm, Rectangle rect, vm_obj_t arg);

static void vm_std_app_draw_tree_children(vm_t *vm, Rectangle rect, vm_table_t *tree) {
    int32_t i = 1;
    while (true) {
        vm_obj_t child = VM_PAIR_PTR_VALUE(vm_table_lookup(tree, (vm_value_t){.i32 = i}, VM_TAG_I32));
        if (child.tag != VM_TAG_TAB) {
            break;
        }
        i += 1;
        vm_std_app_draw_tree(vm, rect, child);
    }
}

static void vm_std_app_draw_tree_children_list(vm_t *vm, Rectangle rect, vm_table_t *tree) {
    size_t len = tree->len;
    if (len == 0) {
        return;
    }
    float height = rect.height / len;
    Rectangle child_rect = (Rectangle){
        rect.x,
        rect.y,
        rect.width,
        height,
    };
    int32_t i = 1;
    while (true) {
        vm_obj_t child = VM_PAIR_PTR_VALUE(vm_table_lookup(tree, (vm_value_t){.i32 = i}, VM_TAG_I32));
        if (child.tag != VM_TAG_TAB) {
            break;
        }
        i += 1;
        vm_std_app_draw_tree(vm, child_rect, child);
        child_rect.y += height;
    }
}

static void vm_std_app_draw_tree_children_split(vm_t *vm, Rectangle rect, vm_table_t *tree) {
    size_t len = tree->len;
    if (len == 0) {
        return;
    }
    float width = rect.width / len;
    Rectangle child_rect = (Rectangle){
        rect.x,
        rect.y,
        width,
        rect.height,
    };
    int32_t i = 1;
    while (true) {
        vm_obj_t child = VM_PAIR_PTR_VALUE(vm_table_lookup(tree, (vm_value_t){.i32 = i}, VM_TAG_I32));
        if (child.tag != VM_TAG_TAB) {
            break;
        }
        i += 1;
        vm_std_app_draw_tree(vm, child_rect, child);
        child_rect.x += width;
    }
}

static void vm_std_app_draw_tree(vm_t *vm, Rectangle rect, vm_obj_t arg) {
    if (arg.tag != VM_TAG_TAB) {
        return;
    }
    vm_table_t *tree = arg.value.table;
    vm_obj_t std_type = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(tree, "type"));
    if (std_type.tag == VM_TAG_NIL) {
        size_t n = (1 << tree->alloc);
        for (size_t i = 0; i < n; i++) {
            vm_table_pair_t *pair = &tree->pairs[i];
            if (pair->key_tag == VM_TAG_UNK) {
                continue;
            }
            vm_std_app_draw_tree(vm, rect, (vm_obj_t){
                                               .tag = pair->val_tag,
                                               .value = pair->val_val,
                                           });
        }
    } else if (std_type.tag == VM_TAG_STR) {
        const char *type = std_type.value.str;
        if (!strcmp(type, "rectangle")) {
            DrawRectangleRec(rect, vm_value_field_to_color(arg, "color"));
            vm_std_app_draw_tree_children(vm, rect, tree);
        } else if (!strcmp(type, "list")) {
            vm_std_app_draw_tree_children_list(vm, rect, tree);
        } else if (!strcmp(type, "split")) {
            vm_std_app_draw_tree_children_split(vm, rect, tree);
        } else if (!strcmp(type, "click")) {
            vm_std_app_draw_tree_children(vm, rect, tree);
            vm_obj_t button = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "button"));
            vm_obj_t run = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "run"));
            if (button.tag == VM_TAG_STR) {
                const char *name = button.value.str;
                if ((!strcmp(name, "left") && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || (!strcmp(name, "right") && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || (!strcmp(name, "middle") && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))) {
                    if (CheckCollisionPointRec(GetMousePosition(), rect)) {
                        vm_std_app_draw_tree(vm, rect, run);
                    }
                }
            }
        } else if (!strcmp(type, "drag")) {
            vm_std_app_draw_tree_children(vm, rect, tree);
            vm_obj_t button = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "button"));
            vm_obj_t run = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "run"));
            if (button.tag == VM_TAG_STR) {
                const char *name = button.value.str;
                if ((!strcmp(name, "left") && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) || (!strcmp(name, "right") && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) || (!strcmp(name, "middle") && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) {
                    if (CheckCollisionPointRec(GetMousePosition(), rect)) {
                        vm_std_app_draw_tree(vm, rect, run);
                    }
                }
            }
        } else if (!strcmp(type, "code")) {
            vm_obj_t run = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "run"));
            if (run.tag == VM_TAG_CLOSURE) {
                vm->regs[0] = run;
                vm_run_repl(vm, vm->blocks->blocks[run.value.closure[0].value.i32]);
            }
        } else if (!strcmp(type, "window")) {
            int64_t width = vm_value_to_i64(VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "width")));
            int64_t height = vm_value_to_i64(VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "height")));
            SetWindowSize(width, height);
        } else if (!strcmp(type, "keydown") || !strcmp(type, "keyup") || !strcmp(type, "keypressed") || !strcmp(type, "keyreleased")) {
            vm_obj_t run = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "run"));
            vm_obj_t key_obj = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(arg.value.table, "key"));
            if (key_obj.tag == VM_TAG_STR) {
                const char *key = key_obj.value.str;
                bool (*func)(int) = NULL;
                if (!strcmp(type, "keydown")) {
                    func = IsKeyDown;
                }
                if (!strcmp(type, "keyup")) {
                    func = IsKeyUp;
                }
                if (!strcmp(type, "keypressed")) {
                    func = IsKeyPressed;
                }
                if (!strcmp(type, "keyreleased")) {
                    func = IsKeyReleased;
                }
                if ((!strcmp(key, "APOSTROPHE") && func('\'')) || (!strcmp(key, "SPACE") && func(' ')) || (!strcmp(key, "COMMA") && func(',')) || (!strcmp(key, "MINUS") && func('-')) || (!strcmp(key, "PERIOD") && func('.')) || (!strcmp(key, "SLASH") && func('/')) || (!strcmp(key, "ZERO") && func('0')) || (!strcmp(key, "ONE") && func('1')) || (!strcmp(key, "TWO") && func('2')) || (!strcmp(key, "THREE") && func('3')) || (!strcmp(key, "FOUR") && func('4')) || (!strcmp(key, "FIVE") && func('5')) || (!strcmp(key, "SIX") && func('6')) || (!strcmp(key, "SEVEN") && func('7')) || (!strcmp(key, "EIGHT") && func('8')) || (!strcmp(key, "NINE") && func('9')) || (!strcmp(key, "SEMICOLON") && func(';')) || (!strcmp(key, "EQUAL") && func('=')) || (!strcmp(key, "A") && func('A')) || (!strcmp(key, "B") && func('B')) || (!strcmp(key, "C") && func('C')) || (!strcmp(key, "D") && func('D')) || (!strcmp(key, "E") && func('E')) || (!strcmp(key, "F") && func('F')) || (!strcmp(key, "G") && func('G')) || (!strcmp(key, "H") && func('H')) || (!strcmp(key, "I") && func('I')) || (!strcmp(key, "J") && func('J')) || (!strcmp(key, "K") && func('K')) || (!strcmp(key, "L") && func('L')) || (!strcmp(key, "M") && func('M')) || (!strcmp(key, "N") && func('N')) || (!strcmp(key, "O") && func('O')) || (!strcmp(key, "P") && func('P')) || (!strcmp(key, "Q") && func('Q')) || (!strcmp(key, "R") && func('R')) || (!strcmp(key, "S") && func('S')) || (!strcmp(key, "T") && func('T')) || (!strcmp(key, "U") && func('U')) || (!strcmp(key, "V") && func('V')) || (!strcmp(key, "W") && func('W')) || (!strcmp(key, "X") && func('X')) || (!strcmp(key, "Y") && func('Y')) || (!strcmp(key, "Z") && func('Z')) || (!strcmp(key, "LEFT_BRACKET") && func('[')) || (!strcmp(key, "BACKSLASH") && func('\\')) || (!strcmp(key, "RIGHT_BRACKET") && func(']')) || (!strcmp(key, "GRAVE") && func('`'))) {
                    vm_std_app_draw_tree(vm, rect, run);
                }
            }
        }
    }
}

#if defined(VM_USE_CANVAS)
#include <emscripten.h>

EM_JS(void, vm_std_app_frame_loop, (vm_t * vm), {
    Module._vm_std_app_frame_loop(vm);
});

void EMSCRIPTEN_KEEPALIVE vm_std_app_frame(vm_t *vm) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    vm_obj_t tree = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(vm->std.value.table, "draw"));
    Rectangle rect = (Rectangle){
        0,
        0,
        GetScreenWidth(),
        GetScreenHeight(),
    };
    vm_std_app_draw_tree(vm, rect, tree);
    EndDrawing();
}

void EMSCRIPTEN_KEEPALIVE vm_std_app_sync(vm_t *vm) {
    vm_save_t save = vm_save_value(vm);
    FILE *f = fopen("/out.bin", "wb");
    fwrite(save.buf, 1, save.len, f);
    fclose(f);
}

void vm_std_app_init(vm_t *vm, vm_obj_t *args) {
    SetTraceLogLevel(LOG_WARNING);
    SetTargetFPS(60);
    InitWindow(960, 540, "MiniVM");
    uint64_t n = 0;
    {
        FILE *f = fopen("/in.bin", "rb");
        if (f != NULL) {
            vm_save_t save = vm_save_load(f);
            fclose(f);
            vm_load_value(vm, save);
        }
    }
    vm_std_app_frame_loop(vm);
    args[0] = VM_OBJ_NIL;
    // vm_std_app_repl(vm);
}

#else
void vm_std_app_init(vm_t *vm, vm_obj_t *args) {
    SetTraceLogLevel(LOG_WARNING);
    SetTargetFPS(60);
    InitWindow(960, 540, "MiniVM");
    uint64_t n = 0;
    {
        FILE *f = fopen("/in.bin", "rb");
        if (f != NULL) {
            vm_save_t save = vm_save_load(f);
            fclose(f);
            vm_load_value(vm, save);
        }
    }
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        vm_obj_t tree = VM_PAIR_PTR_VALUE(VM_TABLE_LOOKUP_STR(vm->std.value.table, "draw"));
        Rectangle rect = (Rectangle){
            0,
            0,
            GetScreenWidth(),
            GetScreenHeight(),
        };
        vm_std_app_draw_tree(vm, rect, tree);
        EndDrawing();
    }
    CloseWindow();
    if (vm->save_file != NULL) {
        vm_save_t save = vm_save_value(vm);
        FILE *f = fopen(vm->save_file, "wb");
        if (f != NULL) {
            fwrite(save.buf, 1, save.len, f);
            fclose(f);
        }
    }
    exit(0);
    return;
}

#endif
#endif

void vm_std_new(vm_t *vm) {
    vm_table_t *std = vm_table_new(vm);

    srand(0);

    {
        vm_table_t *io = vm_table_new(vm);
        VM_TABLE_SET(std, str, vm_str(vm, "io").value.str, table, io);
        VM_TABLE_SET(io, str, vm_str(vm, "write").value.str, ffi, VM_STD_REF(vm, vm_std_io_write));
    }

    {
        vm_table_t *string = vm_table_new(vm);
        VM_TABLE_SET(std, str, vm_str(vm, "string").value.str, table, string);
        VM_TABLE_SET(string, str, vm_str(vm, "format").value.str, ffi, VM_STD_REF(vm, vm_std_string_format));
    }

    {
        vm_table_t *tvm = vm_table_new(vm);
        VM_TABLE_SET(std, str, vm_str(vm, "vm").value.str, table, tvm);
        VM_TABLE_SET(tvm, str, vm_str(vm, "import").value.str, ffi, VM_STD_REF(vm, vm_std_vm_import));
        VM_TABLE_SET(tvm, str, vm_str(vm, "gc").value.str, ffi, VM_STD_REF(vm, vm_std_vm_gc));
        VM_TABLE_SET(tvm, str, vm_str(vm, "print").value.str, ffi, VM_STD_REF(vm, vm_std_vm_print));
        VM_TABLE_SET(tvm, str, vm_str(vm, "version").value.str, str, vm_str(vm, VM_VERSION).value.str);
        VM_TABLE_SET(tvm, str, vm_str(vm, "typename").value.str, ffi, VM_STD_REF(vm, vm_std_vm_typename));
        VM_TABLE_SET(tvm, str, vm_str(vm, "typeof").value.str, ffi, VM_STD_REF(vm, vm_std_vm_typeof));
        VM_TABLE_SET(tvm, str, vm_str(vm, "concat").value.str, ffi, VM_STD_REF(vm, vm_std_vm_concat));
    }

    {
        vm_table_t *math = vm_table_new(vm);
        VM_TABLE_SET(std, str, vm_str(vm, "math").value.str, table, math);
        VM_TABLE_SET(math, str, vm_str(vm, "randint").value.str, ffi, VM_STD_REF(vm, vm_std_math_rand_int));
    }

    {
        vm_table_t *os = vm_table_new(vm);
        VM_TABLE_SET(os, str, vm_str(vm, "exit").value.str, ffi, VM_STD_REF(vm, vm_std_os_exit));
        VM_TABLE_SET(std, str, vm_str(vm, "os").value.str, table, os);
    }

    {
        vm_table_t *table = vm_table_new(vm);
        VM_TABLE_SET(table, str, vm_str(vm, "keys").value.str, ffi, VM_STD_REF(vm, vm_std_table_keys));
        VM_TABLE_SET(table, str, vm_str(vm, "values").value.str, ffi, VM_STD_REF(vm, vm_std_table_values));
        VM_TABLE_SET(std, str, vm_str(vm, "table").value.str, table, table);
    }

    VM_TABLE_SET(std, str, vm_str(vm, "error").value.str, ffi, VM_STD_REF(vm, vm_std_error));
    VM_TABLE_SET(std, str, vm_str(vm, "tostring").value.str, ffi, VM_STD_REF(vm, vm_std_tostring));
    VM_TABLE_SET(std, str, vm_str(vm, "tonumber").value.str, ffi, VM_STD_REF(vm, vm_std_tonumber));
    VM_TABLE_SET(std, str, vm_str(vm, "type").value.str, ffi, VM_STD_REF(vm, vm_std_type));
    VM_TABLE_SET(std, str, vm_str(vm, "print").value.str, ffi, VM_STD_REF(vm, vm_std_print));
    VM_TABLE_SET(std, str, vm_str(vm, "assert").value.str, ffi, VM_STD_REF(vm, vm_std_assert));
    VM_TABLE_SET(std, str, vm_str(vm, "load").value.str, ffi, VM_STD_REF(vm, vm_std_load));
    VM_TABLE_SET(std, str, vm_str(vm, "_G").value.str, table, std);

    vm_config_add_extern(vm, &vm_lua_comp_op_std_pow);
    vm_config_add_extern(vm, &vm_std_vm_closure);

#if VM_USE_RAYLIB
    {
        VM_TABLE_SET(std, str, vm_str(vm, "app").value.str, ffi, VM_STD_REF(vm, vm_std_app_init));
        VM_TABLE_SET(std, str, vm_str(vm, "draw").value.str, table, vm_table_new(vm));
    }
#endif

    vm->std = (vm_obj_t){
        .tag = VM_TAG_TAB,
        .value.table = std,
    };
}
