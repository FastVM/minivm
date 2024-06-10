
#if !defined(OP)
#error before including binop.inc, #define OP
#endif

#if !defined(OP_F)
#define OP_F(x, y) OP(x, y)
#endif

switch (v1.tag) {
    case VM_TAG_I8: {
        switch (v2.tag) {
            case VM_TAG_I8: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I8,
                    .value.i8 = OP(v1.value.i8, v2.value.i8),
                };
                break;
            }
            case VM_TAG_I16: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I16,
                    .value.i16 = (int16_t) OP(v1.value.i8, v2.value.i16),
                };
                break;
            }
            case VM_TAG_I32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I32,
                    .value.i32 = (int32_t) OP(v1.value.i8, v2.value.i32),
                };
                break;
            }
            case VM_TAG_I64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I64,
                    .value.i64 = (int64_t) OP(v1.value.i8, v2.value.i64),
                };
                break;
            }
            case VM_TAG_F32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F((float) v1.value.i8, v2.value.f32),
                };
                break;
            }
            case VM_TAG_F64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f64 = OP_F((double) v1.value.i8, v2.value.f64),
                };
                break;
            }
            default: {
                return (vm_std_value_t) {
                    .tag = VM_TAG_ERROR,
                    .value.str = "can only do math with numbers",
                };
            }
        }
        break;
    }
    case VM_TAG_I16: {
        switch (v2.tag) {
            case VM_TAG_I8: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I16,
                    .value.i16 = OP(v1.value.i16, (int16_t) v2.value.i8),
                };
                break;
            }
            case VM_TAG_I16: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I16,
                    .value.i16 = OP(v1.value.i16, v2.value.i16),
                };
                break;
            }
            case VM_TAG_I32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I32,
                    .value.i32 = (int32_t) OP(v1.value.i16, v2.value.i32),
                };
                break;
            }
            case VM_TAG_I64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I64,
                    .value.i64 = (int64_t) OP(v1.value.i16, v2.value.i64),
                };
                break;
            }
            case VM_TAG_F32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F((float) v1.value.i16, v2.value.f32),
                };
                break;
            }
            case VM_TAG_F64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f64 = OP_F((double) v1.value.i16, v2.value.f64),
                };
                break;
            }
            default: {
                return (vm_std_value_t) {
                    .tag = VM_TAG_ERROR,
                    .value.str = "can only do math with numbers",
                };
            }
        }
        break;
    }
    case VM_TAG_I32: {
        switch (v2.tag) {
            case VM_TAG_I8: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I32,
                    .value.i16 = OP(v1.value.i32, (int32_t) v2.value.i8),
                };
                break;
            }
            case VM_TAG_I16: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I32,
                    .value.i16 = OP(v1.value.i32, (int32_t) v2.value.i16),
                };
                break;
            }
            case VM_TAG_I32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I32,
                    .value.i32 = OP(v1.value.i32, v2.value.i32),
                };
                break;
            }
            case VM_TAG_I64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I64,
                    .value.i64 = (int64_t) OP(v1.value.i32, v2.value.i64),
                };
                break;
            }
            case VM_TAG_F32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F((float) v1.value.i32, v2.value.f32),
                };
                break;
            }
            case VM_TAG_F64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f64 = OP_F((double) v1.value.i32, v2.value.f64),
                };
                break;
            }
            default: {
                return (vm_std_value_t) {
                    .tag = VM_TAG_ERROR,
                    .value.str = "can only do math with numbers",
                };
            }
        }
        break;
    }
    case VM_TAG_I64: {
        switch (v2.tag) {
            case VM_TAG_I8: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I64,
                    .value.i16 = OP(v1.value.i64, (int64_t) v2.value.i8),
                };
                break;
            }
            case VM_TAG_I16: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I64,
                    .value.i16 = OP(v1.value.i64, (int64_t) v2.value.i16),
                };
                break;
            }
            case VM_TAG_I32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I64,
                    .value.i32 = OP(v1.value.i64, (int64_t) v2.value.i32),
                };
                break;
            }
            case VM_TAG_I64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_I64,
                    .value.i64 = OP(v1.value.i64, v2.value.i64),
                };
                break;
            }
            case VM_TAG_F32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F((float) v1.value.i64, v2.value.f32),
                };
                break;
            }
            case VM_TAG_F64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f64 = OP_F((double) v1.value.i64, v2.value.f64),
                };
                break;
            }
            default: {
                return (vm_std_value_t) {
                    .tag = VM_TAG_ERROR,
                    .value.str = "can only do math with numbers",
                };
            }
        }
        break;
    }
    case VM_TAG_F32: {
        switch (v2.tag) {
            case VM_TAG_I8: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F(v1.value.f32, (float) v2.value.i8),
                };
                break;
            }
            case VM_TAG_I16: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F(v1.value.f32, (float) v2.value.i16),
                };
                break;
            }
            case VM_TAG_I32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F(v1.value.f32, (float) v2.value.i32),
                };
                break;
            }
            case VM_TAG_I64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F(v1.value.f32, (float) v2.value.i64),
                };
                break;
            }
            case VM_TAG_F32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F32,
                    .value.f32 = OP_F(v1.value.f32, v2.value.f32),
                };
                break;
            }
            case VM_TAG_F64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f64 = OP_F((double) v1.value.f32, v2.value.f64),
                };
                break;
            }
            default: {
                return (vm_std_value_t) {
                    .tag = VM_TAG_ERROR,
                    .value.str = "can only do math with numbers",
                };
            }
        }
        break;
    }
    case VM_TAG_F64: {
        switch (v2.tag) {
            case VM_TAG_I8: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f32 = OP_F(v1.value.f64, (double) v2.value.i8),
                };
                break;
            }
            case VM_TAG_I16: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f32 = OP_F(v1.value.f64, (double) v2.value.i16),
                };
                break;
            }
            case VM_TAG_I32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f32 = OP_F(v1.value.f64, (double) v2.value.i32),
                };
                break;
            }
            case VM_TAG_I64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f32 = OP_F(v1.value.f64, (double) v2.value.i64),
                };
                break;
            }
            case VM_TAG_F32: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f32 = OP_F(v1.value.f64, (double) v2.value.f32),
                };
                break;
            }
            case VM_TAG_F64: {
                out = (vm_std_value_t) {
                    .tag = VM_TAG_F64,
                    .value.f64 = OP_F(v1.value.f64, v2.value.f64),
                };
                break;
            }
            default: {
                return (vm_std_value_t) {
                    .tag = VM_TAG_ERROR,
                    .value.str = "can only do math with numbers",
                };
            }
        }
        break;
    }
    default: {
        return (vm_std_value_t) {
            .tag = VM_TAG_ERROR,
            .value.str = "can only add to a number",
        };
    }
}

#undef OP
#undef OP_F
