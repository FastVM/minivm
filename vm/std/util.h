#if !defined(VM_HEADER_STD_UTIL)
#define VM_HEADER_STD_UTIL

#include "./std.h"

static inline bool vm_std_parse_args(vm_std_value_t *args, const char *fmt, ...) {
    size_t head = 0;
    va_list ap;
    va_start(ap, fmt);
    while (*fmt != '\0') {
        switch (*fmt++) {
            case 's': {
                if (vm_type_eq(args[head].tag, VM_TYPE_STR)) {
                    return false;
                }
                *va_arg(ap, const char **) = args[head++].value.str;
                break;
            }
            // case 'i': {
            //     if (vm_type_eq(args[head].tag, VM_TYPE_I64)) {
            //         return false;
            //     }
            //     *va_arg(ap, int64_t *) = args[head++].value.i64;
            //     break;
            // }
            case 'f': {
                if (vm_type_eq(args[head].tag, VM_TYPE_F64)) {
                    return false;
                }
                *va_arg(ap, double *) = args[head++].value.f64;
                break;
            }
            case 't': {
                if (vm_type_eq(args[head].tag, VM_TYPE_TAB)) {
                    return false;
                }
                *va_arg(ap, vm_table_t **) = args[head++].value.table;
                break;
            }
            case 'a': {
                if (vm_type_eq(args[head].tag, VM_TYPE_TAB)) {
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

#define VM_STD_FUNC_DEF(func_) \
    void func_ ## _impl(vm_std_closure_t *closure, vm_std_value_t *args); \
    const vm_ffi_func_t func_ = (vm_ffi_func_t) {.name = #func_ "_impl", .func = &func_ ## _impl}; \
    void func_ ## _impl(vm_std_closure_t *closure, vm_std_value_t *args)

#endif
