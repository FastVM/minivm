
#if !defined(OP)
#error before including binop.inc, #define OP
#endif

#if !defined(OP_F)
#define OP_F(x, y) OP(x, y)
#endif

#define COMBINE(x, y) ((x)*VM_TAG_MAX + (y))

static void *binop_table[COMBINE(VM_TAG_MAX, VM_TAG_MAX)] = {
    [COMBINE(VM_TAG_I8, VM_TAG_I8)] = &&I8I8,
    [COMBINE(VM_TAG_I8, VM_TAG_I16)] = &&I8I16,
    [COMBINE(VM_TAG_I8, VM_TAG_I32)] = &&I8I32,
    [COMBINE(VM_TAG_I8, VM_TAG_I64)] = &&I8I64,
    [COMBINE(VM_TAG_I8, VM_TAG_F32)] = &&I8F32,
    [COMBINE(VM_TAG_I8, VM_TAG_F64)] = &&I8F64,
    
    [COMBINE(VM_TAG_I16, VM_TAG_I8)] = &&I16I8,
    [COMBINE(VM_TAG_I16, VM_TAG_I16)] = &&I16I16,
    [COMBINE(VM_TAG_I16, VM_TAG_I32)] = &&I16I32,
    [COMBINE(VM_TAG_I16, VM_TAG_I64)] = &&I16I64,
    [COMBINE(VM_TAG_I16, VM_TAG_F32)] = &&I16F32,
    [COMBINE(VM_TAG_I16, VM_TAG_F64)] = &&I16F64,
    
    [COMBINE(VM_TAG_I32, VM_TAG_I8)] = &&I32I8,
    [COMBINE(VM_TAG_I32, VM_TAG_I16)] = &&I32I16,
    [COMBINE(VM_TAG_I32, VM_TAG_I32)] = &&I32I32,
    [COMBINE(VM_TAG_I32, VM_TAG_I64)] = &&I32I64,
    [COMBINE(VM_TAG_I32, VM_TAG_F32)] = &&I32F32,
    [COMBINE(VM_TAG_I32, VM_TAG_F64)] = &&I32F64,
    
    [COMBINE(VM_TAG_I64, VM_TAG_I8)] = &&I64I8,
    [COMBINE(VM_TAG_I64, VM_TAG_I16)] = &&I64I16,
    [COMBINE(VM_TAG_I64, VM_TAG_I32)] = &&I64I32,
    [COMBINE(VM_TAG_I64, VM_TAG_I64)] = &&I64I64,
    [COMBINE(VM_TAG_I64, VM_TAG_F32)] = &&I64F32,
    [COMBINE(VM_TAG_I64, VM_TAG_F64)] = &&I64F64,
    
    [COMBINE(VM_TAG_F32, VM_TAG_I8)] = &&F32I8,
    [COMBINE(VM_TAG_F32, VM_TAG_I16)] = &&F32I16,
    [COMBINE(VM_TAG_F32, VM_TAG_I32)] = &&F32I32,
    [COMBINE(VM_TAG_F32, VM_TAG_I64)] = &&F32I64,
    [COMBINE(VM_TAG_F32, VM_TAG_F32)] = &&F32F32,
    [COMBINE(VM_TAG_F32, VM_TAG_F64)] = &&F32F64,
    
    [COMBINE(VM_TAG_F64, VM_TAG_I8)] = &&F64I8,
    [COMBINE(VM_TAG_F64, VM_TAG_I16)] = &&F64I16,
    [COMBINE(VM_TAG_F64, VM_TAG_I32)] = &&F64I32,
    [COMBINE(VM_TAG_F64, VM_TAG_I64)] = &&F64I64,
    [COMBINE(VM_TAG_F64, VM_TAG_F32)] = &&F64F32,
    [COMBINE(VM_TAG_F64, VM_TAG_F64)] = &&F64F64,
};

goto *binop_table[COMBINE(v1.tag, v2.tag)];
I8I8:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I8,
        .value.i8 = OP(v1.value.i8, v2.value.i8),
    };
}
I8I16:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I16,
        .value.i16 = (int16_t) OP(v1.value.i8, v2.value.i16),
    };
}
I8I32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I32,
        .value.i32 = (int32_t) OP(v1.value.i8, v2.value.i32),
    };
}
I8I64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I64,
        .value.i64 = (int64_t) OP(v1.value.i8, v2.value.i64),
    };
}
I8F32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F((float) v1.value.i8, v2.value.f32),
    };
}
I8F64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.i8, v2.value.f64),
    };
}
I16I8:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I16,
        .value.i16 = OP(v1.value.i16, (int16_t) v2.value.i8),
    };
}
I16I16:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I16,
        .value.i16 = OP(v1.value.i16, v2.value.i16),
    };
}
I16I32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I32,
        .value.i32 = (int32_t) OP(v1.value.i16, v2.value.i32),
    };
}
I16I64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I64,
        .value.i64 = (int64_t) OP(v1.value.i16, v2.value.i64),
    };
}
I16F32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F((float) v1.value.i16, v2.value.f32),
    };
}
I16F64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.i16, v2.value.f64),
    };
}
I32I8:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I32,
        .value.i16 = OP(v1.value.i32, (int32_t) v2.value.i8),
    };
}
I32I16:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I32,
        .value.i16 = OP(v1.value.i32, (int32_t) v2.value.i16),
    };
}
I32I32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I32,
        .value.i32 = OP(v1.value.i32, v2.value.i32),
    };
}
I32I64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I64,
        .value.i64 = (int64_t) OP(v1.value.i32, v2.value.i64),
    };
}
I32F32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F((float) v1.value.i32, v2.value.f32),
    };
}
I32F64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.i32, v2.value.f64),
    };
}
I64I8:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I64,
        .value.i16 = OP(v1.value.i64, (int64_t) v2.value.i8),
    };
}
I64I16:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I64,
        .value.i16 = OP(v1.value.i64, (int64_t) v2.value.i16),
    };
}
I64I32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I64,
        .value.i32 = OP(v1.value.i64, (int64_t) v2.value.i32),
    };
}
I64I64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_I64,
        .value.i64 = OP(v1.value.i64, v2.value.i64),
    };
}
I64F32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F((float) v1.value.i64, v2.value.f32),
    };
}
I64F64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.i64, v2.value.f64),
    };
}
F32I8:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, (float) v2.value.i8),
    };
}
F32I16:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, (float) v2.value.i16),
    };
}
F32I32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, (float) v2.value.i32),
    };
}
F32I64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, (float) v2.value.i64),
    };
}
F32F32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, v2.value.f32),
    };
}
F32F64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.f32, v2.value.f64),
    };
}
F64I8:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.i8),
    };
}
F64I16:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.i16),
    };
}
F64I32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.i32),
    };
}
F64I64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.i64),
    };
}
F64F32:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.f32),
    };
}
F64F64:; {
    return (vm_std_value_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F(v1.value.f64, v2.value.f64),
    };
}

#undef COMBINE

#undef OP
#undef OP_F
