
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

if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
    
}
switch (COMBINE(v1.tag, v2.tag)) {
    default: {
        WRITE vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "bad atithmatic"));
        goto CONCAT(NAME, END);
    }
    LABEL(VM_TAG_NUMBER, VM_TAG_NUMBER) {
        WRITE vm_obj_of_number(OP_F(vm_obj_get_number(v1), vm_obj_get_number(v2)));
        goto CONCAT(NAME, END);
    }
}
CONCAT(NAME, END):;

#undef COMBINE

#undef NAME
#undef WRITE
#undef OP
#undef OP_F
