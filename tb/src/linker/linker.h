#pragma once
#include "../tb_internal.h"

typedef struct TB_LinkerSymbol TB_LinkerSymbol;
typedef struct TB_LinkerThreadInfo TB_LinkerThreadInfo;

// this is our packed object file handle (0 is a null entry)
typedef uint16_t TB_LinkerInputHandle;

typedef enum {
    TB_LINKER_INPUT_NULL,

    TB_LINKER_INPUT_ARCHIVE, // .lib .a
    TB_LINKER_INPUT_OBJECT,  // .obj .o
    TB_LINKER_INPUT_MODULE,  // TB_Module*
} TB_LinkerInputTag;

// usually it's object files
typedef struct {
    uint16_t tag;
    uint16_t parent;

    union {
        TB_Module* module;

        // compress into string handle
        TB_Slice name;
    };
} TB_LinkerInput;

typedef enum {
    TB_LINKER_PIECE_IMMUTABLE = 1,

    // by the time GC is done, this is resolved and we can
    // assume any pieces without this set are dead.
    TB_LINKER_PIECE_LIVE      = 2,
} TB_LinkerPieceFlags;

typedef struct {
    TB_LinkerThreadInfo* info;
    size_t index;
} TB_LinkerRelocRef;

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

    TB_LinkerInputHandle input;
    TB_LinkerSection* parent;

    TB_LinkerSymbol* first_sym;
    DynArray(TB_LinkerRelocRef) abs_refs;
    DynArray(TB_LinkerRelocRef) rel_refs;

    // NULL if doesn't apply.
    //   for a COFF .text section, this is the .pdata
    TB_LinkerSectionPiece* associate;

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
    NL_Slice name;

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

typedef struct {
    TB_Slice name;
    // this is the location the thunk will call
    uint32_t ds_address;
    // this is the ID of the thunk
    uint32_t thunk_id;
    uint16_t ordinal;
} ImportThunk;

typedef enum TB_LinkerSymbolFlags {
    TB_LINKER_SYMBOL_WEAK   = 1,
    TB_LINKER_SYMBOL_COMDAT = 2,
} TB_LinkerSymbolFlags;

// all symbols appended to the linker are converted into
// these and used for all kinds of relocation resolution.
struct TB_LinkerSymbol {
    // key
    TB_Slice name;

    // value
    TB_LinkerSymbolTag tag;
    TB_LinkerSymbolFlags flags;
    TB_LinkerSymbol* next;
    TB_Slice object_name;

    union {
        // for normal symbols,
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

        // for imports, refers to the imports array in TB_Linker
        struct {
            uint32_t id;
            uint16_t ordinal;
            ImportThunk* thunk;
        } import;

        struct {
            TB_LinkerSymbol* import_sym;
        } thunk;
    };
};

// MSI hash table
typedef struct TB_SymbolTable {
    size_t exp, len;
    TB_LinkerSymbol* ht; // [1 << exp]
} TB_SymbolTable;

typedef struct {
    TB_Slice libpath;
    DynArray(ImportThunk) thunks;

    uint64_t *iat, *ilt;
} ImportTable;

typedef struct TB_LinkerRelocRel TB_LinkerRelocRel;
struct TB_LinkerRelocRel {
    // within the same piece
    TB_LinkerRelocRel* next;

    // if target is NULL, check name
    TB_LinkerSymbol* target;
    TB_Slice name;
    TB_Slice* alt;

    TB_LinkerSectionPiece* src_piece;
    uint32_t src_offset;

    TB_LinkerInputHandle input;

    uint16_t addend;
    uint16_t type;
};

typedef struct TB_LinkerRelocAbs TB_LinkerRelocAbs;
struct TB_LinkerRelocAbs {
    // within the same piece
    TB_LinkerRelocAbs* next;

    // if target is NULL, check name
    TB_LinkerSymbol* target;
    TB_Slice name;
    TB_Slice* alt;

    TB_LinkerSectionPiece* src_piece;
    uint32_t src_offset;

    TB_LinkerInputHandle input;
};

typedef struct {
    TB_Slice from, to;
} TB_LinkerCmd;

struct TB_LinkerThreadInfo {
    TB_Linker* parent;

    // this refers to a separate TB_Linker's thread info
    TB_LinkerThreadInfo* next_in_thread;
    // this is the next thread info for the same TB_Linker
    TB_LinkerThreadInfo* next;

    // commands
    //   these are generated in object files and such but won't get
    //   executed until export time
    DynArray(TB_LinkerCmd) merges;
    DynArray(TB_LinkerCmd) alternates;

    // relocations
    //   we store different kinds of relocations in different arrays
    //   to simplify.
    DynArray(TB_LinkerRelocRel) relatives;
    DynArray(TB_LinkerRelocAbs) absolutes;
};

// Format-specific vtable:
typedef struct TB_LinkerVtbl {
    void (*init)(TB_Linker* l);
    void (*append_object)(TB_Linker* l, TB_Slice obj_name, TB_Slice content);
    void (*append_library)(TB_Linker* l, TB_Slice ar_name, TB_Slice ar_file);
    void (*append_module)(TB_Linker* l, TB_Module* m);
    TB_ExportBuffer (*export)(TB_Linker* l);
} TB_LinkerVtbl;

typedef struct TB_UnresolvedSymbol TB_UnresolvedSymbol;
struct TB_UnresolvedSymbol {
    TB_UnresolvedSymbol* next;

    TB_Slice name;
    uint32_t reloc;
};

typedef TB_LinkerSymbol* TB_SymbolResolver(TB_Linker* l, TB_LinkerSymbol* sym, TB_Slice name, TB_Slice* alt, uint32_t reloc_i);

// 1 << QEXP is the size of the queue
#define QEXP 6

typedef struct TB_Linker {
    TB_Arch target_arch;

    const char* entrypoint;
    TB_WindowsSubsystem subsystem;
    TB_SymbolResolver* resolve_sym;

    NL_Strmap(TB_LinkerSection*) sections;

    // we keep track of which object files exist in a
    // compact form, it's not like there's any dudplicate
    // entries so we don't need a hash map or something.
    DynArray(TB_LinkerInput) inputs;

    // for relocations
    TB_LinkerThreadInfo* first_thread_info;

    DynArray(TB_Module*) ir_modules;
    TB_SymbolTable symtab;

    size_t trampoline_pos;  // relative to the .text section
    TB_Emitter trampolines; // these are for calling imported functions

    NL_Strmap(TB_UnresolvedSymbol*) unresolved_symbols;

    // Message pump:
    //   this is how the user and linker communicate
    //
    // both head and tail exist in the queue field, it's a skeeto trick:
    //    https://github.com/skeeto/scratch/blob/master/misc/queue.c
    _Atomic uint32_t queue;
    _Atomic uint32_t queue_count;
    TB_LinkerMsg* messages;

    // Windows specific:
    //   on windows, we use DLLs to interact with the OS so
    //   there needs to be a way to load these immediately,
    //   imports do just that.
    //
    // this is where all the .reloc stuff from object files goes
    TB_LinkerSectionPiece* main_reloc;
    uint32_t iat_pos;
    DynArray(ImportTable) imports;

    TB_LinkerVtbl vtbl;
} TB_Linker;

void tb_linker_send_msg(TB_Linker* l, TB_LinkerMsg* msg);

// Error handling
TB_UnresolvedSymbol* tb__unresolved_symbol(TB_Linker* l, TB_Slice name);

TB_LinkerSectionPiece* tb__get_piece(TB_Linker* l, TB_LinkerSymbol* restrict sym);

// TB helpers
size_t tb__get_symbol_pos(TB_Symbol* s);
void tb__append_module_section(TB_Linker* l, TB_LinkerInputHandle mod, TB_ModuleSection* section, const char* name, uint32_t flags);

ImportThunk* tb__find_or_create_import(TB_Linker* l, TB_LinkerSymbol* restrict sym);

// Inputs
TB_LinkerInputHandle tb__track_module(TB_Linker* l, TB_LinkerInputHandle parent, TB_Module* mod);
TB_LinkerInputHandle tb__track_object(TB_Linker* l, TB_LinkerInputHandle parent, TB_Slice name);

// Symbol table
TB_LinkerSymbol* tb__find_symbol_cstr(TB_SymbolTable* restrict symtab, const char* name);
TB_LinkerSymbol* tb__find_symbol(TB_SymbolTable* restrict symtab, TB_Slice name);
TB_LinkerSymbol* tb__append_symbol(TB_SymbolTable* restrict symtab, const TB_LinkerSymbol* sym);
uint64_t tb__compute_rva(TB_Linker* l, TB_Module* m, const TB_Symbol* s);
uint64_t tb__get_symbol_rva(TB_Linker* l, TB_LinkerSymbol* sym);

// Section management
void tb__merge_sections(TB_Linker* linker, TB_LinkerSection* from, TB_LinkerSection* to);

TB_LinkerSection* tb__find_section(TB_Linker* linker, const char* name);
TB_LinkerSection* tb__find_or_create_section(TB_Linker* linker, const char* name, uint32_t flags);
TB_LinkerSection* tb__find_or_create_section2(TB_Linker* linker, size_t name_len, const uint8_t* name_str, uint32_t flags);
TB_LinkerSectionPiece* tb__append_piece(TB_LinkerSection* section, int kind, size_t size, const void* data, TB_LinkerInputHandle input);

size_t tb__pad_file(uint8_t* output, size_t write_pos, char pad, size_t align);
void tb__apply_module_relocs(TB_Linker* l, TB_Module* m, uint8_t* output);
size_t tb__apply_section_contents(TB_Linker* l, uint8_t* output, size_t write_pos, TB_LinkerSection* text, TB_LinkerSection* data, TB_LinkerSection* rdata, size_t section_alignment, size_t image_base);

// do layouting (requires GC step to complete)
bool tb__finalize_sections(TB_Linker* l);
