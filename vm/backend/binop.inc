
#if !defined(OP)
#error before including test.inc, #define OP
#endif

#if !defined(WRITE)
#error before including test.inc, #define WRITE
#endif

#if !defined(NAME)
#error before including test.inc, #define NAME
#endif

#if !defined(OP_F)
#define OP_F(x, y) OP(x, y)
#endif

#define LABEL(X, Y) case COMBINE(X, Y):;

#define COMBINE(x, y) ((x)*VM_TAG_MAX + (y))

switch (COMBINE(v1.tag, v2.tag)) {
    default: {
        // WRITE (vm_obj_t) {
        //     .tag = VM_TAG_ERROR,
        //     .value.error = vm_error_from_msg(vm_location_range_unknown, "bad atithmatic"),
        // };
        __builtin_trap();
        // goto CONCAT(NAME, END);
    }
    // default: {
    //     __builtin_unreachable();
    // }
LABEL(VM_TAG_TAB, VM_TAG_NIL)
LABEL(VM_TAG_TAB, VM_TAG_BOOL)
LABEL(VM_TAG_TAB, VM_TAG_NUMBER)
LABEL(VM_TAG_TAB, VM_TAG_STR)
LABEL(VM_TAG_TAB, VM_TAG_CLOSURE)
LABEL(VM_TAG_TAB, VM_TAG_FUN)
LABEL(VM_TAG_TAB, VM_TAG_TAB)
LABEL(VM_TAG_TAB, VM_TAG_FFI)
{
    WRITE (vm_obj_t) {
        .tag = VM_TAG_ERROR,
        .value.error = vm_error_from_msg(vm_location_range_unknown, "metatables not implemented yet"),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_NUMBER, VM_TAG_NUMBER) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_NUMBER,
        .value.f64 = OP_F(v1.value.f64, v2.value.f64),
    };
    goto CONCAT(NAME, END);
}
}
CONCAT(NAME, END):;

#undef COMBINE

#undef NAME
#undef WRITE
#undef OP
#undef OP_F
