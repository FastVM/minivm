#if 0
#include "../codegen/emitter.h"
#include "../tb_internal.h"

// used to add patches since there's separate arrays per thread
static thread_local size_t s_local_thread_id;

typedef struct WasmVal {
    int users_left;  // each time we use it, we decrement. it starts with all the direct users
    int local_slot;  // -1 when invalid
} WasmVal;

typedef struct WasmCtx {
    TB_CGEmitter emit;

    TB_Function* f;
    WasmVal* vals;
} WasmCtx;

static size_t wasm_emit_call_patches(TB_Module* restrict m) {
    return 0;
}

static void wasm_get_data_type_size(TB_DataType dt, TB_CharUnits* out_size, TB_CharUnits* out_align) {
    switch (dt.type) {
        case TB_INT: {
            // round up bits to a byte
            bool is_big_int = dt.data > 64;
            int bits = is_big_int ? ((dt.data + 7) / 8) : tb_next_pow2(dt.data);

            *out_size  = ((bits+7) / 8) << dt.width;
            *out_align = is_big_int ? 8 : bits/8;
            break;
        }
        case TB_FLOAT: {
            int s = 0;
            if (dt.data == TB_FLT_32) s = 4;
            else if (dt.data == TB_FLT_64) s = 8;
            else tb_unreachable();

            *out_size = s << dt.width;
            *out_align = s;
            break;
        }
        case TB_PTR: {
            *out_size = 8;
            *out_align = 8;
            break;
        }
        default: tb_unreachable();
    }
}

size_t wasm_emit_epilogue(uint8_t* out, uint64_t saved, uint64_t stack_usage) {
    return 0;
}

size_t wasm_emit_prologue(uint8_t* out, uint64_t saved, uint64_t stack_usage) {
    return 0;
}

// We do a little stackify system, every time we use a value that
// has multiple users we allocate a local slot for it and use that.
void push_val(WasmCtx* restrict ctx, TB_Reg r) {

}

TB_FunctionOutput wasm_fast_compile_function(TB_Function* restrict f, const TB_FeatureSet* features, uint8_t* out, size_t out_capacity, size_t local_thread_id) {
    s_local_thread_id = local_thread_id;
    WasmCtx ctx = { .vals = (WasmVal*) use_count };

    // Evaluate basic blocks
    TB_FOR_BASIC_BLOCK(bb, f) {
        EMIT1(&ctx.emit, 0x02);

        TB_Reg terminator = f->bbs[bb].end;
        TB_FOR_NODE(r, f, bb) {
            if (r == terminator) break;

            TB_Node* restrict n = &f->nodes[r];
            TB_NodeTypeEnum reg_type = n->type;
            // TB_DataType dt = n->dt;

            switch (reg_type) {
                default: tb_todo();
            }
        }

        // Evaluate terminator
        TB_Reg bb_end = f->bbs[bb].end;
        TB_Node* end = &f->nodes[bb_end];
        if (end->type == TB_TRAP) {
            EMIT1(&ctx.emit, 0x00);
        } else if (end->type == TB_NULL) {
            // empty basic block
        } else if (end->type == TB_UNREACHABLE) {
            /* lmao you thought we'd help you */
        } else if (end->type == TB_RET) {
            // Evaluate return value
            if (end->ret.value) {
                push_val(&ctx, end->ret.value);
            }

            EMIT1(&ctx.emit, 0x0F); // return
        } else {
            tb_todo();
        }

        EMIT1(&ctx.emit, 0x0B);
    }

    TB_FunctionOutput func_out = {
        .linkage = f->linkage,
        .code = ctx.emit.data,
        .code_size = ctx.emit.count,
    };
    return func_out;
}

#if _MSC_VER
_Pragma("warning (push)") _Pragma("warning (disable: 4028)")
#endif
ICodeGen tb__wasm32_codegen = {
    .minimum_addressable_size = 8,
    .pointer_size = 32,

    .get_data_type_size  = wasm_get_data_type_size,
    .emit_call_patches   = wasm_emit_call_patches,
    .emit_prologue       = wasm_emit_prologue,
    .emit_epilogue       = wasm_emit_epilogue,

    .fast_path = wasm_fast_compile_function,
    //.complex_path = x64_complex_compile_function
};
#if _MSC_VER
_Pragma("warning (pop)")
#endif
#endif
