
#include "../util.h"

vm_std_value_t VM_STD_EXPORT vm_std_tag(vm_std_value_t *args) {
    int64_t ival;
    if (vm_std_parse_args(args, "i", &ival)) {
        return (vm_std_value_t){
            .tag = VM_TAG_F64,
            .value.f64 = (double)ival,
        };
    }
    double flt;
    if (vm_std_parse_args(args, "f", &flt)) {
        return (vm_std_value_t){
            .tag = VM_TAG_F64,
            .value.f64 = flt,
        };
    }
    const char *str;
    if (vm_std_parse_args(args, "s", &str)) {
        if (!strcmp(str, "i64")) {
            return (vm_std_value_t){
                .tag = VM_TAG_F64,
                .value.f64 = (double)VM_TAG_I64,
            };
        }
        if (!strcmp(str, "f64")) {
            return (vm_std_value_t){
                .tag = VM_TAG_F64,
                .value.f64 = (double)VM_TAG_F64,
            };
        }
        if (!strcmp(str, "bool")) {
            return (vm_std_value_t){
                .tag = VM_TAG_F64,
                .value.f64 = (double)VM_TAG_BOOL,
            };
        }
        if (!strcmp(str, "str")) {
            return (vm_std_value_t){
                .tag = VM_TAG_F64,
                .value.f64 = (double)VM_TAG_STR,
            };
        }
        if (!strcmp(str, "nil")) {
            return (vm_std_value_t){
                .tag = VM_TAG_F64,
                .value.f64 = (double)VM_TAG_NIL,
            };
        }
        if (!strcmp(str, "fun")) {
            return (vm_std_value_t){
                .tag = VM_TAG_F64,
                .value.f64 = (double)VM_TAG_FUN,
            };
        }
        if (!strcmp(str, "ffi")) {
            return (vm_std_value_t){
                .tag = VM_TAG_F64,
                .value.f64 = (double)VM_TAG_FFI,
            };
        }
        if (!strcmp(str, "tab")) {
            return (vm_std_value_t){
                .tag = VM_TAG_F64,
                .value.f64 = (double)VM_TAG_TAB,
            };
        }
        fprintf(stderr, "error: tag: unknown name: \"%s\"", str);
        exit(1);
    }
    fprintf(stderr, "error: tag: bad argument");
    exit(1);
}
