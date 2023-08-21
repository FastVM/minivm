
#include "./std.h"

bool vm_std_parse_args(vm_std_value_t *args, const char *fmt, ...) {
    size_t head = 0;
    va_list ap;
    va_start(ap, fmt);
    while (*fmt != '\0') {
        switch (*fmt++) {
        case 's': {
            if (args[head].tag != VM_TAG_STR) {
                return false;
            }
            *va_arg(ap, const char **) = args[head++].value.str;
            break;
        }
        case 'i': {
            if (args[head].tag != VM_TAG_I64) {
                return false;
            }
            *va_arg(ap, ptrdiff_t *) = args[head++].value.i64;
            break;
        }
        case 'f': {
            if (args[head].tag != VM_TAG_F64) {
                return false;
            }
            *va_arg(ap, double *) = args[head++].value.f64;
            break;
        }
        case 't': {
            if (args[head].tag != VM_TAG_TABLE) {
                return false;
            }
            *va_arg(ap, vm_table_t **) = args[head++].value.table;
            break;
        }
        case 'a': {
            if (args[head].tag != VM_TAG_TABLE) {
                return false;
            }
            *va_arg(ap, void **) = args[head++].value.all;
            break;
        }
        default: {
            return false;
        }
        }
    }
    va_end(ap);
    return true;
}

vm_std_value_t vm_std_new_write(vm_std_value_t *args) {
    double num = 0;
    if (vm_std_parse_args(args, "f", &num)) {
        printf("%c", (int) num);
        return (vm_std_value_t) {
            .tag = VM_TAG_NIL,
        };
    }
    ptrdiff_t inum = 0;
    if (vm_std_parse_args(args, "i", &inum)) {
        printf("%c", (int) inum);
        return (vm_std_value_t) {
            .tag = VM_TAG_NIL,
        };
    }
    printf("%zu\n", (size_t) args[0].tag);
    return (vm_std_value_t) {
        .tag = VM_TAG_NIL,
    };
}

vm_table_t *vm_std_new(void) {
    vm_table_t *ret = vm_table_new();
    vm_table_set(
        ret,
        (vm_value_t) {
            .str = "write",
        },
        (vm_value_t) {
            .all = &vm_std_new_write,
        },
        VM_TAG_STR,
        VM_TAG_FFI
    );
    return ret;
}
