
#pragma once

#if !defined(MX_HEADER_VALUE_TABLE)
#define MX_HEADER_VALUE_TABLE

#include "obj.h"
#include "num.h"
#include "../vm/vm.h"

#define MX_TABLE_INLINE_ARRAY 3
#define MX_TABLE_HASH_FOLD 32
#define MX_TABLE_HASH_INT_MULT 31
#define MX_TABLE_HASH_FLOAT_MULT 37
#define MX_TABLE_HASH_PTR_SHIFT 4
#define MX_TABLE_HASH_MIN_CAP 2
#define MX_TABLE_LOAD_NUM 3
#define MX_TABLE_LOAD_DEN 4

typedef uint8_t mx_table_step_t;
enum {
    MX_TABLE_STEP_PAIR,
    MX_TABLE_STEP_END,
    MX_TABLE_STEP_INVALID,
};

typedef struct mx_table_entry_t mx_table_entry_t;
typedef struct mx_table_pair_t mx_table_pair_t;
typedef struct mx_table_steal_t mx_table_steal_t;
typedef struct mx_table_slot_t mx_table_slot_t;

struct mx_table_pair_t {
    mx_obj_t key;
    mx_obj_t val;
    bool is_valid: 1;
};

struct mx_table_steal_t {
    mx_obj_t value;
    bool is_stolen: 1;
};

struct mx_table_slot_t {
    mx_obj_t value;
    int32_t slot;
};

struct mx_table_entry_t {
    mx_val_t key;
    mx_val_t val;
    mx_kind_t key_kind;
    mx_kind_t val_kind;
    uint32_t hash;
};

struct mx_obj_table_t {
    mx_obj_head_t head;
    uint8_t size: 6;
    bool is_plain: 1;
    bool is_short: 1;
    bool is_weak: 1;
    bool is_finalizable: 1;
    bool has_int_hash: 1;
    mx_table_entry_t *restrict entries;
    void *gc_next;
    mx_obj_t *restrict array;
    uint32_t array_len;
    uint32_t array_cap;
    uint32_t used;
    uint32_t len;
    mx_obj_table_t *meta;
    uint64_t hidden;
    mx_obj_t inline_array[MX_TABLE_INLINE_ARRAY];
};

#define MX_TABLE_SHORT_SIZE (sizeof(mx_obj_table_t) - sizeof(mx_obj_t) * MX_TABLE_INLINE_ARRAY)

static inline size_t mx_table_hash_cap_of(uint8_t size) {
    return (size_t)MX_TABLE_HASH_MIN_CAP << size;
}

static inline size_t mx_table_hash_cap(const mx_obj_table_t *tbl) {
    return mx_table_hash_cap_of(tbl->size);
}

static inline mx_obj_t mx_table_entry_key(const mx_table_entry_t *e) {
    return mx_obj_of_raw(e->key_kind, e->key);
}

static inline mx_obj_t mx_table_entry_val(const mx_table_entry_t *e) {
    return mx_obj_of_raw(e->val_kind, e->val);
}

static inline bool mx_table_entry_was_key(const mx_table_entry_t *e, mx_obj_t key) {
    return e->key_kind == MX_KIND_DEAD_KEY && e->key.bits == key.val.bits
        && (mx_obj_is_string(key) || mx_obj_is_closure(key) || mx_obj_is_data(key));
}

static inline void mx_table_entry_set_key(mx_table_entry_t *e, mx_obj_t k, uint32_t hash) {
    e->key_kind = k.kind;
    e->key = k.val;
    e->hash = hash;
}

static inline void mx_table_entry_set_val(mx_table_entry_t *e, mx_obj_t v) {
    e->val_kind = v.kind;
    e->val = v.val;
}

mx_obj_table_t *mx_table_init(mx_t *vm, size_t bytes, bool is_short);
mx_obj_table_t *mx_table_new(mx_t *vm);
mx_obj_table_t *mx_table_new_short(mx_t *vm);
mx_obj_table_t *mx_table_new_sized(mx_t *vm, uint32_t narray, uint32_t nhash);
mx_obj_table_t *mx_table_new_pack(mx_t *vm, const mx_obj_t *src, uint32_t count);
mx_table_entry_t *mx_table_entries_new(mx_t *vm, uint8_t size);
void mx_table_entries_free(mx_t *vm, mx_table_entry_t *entries, uint8_t size);
void mx_table_set(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key, mx_obj_t value);
int32_t mx_table_hash_set(mx_t *vm, mx_obj_table_t *restrict tbl, mx_obj_t key, mx_obj_t value);
void mx_table_hash_reserve(mx_t *vm, mx_obj_table_t *restrict tbl, uint32_t count);
void mx_table_array_push(mx_t *vm, mx_obj_table_t *restrict tbl, mx_obj_t value);
void mx_table_array_free(mx_t *vm, mx_obj_t *array, uint32_t cap);
void mx_table_array_reserve(mx_t *vm, mx_obj_table_t *restrict tbl, uint32_t cap);
mx_table_pair_t mx_table_pairs_hash_from(mx_t *vm, mx_obj_table_t *tbl, size_t slot);
mx_table_pair_t mx_table_pairs_array_from(mx_t *vm, mx_obj_table_t *tbl, size_t astart);
mx_table_pair_t mx_table_pairs_next(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key);
mx_table_step_t mx_table_next_into(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key, mx_obj_t *out);
uint32_t mx_table_key_hash(mx_obj_t key);
bool mx_table_key_eq(mx_obj_t a, mx_obj_t b);
mx_obj_t mx_table_get(mx_obj_table_t *tbl, mx_obj_t key);
mx_table_slot_t mx_table_get_string_slot(mx_obj_table_t *tbl, mx_obj_t key);
mx_obj_t mx_table_get_slow(mx_obj_table_t *tbl, mx_obj_t key);
int32_t mx_table_hash_slot(mx_obj_table_t *tbl, mx_obj_t key);
void mx_table_hash_grow(mx_t *vm, mx_obj_table_t *restrict tbl);
void mx_table_extend_len(mx_obj_table_t *restrict tbl, uint32_t from);
mx_table_steal_t mx_table_hash_steal(mx_obj_table_t *restrict tbl, uint32_t key);

static inline mx_obj_t mx_table_get_inline(mx_obj_table_t *tbl, mx_obj_t key) {
    if (key.kind == MX_KIND_INT) {
        uint64_t at = (uint64_t)(key.val.i64 - 1);
        if (at < (uint64_t)tbl->array_len) {
            return tbl->array[at];
        }
    } else if (key.kind == MX_KIND_STRING && tbl->entries != NULL) {
        mx_str_t *buf = (mx_str_t *)mx_ref_obj(key.val.ref);
        uint32_t hash = buf->hash;
        if (!buf->is_rope && hash != 0) {
            size_t len = mx_table_hash_cap(tbl);
            size_t look = hash & (len - 1);
            for (size_t probe = 0; probe < len; probe++) {
                const mx_table_entry_t *e = &tbl->entries[look];
                if (e->key_kind == MX_KIND_STRING && e->hash == hash) {
                    if (e->key.bits == key.val.bits) {
                        return mx_table_entry_val(e);
                    }
                    break;
                } else if (e->key_kind == MX_KIND_NIL) {
                    return mx_obj_of_nil();
                }
                look = (look + 1) & (len - 1);
            }
        }
    }
    return mx_table_get_slow(tbl, key);
}

#endif
