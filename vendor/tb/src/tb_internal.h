#pragma once

// Windows likes it's secure functions, i kinda do too
// but only sometimes and this isn't one of them
#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "tb.h"
#include "tb_formats.h"

#include <limits.h>
#include <time.h>
#include <stdalign.h>

#if defined(_MSC_VER) && !defined(__clang__)
#include <immintrin.h>
#define thread_local __declspec(thread)
#define alignas(x) __declspec(align(x))
#else
#define thread_local _Thread_local
#endif

#ifndef _WIN32
// NOTE(NeGate): I love how we assume that if it's not windows
// its just posix, these are the only options i guess
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "tb_platform.h"
#include "dyn_array.h"
#include "builtins.h"

#define NL_HASH_MAP_INLINE
#include <hash_map.h>
#include <new_hash_map.h>

#include <perf.h>

#define FOREACH_N(it, start, end) \
for (ptrdiff_t it = (start), end__ = (end); it < end__; ++it)

#define FOREACH_REVERSE_N(it, start, end) \
for (ptrdiff_t it = (end), start__ = (start); (it--) > start__;)

#define FOREACH_BIT(it, start, bits) \
for (uint64_t _bits_ = (bits), it = (start); _bits_; _bits_ >>= 1, ++it) if (_bits_ & 1)

#define TB_MIN(x, y) ((x) < (y) ? (x) : (y))
#define TB_MAX(x, y) ((x) > (y) ? (x) : (y))

#define PP_ARG0(a, ...) a
#define PP_AFTER0(a, ...) __VA_ARGS__

#define PP_ID(...) __VA_ARGS__
#define PP_VOID(...)
#define PP_JOIN(a, b) a b

#ifndef CONCAT
#define CONCAT_(x, y) x ## y
#define CONCAT(x, y) CONCAT_(x, y)
#endif

#include <arena.h>
#include "set.h"

#include <threads.h>
#include <stdatomic.h>

// Per-thread
#ifndef TB_TEMPORARY_STORAGE_SIZE
#define TB_TEMPORARY_STORAGE_SIZE (1 << 20)
#endif

#define CODE_REGION_BUFFER_SIZE (32 * 1024 * 1024)

typedef struct TB_Emitter {
    size_t capacity, count;
    uint8_t* data;
} TB_Emitter;

#define TB_DATA_TYPE_EQUALS(a, b) ((a).raw == (b).raw)

// i love my linked lists don't i?
typedef struct TB_SymbolPatch TB_SymbolPatch;
struct TB_SymbolPatch {
    TB_SymbolPatch* next;
    uint32_t pos;
    bool internal; // handled already by the code gen's emit_call_patches
    TB_Symbol* target;
};

struct TB_External {
    TB_Symbol super;
    TB_ExternalType type;

    void* thunk; // JIT will cache a thunk here because it's helpful

    // if non-NULL, the external was resolved
    _Atomic(TB_Symbol*) resolved;
};

typedef struct TB_InitObj {
    enum {
        TB_INIT_OBJ_REGION,
        TB_INIT_OBJ_RELOC,
    } type;
    TB_CharUnits offset;
    union {
        struct {
            TB_CharUnits size;
            const void* ptr;
        } region;

        TB_Symbol* reloc;
    };
} TB_InitObj;

struct TB_Global {
    TB_Symbol super;
    TB_Linkage linkage;
    TB_ModuleSectionHandle parent;

    // layout stuff
    void* address; // JIT-only
    uint32_t pos;
    TB_CharUnits size, align;

    // debug info
    TB_DebugType* dbg_type;

    // contents
    uint32_t obj_count, obj_capacity;
    TB_InitObj* objects;
};

struct TB_DebugType {
    enum {
        TB_DEBUG_TYPE_VOID,
        TB_DEBUG_TYPE_BOOL,

        TB_DEBUG_TYPE_UINT,
        TB_DEBUG_TYPE_INT,
        TB_DEBUG_TYPE_FLOAT,

        TB_DEBUG_TYPE_ARRAY,
        TB_DEBUG_TYPE_POINTER,

        // special types
        TB_DEBUG_TYPE_ALIAS,
        TB_DEBUG_TYPE_FIELD,

        // aggregates
        // TODO(NeGate): apparently codeview has cool vector and matrix types... yea
        TB_DEBUG_TYPE_STRUCT,
        TB_DEBUG_TYPE_UNION,

        TB_DEBUG_TYPE_FUNCTION,
    } tag;

    // debug-info target specific data
    union {
        struct {
            uint16_t type_id;
            uint16_t type_id_fwd; // used by records to manage forward decls
        };
    };

    // tag specific
    union {
        int int_bits;
        TB_FloatFormat float_fmt;
        TB_DebugType* ptr_to;
        struct {
            TB_DebugType* base;
            size_t count;
        } array;
        struct {
            size_t len;
            char* name;
            TB_DebugType* type;
        } alias;
        struct {
            size_t len;
            char* name;
            TB_CharUnits offset;
            TB_DebugType* type;
        } field;
        struct TB_DebugTypeRecord {
            size_t len;
            char* tag;
            TB_CharUnits size, align;

            size_t count;
            TB_DebugType** members;
        } record;
        struct TB_DebugTypeFunc {
            TB_CallingConv cc;
            bool has_varargs;

            size_t param_count, return_count;
            TB_DebugType** params;
            TB_DebugType** returns;
        } func;
    };
};

// TODO(NeGate): support complex variable descriptions
// currently we only support stack relative
typedef struct {
    int32_t offset;
} TB_DebugValue;

typedef struct TB_StackSlot {
    const char* name;
    TB_DebugType* type;
    TB_DebugValue storage;
} TB_StackSlot;

typedef struct TB_Comdat {
    TB_ComdatType type;
    uint32_t reloc_count;
} TB_Comdat;

typedef struct {
    uint32_t ip; // relative to the function body.
    TB_Safepoint* sp;
} TB_SafepointKey;

typedef struct COFF_UnwindInfo COFF_UnwindInfo;
typedef struct ICodeGen ICodeGen;

typedef struct TB_FunctionOutput {
    TB_Function* parent;
    TB_ModuleSectionHandle section;

    TB_Linkage linkage;

    uint64_t ordinal;
    uint8_t prologue_length;
    uint8_t epilogue_length;
    uint8_t nop_pads;

    TB_Assembly* asm_out;
    uint64_t stack_usage;

    uint8_t* code;
    size_t code_pos; // relative to the export-specific text section
    size_t code_size;

    // export-specific
    uint32_t unwind_info;
    uint32_t unwind_size;

    DynArray(TB_StackSlot) stack_slots;

    // Part of the debug info
    DynArray(TB_Location) locations;

    // Relocations
    uint32_t patch_pos;
    uint32_t patch_count;

    TB_SymbolPatch* first_patch;
    TB_SymbolPatch* last_patch;
} TB_FunctionOutput;

struct TB_Function {
    TB_Symbol super;
    TB_ModuleSectionHandle section;
    TB_Linkage linkage;

    TB_DebugType* dbg_type;
    TB_FunctionPrototype* prototype;

    // raw parameters
    size_t param_count;
    TB_Node** params;

    // IR allocation
    TB_Arena* arena;
    size_t node_count;

    TB_Node* root_node;
    TB_Trace trace;

    TB_NodeLocation* line_loc;
    NL_Table locations; // TB_Node* -> TB_NodeLocation*

    // how we track duplicates for GVN, it's possible to run while optimizing
    // the IR so we optionally do it.
    NL_HashSet gvn_nodes;

    // Compilation output
    union {
        void* compiled_pos;
        size_t compiled_symbol_id;
    };

    TB_FunctionOutput* output;
};

struct TB_ModuleSection {
    char* name;
    TB_LinkerSectionPiece* piece;

    int section_num;
    TB_ModuleSectionFlags flags;

    TB_Comdat comdat;

    // export-specific
    uint32_t export_flags;
    uint32_t name_pos;
    COFF_UnwindInfo* unwind;

    // this isn't computed until export time
    uint32_t raw_data_pos;
    uint32_t total_size;
    uint32_t reloc_count;
    uint32_t reloc_pos;

    DynArray(TB_Global*) globals;
    DynArray(TB_FunctionOutput*) funcs;
};

typedef struct {
    int len;
    char data[16];
} SmallConst;

// only next_in_module is ever mutated on multiple threads (when first attached)
struct TB_ThreadInfo {
    TB_Module* owner;
    _Atomic(TB_ThreadInfo*) next_in_module;

    TB_ThreadInfo* prev;
    TB_ThreadInfo* next;

    mtx_t* lock;

    // used for moving the start of the
    // linked list forward.
    TB_ThreadInfo** chain;

    TB_Arena* perm_arena;
    TB_Arena* tmp_arena;

    // live symbols (globals, functions and externals)
    //   we'll be iterating these during object/executable
    //   export to get all the symbols compiled.
    NL_HashSet symbols;
};

typedef struct {
    size_t count;
    TB_External** data;
} ExportList;

struct TB_Module {
    bool is_jit;
    ICodeGen* codegen;

    atomic_flag is_tls_defined;

    // we have a global lock since the arena can be accessed
    // from any thread.
    mtx_t lock;

    // thread info
    _Atomic(TB_ThreadInfo*) first_info_in_module;

    // small constants are interned because they
    // come up a lot.
    NL_Map(SmallConst, TB_Global*) global_interns;

    TB_ABI target_abi;
    TB_Arch target_arch;
    TB_System target_system;
    TB_FeatureSet features;
    ExportList exports;

    // This is a hack for windows since they've got this idea
    // of a _tls_index
    TB_Symbol* tls_index_extern;
    TB_Symbol* chkstk_extern;

    _Atomic uint32_t compiled_function_count;
    _Atomic uint32_t symbol_count[TB_SYMBOL_MAX];

    // needs to be locked with 'TB_Module.lock'
    NL_Strmap(TB_SourceFile*) files;

    // unused by the JIT
    DynArray(TB_ModuleSection) sections;

    // windows specific lol
    TB_LinkerSectionPiece* xdata;
};

typedef struct {
    size_t length;
    TB_ObjectSection* data;
} TB_SectionGroup;

typedef struct {
    uint32_t used;
    uint8_t data[];
} TB_TemporaryStorage;

struct ICodeGen {
    // what does CHAR_BIT mean on said platform
    int minimum_addressable_size, pointer_size;

    void (*get_data_type_size)(TB_DataType dt, size_t* out_size, size_t* out_align);

    // return the number of non-local patches
    size_t (*emit_call_patches)(TB_Module* restrict m, TB_FunctionOutput* out_f);

    // NULLable if doesn't apply
    void (*emit_win64eh_unwind_info)(TB_Emitter* e, TB_FunctionOutput* out_f, uint64_t stack_usage);

    void (*compile_function)(TB_Passes* p, TB_FunctionOutput* restrict func_out, const TB_FeatureSet* features, TB_Arena* arena, bool emit_asm);
};

// All debug formats i know of boil down to adding some extra sections to the object file
typedef struct {
    const char* name;

    bool (*supported_target)(TB_Module* m);
    int (*number_of_debug_sections)(TB_Module* m);

    // functions are laid out linearly based on their function IDs and
    // thus function_sym_start tells you what the starting point is in the symbol table
    TB_SectionGroup (*generate_debug_info)(TB_Module* m, TB_Arena* arena);
} IDebugFormat;

#define TB_FITS_INTO(T,x) ((x) == (T)(x))

// tb_todo means it's something we fill in later
// tb_unreachable means it's logically impossible to reach
// tb_assume means we assume some expression cannot be false
//
// in debug builds these are all checked and tb_todo is some sort of trap
#if defined(_MSC_VER) && !defined(__clang__)
#if TB_DEBUG_BUILD
#define tb_todo()            (assert(0 && "TODO"), __assume(0))
#define tb_unreachable()     (assert(0), __assume(0), 0)
#else
#define tb_todo()            abort()
#define tb_unreachable()     (__assume(0), 0)
#endif
#else
#if TB_DEBUG_BUILD
#define tb_todo()            __builtin_trap()
#define tb_unreachable()     (assert(0), 0)
#else
#define tb_todo()            __builtin_trap()
#define tb_unreachable()     (__builtin_unreachable(), 0)
#endif
#endif

#ifndef NDEBUG
#define tb_assert(condition, ...) ((condition) ? 0 : (fprintf(stderr, __FILE__ ":" STR(__LINE__) ": assertion failed: " #condition "\n  "), fprintf(stderr, __VA_ARGS__), abort(), 0))
#else
#define tb_assert(condition, ...) (0)
#endif

#if defined(_WIN32) && !defined(__GNUC__)
#define tb_panic(...)                     \
do {                                      \
    printf(__VA_ARGS__);                  \
    __fastfail(FAST_FAIL_FATAL_APP_EXIT); \
} while (0)
#else
#define tb_panic(...)                     \
do {                                      \
    printf(__VA_ARGS__);                  \
    abort();                              \
} while (0)
#endif

#ifndef COUNTOF
#define COUNTOF(...) (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
#endif

TB_ThreadInfo* tb_thread_info(TB_Module* m);

void* tb_out_reserve(TB_Emitter* o, size_t count);
void tb_out_commit(TB_Emitter* o, size_t count);

// reserves & commits
void* tb_out_grab(TB_Emitter* o, size_t count);
size_t tb_out_grab_i(TB_Emitter* o, size_t count);
size_t tb_out_get_pos(TB_Emitter* o, void* p);

// Adds null terminator onto the end and returns the starting position of the string
size_t tb_outstr_nul_UNSAFE(TB_Emitter* o, const char* str);

void tb_out1b_UNSAFE(TB_Emitter* o, uint8_t i);
void tb_out4b_UNSAFE(TB_Emitter* o, uint32_t i);
void tb_outstr_UNSAFE(TB_Emitter* o, const char* str);
void tb_outs_UNSAFE(TB_Emitter* o, size_t len, const void* str);
size_t tb_outs(TB_Emitter* o, size_t len, const void* str);
void* tb_out_get(TB_Emitter* o, size_t pos);

// fills region with zeros
void tb_out_zero(TB_Emitter* o, size_t len);

void tb_out1b(TB_Emitter* o, uint8_t i);
void tb_out2b(TB_Emitter* o, uint16_t i);
void tb_out4b(TB_Emitter* o, uint32_t i);
void tb_out8b(TB_Emitter* o, uint64_t i);
void tb_patch1b(TB_Emitter* o, uint32_t pos, uint8_t i);
void tb_patch2b(TB_Emitter* o, uint32_t pos, uint16_t i);
void tb_patch4b(TB_Emitter* o, uint32_t pos, uint32_t i);
void tb_patch8b(TB_Emitter* o, uint32_t pos, uint64_t i);

uint8_t  tb_get1b(TB_Emitter* o, uint32_t pos);
uint16_t tb_get2b(TB_Emitter* o, uint32_t pos);
uint32_t tb_get4b(TB_Emitter* o, uint32_t pos);

inline static uint64_t align_up(uint64_t a, uint64_t b) {
    return a + (b - (a % b)) % b;
}

// NOTE(NeGate): Considers 0 as a power of two
inline static bool tb_is_power_of_two(uint64_t x) {
    return (x & (x - 1)) == 0;
}

////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////
#ifdef _MSC_VER
#define TB_LIKELY(x)   (!!(x))
#define TB_UNLIKELY(x) (!!(x))
#else
#define TB_LIKELY(x)   __builtin_expect(!!(x), 1)
#define TB_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

TB_Node* tb_alloc_node_dyn(TB_Function* f, int type, TB_DataType dt, int input_count, int input_cap, size_t extra);
TB_Node* tb_alloc_node(TB_Function* f, int type, TB_DataType dt, int input_count, size_t extra);
TB_Node* tb__make_proj(TB_Function* f, TB_DataType dt, TB_Node* src, int index);

ExportList tb_module_layout_sections(TB_Module* m);

////////////////////////////////
// EXPORTER HELPER
////////////////////////////////
size_t tb_helper_write_section(TB_Module* m, size_t write_pos, TB_ModuleSection* section, uint8_t* output, uint32_t pos);
size_t tb__layout_relocations(TB_Module* m, DynArray(TB_ModuleSection) sections, const ICodeGen* restrict code_gen, size_t output_size, size_t reloc_size);

TB_ExportChunk* tb_export_make_chunk(size_t size);
void tb_export_append_chunk(TB_ExportBuffer* buffer, TB_ExportChunk* c);

////////////////////////////////
// ANALYSIS
////////////////////////////////
void set_input(TB_Function* f, TB_Node* n, TB_Node* in, int slot);
void add_user(TB_Function* f, TB_Node* n, TB_Node* in, int slot, User* recycled);
void print_node_sexpr(TB_Node* n, int depth);

TB_Symbol* tb_symbol_alloc(TB_Module* m, TB_SymbolTag tag, ptrdiff_t len, const char* name, size_t size);
void tb_symbol_append(TB_Module* m, TB_Symbol* s);

void tb_emit_symbol_patch(TB_FunctionOutput* func_out, TB_Symbol* target, size_t pos);
TB_Global* tb__small_data_intern(TB_Module* m, size_t len, const void* data);

// out_bytes needs at least 16 bytes
void tb__md5sum(uint8_t* out_bytes, uint8_t* initial_msg, size_t initial_len);

uint64_t tb__sxt(uint64_t src, uint64_t src_bits, uint64_t dst_bits);

char* tb__arena_strdup(TB_Module* m, ptrdiff_t len, const char* src);
TB_Node* tb__gvn(TB_Function* f, TB_Node* n, size_t extra);

static bool is_same_location(TB_Location* a, TB_Location* b) {
    return a->file == b->file && a->line == b->line && a->column == b->column;
}

static TB_Arena* get_temporary_arena(TB_Module* m) {
    return tb_thread_info(m)->tmp_arena;
}

static TB_Arena* get_permanent_arena(TB_Module* m) {
    return tb_thread_info(m)->perm_arena;
}
