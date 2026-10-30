
#pragma once

#if !defined(MX_HEADER_VM_GC)
#define MX_HEADER_VM_GC

#include "vm.h"

#define MX_GC_BLOCK_SIZE (1 << 15)
#define MX_GC_LINE_SIZE 128
#define MX_GC_LINES_PER_BLOCK (MX_GC_BLOCK_SIZE / MX_GC_LINE_SIZE)
#define MX_GC_BLOCK_HEADER_LINES 8
#define MX_GC_USABLE_LINES (MX_GC_LINES_PER_BLOCK - MX_GC_BLOCK_HEADER_LINES)
#define MX_GC_MEDIUM_MAX 2048
#define MX_GC_BLOCK_MAGIC 0x47434D56

#if !defined(MX_GC_INITIAL_THRESHOLD)
#define MX_GC_INITIAL_THRESHOLD (1024 * 1024)
#endif

#if !defined(MX_GC_NURSERY)
#define MX_GC_NURSERY (1024 * 1024)
#endif

#if !defined(MX_GC_NURSERY_MIN)
#define MX_GC_NURSERY_MIN (512 * 1024)
#endif

#if !defined(MX_GC_COLLECT_ENABLED)
#define MX_GC_COLLECT_ENABLED 1
#endif

#if !defined(MX_GC_STRESS)
#define MX_GC_STRESS 0
#endif

#if !defined(MX_GC_EVAC_ENABLED)
#define MX_GC_EVAC_ENABLED 1
#endif

#if !defined(MX_GC_MINOR_ENABLED)
#define MX_GC_MINOR_ENABLED 1
#endif

#if !defined(MX_GC_STAT)
#define MX_GC_STAT 0
#endif

#if !defined(MX_GC_FRAG)
#define MX_GC_FRAG 0
#endif

#if !defined(MX_GC_VERIFY)
#define MX_GC_VERIFY 0
#endif

#define MX_GC_IDENT_KEYS_INITIAL 64
#define MX_GC_TENURE_AGE 2
#define MX_GC_POOL_CLASSES 24
#define MX_GC_POOL_MIN_SHIFT 5
#define MX_GC_POOL_SLAB_CLASSES 4
#define MX_GC_POOL_SLAB_BYTES ((size_t)64 * 1024)
#define MX_GC_POOL_SLAB_HEADER 16
#define MX_GC_OWNED_CACHE_SLOTS 64
#define MX_GC_MARK_WRAP 250
#define MX_GC_MARK_OLD 254
#define MX_GC_MARK_STALE 255
#define MX_GC_MINORS_PER_MAJOR 1024
#define MX_GC_MAJOR_GROWTH_MAX 8
#define MX_GC_SLAB_BYTES ((size_t)2 << 20)
#define MX_GC_HASH_MULT UINT64_C(0x9E3779B97F4A7C15)

typedef uint8_t mx_gc_param_t;
enum {
    MX_GC_PARAM_PAUSE,
    MX_GC_PARAM_STEPMUL,
    MX_GC_PARAM_STEPSIZE,
    MX_GC_PARAM_MINORMUL,
    MX_GC_PARAM_MAJORMINOR,
    MX_GC_PARAM_MINORMAJOR,
    MX_GC_PARAM_COUNT,
};

typedef struct mx_gc_block_t mx_gc_block_t;
typedef struct mx_gc_large_t mx_gc_large_t;
typedef struct mx_gc_weak_t mx_gc_weak_t;

struct mx_gc_weak_t {
    bool is_weak_keys: 1;
    bool is_weak_vals: 1;
};

struct mx_gc_block_t {
    uint32_t magic;
    uint32_t final_count;
    uint32_t live_lines;
    mx_gc_block_t *next;
    mx_gc_block_t *recycle_next;
    mx_obj_head_t *old_head;
    uint8_t line_mark[MX_GC_LINES_PER_BLOCK];
    uint16_t starts[MX_GC_LINES_PER_BLOCK];
    uint8_t card[MX_GC_LINES_PER_BLOCK / 8];
    uint8_t old_only[MX_GC_LINES_PER_BLOCK / 8];
    uint8_t final_lines[MX_GC_LINES_PER_BLOCK / 8];
    bool is_evac: 1;
    bool is_old: 1;
    bool is_dirty: 1;
    bool has_young: 1;
};

_Static_assert(sizeof(mx_gc_block_t) <= MX_GC_BLOCK_HEADER_LINES * MX_GC_LINE_SIZE, "block header");
_Static_assert(MX_GC_BLOCK_HEADER_LINES % 8 == 0, "the sweep clears lines in aligned groups of eight");

struct mx_gc_large_t {
    mx_gc_large_t *next;
    size_t size;
    uint8_t mark;
};

#define MX_GC_LARGE_HEADER ((sizeof(mx_gc_large_t) + 15) & ~(size_t)15)

typedef struct mx_gc_blocks_t mx_gc_blocks_t;
typedef struct mx_gc_slabs_t mx_gc_slabs_t;
typedef struct mx_gc_index_t mx_gc_index_t;
typedef struct mx_gc_larges_t mx_gc_larges_t;
typedef struct mx_gc_roots_t mx_gc_roots_t;
typedef struct mx_gc_bump_t mx_gc_bump_t;
typedef struct mx_gc_pinned_t mx_gc_pinned_t;
typedef struct mx_gc_pool_t mx_gc_pool_t;
typedef struct mx_gc_gray_t mx_gc_gray_t;
typedef struct mx_gc_dirty_t mx_gc_dirty_t;
typedef struct mx_gc_objs_t mx_gc_objs_t;
typedef struct mx_gc_weaks_t mx_gc_weaks_t;
typedef struct mx_gc_threads_t mx_gc_threads_t;
typedef struct mx_gc_ident_keys_t mx_gc_ident_keys_t;
typedef struct mx_gc_cycle_t mx_gc_cycle_t;
typedef struct mx_gc_stats_t mx_gc_stats_t;

struct mx_gc_blocks_t {
    mx_gc_block_t *live;
    mx_gc_block_t *free;
    mx_gc_block_t *recycle;
    size_t count;
};

struct mx_gc_slabs_t {
    void **ptr;
    size_t len;
    size_t cap;
    uint8_t *cursor;
    uint8_t *end;
};

struct mx_gc_index_t {
    uintptr_t *ptr;
    size_t len;
    size_t cap;
    bool is_dirty: 1;
};

struct mx_gc_larges_t {
    mx_gc_large_t *list;
    mx_gc_index_t index;
};

struct mx_gc_roots_t {
    mx_obj_t **ptr;
    size_t len;
    size_t cap;
};

struct mx_gc_bump_t {
    uint8_t *cursor;
    uint8_t *limit;
    mx_gc_block_t *block;
};

struct mx_gc_pinned_t {
    mx_obj_head_t **ptr;
    size_t len;
    size_t cap;
};

struct mx_gc_pool_t {
    void *free[MX_GC_POOL_SLAB_CLASSES];
    uint8_t *bump[MX_GC_POOL_SLAB_CLASSES];
    uint8_t *bump_end[MX_GC_POOL_SLAB_CLASSES];
    size_t bytes;
    void *slabs;
};

struct mx_gc_gray_t {
    mx_obj_head_t **ptr;
    size_t len;
    size_t cap;
};

struct mx_gc_dirty_t {
    mx_gc_block_t **ptr;
    size_t len;
    size_t cap;
    mx_gc_block_t **scan;
    size_t scan_cap;
    uintptr_t last_base;
};

struct mx_gc_objs_t {
    mx_obj_t *ptr;
    size_t len;
    size_t cap;
};

struct mx_gc_weaks_t {
    mx_obj_table_t **ptr;
    size_t len;
    size_t cap;
};

struct mx_gc_threads_t {
    mx_thread_t **ptr;
    size_t len;
    size_t cap;
};

struct mx_gc_ident_keys_t {
    uintptr_t *ptr;
    size_t len;
    size_t cap;
    uintptr_t last;
};

struct mx_gc_cycle_t {
    size_t promoted_bytes;
    size_t old_live_after_major;
    size_t minors_since_major;
    size_t blocks_after_major;
    size_t major_growth_factor;
    size_t bytes_since_gc;
    size_t bytes_since_major;
    size_t threshold;
    size_t live_line_bytes;
    size_t young_line_bytes;
    size_t live_bytes;
};

struct mx_gc_stats_t {
    size_t collections;
    size_t peak_blocks;
    size_t peak_live_bytes;
};

struct mx_gc_t {
    mx_t *vm;
    mx_gc_blocks_t blocks;
    mx_gc_slabs_t slabs;
    mx_gc_index_t block_index;
    uintptr_t owned_cache[MX_GC_OWNED_CACHE_SLOTS];
    mx_gc_larges_t larges;
    mx_gc_roots_t roots;
    mx_gc_bump_t small;
    mx_gc_bump_t overflow;
    mx_gc_bump_t evac;
    size_t evac_bytes;
    mx_gc_pinned_t pinned;
    mx_gc_pool_t pool;
    mx_gc_gray_t gray;
    mx_gc_dirty_t dirty;
    mx_gc_objs_t finobjs;
    mx_gc_objs_t tobefnz;
    mx_gc_weaks_t weaks;
    mx_gc_threads_t threads;
    mx_gc_ident_keys_t ident_keys;
    mx_gc_cycle_t cycle;
    mx_gc_stats_t stats;
    uint32_t params[MX_GC_PARAM_COUNT];
    uint32_t consts_old;
    uint8_t mark_value;
    bool is_generational: 1;
    bool is_major_mode: 1;
    bool is_evacuating: 1;
    bool is_minor: 1;
    bool is_major_forced: 1;
    bool is_scanning_dirty: 1;
    bool has_found_young: 1;
    bool is_tracing: 1;
    bool is_finalizing: 1;
    bool is_closing: 1;
};

mx_gc_t *mx_gc_new(void);
void mx_gc_finalize_all(mx_t *vm, mx_gc_t *gc);
void mx_gc_free(mx_t *vm, mx_gc_t *gc);

#if MX_GC_STAT
size_t mx_gc_stat_arena_bytes(const mx_arena_t *arena);
void mx_gc_stat_dump(mx_t *vm);
#endif

size_t mx_gc_block_index_slot(mx_gc_t *gc, uintptr_t base);
void mx_gc_block_index_rebuild(mx_gc_t *gc);
bool mx_gc_block_owned(mx_gc_t *gc, void *ptr);
void mx_gc_interns_sweep(mx_t *vm, mx_gc_t *gc);
void mx_gc_obj_start_set(mx_gc_block_t *block, void *ptr);
bool mx_gc_obj_start_test(mx_gc_block_t *block, void *ptr);
uint8_t *mx_gc_block_data(mx_gc_block_t *block);
size_t mx_gc_line_of(mx_gc_block_t *block, void *ptr);
mx_gc_block_t *mx_gc_map_aligned_block(mx_gc_t *gc);
mx_gc_block_t *mx_gc_new_block(mx_gc_t *gc);
bool mx_gc_next_hole_in_block(mx_gc_t *gc, mx_gc_block_t *block, size_t from_line);
void mx_gc_find_hole(mx_gc_t *gc);
void mx_gc_note_alloc(mx_t *vm, size_t bytes);
void *mx_gc_alloc_large(mx_t *vm, size_t size);
mx_gc_large_t *mx_gc_large_of(void *payload);
bool mx_gc_large_owns(mx_gc_t *gc, void *ptr);
bool mx_gc_is_heap(mx_gc_t *gc, void *ptr);
void mx_gc_dirty_block(mx_gc_t *gc, mx_gc_block_t *b);
void mx_gc_card_set(mx_gc_block_t *b, void *ptr);
bool mx_gc_card_test(const uint8_t *cards, mx_gc_block_t *b, void *ptr);
int32_t mx_gc_pool_class(size_t bytes);
size_t mx_gc_large_index_slot(mx_gc_t *gc, uintptr_t payload);
void mx_gc_large_index_rebuild(mx_gc_t *gc);
size_t mx_gc_obj_size(mx_t *vm, mx_obj_head_t *head);
size_t mx_gc_obj_payload_size(mx_obj_head_t *head);
void *mx_gc_forward_get(mx_obj_head_t *head);
void mx_gc_forward_set(mx_obj_head_t *head, void *to);
void *mx_gc_evac_alloc(mx_gc_t *gc, size_t size);
size_t mx_gc_ident_key_slot(const mx_gc_t *gc, uintptr_t p);
bool mx_gc_ident_key_has(const mx_gc_t *gc, void *obj);
void *mx_gc_maybe_evacuate(mx_t *vm, mx_gc_t *gc, void *obj);
mx_obj_head_t **mx_gc_next_slot(mx_obj_head_t *head);
void mx_gc_old_record_block(mx_gc_block_t *block, mx_obj_head_t *head);
void mx_gc_promote_block(mx_gc_t *gc, mx_obj_head_t *head, mx_gc_block_t *block);
void mx_gc_gray_push(mx_gc_t *gc, mx_obj_head_t *head);

static inline void mx_gc_mark_lines_block(mx_gc_block_t *block, void *ptr, size_t size, uint8_t mark) {
    size_t start_line = ((size_t)((uint8_t *)ptr - (uint8_t *)block)) / MX_GC_LINE_SIZE;
    size_t end_line = ((size_t)((uint8_t *)ptr + size - 1 - (uint8_t *)block)) / MX_GC_LINE_SIZE;
    for (size_t i = start_line; i <= end_line; i++) {
        if (block->line_mark[i] != MX_GC_MARK_OLD) {
            block->line_mark[i] = mark;
        }
    }
}

static inline bool mx_gc_line_is_live(const mx_gc_t *gc, uint8_t mark) {
    return mark == gc->mark_value || mark == MX_GC_MARK_OLD;
}

static inline void mx_gc_head_clear_flags(void *obj) {
    mx_obj_head_t *head = obj;
    *head = (mx_obj_head_t) { .kind = head->kind };
}

void *mx_gc_alloc_slow(mx_t *vm, size_t size) __attribute__((noinline));
void mx_gc_note_bytes(mx_t *vm, size_t bytes);

static inline void *mx_gc_alloc(mx_t *vm, size_t size) {
    mx_gc_t *gc = vm->gc;
    size_t bytes = (size + 7) & ~(size_t)7;
    uint8_t *ret = gc->small.cursor;
    if (bytes <= MX_GC_LINE_SIZE && ret != NULL && bytes <= (size_t)(gc->small.limit - ret)) {
        gc->small.cursor = ret + bytes;
        mx_gc_block_t *block = gc->small.block;
        size_t slot = ((uintptr_t)ret - (uintptr_t)block) / 8;
        block->starts[slot >> 4] |= (uint16_t)(1u << (slot & 15));
        *(mx_obj_head_t *)ret = (mx_obj_head_t) {0};
        gc->cycle.bytes_since_gc += bytes;
        if (MX_GC_STRESS || gc->cycle.bytes_since_gc >= gc->cycle.threshold) {
            vm->gc_pending = 1;
            vm->interrupt = 1;
        }
        return ret;
    } else {
        return mx_gc_alloc_slow(vm, bytes);
    }
}
void mx_gc_note_final(mx_t *vm, mx_obj_head_t *head);
void mx_gc_note_final_owned(mx_obj_head_t *head);
void mx_gc_final_line_set(mx_gc_block_t *b, void *ptr);
void mx_gc_note_thread(mx_t *vm, mx_thread_t *th);
void mx_gc_write_barrier_slow(mx_t *vm, void *container);

static inline __attribute__((always_inline)) void mx_gc_write_barrier(mx_t *vm, void *container, mx_obj_t value) {
    mx_obj_head_t *ch = container;
    if (ch->is_old) {
        mx_gc_write_barrier_slow(vm, container);
    }
}

static inline __attribute__((always_inline)) void mx_gc_write_barrier_slot(mx_t *vm, void *container, mx_obj_t value) {
    mx_obj_head_t *ch = container;
    bool is_ref = value.kind == MX_KIND_STRING || value.kind == MX_KIND_CLOSURE || value.kind == MX_KIND_ERROR || value.kind >= MX_KIND_FIRST_DATA;
    if (ch->is_old && is_ref) {
        mx_gc_write_barrier_slow(vm, container);
    }
}

void *mx_gc_pool_slab_take(mx_gc_t *gc, int32_t cls);
void *mx_gc_pool_alloc(mx_t *vm, size_t bytes);
void *mx_gc_pool_realloc(mx_t *vm, void *ptr, size_t old_bytes, size_t bytes);
void mx_gc_pool_free(mx_t *vm, void *ptr, size_t bytes);

void mx_gc_note_ident_key(mx_t *vm, void *obj);
bool mx_gc_collect(mx_t *vm);
bool mx_gc_step(mx_t *vm, size_t kilobytes);
size_t mx_gc_bytes(mx_t *vm);
size_t mx_gc_budget(const mx_gc_t *gc, size_t live_bytes);
void mx_gc_add_root(mx_t *vm, mx_obj_t *slot);

static inline bool mx_gc_over_budget(const mx_gc_t *gc) {
    return gc->cycle.bytes_since_gc >= gc->cycle.threshold;
}

static inline bool mx_gc_kind_is_ref(const mx_t *vm, mx_kind_t kind) {
    return kind == MX_KIND_STRING || kind == MX_KIND_CLOSURE || kind == MX_KIND_ERROR || (kind >= MX_KIND_FIRST_DATA && kind < vm->kinds.len);
}

void mx_gc_pin(mx_gc_t *gc, mx_obj_head_t *head);
void mx_gc_mark_obj(mx_gc_t *gc, mx_obj_head_t *head);
void mx_gc_drain(mx_gc_t *gc);
void mx_gc_visit(mx_gc_t *gc, mx_obj_t *slot, bool is_old_scan);
void mx_gc_visit_ptr(mx_gc_t *gc, mx_kind_t kind, void **ptr, bool is_old_scan);
void mx_gc_stack_value(mx_gc_t *gc, mx_kind_t kind, mx_val_t *val, bool is_pin);
void mx_gc_stack_objs(mx_gc_t *gc, mx_val_t *from, mx_val_t *to, bool is_pin);
void mx_gc_scan_lua_frame(mx_gc_t *gc, mx_val_t *base, const mx_kind_t *kinds, uint32_t nkinds, const mx_version_t *rb, bool is_pin);
void mx_gc_scan_stack(mx_gc_t *gc, mx_thread_t *th, bool is_pin);
void mx_gc_scan_obj(mx_gc_t *gc, mx_obj_head_t *head, bool is_old_scan);
void mx_gc_mark_ref(mx_gc_t *gc, mx_obj_t *slot);
void mx_gc_mark_entry(mx_gc_t *gc, mx_kind_t kind, mx_val_t *val);
void mx_gc_mark_holds(mx_gc_t *gc, mx_holds_t *holds);
void mx_gc_mark_rets(mx_gc_t *gc, mx_rets_t *rets);
mx_obj_head_t *mx_gc_heap_head(mx_t *vm, mx_obj_t v);
void mx_gc_pin_roots(mx_t *vm, mx_gc_t *gc, bool is_evac);
void mx_gc_mark_threads(mx_t *vm, mx_gc_t *gc);
void mx_gc_mark_precise_roots(mx_t *vm);
void mx_gc_mark_roots(mx_t *vm);

mx_gc_weak_t mx_gc_table_weak_mode(mx_gc_t *gc, mx_obj_table_t *table);
void mx_gc_weak_note(mx_gc_t *gc, mx_obj_table_t *table);
void mx_gc_scan_table(mx_gc_t *gc, mx_obj_table_t *table, bool is_old_scan);
bool mx_gc_ephemeron_pass(mx_gc_t *gc);
bool mx_gc_slot_collectable(mx_gc_t *gc, mx_obj_t slot);
bool mx_gc_slot_is_dead(mx_gc_t *gc, mx_obj_t *slot);
void mx_gc_weak_clear_table(mx_gc_t *gc, mx_obj_table_t *table, bool weak_keys, bool weak_vals);
void mx_gc_weak_clear(mx_gc_t *gc);
void mx_gc_threads_sweep(mx_gc_t *gc);
void mx_gc_note_finalizer(mx_t *vm, mx_obj_table_t *table);
void mx_gc_tobefnz_push(mx_gc_t *gc, mx_obj_t obj);
void mx_gc_finalizers_separate(mx_gc_t *gc);
void mx_gc_finalize_on_close(mx_t *vm);
bool mx_gc_run_finalizers(mx_t *vm);
void mx_gc_scan_dirty_cards(mx_gc_t *gc);
void mx_gc_scan_old_child(mx_gc_t *gc, mx_obj_t *slot);

bool mx_gc_retire_obj_starts(mx_gc_t *gc, mx_gc_block_t *block, size_t line);
void mx_gc_finalize_dead_line(mx_gc_t *gc, mx_gc_block_t *block, size_t line);
void mx_gc_finalize(mx_t *vm, mx_obj_head_t *head);
void mx_gc_sweep(mx_gc_t *gc);
void mx_gc_sweep_large(mx_gc_t *gc);
void mx_gc_select_evac(mx_gc_t *gc);
void mx_gc_verify_child(mx_gc_t *gc, mx_obj_head_t *parent, const char *what, size_t index, mx_obj_t child);
void mx_gc_verify_obj(mx_gc_t *gc, mx_obj_head_t *h);
void mx_gc_verify(mx_t *vm);

#endif
