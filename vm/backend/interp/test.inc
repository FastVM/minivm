
#define VM_MATH_SWITCH 1

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

#if VM_MATH_SWITCH
#define LABEL(X, Y) case COMBINE(X, Y):;
#else
#define LABEL(X, Y) CONCAT3(NAME, X, Y) :;
#endif

#define COMBINE(x, y) ((x)*VM_TAG_MAX + (y))

#if !VM_MATH_SWITCH
static void *binop_table[COMBINE(VM_TAG_MAX, VM_TAG_MAX)] = {
    [COMBINE(VM_TAG_I8, VM_TAG_I8)] = &&CONCAT3(NAME, VM_TAG_I8, VM_TAG_I8),
    [COMBINE(VM_TAG_I8, VM_TAG_I16)] = &&CONCAT3(NAME, VM_TAG_I8, VM_TAG_I16),
    [COMBINE(VM_TAG_I8, VM_TAG_I32)] = &&CONCAT3(NAME, VM_TAG_I8, VM_TAG_I32),
    [COMBINE(VM_TAG_I8, VM_TAG_I64)] = &&CONCAT3(NAME, VM_TAG_I8, VM_TAG_I64),
    [COMBINE(VM_TAG_I8, VM_TAG_F32)] = &&CONCAT3(NAME, VM_TAG_I8, VM_TAG_F32),
    [COMBINE(VM_TAG_I8, VM_TAG_F64)] = &&CONCAT3(NAME, VM_TAG_I8, VM_TAG_F64),
    
    [COMBINE(VM_TAG_I16, VM_TAG_I8)] = &&CONCAT3(NAME, VM_TAG_I16, VM_TAG_I8),
    [COMBINE(VM_TAG_I16, VM_TAG_I16)] = &&CONCAT3(NAME, VM_TAG_I16, VM_TAG_I16),
    [COMBINE(VM_TAG_I16, VM_TAG_I32)] = &&CONCAT3(NAME, VM_TAG_I16, VM_TAG_I32),
    [COMBINE(VM_TAG_I16, VM_TAG_I64)] = &&CONCAT3(NAME, VM_TAG_I16, VM_TAG_I64),
    [COMBINE(VM_TAG_I16, VM_TAG_F32)] = &&CONCAT3(NAME, VM_TAG_I16, VM_TAG_F32),
    [COMBINE(VM_TAG_I16, VM_TAG_F64)] = &&CONCAT3(NAME, VM_TAG_I16, VM_TAG_F64),
    
    [COMBINE(VM_TAG_I32, VM_TAG_I8)] = &&CONCAT3(NAME, VM_TAG_I32, VM_TAG_I8),
    [COMBINE(VM_TAG_I32, VM_TAG_I16)] = &&CONCAT3(NAME, VM_TAG_I32, VM_TAG_I16),
    [COMBINE(VM_TAG_I32, VM_TAG_I32)] = &&CONCAT3(NAME, VM_TAG_I32, VM_TAG_I32),
    [COMBINE(VM_TAG_I32, VM_TAG_I64)] = &&CONCAT3(NAME, VM_TAG_I32, VM_TAG_I64),
    [COMBINE(VM_TAG_I32, VM_TAG_F32)] = &&CONCAT3(NAME, VM_TAG_I32, VM_TAG_F32),
    [COMBINE(VM_TAG_I32, VM_TAG_F64)] = &&CONCAT3(NAME, VM_TAG_I32, VM_TAG_F64),
    
    [COMBINE(VM_TAG_I64, VM_TAG_I8)] = &&CONCAT3(NAME, VM_TAG_I64, VM_TAG_I8),
    [COMBINE(VM_TAG_I64, VM_TAG_I16)] = &&CONCAT3(NAME, VM_TAG_I64, VM_TAG_I16),
    [COMBINE(VM_TAG_I64, VM_TAG_I32)] = &&CONCAT3(NAME, VM_TAG_I64, VM_TAG_I32),
    [COMBINE(VM_TAG_I64, VM_TAG_I64)] = &&CONCAT3(NAME, VM_TAG_I64, VM_TAG_I64),
    [COMBINE(VM_TAG_I64, VM_TAG_F32)] = &&CONCAT3(NAME, VM_TAG_I64, VM_TAG_F32),
    [COMBINE(VM_TAG_I64, VM_TAG_F64)] = &&CONCAT3(NAME, VM_TAG_I64, VM_TAG_F64),
    
    [COMBINE(VM_TAG_F32, VM_TAG_I8)] = &&CONCAT3(NAME, VM_TAG_F32, VM_TAG_I8),
    [COMBINE(VM_TAG_F32, VM_TAG_I16)] = &&CONCAT3(NAME, VM_TAG_F32, VM_TAG_I16),
    [COMBINE(VM_TAG_F32, VM_TAG_I32)] = &&CONCAT3(NAME, VM_TAG_F32, VM_TAG_I32),
    [COMBINE(VM_TAG_F32, VM_TAG_I64)] = &&CONCAT3(NAME, VM_TAG_F32, VM_TAG_I64),
    [COMBINE(VM_TAG_F32, VM_TAG_F32)] = &&CONCAT3(NAME, VM_TAG_F32, VM_TAG_F32),
    [COMBINE(VM_TAG_F32, VM_TAG_F64)] = &&CONCAT3(NAME, VM_TAG_F32, VM_TAG_F64),
    
    [COMBINE(VM_TAG_F64, VM_TAG_I8)] = &&CONCAT3(NAME, VM_TAG_F64, VM_TAG_I8),
    [COMBINE(VM_TAG_F64, VM_TAG_I16)] = &&CONCAT3(NAME, VM_TAG_F64, VM_TAG_I16),
    [COMBINE(VM_TAG_F64, VM_TAG_I32)] = &&CONCAT3(NAME, VM_TAG_F64, VM_TAG_I32),
    [COMBINE(VM_TAG_F64, VM_TAG_I64)] = &&CONCAT3(NAME, VM_TAG_F64, VM_TAG_I64),
    [COMBINE(VM_TAG_F64, VM_TAG_F32)] = &&CONCAT3(NAME, VM_TAG_F64, VM_TAG_F32),
    [COMBINE(VM_TAG_F64, VM_TAG_F64)] = &&CONCAT3(NAME, VM_TAG_F64, VM_TAG_F64),

    [COMBINE(VM_TAG_BOOL, VM_TAG_BOOL)] = &&CONCAT3(NAME, VM_TAG_BOOL, VM_TAG_BOOL),
    [COMBINE(VM_TAG_STR, VM_TAG_STR)] = &&CONCAT3(NAME, VM_TAG_STR, VM_TAG_STR),
    [COMBINE(VM_TAG_CLOSURE, VM_TAG_CLOSURE)] = &&CONCAT3(NAME, VM_TAG_STR, VM_TAG_STR),
    [COMBINE(VM_TAG_TAB, VM_TAG_TAB)] = &&CONCAT3(NAME, VM_TAG_PTR, VM_TAG_PTR),
};
#endif

#if VM_MATH_SWITCH
switch (COMBINE(v1.tag, v2.tag)) {
#else
goto *binop_table[COMBINE(v1.tag, v2.tag)];
#endif
LABEL(VM_TAG_BOOL, VM_TAG_BOOL) {
    WRITE OP(v1.value.all, v2.value.all);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_STR, VM_TAG_STR) {
    WRITE OP_S(v1.value.str, v2.value.str);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_CLOSURE, VM_TAG_CLOSURE) {
    WRITE OP(v1.value.str, v2.value.str);
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_TAB, VM_TAG_CLOSURE) {
    WRITE OP(v1.value.str, v2.value.str);
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
#if VM_MATH_SWITCH
}
#endif
CONCAT(NAME, END):;

#undef LABEL
#undef COMBINE

#undef NAME
#undef WRITE
#undef OP
#undef OP_S
