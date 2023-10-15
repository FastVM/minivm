#if 0
#include "../../tb_internal.h"

// builtin primitives (the custom types start at 0x100)
//
// if the top bit is set, we're using a pointer to these
// types rather than a direct type.
typedef enum {
    FUT_TYPE_VOID,

    // builtin bools
    FUT_TYPE_BOOL8, FUT_TYPE_BOOL16, FUT_TYPE_BOOL32, FUT_TYPE_BOOL64,

    // builtin char
    FUT_TYPE_CHAR8, FUT_TYPE_CHAR16, FUT_TYPE_CHAR32,

    // builtin integers
    FUT_TYPE_INT8,  FUT_TYPE_UINT8,
    FUT_TYPE_INT16, FUT_TYPE_UINT16,
    FUT_TYPE_INT32, FUT_TYPE_UINT32,
    FUT_TYPE_INT64, FUT_TYPE_UINT64,

    // builtin floats
    FUT_TYPE_FLOAT, FUT_TYPE_DOUBLE, FUT_TYPE_LONG_DOUBLE,

    // NOTE(NeGate): if set, the type is now a pointer to the described type.
    FUT_TYPE_POINTER = 0x7F,

    CUSTOM_TYPE_START = 0x100,
} FUT_TypeIndex;

// symbol table
typedef enum {
    // normal symbols
    FUT_SYMBOL_PROC,
    FUT_SYMBOL_DATA,

    // magic symbols
    FUT_SYMBOL_FILE,
    FUT_SYMBOL_MODULE,
} FUT_SymbolTag;

typedef struct {
    uint32_t tag  : 8;
    // 0 if doesn't have one
    uint32_t next : 24;
} FUT_SymbolHeader;

typedef struct {
    uint32_t tag  : 8;
    // 0 if doesn't have one
    uint32_t next : 24;
    // number of bytes to skip to reach the first element in the body of the symbol.
    // this only applies for procedures because they have nested elements.
    uint32_t content_ptr  : 8;
    // size in memory
    uint32_t storage_size : 24;
    // type
    FUT_TypeIndex type;
    // next
    uint32_t rva;
    char name[];
} FUT_NormalSymbol;

typedef struct {
    uint32_t tag  : 8;
    // 0 if doesn't have one
    uint32_t next : 24;
    char name[];
} FUT_File;

typedef struct {
    uint32_t tag  : 8;
    // 0 if doesn't have one
    uint32_t next : 24;
    char name[];
} FUT_Module;

static FUT_TypeIndex fut_get_type_from_dt(TB_DataType dt) {
    assert(dt.width == 0 && "TODO: implement vector types in CodeView output");
    switch (dt.type) {
        case TB_INT: {
            if (dt.data <= 0)  return FUT_TYPE_VOID;
            if (dt.data <= 1)  return FUT_TYPE_BOOL8;
            if (dt.data <= 8)  return FUT_TYPE_UINT8;
            if (dt.data <= 16) return FUT_TYPE_UINT16;
            if (dt.data <= 32) return FUT_TYPE_UINT32;
            if (dt.data <= 64) return FUT_TYPE_UINT64;
            return FUT_TYPE_VOID;
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) return FUT_TYPE_FLOAT;
            if (dt.data == TB_FLT_64) return FUT_TYPE_DOUBLE;

            assert(0 && "Unknown float type");
        }
        case TB_PTR: {
            return FUT_TYPE_POINTER | FUT_TYPE_INT8;
        }
        default: return tb_assert(0, "todo: missing type in CodeView output");
    }
}

static bool fut_supported_target(TB_Module* m) {
    return true;
}

static int fut_number_of_debug_sections(TB_Module* m) {
    return 1;
}

// there's quite a few places that mark the next field for symbols
#define MARK_NEXT(patch_pos) (((FUT_SymbolHeader*) tb_out_get(&symtab, mod_length_patch))->next = symtab.count)
static TB_SectionGroup fut_generate_debug_info(TB_Module* m, TB_TemporaryStorage* tls) {
    TB_ObjectSection* sections = tb_platform_heap_alloc(1 * sizeof(TB_ObjectSection));
    sections[0] = (TB_ObjectSection){ gimme_cstr_as_slice(".debug") };

    TB_Emitter symtab = { 0 };

    // we only store one module so we never fill the next
    FUT_Module mod = { FUT_SYMBOL_MODULE };
    size_t mod_length_patch = tb_outs(&symtab, sizeof(mod), &mod);
    tb_outstr_nul(&symtab, "fallback.o");

    // emit file table into symbol table.
    // skip the NULL file entry
    size_t file_count = dyn_array_length(m->files);
    FOREACH_N(i, 1, file_count) {
        size_t len = strlen(m->files[i].path);

        FUT_File file = { FUT_SYMBOL_FILE };
        size_t field_length_patch = tb_outs(&symtab, sizeof(file), &file);

        tb_outstr_nul(&symtab, m->files[i].path);
        MARK_NEXT(file_length_patch);
    }

    // globals

    // functions
    TB_FOR_FUNCTIONS(f, m) {
        TB_FunctionOutput* out_f = f->output;
        if (!out_f) continue;

        FUT_NormalSymbol sym = { FUT_SYMBOL_PROC };
        size_t sym_next_patch = tb_outs(&symtab, sizeof(sym), &sym);
        tb_outstr_nul(&symtab, f->super.name);

        // fill RVA
        size_t func_id = f->super.symbol_id;
        add_reloc(&sections[0], &(TB_ObjectReloc){
                TB_OBJECT_RELOC_ADDR32NB, func_id, sym_next_patch + offsetof(FUT_NormalSymbol, rva)
            });

        MARK_NEXT(sym_next_patch);
    }

    /*TB_FOR_FUNCTIONS(f, m) {
        TB_FunctionOutput* out_f = f->output;
        if (!out_f) continue;
    }*/

    MARK_NEXT(mod_length_patch);

    sections[0].raw_data = (TB_Slice){ symtab.count, symtab.data };
    return (TB_SectionGroup) { 1, sections };
}

IDebugFormat tb__fut_debug_format = {
    "FUT",
    fut_supported_target,
    fut_number_of_debug_sections,
    fut_generate_debug_info
};
#endif