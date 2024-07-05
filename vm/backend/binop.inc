
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
    WRITE (vm_obj_t) {
        .tag = VM_TAG_ERROR,
        .value.error = vm_error_from_msg(vm_location_range_unknown, "bad atithmatic"),
    };
    goto CONCAT(NAME, END);
    }
LABEL(VM_TAG_TAB, VM_TAG_NIL)
LABEL(VM_TAG_TAB, VM_TAG_BOOL)
LABEL(VM_TAG_TAB, VM_TAG_I8)
LABEL(VM_TAG_TAB, VM_TAG_I16)
LABEL(VM_TAG_TAB, VM_TAG_I32)
LABEL(VM_TAG_TAB, VM_TAG_I64)
LABEL(VM_TAG_TAB, VM_TAG_F32)
LABEL(VM_TAG_TAB, VM_TAG_F64)
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
LABEL(VM_TAG_I8, VM_TAG_I8) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I8,
        .value.i8 = OP(v1.value.i8, v2.value.i8),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_I16) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I16,
        .value.i16 = (int16_t) OP(v1.value.i8, v2.value.i16),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_I32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I32,
        .value.i32 = (int32_t) OP(v1.value.i8, v2.value.i32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_I64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I64,
        .value.i64 = (int64_t) OP(v1.value.i8, v2.value.i64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_F32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F((float) v1.value.i8, v2.value.f32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I8, VM_TAG_F64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.i8, v2.value.f64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_I8) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I16,
        .value.i16 = OP(v1.value.i16, (int16_t) v2.value.i8),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_I16) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I16,
        .value.i16 = OP(v1.value.i16, v2.value.i16),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_I32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I32,
        .value.i32 = (int32_t) OP(v1.value.i16, v2.value.i32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_I64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I64,
        .value.i64 = (int64_t) OP(v1.value.i16, v2.value.i64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_F32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F((float) v1.value.i16, v2.value.f32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I16, VM_TAG_F64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.i16, v2.value.f64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_I8) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I32,
        .value.i16 = OP(v1.value.i32, (int32_t) v2.value.i8),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_I16) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I32,
        .value.i16 = OP(v1.value.i32, (int32_t) v2.value.i16),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_I32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I32,
        .value.i32 = OP(v1.value.i32, v2.value.i32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_I64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I64,
        .value.i64 = (int64_t) OP(v1.value.i32, v2.value.i64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_F32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F((float) v1.value.i32, v2.value.f32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I32, VM_TAG_F64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.i32, v2.value.f64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_I8) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I64,
        .value.i16 = OP(v1.value.i64, (int64_t) v2.value.i8),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_I16) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I64,
        .value.i16 = OP(v1.value.i64, (int64_t) v2.value.i16),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_I32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I64,
        .value.i32 = OP(v1.value.i64, (int64_t) v2.value.i32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_I64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_I64,
        .value.i64 = OP(v1.value.i64, v2.value.i64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_F32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F((float) v1.value.i64, v2.value.f32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_I64, VM_TAG_F64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.i64, v2.value.f64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_I8) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, (float) v2.value.i8),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_I16) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, (float) v2.value.i16),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_I32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, (float) v2.value.i32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_I64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, (float) v2.value.i64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_F32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F32,
        .value.f32 = OP_F(v1.value.f32, v2.value.f32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F32, VM_TAG_F64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f64 = OP_F((double) v1.value.f32, v2.value.f64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_I8) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.i8),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_I16) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.i16),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_I32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.i32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_I64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.i64),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_F32) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
        .value.f32 = OP_F(v1.value.f64, (double) v2.value.f32),
    };
    goto CONCAT(NAME, END);
}
LABEL(VM_TAG_F64, VM_TAG_F64) {
    WRITE (vm_obj_t) {
        .tag = VM_TAG_F64,
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
