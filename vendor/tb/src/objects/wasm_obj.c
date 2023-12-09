#include "coff.h"
#include "../codegen/emitter.h"

typedef struct TB_ModuleExporter {
    const ICodeGen* code_gen;
    size_t write_pos;
} TB_ModuleExporter;

#define WRITE(data, length_) write_data(e, output, length_, data)
static void write_data(TB_ModuleExporter* e, uint8_t* output, size_t length, const void* data) {
    memcpy(output + e->write_pos, data, length);
    e->write_pos += length;
}

TB_API TB_Exports tb_wasm_write_output(TB_Module* m, const IDebugFormat* dbg) {
    TB_ModuleExporter* e = tb_platform_heap_alloc(sizeof(TB_ModuleExporter));
    memset(e, 0, sizeof(TB_ModuleExporter));

    // Module header
    TB_Emitter emit = { 0 };
    tb_out4b(&emit, 0x6D736100); // MAGIC   \0asm
    tb_out4b(&emit, 0x1);        // VERSION 1

    tb_platform_heap_free(e);
    return (TB_Exports){ .count = 1, .files = { { emit.count, emit.data } } };
}
