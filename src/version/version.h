
#pragma once

#if !defined(MX_HEADER_VERSION)
#define MX_HEADER_VERSION

#include "../ir/ir.h"
#include "../vm/vm.h"

#if !defined(MX_RESUME_WAYS)
#define MX_RESUME_WAYS 4
#endif

#define MX_KIND_CELL ((mx_kind_t)0x8000)
#define MX_KIND_CELL_ANY ((mx_kind_t)0xffff)
#define MX_KIND_SLOT ((mx_kind_t)0xfffe)
#define MX_VERSION_KINDS_INLINE 64

typedef uint8_t mx_resume_kind_t;
enum {
    MX_RESUME_VALUES,
    MX_RESUME_BRANCH,
    MX_RESUME_AT,
    MX_RESUME_VOID,
    MX_RESUME_ENTRY,
    MX_RESUME_RETURN,
};

typedef struct mx_resume_cache_t mx_resume_cache_t;
typedef struct mx_kind_vec_t mx_kind_vec_t;
typedef struct mx_versions_t mx_versions_t;
typedef struct mx_version_table_t mx_version_table_t;
typedef struct mx_version_key_t mx_version_key_t;

struct mx_resume_cache_t {
    uintptr_t key;
    uintptr_t code;
    uintptr_t *more;
};

#define MX_RESUME_CACHE_WORDS (sizeof(mx_resume_cache_t) / sizeof(uintptr_t))

struct mx_resume_t {
    mx_resume_cache_t cache;
    uint32_t want;
    uint16_t out;
    mx_resume_kind_t kind;
    uint8_t meta;
    bool is_override: 1;
    bool has_cell_out: 1;
    bool is_meta: 1;
    uint16_t nkinds;
    uint32_t site_at;
    uint32_t resume_at;
    mx_version_t *rb;
    const mx_kind_t *kinds;
    const mx_ir_block_t *site;
    mx_ir_block_t *target;
    union {
        mx_ir_block_t *target2;
        uintptr_t *next_ip;
    };
};

struct mx_version_t {
    uintptr_t *code;
    mx_ir_block_t *block;
    const mx_kind_t *kinds;
    const mx_kind_t *out;
    uint32_t nkinds;
    uint32_t nout;
    uint32_t start_at;
    uint32_t key_hash;
    uint32_t frame;
    bool is_hooked: 1;
    bool is_vararg: 1;
    uint16_t nparams;
};

struct mx_kind_vec_t {
    const mx_kind_t *ptr;
    uint32_t len;
    uint32_t hash;
};

struct mx_versions_t {
    mx_version_t **ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_version_table_t {
    mx_version_t **ptr;
    uint32_t cap;
};

struct mx_version_cache_t {
    mx_version_cache_t *next;
    mx_versions_t list;
    mx_version_table_t table;
};

struct mx_version_key_t {
    mx_kind_t *kinds;
    uint32_t nkinds;
    mx_kind_t inline_kinds[MX_VERSION_KINDS_INLINE];
};

static inline bool mx_kind_is_cell(mx_kind_t kind) {
    return (kind & MX_KIND_CELL) != 0;
}

static inline bool mx_kind_is_open(mx_kind_t kind) {
    return (kind & MX_KIND_CELL) != 0 && kind != MX_KIND_SLOT;
}

static inline bool mx_kind_is_known(mx_kind_t kind) {
    return (kind & MX_KIND_CELL) == 0 || (kind != MX_KIND_SLOT && kind != MX_KIND_CELL_ANY);
}

static inline mx_kind_t mx_kind_plain(mx_kind_t kind) {
    return kind & (mx_kind_t)~MX_KIND_CELL;
}

static inline bool mx_kind_is_number(mx_kind_t kind) {
    return kind == MX_KIND_INT || kind == MX_KIND_FLOAT;
}

mx_location_range_t mx_resume_range(const mx_resume_t *rec);
uint32_t mx_version_kinds_hash(const mx_kind_t *kinds, uint32_t n);
const mx_kind_t *mx_version_kinds_intern(mx_t *vm, const mx_kind_t *kinds, uint32_t n);
void mx_version_caches_free(mx_t *vm);
uintptr_t *mx_version_slot(mx_t *vm, mx_ir_block_t *block);
uint32_t mx_version_key_hash(const mx_kind_t *kinds, uint32_t nkinds, uint32_t start_at, bool is_hooked);
void mx_version_cache_add(mx_version_cache_t *cache, mx_version_t *rb);
void mx_version_slot_add(mx_t *vm, uintptr_t *slot, mx_version_t *rb);
bool mx_version_matches(const mx_version_t *rb, const mx_kind_t *kinds, uint32_t nkinds, uint32_t start_at, bool is_hooked);
mx_version_t *mx_version_cache_find(const mx_version_cache_t *cache, uint32_t hash, const mx_kind_t *kinds, uint32_t nkinds, uint32_t start_at, bool is_hooked);
mx_version_t *mx_version_for_at(mx_t *vm, mx_ir_block_t *block, const mx_kind_t *kinds, uint32_t nkinds, uint32_t start_at);

void mx_version_key_init(mx_version_key_t *key, uint32_t n);
void mx_version_key_free(mx_version_key_t *key);
void mx_version_key_copy(mx_version_key_t *key, const mx_kind_t *kinds, uint32_t nkinds, bool is_resumed);
void mx_version_key_live(mx_t *vm, mx_version_key_t *key, mx_ir_block_t *target);
void mx_version_key_outs(mx_version_key_t *key, uint32_t out, const mx_kind_t *outs, uint32_t nouts);
mx_version_t *mx_version_edge(mx_t *vm, const mx_kind_t *kinds, uint32_t nkinds, mx_ir_block_t *target);
mx_version_t *mx_version_values(mx_t *vm, const mx_resume_t *rec, const mx_kind_t *outs, uint32_t nouts, bool is_resumed);
mx_version_t *mx_version_branch(mx_t *vm, const mx_resume_t *rec, bool is_true, bool is_resumed);
mx_version_t *mx_version_at(mx_t *vm, const mx_resume_t *rec, const mx_kind_t *outs, uint32_t nouts, bool is_resumed);
mx_version_t *mx_version_again(mx_t *vm, mx_version_t *rb, bool is_resumed);
mx_version_t *mx_version_entry(mx_t *vm, mx_ir_block_t *block, const mx_obj_t *args, uint32_t nargs);
mx_version_t *mx_version_entry_slow(mx_t *vm, mx_ir_block_t *block, const mx_obj_t *args, uint32_t nargs, uint64_t short_key) __attribute__((noinline));
mx_version_t *mx_version_args(mx_t *vm, mx_ir_block_t *block, const mx_kind_t *kinds, uint32_t nargs);
mx_version_t *mx_version_args_slow(mx_t *vm, mx_ir_block_t *block, const mx_kind_t *kinds, uint32_t nargs, uint64_t short_key) __attribute__((noinline));

static inline uint64_t mx_version_kinds_key(const mx_kind_t *kinds, uint32_t n) {
    uint64_t key = (uint64_t)(n + 1) << 48;
    if (n <= 3) {
        for (uint32_t i = 0; i < n; i++) {
            key |= (uint64_t)kinds[i] << (16 * i);
        }
        return key;
    } else if (n <= 6) {
        for (uint32_t i = 0; i < n; i++) {
            if (kinds[i] > 0xff) {
                return 0;
            }
            key |= (uint64_t)kinds[i] << (8 * i);
        }
        return key;
    } else {
        return 0;
    }
}

#endif
