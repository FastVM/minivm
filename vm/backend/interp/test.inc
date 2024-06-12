
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

#define COMBINE(x, y) ((x)*VM_TAG_MAX + (y))

static void *binop_table[COMBINE(VM_TAG_MAX, VM_TAG_MAX)] = {
    [COMBINE(VM_TAG_I8, VM_TAG_I8)] = &&CONCAT(NAME, I8I8),
    [COMBINE(VM_TAG_I8, VM_TAG_I16)] = &&CONCAT(NAME, I8I16),
    [COMBINE(VM_TAG_I8, VM_TAG_I32)] = &&CONCAT(NAME, I8I32),
    [COMBINE(VM_TAG_I8, VM_TAG_I64)] = &&CONCAT(NAME, I8I64),
    [COMBINE(VM_TAG_I8, VM_TAG_F32)] = &&CONCAT(NAME, I8F32),
    [COMBINE(VM_TAG_I8, VM_TAG_F64)] = &&CONCAT(NAME, I8F64),
    
    [COMBINE(VM_TAG_I16, VM_TAG_I8)] = &&CONCAT(NAME, I16I8),
    [COMBINE(VM_TAG_I16, VM_TAG_I16)] = &&CONCAT(NAME, I16I16),
    [COMBINE(VM_TAG_I16, VM_TAG_I32)] = &&CONCAT(NAME, I16I32),
    [COMBINE(VM_TAG_I16, VM_TAG_I64)] = &&CONCAT(NAME, I16I64),
    [COMBINE(VM_TAG_I16, VM_TAG_F32)] = &&CONCAT(NAME, I16F32),
    [COMBINE(VM_TAG_I16, VM_TAG_F64)] = &&CONCAT(NAME, I16F64),
    
    [COMBINE(VM_TAG_I32, VM_TAG_I8)] = &&CONCAT(NAME, I32I8),
    [COMBINE(VM_TAG_I32, VM_TAG_I16)] = &&CONCAT(NAME, I32I16),
    [COMBINE(VM_TAG_I32, VM_TAG_I32)] = &&CONCAT(NAME, I32I32),
    [COMBINE(VM_TAG_I32, VM_TAG_I64)] = &&CONCAT(NAME, I32I64),
    [COMBINE(VM_TAG_I32, VM_TAG_F32)] = &&CONCAT(NAME, I32F32),
    [COMBINE(VM_TAG_I32, VM_TAG_F64)] = &&CONCAT(NAME, I32F64),
    
    [COMBINE(VM_TAG_I64, VM_TAG_I8)] = &&CONCAT(NAME, I64I8),
    [COMBINE(VM_TAG_I64, VM_TAG_I16)] = &&CONCAT(NAME, I64I16),
    [COMBINE(VM_TAG_I64, VM_TAG_I32)] = &&CONCAT(NAME, I64I32),
    [COMBINE(VM_TAG_I64, VM_TAG_I64)] = &&CONCAT(NAME, I64I64),
    [COMBINE(VM_TAG_I64, VM_TAG_F32)] = &&CONCAT(NAME, I64F32),
    [COMBINE(VM_TAG_I64, VM_TAG_F64)] = &&CONCAT(NAME, I64F64),
    
    [COMBINE(VM_TAG_F32, VM_TAG_I8)] = &&CONCAT(NAME, F32I8),
    [COMBINE(VM_TAG_F32, VM_TAG_I16)] = &&CONCAT(NAME, F32I16),
    [COMBINE(VM_TAG_F32, VM_TAG_I32)] = &&CONCAT(NAME, F32I32),
    [COMBINE(VM_TAG_F32, VM_TAG_I64)] = &&CONCAT(NAME, F32I64),
    [COMBINE(VM_TAG_F32, VM_TAG_F32)] = &&CONCAT(NAME, F32F32),
    [COMBINE(VM_TAG_F32, VM_TAG_F64)] = &&CONCAT(NAME, F32F64),
    
    [COMBINE(VM_TAG_F64, VM_TAG_I8)] = &&CONCAT(NAME, F64I8),
    [COMBINE(VM_TAG_F64, VM_TAG_I16)] = &&CONCAT(NAME, F64I16),
    [COMBINE(VM_TAG_F64, VM_TAG_I32)] = &&CONCAT(NAME, F64I32),
    [COMBINE(VM_TAG_F64, VM_TAG_I64)] = &&CONCAT(NAME, F64I64),
    [COMBINE(VM_TAG_F64, VM_TAG_F32)] = &&CONCAT(NAME, F64F32),
    [COMBINE(VM_TAG_F64, VM_TAG_F64)] = &&CONCAT(NAME, F64F64),

    [COMBINE(VM_TAG_BOOL, VM_TAG_BOOL)] = &&CONCAT(NAME, BB),
    [COMBINE(VM_TAG_STR, VM_TAG_STR)] = &&CONCAT(NAME, SS),
    [COMBINE(VM_TAG_CLOSURE, VM_TAG_CLOSURE)] = &&CONCAT(NAME, PP),
    [COMBINE(VM_TAG_TAB, VM_TAG_TAB)] = &&CONCAT(NAME, PP),
};

goto *binop_table[COMBINE(v1.tag, v2.tag)];
CONCAT(NAME, PP):; {
    WRITE OP(v1.value.all, v2.value.all);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, BB):; {
    WRITE OP(v1.value.b, v2.value.b);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, SS):; {
    WRITE OP_S(v1.value.str, v2.value.str);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I8I8):; {
    WRITE OP(v1.value.i8, v2.value.i8);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I8I16):; {
    WRITE OP(v1.value.i8, v2.value.i16);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I8I32):; {
    WRITE OP(v1.value.i8, v2.value.i32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I8I64):; {
    WRITE OP(v1.value.i8, v2.value.i64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I8F32):; {
    WRITE OP((float) v1.value.i8, v2.value.f32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I8F64):; {
    WRITE OP((double) v1.value.i8, v2.value.f64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I16I8):; {
    WRITE OP(v1.value.i16, (int16_t) v2.value.i8);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I16I16):; {
    WRITE OP(v1.value.i16, v2.value.i16);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I16I32):; {
    WRITE OP(v1.value.i16, v2.value.i32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I16I64):; {
    WRITE OP(v1.value.i16, v2.value.i64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I16F32):; {
    WRITE OP((float) v1.value.i16, v2.value.f32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I16F64):; {
    WRITE OP((double) v1.value.i16, v2.value.f64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I32I8):; {
    WRITE OP(v1.value.i32, (int32_t) v2.value.i8);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I32I16):; {
    WRITE OP(v1.value.i32, (int32_t) v2.value.i16);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I32I32):; {
    WRITE OP(v1.value.i32, v2.value.i32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I32I64):; {
    WRITE OP(v1.value.i32, v2.value.i64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I32F32):; {
    WRITE OP((float) v1.value.i32, v2.value.f32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I32F64):; {
    WRITE OP((double) v1.value.i32, v2.value.f64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I64I8):; {
    WRITE OP(v1.value.i64, (int64_t) v2.value.i8);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I64I16):; {
    WRITE OP(v1.value.i64, (int64_t) v2.value.i16);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I64I32):; {
    WRITE OP(v1.value.i64, (int64_t) v2.value.i32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I64I64):; {
    WRITE OP(v1.value.i64, v2.value.i64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I64F32):; {
    WRITE OP((float) v1.value.i64, v2.value.f32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, I64F64):; {
    WRITE OP((double) v1.value.i64, v2.value.f64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F32I8):; {
    WRITE OP(v1.value.f32, (float) v2.value.i8);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F32I16):; {
    WRITE OP(v1.value.f32, (float) v2.value.i16);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F32I32):; {
    WRITE OP(v1.value.f32, (float) v2.value.i32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F32I64):; {
    WRITE OP(v1.value.f32, (float) v2.value.i64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F32F32):; {
    WRITE OP(v1.value.f32, v2.value.f32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F32F64):; {
    WRITE OP((double) v1.value.f32, v2.value.f64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F64I8):; {
    WRITE OP(v1.value.f64, (double) v2.value.i8);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F64I16):; {
    WRITE OP(v1.value.f64, (double) v2.value.i16);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F64I32):; {
    WRITE OP(v1.value.f64, (double) v2.value.i32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F64I64):; {
    WRITE OP(v1.value.f64, (double) v2.value.i64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F64F32):; {
    WRITE OP(v1.value.f64, (double) v2.value.f32);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, F64F64):; {
    WRITE OP(v1.value.f64, v2.value.f64);
    goto CONCAT(NAME, END);
}
CONCAT(NAME, END):;

#undef COMBINE

#undef NAME
#undef WRITE
#undef OP
#undef OP_S
