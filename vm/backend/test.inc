
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
    WRITE OP(v1.value.boolean, v2.value.boolean);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_STR, VM_TAG_STR) {
    WRITE OP_S(v1.value.str->buf, v2.value.str->buf);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_CLOSURE, VM_TAG_CLOSURE) {
    WRITE OP(v1.value.closure, v2.value.closure);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_TAB, VM_TAG_TAB) {
    WRITE OP(v1.value.table, v2.value.table);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_NUMBER, VM_TAG_NUMBER) {
    WRITE OP(v1.value.f64, v2.value.f64);
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
