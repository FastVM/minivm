
#include "./std.h"

#include "./libs/io.h"
#include "./util.h"

void vm_std_os_exit(vm_std_value_t *args) {
    exit((int)vm_value_to_i64(args[0]));
}

void vm_std_assert(vm_std_value_t *args) {
    vm_std_value_t val = args[0];
    if (val.tag == VM_TAG_NIL || (val.tag == VM_TAG_BOOL && !val.value.b)) {
        vm_std_value_t msg = args[1];
        vm_io_print_lit(stderr, msg);
        fprintf(stderr, "\n");
    }
    *args = (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
}

void vm_std_vm_closure(vm_std_value_t *args) {
    int64_t nargs = 0;
    for (size_t i = 0; args[i].tag != 0; i++) {
        nargs += 1;
    }
    if (nargs == 0 || args[0].tag != VM_TAG_FUN) {
        *args = (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
        return;
    }
    vm_std_value_t *vals = vm_malloc(sizeof(vm_std_value_t) * (nargs + 1));
    vals[0] = (vm_std_value_t) {
        .tag = VM_TAG_I32,
        .value.i32 = (int32_t) nargs,
    };
    vals += 1;
    for (size_t i = 0; args[i].tag != 0; i++) {
        vals[i] = args[i];
    }
    *args = (vm_std_value_t){
        .tag = VM_TAG_CLOSURE,
        .value.closure = vals,
    };
    return;
}

void vm_std_vm_print(vm_std_value_t *args) {
    for (size_t i = 0; args[i].tag; i++) {
        vm_io_debug(stdout, 0, "", args[i], NULL);
    }
}

void vm_std_vm_concat(vm_std_value_t *args) {
    size_t len = 1;
    for (size_t i = 0; args[i].tag == VM_TAG_STR; i++) {
        len += strlen(args[i].value.str);
    }
    char *buf = vm_malloc(sizeof(char) * len);
    size_t head = 0;
    for (size_t i = 0; args[i].tag == VM_TAG_STR; i++) {
        size_t len = strlen(args[i].value.str);
        if (len == 0) {
            continue;
        }
        memcpy(&buf[head], args[i].value.str, len);
        head += len;
    }
    buf[len-1] = '\0';
    *args = (vm_std_value_t) {
        .tag = VM_TAG_STR,
        .value.str = buf,
    };
}

void vm_std_type(vm_std_value_t *args) {
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
    *args = (vm_std_value_t) {
        .tag = VM_TAG_STR,
        .value.str = ret,
    };
}

void vm_std_print(vm_std_value_t *args) {
    vm_std_value_t *ret = args;
    FILE *out = stdout;
    bool first = true;
    while (args->tag != 0) {
        if (!first) {
            fprintf(out, "\t");
        }
        vm_std_value_t value = *args++;
        switch (value.tag) {
            case VM_TAG_NIL: {
                fprintf(out, "nil");
                break;
            }
            case VM_TAG_BOOL: {
                fprintf(out, "%s", value.value.b ? "true" : "false");
                break;
            }
            case VM_TAG_I8: {
                fprintf(out, "%" PRIi8, value.value.i8);
                break;
            }
            case VM_TAG_I16: {
                fprintf(out, "%" PRIi16, value.value.i16);
                break;
            }
            case VM_TAG_I32: {
                fprintf(out, "%" PRIi32, value.value.i32);
                break;
            }
            case VM_TAG_I64: {
                fprintf(out, "%" PRIi64, value.value.i64);
                break;
            }
            case VM_TAG_F32: {
                fprintf(out, "%f", value.value.f32);
                break;
            }
            case VM_TAG_F64: {
                fprintf(out, "%f", value.value.f64);
                break;
            }
            case VM_TAG_STR: {
                fprintf(out, "%s", value.value.str);
                break;
            }
            case VM_TAG_CLOSURE: {
                fprintf(out, "<function: %p>", value.value.closure);
                break;
            }
            case VM_TAG_FUN: {
                fprintf(out, "<code: %p>", value.value.all);
                break;
            }
            case VM_TAG_TAB: {
                fprintf(out, "<table: %p>", value.value.table);
                break;
            }
            case VM_TAG_FFI: {
                fprintf(out, "<function: %p>", value.value.all);
                break;
            }
        }
        first = false;
    }
    fprintf(out, "\n");
    *ret = (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
}

vm_table_t *vm_std_new(void) {
    vm_table_t *std = vm_table_new();

    {
        vm_table_t *io = vm_table_new();
        VM_STD_SET_TAB(std, "io", io);
    }

    {
        vm_table_t *vm = vm_table_new();
        VM_STD_SET_FFI(vm, "print", &vm_std_vm_print);
        VM_STD_SET_TAB(std, "vm", vm);
    }

    {
        vm_table_t *os = vm_table_new();
        VM_STD_SET_FFI(os, "exit", &vm_std_os_exit);
        VM_STD_SET_TAB(std, "os", os);
    }

    VM_STD_SET_FFI(std, "type", &vm_std_type);
    VM_STD_SET_FFI(std, "print", &vm_std_print);
    VM_STD_SET_FFI(std, "assert", &vm_std_assert);

    return std;
}
