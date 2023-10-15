#include "coff.h"

#if 0
#define IMAGE_SYM_CLASS_EXTERNAL 0x0002
#define IMAGE_SYM_CLASS_STATIC   0x0003
#define IMAGE_SYM_CLASS_LABEL    0x0006
#define IMAGE_SYM_CLASS_FILE     0x0067

// let's ignore error handling for now :p
// buffered reading i guess?
TB_ObjectFile* tb_object_parse_coff(const TB_Slice file) {
    COFF_FileHeader* header = (COFF_FileHeader*) &file.data[0];

    TB_ObjectFile* obj_file = tb_platform_heap_alloc(sizeof(TB_ObjectFile) + (header->section_count * sizeof(TB_ObjectSection)));

    // not using calloc since i only really wanna clear the header
    memset(obj_file, 0, sizeof(TB_ObjectFile));
    obj_file->type = TB_OBJECT_FILE_COFF;

    switch (header->machine) {
        case COFF_MACHINE_AMD64: obj_file->arch = TB_ARCH_X86_64; break;
        case COFF_MACHINE_ARM64: obj_file->arch = TB_ARCH_AARCH64; break;
        default: obj_file->arch = TB_ARCH_UNKNOWN; break;
    }

    size_t string_table_pos = header->symbol_table + (header->symbol_count * sizeof(COFF_Symbol));

    // Read string table
    TB_Slice string_table = {
        .length = file.length - string_table_pos,
        .data   = &file.data[string_table_pos]
    };

    obj_file->section_count = header->section_count;
    FOREACH_N(i, 0, header->section_count) {
    }

    obj_file->symbols = tb_platform_heap_alloc(header->symbol_count * sizeof(TB_ObjectSymbol));
    obj_file->symbol_count = 0;

    size_t sym_id = 0;
    while (sym_id < header->symbol_count) {
        size_t symbol_offset = header->symbol_table + (sym_id * sizeof(COFF_Symbol));
        COFF_Symbol* sym = (COFF_Symbol*) &file.data[symbol_offset];

    }

    // trim the symbol table
    obj_file->symbols = tb_platform_heap_realloc(obj_file->symbols, obj_file->symbol_count * sizeof(TB_ObjectSymbol));

    return obj_file;
}
#endif
