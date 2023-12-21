#include <stdint.h>

// builtin primitives (the custom types start at 0x100)
//
// if the top bit is set, we're using a pointer to these
// types rather than a direct type.
typedef enum {
    SDG_PRIM_VOID,

    // builtin bools
    SDG_PRIM_BOOL8, SDG_PRIM_BOOL16, SDG_PRIM_BOOL32, SDG_PRIM_BOOL64,

    // builtin char
    SDG_PRIM_CHAR8, SDG_PRIM_CHAR16, SDG_PRIM_CHAR32,

    // builtin integers
    SDG_PRIM_INT8,  SDG_PRIM_UINT8,
    SDG_PRIM_INT16, SDG_PRIM_UINT16,
    SDG_PRIM_INT32, SDG_PRIM_UINT32,
    SDG_PRIM_INT64, SDG_PRIM_UINT64,

    // builtin floats
    SDG_PRIM_FLOAT, SDG_PRIM_DOUBLE, SDG_PRIM_LONG_DOUBLE,

    // NOTE(NeGate): if set, the type is now a pointer to the described type.
    SDG_PRIM_POINTER = 0x80,

    CUSTOM_TYPE_START = 0x100,
} SDG_TypeIndex;

typedef struct {
    enum {
        SDG_TYPE_PTR,
        SDG_TYPE_FUNC,
    } tag;

    int arg_count;
    SDG_TypeIndex base;
    SDG_TypeIndex args[];
} SDG_Type;

// symbol table
typedef enum {
    // normal symbols
    SDG_SYMBOL_PROC,
    SDG_SYMBOL_GLOBAL,

    // magic symbols
    SDG_SYMBOL_FILE,
    SDG_SYMBOL_MODULE,
} SDG_SymbolTag;

typedef struct {
    uint8_t tag;
    // number of bytes to skip to reach the first element in the body of the symbol.
    // this only applies for procedures because they have nested elements.
    uint16_t content_ptr;
    uint32_t kid_count;
    // 0 if doesn't have one
    uint32_t next;
} SDG_Symbol;

typedef struct {
    SDG_Symbol super;
    // type
    SDG_TypeIndex type;
    // in program's memory
    uint32_t rva, size;
    char name[];
} SDG_NormalSymbol;

typedef struct {
    SDG_Symbol super;
    // type
    SDG_TypeIndex type;
    // locals consist of pieces (for now, eventually
    // some location -> pieces)
    char name[];
} SDG_Local;

// Each piece can do a limited amount of processing
// on a variable to get it copied into the logical
// view of the variable.
typedef struct {
    // place to copy into the logical view.
    uint64_t offset;

    // indirection:
    //
    int32_t disp;
    uint8_t base;

    // bit extraction:
    //   (x >> bit_offset) & (~0 >> (64 - bit_len))
    //
    uint8_t bit_offset, bit_len;
} SDG_Piece;

typedef struct {
    SDG_Symbol super;
    // used to track changes
    uint32_t last_write;
    char name[];
} SDG_File;

typedef struct {
    SDG_Symbol super;
    uint32_t type_table;
    uint32_t type_count;
    char name[];
} SDG_Module;
