#include "coff.h"

// uleb128 encode
static void emit_uint(TB_Emitter* emit, uint64_t x) {
    do {
        uint32_t lo = x & 0x7F;
        x >>= 7;
        if (x) {
            lo |= 0x80;
        }
        tb_out1b(emit, lo);
    } while (x);
}

static uint8_t* emit_uint2(uint8_t* p, uint64_t x) {
    do {
        uint32_t lo = x & 0x7F;
        x >>= 7;
        if (x) {
            lo |= 0x80;
        }
        *p++ = lo;
    } while (x);
    return p;
}

static int len_uint(uint64_t x) {
    int c = 0;
    do {
        uint32_t lo = x & 0x7F;
        x >>= 7;
        if (x) {
            lo |= 0x80;
        }
        c += 1;
    } while (x);
    return c;
}

static uint8_t get_wasm_type(TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data <= 8)  return 0x7F;
            if (dt.data <= 16) return 0x7F;
            if (dt.data <= 32) return 0x7F;
            if (dt.data <= 64) return 0x7E;
            break;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) return 0x7D;
            if (dt.data == TB_FLT_64) return 0x7C;
            break;
        }
        case TB_PTR: return 0x7F;
    }

    assert(0 && "TODO");
    return 0;
}

static TB_ExportChunk* emitter_to_chunk(TB_Arena* dst_arena, TB_Emitter* e) {
    TB_ExportChunk* stuff = tb_export_make_chunk(dst_arena, e->count);
    memcpy(stuff->data, e->data, e->count);
    tb_platform_heap_free(e->data);
    return stuff;
}

TB_ExportBuffer tb_wasm_write_output(TB_Module* m, TB_Arena* dst_arena, const IDebugFormat* dbg) {
    TB_Arena* arena = get_temporary_arena(m);

    ExportList exports;
    CUIK_TIMED_BLOCK("layout section") {
        exports = tb_module_layout_sections(m);
    }

    // Construct type section
    size_t func_type_count = 0;
    TB_Emitter func_types = { 0 };

    DynArray(TB_ModuleSection) sections = m->sections;
    dyn_array_for(i, sections) {
        DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
        dyn_array_for(j, funcs) {
            funcs[j]->wasm_type = func_type_count++;
            TB_FunctionPrototype* proto = funcs[j]->parent->prototype;

            tb_out1b(&func_types, 0x60);
            emit_uint(&func_types, proto->param_count);
            FOREACH_N(k, 0, proto->param_count) {
                tb_out1b(&func_types, get_wasm_type(proto->params[k].dt));
            }
            emit_uint(&func_types, proto->return_count);
            FOREACH_N(k, 0, proto->return_count) {
                tb_out1b(&func_types, get_wasm_type(proto->params[proto->param_count + k].dt));
            }
        }
    }

    size_t func_section_size = 0;
    size_t func_count = 0;
    dyn_array_for(i, sections) {
        DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
        dyn_array_for(j, funcs) {
            func_section_size += len_uint(funcs[j]->wasm_type);
            func_count += 1;
        }
    }
    func_section_size += len_uint(func_count);

    // Module header
    TB_Emitter emit = { 0 };
    tb_out4b(&emit, 0x6D736100); // MAGIC   \0asm
    tb_out4b(&emit, 0x1);        // VERSION 1

    // Type section
    tb_out1b(&emit, 0x01);
    emit_uint(&emit, func_types.count + len_uint(func_type_count));
    emit_uint(&emit, func_type_count);

    tb_out_reserve(&emit, func_types.count + len_uint(func_type_count));
    tb_outs_UNSAFE(&emit, func_types.count, (const uint8_t*) func_types.data);

    // Function section
    tb_out1b(&emit, 0x03);
    emit_uint(&emit, func_section_size);
    emit_uint(&emit, func_count);
    dyn_array_for(i, sections) {
        DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
        dyn_array_for(j, funcs) {
            emit_uint(&emit, funcs[j]->wasm_type);
        }
    }

    TB_ExportBuffer buffer = { 0 };
    tb_export_append_chunk(&buffer, emitter_to_chunk(dst_arena, &emit));

    #if 1
    dyn_array_for(i, sections) {
        if ((sections[i].flags & TB_MODULE_SECTION_EXEC) == 0) {
            continue;
        }

        size_t body_size = len_uint(dyn_array_length(sections[i].funcs)) + sections[i].total_size;
        TB_ExportChunk* sec = tb_export_make_chunk(dst_arena, 1 + body_size + len_uint(body_size));

        uint8_t* p = sec->data;
        *p++ = 0x0A;
        p = emit_uint2(p, body_size);
        p = emit_uint2(p, dyn_array_length(sections[i].funcs));
        tb_helper_write_section(m, 0, &sections[i], p, 0);

        tb_export_append_chunk(&buffer, sec);
    }
    #endif
    return buffer;
}

