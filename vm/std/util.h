#if !defined(VM_HEADER_STD_UTIL)
#define VM_HEADER_STD_UTIL

#include "./std.h"

#define VM_STD_SET_TAB(table, key, value) vm_table_set( \
    (table),                                            \
    (vm_value_t){.str = (key)},                         \
    (vm_value_t){.all = (void*)(value)},                       \
    VM_TAG_STR,                                         \
    VM_TAG_TAB)
#define VM_STD_SET_FFI(table, key, value) vm_table_set( \
    (table),                                            \
    (vm_value_t){.str = (key)},                         \
    (vm_value_t){.all = (void*)(value)},                       \
    VM_TAG_STR,                                         \
    VM_TAG_FFI)

static inline bool vm_std_parse_args(vm_std_value_t *args, const char *fmt, ...) {
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
                *va_arg(ap, int64_t *) = args[head++].value.i64;
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
                if (args[head].tag != VM_TAG_TAB) {
                    return false;
                }
                *va_arg(ap, vm_table_t **) = args[head++].value.table;
                break;
            }
            case 'a': {
                if (args[head].tag != VM_TAG_TAB) {
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

#endif
