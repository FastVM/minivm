
#pragma once

#if !defined(MX_HEADER_LUA_FRONT_DUMP)
#define MX_HEADER_LUA_FRONT_DUMP

#include "../../ir/ir.h"
#include "../../value/string.h"
#include "../../vm/vm.h"

#define MX_DUMP_MAGIC "\x1bMXIR\x02"
#define MX_DUMP_MAGIC_LEN 6

typedef uint8_t mx_dump_lit_t;
enum {
    MX_DUMP_LIT_NIL,
    MX_DUMP_LIT_FALSE,
    MX_DUMP_LIT_TRUE,
    MX_DUMP_LIT_INT,
    MX_DUMP_LIT_FLOAT,
    MX_DUMP_LIT_STRING,
    MX_DUMP_LIT_FUNC,
    MX_DUMP_LIT_PRIM,
};

typedef struct mx_dump_writer_t mx_dump_writer_t;
typedef struct mx_dump_reader_t mx_dump_reader_t;
typedef struct mx_dump_loaded_t mx_dump_loaded_t;

struct mx_dump_writer_t {
    mx_t *vm;
    mx_str_t *out;
    bool is_bad: 1;
    bool is_strip: 1;
};

struct mx_dump_reader_t {
    mx_t *vm;
    const uint8_t *p;
    const uint8_t *end;
    const mx_source_t *source;
    const mx_ir_func_t *func;
    mx_ir_blocks_t blocks;
    uint32_t depth;
    bool is_bad: 1;
    bool is_truncated: 1;
};

struct mx_dump_loaded_t {
    mx_ir_block_t *entry;
    const char *error;
};

extern mx_ffi_t *const mx_dump_prims[];
extern const uint32_t mx_dump_nprims;

void mx_dump_uv(mx_dump_writer_t *w, uint64_t v);
void mx_dump_bytes(mx_dump_writer_t *w, const char *data, size_t len);
void mx_dump_name(mx_dump_writer_t *w, const char *name);
void mx_dump_range(mx_dump_writer_t *w, const mx_location_range_t *range);
void mx_dump_arg(mx_dump_writer_t *w, mx_ir_arg_t arg);
uint32_t mx_dump_index(const mx_ir_blocks_t *blocks, const mx_ir_block_t *block);
void mx_dump_func(mx_dump_writer_t *w, mx_ir_block_t *head);
mx_str_t *mx_dump_function(mx_t *vm, mx_ir_block_t *head, bool is_strip);

uint64_t mx_dump_read_uv(mx_dump_reader_t *r);
const uint8_t *mx_dump_read_bytes(mx_dump_reader_t *r, size_t len);
const char *mx_dump_read_name(mx_dump_reader_t *r);
mx_location_range_t mx_dump_read_range(mx_dump_reader_t *r);
mx_obj_t mx_dump_read_lit(mx_dump_reader_t *r, uint8_t tag);
mx_ir_arg_t mx_dump_read_arg(mx_dump_reader_t *r, uint32_t nregs);
bool mx_dump_is_lit(mx_ir_arg_t arg, mx_kind_t kind);
bool mx_dump_is_index(mx_ir_arg_t arg, uint64_t limit);
bool mx_dump_args_ok(const mx_ir_arg_t *args, uint32_t nargs, bool can_spread, bool can_prim, bool can_block);
bool mx_dump_instr_ok(mx_dump_reader_t *r, mx_ir_instr_opcode_t op, bool has_out, const mx_ir_arg_t *args, uint32_t nargs, bool is_entry);
bool mx_dump_prim_ok(const mx_ir_arg_t *args, uint32_t nargs);
bool mx_dump_branch_ok(mx_dump_reader_t *r, const mx_ir_branch_t *br, const mx_ir_arg_t *args, uint32_t nargs);
mx_ir_block_t *mx_dump_read_func(mx_dump_reader_t *r);
mx_dump_loaded_t mx_dump_load(mx_t *vm, const char *data, size_t len);

#endif
