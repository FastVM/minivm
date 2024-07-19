
#if !defined(OP)
#error before including test.inc, #define OP
#endif

#if !defined(OP_S)
#error before including test.inc, #define OP_S
#endif

#if !defined(WRITE)
#error before including test.inc, #define WRITE
#endif

#if !defined(NAME)
#error before including test.inc, #define NAME
#endif

#define LABEL(X, Y) case COMBINE(X, Y):;

#define COMBINE(x, y) ((x)*VM_TAG_MAX + (y))

switch (COMBINE(v1.tag, v2.tag)) {
LABEL(VM_TAG_BOOL, VM_TAG_BOOL) {
    WRITE OP(vm_obj_get_boolean(v1), vm_obj_get_boolean(v2));
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_STR, VM_TAG_STR) {
    WRITE OP_S(vm_obj_get_string(v1)->buf, vm_obj_get_string(v2)->buf);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_CLOSURE, VM_TAG_CLOSURE) {
    WRITE OP(vm_obj_get_closure(v1), vm_obj_get_closure(v2));
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_TAB, VM_TAG_TAB) {
    WRITE OP(vm_obj_get_table(v1), vm_obj_get_table(v2));
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_NUMBER, VM_TAG_NUMBER) {
    WRITE OP(vm_obj_get_number(v1), vm_obj_get_number(v2));
    goto CONCAT(NAME, END);
}
}
CONCAT(NAME, END):;

#undef LABEL
#undef COMBINE

#undef NAME
#undef WRITE
#undef OP
#undef OP_S
