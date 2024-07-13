
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
    WRITE OP(v1.value.b, v2.value.b);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_STR, VM_TAG_STR) {
    WRITE OP_S(v1.value.str->buf, v2.value.str->buf);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_CLOSURE, VM_TAG_CLOSURE) {
    WRITE OP(v1.value.all, v2.value.all);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_TAB, VM_TAG_CLOSURE) {
    WRITE OP(v1.value.all, v2.value.all);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_I8) {
    WRITE OP(v1.value.i8, v2.value.i8);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_I16) {
    WRITE OP(v1.value.i8, v2.value.i16);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_I32) {
    WRITE OP(v1.value.i8, v2.value.i32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_I64) {
    WRITE OP(v1.value.i8, v2.value.i64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_F32) {
    WRITE OP((float) v1.value.i8, v2.value.f32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_F64) {
    WRITE OP((double) v1.value.i8, v2.value.f64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_I8) {
    WRITE OP(v1.value.i16, (int16_t) v2.value.i8);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_I16) {
    WRITE OP(v1.value.i16, v2.value.i16);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_I32) {
    WRITE OP(v1.value.i16, v2.value.i32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_I64) {
    WRITE OP(v1.value.i16, v2.value.i64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_F32) {
    WRITE OP((float) v1.value.i16, v2.value.f32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_F64) {
    WRITE OP((double) v1.value.i16, v2.value.f64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_I8) {
    WRITE OP(v1.value.i32, (int32_t) v2.value.i8);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_I16) {
    WRITE OP(v1.value.i32, (int32_t) v2.value.i16);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_I32) {
    WRITE OP(v1.value.i32, v2.value.i32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_I64) {
    WRITE OP(v1.value.i32, v2.value.i64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_F32) {
    WRITE OP((float) v1.value.i32, v2.value.f32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_F64) {
    WRITE OP((double) v1.value.i32, v2.value.f64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_I8) {
    WRITE OP(v1.value.i64, (int64_t) v2.value.i8);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_I16) {
    WRITE OP(v1.value.i64, (int64_t) v2.value.i16);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_I32) {
    WRITE OP(v1.value.i64, (int64_t) v2.value.i32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_I64) {
    WRITE OP(v1.value.i64, v2.value.i64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_F32) {
    WRITE OP((float) v1.value.i64, v2.value.f32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_F64) {
    WRITE OP((double) v1.value.i64, v2.value.f64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_I8) {
    WRITE OP(v1.value.f32, (float) v2.value.i8);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_I16) {
    WRITE OP(v1.value.f32, (float) v2.value.i16);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_I32) {
    WRITE OP(v1.value.f32, (float) v2.value.i32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_I64) {
    WRITE OP(v1.value.f32, (float) v2.value.i64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_F32) {
    WRITE OP(v1.value.f32, v2.value.f32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_F64) {
    WRITE OP((double) v1.value.f32, v2.value.f64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_I8) {
    WRITE OP(v1.value.f64, (double) v2.value.i8);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_I16) {
    WRITE OP(v1.value.f64, (double) v2.value.i16);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_I32) {
    WRITE OP(v1.value.f64, (double) v2.value.i32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_I64) {
    WRITE OP(v1.value.f64, (double) v2.value.i64);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_F32) {
    WRITE OP(v1.value.f64, (double) v2.value.f32);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_F64) {
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
