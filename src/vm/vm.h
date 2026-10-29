
#pragma once

#if !defined(MX_HEADER_VM)
#define MX_HEADER_VM

#include "../value/val.h"

#define MX_VERSION "0.1.0"
#define MX_KIND_MIXED ((mx_kind_t)0xffff)
#define MX_KIND_DEAD_KEY ((mx_kind_t)0xfffd)
#define MX_ARENA_CHUNK_BYTES ((size_t)1 << 20)
#define MX_ARENA_ALIGN 8
#define MX_VM_FNV_OFFSET 0x811c9dc5
#define MX_VM_FNV_PRIME 0x01000193

typedef uint16_t mx_kind_t;
enum {
    MX_KIND_NIL,
    MX_KIND_BOOLEAN,
    MX_KIND_INT,
    MX_KIND_FLOAT,
    MX_KIND_STRING,
    MX_KIND_CLOSURE,
    MX_KIND_FFI,
    MX_KIND_BLOCK,
    MX_KIND_ERROR,
    MX_KIND_LIGHTUSERDATA,
    MX_KIND_FIRST_DATA,
};

typedef struct mx_arena_t mx_arena_t;
typedef struct mx_arena_chunk_t mx_arena_chunk_t;
typedef struct mx_str_t mx_str_t;
typedef struct mx_t mx_t;
typedef struct mx_error_t mx_error_t;
typedef struct mx_ir_block_t mx_ir_block_t;
typedef struct mx_obj_closure_t mx_obj_closure_t;
typedef struct mx_obj_upval_t mx_obj_upval_t;
typedef struct mx_obj_table_t mx_obj_table_t;
typedef struct mx_obj_t mx_obj_t;
typedef struct mx_obj_head_t mx_obj_head_t;
typedef struct mx_obj_cfn_t mx_obj_cfn_t;
typedef struct mx_thread_t mx_thread_t;
typedef struct mx_version_t mx_version_t;
typedef struct mx_version_cache_t mx_version_cache_t;
typedef struct mx_resume_t mx_resume_t;
typedef struct mx_location_range_t mx_location_range_t;
typedef struct mx_kind_info_t mx_kind_info_t;
typedef struct mx_scratch_t mx_scratch_t;
typedef struct mx_lua_t mx_lua_t;
typedef struct mx_gc_t mx_gc_t;
typedef struct mx_std_state_t mx_std_state_t;
typedef struct mx_kind_vec_t mx_kind_vec_t;
typedef int32_t mx_std_fn_t(mx_std_state_t *ls);

struct mx_arena_chunk_t {
    mx_arena_chunk_t *next;
    size_t used;
    size_t size;
};

struct mx_arena_t {
    mx_arena_chunk_t *chunks;
    mx_arena_chunk_t *spare;
    uint32_t total;
    uint32_t pad;
    uint32_t allocs;
    uint32_t hops;
    uint32_t blocks;
    uint32_t sealed;
    uint32_t branches;
    uint32_t sources;
    uint32_t names;
    bool is_tracking: 1;
};

typedef struct mx_arena_mark_t mx_arena_mark_t;

struct mx_arena_mark_t {
    mx_arena_chunk_t *chunk;
    size_t used;
};

typedef struct mx_chunk_cache_t mx_chunk_cache_t;

struct mx_chunk_cache_t {
    const char *src;
    const char *file;
    mx_ir_block_t *entry;
    uint32_t len;
    uint32_t hash;
};

typedef void mx_ffi_t(mx_t *vm, size_t nargs, mx_obj_t *args);
typedef bool mx_ffi_fast_t(mx_t *vm, size_t nargs, mx_obj_t *args);

typedef struct mx_ffi_fast_reg_t mx_ffi_fast_reg_t;

struct mx_ffi_fast_reg_t {
    mx_ffi_t *ffi;
    mx_ffi_fast_t *fast;
};

struct mx_obj_t {
    mx_kind_t kind;
    mx_val_t val;
};

typedef uint8_t mx_run_pending_kind_t;
enum {
    MX_RUN_PENDING_KIND_CALL,
    MX_RUN_PENDING_KIND_PROTECT,
    MX_RUN_PENDING_KIND_PROTECT_MSGH,
    MX_RUN_PENDING_KIND_CALLCC,
    MX_RUN_PENDING_KIND_RESUME,
    MX_RUN_PENDING_KIND_RESUME_WRAP,
    MX_RUN_PENDING_KIND_YIELD,
    MX_RUN_PENDING_KIND_KJUMP,
};

struct mx_obj_head_t {
    uint16_t kind: 16;
    uint8_t mark: 8;
    uint8_t age: 2;
    bool is_container: 1;
    bool is_pinned: 1;
    bool is_forwarded: 1;
    bool is_old: 1;
    bool is_final: 1;
};

_Static_assert(sizeof(mx_obj_head_t) == 4, "head layout");

typedef bool mx_gc_finalizer_t(mx_t *vm);
typedef mx_obj_t mx_hook_cb_t(mx_t *vm, mx_thread_t *th, const mx_resume_t *at, uint32_t event);
typedef mx_obj_t mx_tbc_cb_t(mx_t *vm, mx_thread_t *th, uint32_t from, mx_obj_t err);
typedef void mx_cont_cb_t(mx_t *vm, mx_obj_t k);

typedef struct mx_kinds_t mx_kinds_t;
typedef struct mx_meta_std_t mx_meta_std_t;
typedef struct mx_tbc_t mx_tbc_t;
typedef struct mx_tbcs_t mx_tbcs_t;
typedef struct mx_run_pending_t mx_run_pending_t;
typedef struct mx_pending_t mx_pending_t;
typedef struct mx_hooks_t mx_hooks_t;
typedef struct mx_arenas_t mx_arenas_t;
typedef struct mx_code_scratch_t mx_code_scratch_t;
typedef struct mx_lower_fixup_t mx_lower_fixup_t;
typedef struct mx_lower_fixups_t mx_lower_fixups_t;
typedef struct mx_lower_recs_t mx_lower_recs_t;
typedef struct mx_lower_scratch_t mx_lower_scratch_t;
typedef struct mx_rets_t mx_rets_t;
typedef struct mx_rets_stack_t mx_rets_stack_t;
typedef struct mx_std_error_t mx_std_error_t;
typedef struct mx_kind_vecs_t mx_kind_vecs_t;
typedef struct mx_str_list_t mx_str_list_t;
typedef struct mx_interns_t mx_interns_t;
typedef struct mx_stats_t mx_stats_t;
typedef struct mx_holds_t mx_holds_t;

struct mx_holds_t {
    mx_obj_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_kinds_t {
    mx_kind_info_t *ptr;
    uint32_t len;
    uint32_t cap;
    mx_kind_t table;
    mx_kind_t cfn;
    mx_kind_t udata;
    mx_kind_t thread;
    mx_kind_t upval;
};

struct mx_meta_std_t {
    mx_ffi_t *fn;
    mx_obj_t key;
};

struct mx_tbc_t {
    mx_obj_t value;
    mx_thread_t *thread;
    uint32_t at;
};

struct mx_tbcs_t {
    mx_tbc_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_run_pending_t {
    mx_obj_t fn;
    mx_obj_t extra;
    mx_thread_t *thread;
    uint32_t nargs;
    mx_run_pending_kind_t kind;
};

struct mx_pending_t {
    mx_run_pending_t call;
    mx_obj_t *args;
    uint32_t cap;
    bool is_set: 1;
    bool is_ok: 1;
};

struct mx_hooks_t {
    mx_gc_finalizer_t *gc_finalizer;
    mx_hook_cb_t *debug;
    mx_tbc_cb_t *tbc_unwind;
    mx_cont_cb_t *cont_done;
};

struct mx_arenas_t {
    mx_arena_t ir;
    mx_arena_t code;
    mx_arena_t scratch;
};

struct mx_code_scratch_t {
    uintptr_t *ptr;
    size_t cap;
};

struct mx_lower_fixup_t {
    uint32_t rec;
    size_t at;
};

struct mx_lower_fixups_t {
    mx_lower_fixup_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_lower_recs_t {
    mx_resume_t **ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_lower_scratch_t {
    mx_kind_t *types;
    uint32_t cap;
    mx_lower_recs_t recs;
    mx_lower_fixups_t refs;
    mx_lower_fixups_t fixups;
};

struct mx_rets_t {
    mx_obj_t *vals;
    uint32_t len;
    uint32_t cap;
};

struct mx_rets_stack_t {
    mx_rets_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_std_error_t {
    mx_obj_t value;
    bool is_set: 1;
};

struct mx_kind_vecs_t {
    mx_kind_vec_t *ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_str_list_t {
    mx_str_t **ptr;
    uint32_t len;
    uint32_t cap;
};

struct mx_interns_t {
    mx_str_t **slots;
    uint32_t used;
    uint32_t tombs;
    uint32_t cap;
    mx_str_list_t young;
};

struct mx_stats_t {
    uint32_t versions;
    uint32_t version_bytes;
    uint64_t deliver_fast;
    uint64_t deliver_resumed_fast;
    uint64_t deliver_found;
    uint64_t deliver_new;
    uint64_t deliver_void;
    uint64_t sk_hit;
    uint64_t sk_miss;
    uint64_t call_lua;
    uint64_t call_ffi;
    uint64_t call_fast;
};

struct mx_t {
    mx_kinds_t kinds;

    mx_meta_std_t meta_call;
    mx_meta_std_t meta_get;
    mx_meta_std_t meta_set;
    mx_meta_std_t meta_eq;
    mx_obj_t weak_mode_key;

    mx_hooks_t hooks;
    const mx_ffi_fast_reg_t *fasts;
    mx_tbcs_t tbcs;
    mx_pending_t pending;
    mx_lua_t *lua_state;

    mx_arenas_t arenas;
    mx_scratch_t *scratch;
    mx_code_scratch_t code_scratch;
    mx_lower_scratch_t lower_scratch;
    void *const *hops;

    mx_version_cache_t *version_caches;
    uint32_t next_block_id;
    mx_stats_t stats;

    mx_gc_t *restrict gc;
    uint32_t no_gc;
    uint32_t gc_pending;

    mx_thread_t *main;
    mx_thread_t *thread;
    uint64_t run_token;
    uint64_t token_cur;
    uint32_t call_depth;

    mx_rets_t rets;
    mx_rets_stack_t rets_saved;
    mx_rets_t tmp;
    mx_std_error_t std_error;
    mx_holds_t holds;
    mx_holds_t consts;

    mx_kind_vecs_t kind_vecs;

    mx_interns_t interns;
    mx_obj_t char_strs[256];
    mx_str_t *str_cache[2048];
    mx_chunk_cache_t chunk_cache[256];

    uint8_t interrupt;
    bool is_gc_stopped: 1;
    bool is_hooked: 1;
};

void mx_fatal(const char *msg);
void *mx_mem_alloc(size_t size);
void *mx_mem_calloc(size_t size);
void *mx_mem_realloc(void *ptr, size_t size);
bool mx_mem_fits(size_t size);
void mx_mem_free(void *ptr);
char *mx_mem_strdup(const char *str);

mx_arena_t mx_arena_new(bool is_tracking);
void *mx_arena_take(mx_arena_t *arena, size_t size, size_t align);
void *mx_arena_alloc(mx_arena_t *arena, size_t size);
void *mx_arena_alloc_bytes(mx_arena_t *arena, size_t size);
const char *mx_arena_strdup(mx_arena_t *arena, const char *str);
mx_arena_mark_t mx_arena_mark(const mx_arena_t *arena);
void mx_arena_release(mx_arena_t *arena, mx_arena_mark_t mark);
void mx_arena_free(mx_arena_t *arena);

mx_t *mx_vm_new(void);
void mx_vm_free(mx_t *vm);

mx_obj_t *mx_vm_rets_reserve(mx_t *vm, uint32_t n);
void mx_vm_rets_set(mx_t *vm, const mx_obj_t *vals, uint32_t n);
void mx_vm_rets_prepend(mx_t *vm, mx_obj_t first, const mx_obj_t *vals, uint32_t n);
void mx_vm_rets_push(mx_t *vm);
void mx_vm_rets_pop(mx_t *vm);
mx_obj_t *mx_vm_tmp_reserve(mx_t *vm, uint32_t n);
void mx_vm_hold(mx_t *vm, mx_obj_t v);
mx_obj_t mx_vm_release(mx_t *vm);
void mx_vm_keep(mx_t *vm, mx_obj_t v);
mx_obj_t *mx_vm_pending_args(mx_t *vm, uint32_t n);

#endif
