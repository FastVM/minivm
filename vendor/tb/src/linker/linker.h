#pragma once
#include "../tb_internal.h"

typedef struct TB_LinkerSymbol TB_LinkerSymbol;
typedef struct TB_LinkerThreadInfo TB_LinkerThreadInfo;

typedef char* TB_LinkerInternStr;

// basically an object file
typedef struct {
    size_t len;
    char* name;

    // if not-NULL, the sections for the are in a TB_Module.
    TB_Module* module;
} TB_LinkerObject;

typedef enum {
    TB_LINKER_PIECE_IMMUTABLE = 1,

    // by the time GC is done, this is resolved and we can
    // assume any pieces without this set are dead.
    TB_LINKER_PIECE_LIVE      = 2,
} TB_LinkerPieceFlags;

// we use a linked list to store these because i couldn't be bothered to allocate
// one giant sequential region for the entire linker.
struct TB_LinkerSectionPiece {
    TB_LinkerSectionPiece* next;

    enum {
        // write the data buffer in this struct
        PIECE_NORMAL,
        // Write TB_ModuleSection
        PIECE_MODULE_SECTION,
        // Write the TB module's pdata section
        PIECE_PDATA,
        // Write the TB module's reloc section
        PIECE_RELOC,
    } kind;

    TB_LinkerObject*  obj;
    TB_LinkerSection* parent;

    TB_LinkerSymbol* first_sym;

    TB_LinkerThreadInfo* info;
    DynArray(uint32_t) inputs;

    // mostly for .pdata crap
    TB_LinkerSectionPiece* assoc;

    // vsize is the virtual size
    size_t offset, vsize, size;
    // this is for COFF $ management
    uint32_t order;
    TB_LinkerPieceFlags flags;
    const uint8_t* data;
};

typedef enum {
    TB_LINKER_SECTION_DISCARD = 1,
    TB_LINKER_SECTION_COMDAT  = 2,
} TB_LinkerSectionFlags;

struct TB_LinkerSection {
    TB_LinkerInternStr name;

    TB_LinkerSectionFlags generic_flags;
    uint32_t flags;
    uint32_t number;

    uint32_t name_pos;

    uint64_t address; // usually a relative virtual address.
    size_t offset;    // in the file.

    size_t piece_count;
    size_t total_size;
    TB_LinkerSectionPiece *first, *last;
};

typedef enum TB_LinkerSymbolTag {
    TB_LINKER_SYMBOL_ABSOLUTE = 0,

    TB_LINKER_SYMBOL_UNKNOWN,

    // external linkage
    TB_LINKER_SYMBOL_NORMAL,

    // used for windows stuff as "__ImageBase"
    TB_LINKER_SYMBOL_IMAGEBASE,

    // TB defined
    TB_LINKER_SYMBOL_TB,

    // import thunks
    TB_LINKER_SYMBOL_THUNK,

    // imported from shared object (named with __imp_)
    TB_LINKER_SYMBOL_IMPORT,
} TB_LinkerSymbolTag;

typedef enum TB_LinkerSymbolFlags {
    TB_LINKER_SYMBOL_WEAK   = 1,
    TB_LINKER_SYMBOL_COMDAT = 2,
    TB_LINKER_SYMBOL_USED   = 4,
} TB_LinkerSymbolFlags;

// all symbols appended to the linker are converted into
// these and used for all kinds of relocation resolution.
struct TB_LinkerSymbol {
    TB_LinkerSymbolTag   tag;
    TB_LinkerSymbolFlags flags;

    TB_LinkerInternStr name;

    // next symbol in the section
    TB_LinkerSymbol* next;

    union {
        // for normal symbols
        struct {
            TB_LinkerSectionPiece* piece;
            uint32_t secrel;
        } normal;

        uint32_t absolute;
        uint32_t imagebase;

        // for IR module symbols
        struct {
            TB_LinkerSectionPiece* piece;
            TB_Symbol* sym;
        } tb;

        // for PE imports
        struct {
            // this is the location the thunk will call
            uint32_t ds_address;
            // this is the ID of the thunk
            uint32_t thunk_id;
            // import table ID
            uint32_t id;
            // TODO(NeGate): i don't remember rn
            uint16_t ordinal;
        } import;

        TB_LinkerSymbol* thunk;
    };
};

typedef struct {
    TB_Slice libpath;
    DynArray(TB_LinkerSymbol*) thunks;

    uint64_t *iat, *ilt;
} ImportTable;

typedef struct TB_LinkerReloc {
    uint8_t type;
    uint8_t addend;

    // source
    TB_LinkerSectionPiece* src_piece;
    uint32_t src_offset;

    // target
    TB_LinkerSymbol* target;
    TB_LinkerSymbol* alt;
} TB_LinkerReloc;

typedef struct {
    TB_Slice from, to;
} TB_LinkerCmd;

struct TB_LinkerThreadInfo {
    TB_Linker* owner;
    TB_LinkerThreadInfo* next_in_link;

    TB_LinkerThreadInfo* prev;
    TB_LinkerThreadInfo* next;

    TB_Arena* perm_arena;
    TB_Arena* tmp_arena;

    // commands
    //   these are generated in object files and such but won't get
    //   executed until export time
    DynArray(TB_LinkerCmd) merges;
    DynArray(TB_LinkerCmd) alternates;

    DynArray(TB_LinkerReloc) relocs;
    DynArray(TB_Module*) ir_modules;
};

// Format-specific vtable:
typedef struct TB_LinkerVtbl {
    void (*init)(TB_Linker* l);
    void (*append_object)(TB_Linker* l, TB_LinkerThreadInfo* info, TB_Slice obj_name, TB_Slice content);
    void (*append_library)(TB_Linker* l, TB_LinkerThreadInfo* info, TB_Slice ar_name, TB_Slice ar_file);
    void (*append_module)(TB_Linker* l, TB_LinkerThreadInfo* info, TB_Module* m);
    TB_ExportBuffer (*export)(TB_Linker* l);
} TB_LinkerVtbl;

typedef struct TB_Linker {
    TB_Arch target_arch;

    const char* entrypoint;
    TB_WindowsSubsystem subsystem;

    TB_LinkerVtbl vtbl;

    // we intern symbol strings to make the rest of the
    // hash table work easier, it's easier to write a giant
    // dumb interner.
    struct {
        size_t exp, cnt;
        TB_LinkerInternStr* arr;
    } interner;

    // we track which symbols have not been resolved yet
    DynArray(TB_LinkerSectionPiece*) worklist;

    // both use intern str keys
    NL_Table symbols;
    NL_Table sections;

    size_t trampoline_pos;  // relative to the .text section
    TB_Emitter trampolines; // these are for calling imported functions

    // for relocations
    _Atomic(TB_LinkerThreadInfo*) first_thread_info;

    // Windows specific:
    //   on windows, we use DLLs to interact with the OS so
    //   there needs to be a way to load these immediately,
    //   imports do just that.
    //
    // this is where all the .reloc stuff from object files goes
    TB_LinkerSectionPiece* main_reloc;
    uint32_t iat_pos;
    DynArray(ImportTable) imports;
} TB_Linker;

TB_LinkerThreadInfo* linker_thread_info(TB_Linker* l);

void tb_linker_unresolved_sym(TB_Linker* l, TB_LinkerInternStr name);

TB_LinkerSectionPiece* tb_linker_get_piece(TB_Linker* l, TB_LinkerSymbol* restrict sym);
void tb_linker_associate(TB_Linker* l, TB_LinkerSectionPiece* a, TB_LinkerSectionPiece* b);

// TB helpers
size_t tb__get_symbol_pos(TB_Symbol* s);

// Symbol table
TB_LinkerInternStr tb_linker_intern_string(TB_Linker* l, size_t len, const char* str);
TB_LinkerInternStr tb_linker_intern_cstring(TB_Linker* l, const char* str);
size_t tb_linker_intern_len(TB_Linker* l, TB_LinkerInternStr str);

TB_LinkerSymbol* tb_linker_new_symbol(TB_Linker* l, TB_LinkerInternStr name);
TB_LinkerSymbol* tb_linker_find_symbol(TB_Linker* l, TB_LinkerInternStr name);
TB_LinkerSymbol* tb_linker_find_symbol2(TB_Linker* l, const char* name);

// Sections
TB_LinkerSection* tb_linker_find_section(TB_Linker* linker, TB_LinkerInternStr name);
TB_LinkerSection* tb_linker_find_section2(TB_Linker* linker, const char* name);

TB_LinkerSection* tb_linker_find_or_create_section(TB_Linker* linker, TB_LinkerInternStr name, uint32_t flags);
TB_LinkerSectionPiece* tb_linker_append_piece(TB_LinkerThreadInfo* info, TB_LinkerSection* section, int kind, size_t size, const void* data, TB_LinkerObject* obj);
void tb_linker_merge_sections(TB_Linker* linker, TB_LinkerSection* from, TB_LinkerSection* to);
void tb_linker_append_module_section(TB_Linker* l, TB_LinkerThreadInfo* info, TB_LinkerObject* mod, TB_ModuleSection* section, uint32_t flags);
void tb_linker_append_module_symbols(TB_Linker* l, TB_Module* m);

uint64_t tb__compute_rva(TB_Linker* l, TB_Module* m, const TB_Symbol* s);
uint64_t tb__get_symbol_rva(TB_Linker* l, TB_LinkerSymbol* sym);

size_t tb__pad_file(uint8_t* output, size_t write_pos, char pad, size_t align);
void tb_linker_apply_module_relocs(TB_Linker* l, TB_Module* m, TB_LinkerSection* text, uint8_t* output);
size_t tb__apply_section_contents(TB_Linker* l, uint8_t* output, size_t write_pos, TB_LinkerSection* text, TB_LinkerSection* data, TB_LinkerSection* rdata, size_t section_alignment, size_t image_base);

TB_LinkerSymbol* tb_linker_get_target(TB_LinkerReloc* r);

void tb_linker_push_piece(TB_Linker* l, TB_LinkerSectionPiece* p);
void tb_linker_push_named(TB_Linker* l, const char* name);
void tb_linker_mark_live(TB_Linker* l);

// do layouting (requires GC step to complete)
bool tb__finalize_sections(TB_Linker* l);
