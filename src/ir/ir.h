
#pragma once

#if !defined(MX_HEADER_IR)
#define MX_HEADER_IR

#include "../value/error.h"
#include "../value/obj.h"
#include "../value/string.h"

#define MX_IR_INSTR_MAX_ARGS 4
#define MX_IR_TABLE_HINT_BITS 16
#define MX_IR_TABLE_HINT_MASK ((1 << MX_IR_TABLE_HINT_BITS) - 1)
#define MX_IR_TABLE_HINT_BYTES 5
// The encoded tag of an integer literal, which is stored as a zigzag varint
// after it. The argument decodes to MX_IR_ARG_TYPE_LIT.
#define MX_IR_ENC_INT 4
#define MX_IR_BRANCH_MAX_ARGS 260
#define MX_IR_FRAME_WORDS 2
#define MX_IR_WANT_MULTI UINT32_MAX
#define MX_IR_HIDDEN_PACK_NAME "(vararg table)"

typedef uint8_t mx_ir_arg_type_t;
enum {
    MX_IR_ARG_TYPE_NONE,
    MX_IR_ARG_TYPE_REG,
    MX_IR_ARG_TYPE_LIT,
    MX_IR_ARG_TYPE_SPREAD,
};

typedef uint8_t mx_ir_branch_opcode_t;
enum {
    MX_IR_BRANCH_OPCODE_FALL,
    MX_IR_BRANCH_OPCODE_JUMP,
    MX_IR_BRANCH_OPCODE_BOOL,
    MX_IR_BRANCH_OPCODE_IF_LT,
    MX_IR_BRANCH_OPCODE_IF_LE,
    MX_IR_BRANCH_OPCODE_IF_EQ,
    MX_IR_BRANCH_OPCODE_RETURN,
    MX_IR_BRANCH_OPCODE_UPVAL_GET,
    MX_IR_BRANCH_OPCODE_TABLE_GET,
    MX_IR_BRANCH_OPCODE_CALL,
};

typedef uint8_t mx_ir_instr_opcode_t;
enum {
    MX_IR_INSTR_OPCODE_MOVE,
    MX_IR_INSTR_OPCODE_ADD,
    MX_IR_INSTR_OPCODE_SUB,
    MX_IR_INSTR_OPCODE_MUL,
    MX_IR_INSTR_OPCODE_DIV,
    MX_IR_INSTR_OPCODE_IDIV,
    MX_IR_INSTR_OPCODE_MOD,
    MX_IR_INSTR_OPCODE_POW,
    MX_IR_INSTR_OPCODE_CONCAT,
    MX_IR_INSTR_OPCODE_BAND,
    MX_IR_INSTR_OPCODE_BOR,
    MX_IR_INSTR_OPCODE_BXOR,
    MX_IR_INSTR_OPCODE_SHL,
    MX_IR_INSTR_OPCODE_SHR,
    MX_IR_INSTR_OPCODE_NEG,
    MX_IR_INSTR_OPCODE_BNOT,
    MX_IR_INSTR_OPCODE_TABLE_LEN,
    MX_IR_INSTR_OPCODE_TABLE_NEW,
    MX_IR_INSTR_OPCODE_TABLE_SET,
    MX_IR_INSTR_OPCODE_CLOSURE,
    MX_IR_INSTR_OPCODE_VARARGS,
    MX_IR_INSTR_OPCODE_UNPACK,
    MX_IR_INSTR_OPCODE_UPVAL_SET,
    MX_IR_INSTR_OPCODE_CLOSE,
    MX_IR_INSTR_OPCODE_KEEP,
};

typedef struct mx_ir_local_t mx_ir_local_t;
typedef struct mx_ir_locals_t mx_ir_locals_t;
typedef struct mx_ir_upval_t mx_ir_upval_t;
typedef struct mx_ir_func_t mx_ir_func_t;
typedef struct mx_ir_arg_t mx_ir_arg_t;
typedef struct mx_ir_branch_t mx_ir_branch_t;
typedef struct mx_ir_instr_t mx_ir_instr_t;
typedef struct mx_ir_cursor_t mx_ir_cursor_t;
typedef struct mx_ir_arg_read_t mx_ir_arg_read_t;
typedef struct mx_ir_step_t mx_ir_step_t;
typedef struct mx_ir_uv_t mx_ir_uv_t;
typedef struct mx_ir_blocks_t mx_ir_blocks_t;

struct mx_ir_local_t {
    const char *name;
    uint32_t reg;
    uint32_t start;
    uint32_t stop;
    bool is_param: 1;
    bool is_pack: 1;
};

struct mx_ir_locals_t {
    mx_ir_local_t *ptr;
    uint32_t len;
};

struct mx_ir_upval_t {
    const char *name;
    uint32_t index;
    bool is_local: 1;
};

// A block's id is its index among the function's blocks. Versions and
// liveness are only made for functions that run, so they live here, indexed
// by block id, instead of in every block. Each word of versions is 0, the
// block's only version, or its mx_version_cache_t with the low bit set.
// entry1 is the version last entered with the arguments in entry1_key.
struct mx_ir_func_t {
    mx_ir_block_t *head;
    const mx_source_t *source;
    mx_kind_t *upval_seen;
    const mx_ir_upval_t *upvals;
    uintptr_t *versions;
    uint64_t *live;
    mx_version_t *entry1;
    uint64_t entry1_key;
    mx_ir_locals_t locals;
    uint32_t nblocks;
    uint32_t start;
    uint32_t stop;
    uint32_t nupvals;
    uint32_t nregs;
    uint32_t nparams;
    bool is_vararg: 1;
    bool has_cells: 1;
};

struct mx_ir_arg_t {
    union {
        mx_obj_t lit;
        uint64_t reg;
    };
    mx_ir_arg_type_t type;
};

struct mx_ir_branch_t {
    mx_location_range_t range;
    mx_ir_block_t *targets[2];
    mx_ir_arg_t *args;
    const uint8_t *args_enc;
    mx_ir_arg_t out;
    uint32_t want;
    uint16_t arg_count;
    mx_ir_branch_opcode_t op;
};

struct mx_ir_instr_t {
    mx_location_range_t range;
    mx_ir_arg_t *args;
    mx_ir_arg_t out;
    mx_ir_instr_opcode_t op;
};

struct mx_ir_cursor_t {
    uint32_t off;
    mx_ir_arg_t args[MX_IR_INSTR_MAX_ARGS + 1];
};

// code holds code_len bytes of instructions followed by branch_len bytes of
// encoded branch, when the branch has one.
struct mx_ir_block_t {
    uint8_t *code;
    mx_ir_block_t *branch_targets[2];
    mx_ir_func_t *func;
    uint32_t id;
    uint32_t code_len;
    uint32_t branch_len;
    mx_ir_branch_opcode_t branch_op;
    bool is_func: 1;
    bool is_main: 1;
    bool is_marked: 1;
};

struct mx_ir_arg_read_t {
    const uint8_t *next;
    mx_ir_arg_t arg;
};

struct mx_ir_step_t {
    mx_ir_instr_t instr;
    bool is_valid: 1;
};

struct mx_ir_uv_t {
    uint64_t value;
    const uint8_t *next;
};

struct mx_ir_blocks_t {
    mx_ir_block_t **ptr;
    uint32_t len;
    uint32_t cap;
};

extern const char *const mx_ir_branch_names[];
extern const char *const mx_ir_instr_names[];

static inline mx_ir_block_t *mx_ir_func_head(mx_ir_block_t *block) {
    return block->func->head;
}

static inline void mx_ir_note_seen(mx_kind_t *seen, mx_kind_t kind) {
    if (*seen == MX_KIND_NIL) {
        *seen = kind;
    } else if (*seen != kind) {
        *seen = MX_KIND_MIXED;
    }
}

static inline void mx_ir_func_note_upval(mx_ir_func_t *func, uint32_t index, mx_kind_t kind) {
    mx_ir_note_seen(&func->upval_seen[index], kind);
}

static inline bool mx_ir_local_is_hidden_pack(const mx_ir_local_t *local) {
    return local->is_pack && local->name != NULL && strcmp(local->name, MX_IR_HIDDEN_PACK_NAME) == 0;
}

static inline bool mx_ir_func_has_hidden_pack(const mx_ir_func_t *func) {
    for (uint32_t i = 0; i < func->locals.len; i++) {
        if (func->locals.ptr[i].is_pack) {
            return mx_ir_local_is_hidden_pack(&func->locals.ptr[i]);
        }
    }
    return false;
}

static inline uint32_t mx_ir_block_cap(const mx_ir_block_t *block) {
    if (block->code == NULL) {
        return 0;
    }
    uint32_t cap = 64;
    while (cap < block->code_len + block->branch_len) {
        cap *= 2;
    }
    return cap;
}

static inline void mx_ir_block_grow(mx_ir_block_t *block, size_t need) {
    if (need > mx_ir_block_cap(block)) {
        uint32_t ncap = 64;
        while (ncap < need) {
            ncap *= 2;
        }
        block->code = mx_mem_realloc(block->code, ncap);
    }
}

static inline uint32_t mx_ir_func_frame(const mx_ir_func_t *func) {
    if (func->has_cells) {
        return func->nregs * 2;
    } else {
        return func->nregs;
    }
}

static inline size_t mx_ir_live_words(const mx_ir_func_t *func) {
    if (func->nregs == 0) {
        return 1;
    } else {
        return ((size_t)func->nregs + 63) / 64;
    }
}

static inline const uint64_t *mx_ir_block_live(const mx_ir_block_t *block) {
    return block->func->live + mx_ir_live_words(block->func) * block->id;
}

static inline bool mx_ir_live_test(const uint64_t *mask, uint32_t reg) {
    return ((mask[reg >> 6] >> (reg & 63)) & 1) != 0;
}

static inline void mx_ir_live_set(uint64_t *mask, uint32_t reg) {
    mask[reg >> 6] |= (uint64_t)1 << (reg & 63);
}

void *mx_ir_alloc(mx_t *vm, size_t size);
void *mx_ir_alloc_bytes(mx_t *vm, size_t size);
const char *mx_ir_strdup(mx_t *vm, const char *str);
mx_ir_block_t *mx_ir_block_new(mx_t *vm, mx_arena_t *arena, mx_ir_func_t *func);
mx_ir_func_t *mx_ir_func_new(mx_arena_t *arena, const mx_source_t *source);

size_t mx_ir_uv_size(uint64_t v);
uint8_t *mx_ir_put_uv(uint8_t *p, uint64_t v);
uint8_t *mx_ir_put_uv_wide(uint8_t *p, uint64_t v, size_t n);
mx_ir_uv_t mx_ir_get_uv(const uint8_t *p);
uint64_t mx_ir_zigzag(int64_t v);
int64_t mx_ir_unzigzag(uint64_t v);
size_t mx_ir_range_size(const mx_location_range_t *range, uint8_t *flags);
uint8_t *mx_ir_range_write(uint8_t *p, const mx_location_range_t *range, uint8_t flags);
const uint8_t *mx_ir_range_read(const uint8_t *p, mx_location_range_t *range, uint8_t flags);
size_t mx_ir_arg_size(mx_ir_arg_t arg);
uint8_t *mx_ir_arg_write(uint8_t *p, mx_ir_arg_t arg);
mx_ir_arg_read_t mx_ir_arg_read(const uint8_t *p);
uint32_t mx_ir_args_count(const mx_ir_arg_t *args);

void mx_ir_keep_lits(mx_t *vm, const mx_ir_arg_t *args);
uint32_t mx_ir_block_emit(mx_ir_block_t *block, mx_ir_instr_t instr);
uint8_t *mx_ir_block_operands(mx_ir_block_t *block, uint32_t at);
void mx_ir_block_patch_hint(mx_ir_block_t *block, uint32_t at, uint64_t hint);
void mx_ir_block_retarget(mx_ir_block_t *block, uint32_t at, uint32_t reg);
mx_ir_step_t mx_ir_block_next(const mx_ir_block_t *block, uint32_t limit, mx_ir_cursor_t *cur);
void mx_ir_block_set_branch(mx_t *vm, mx_ir_block_t *block, mx_ir_branch_t branch);
mx_ir_branch_t mx_ir_block_branch(const mx_ir_block_t *block);
mx_location_range_t mx_ir_block_range(const mx_ir_block_t *block, uint32_t at);
uint16_t mx_ir_branch_args(const mx_ir_branch_t *br, mx_ir_arg_t *out_args);
void mx_ir_blocks_seal(mx_t *vm, mx_ir_blocks_t *blocks, uint32_t from);

void mx_ir_live_use_arg(uint64_t *uevar, const uint64_t *kill, mx_ir_arg_t arg, uint32_t nregs);
void mx_ir_live_use_range(uint64_t *uevar, const uint64_t *kill, uint32_t from, uint32_t to, uint32_t nregs);
void mx_ir_live_local(mx_ir_block_t *block, uint64_t *uevar, uint64_t *kill, uint32_t nregs);
void mx_ir_liveness(mx_t *vm, mx_ir_block_t *entry);

void mx_ir_format_arg(mx_str_t *out, mx_ir_arg_t val);
void mx_ir_format_branch(mx_str_t *out, mx_ir_branch_t val);
void mx_ir_format_instr(mx_str_t *out, mx_ir_instr_t val);
void mx_ir_format_block(mx_str_t *out, mx_ir_block_t *val);

#endif
