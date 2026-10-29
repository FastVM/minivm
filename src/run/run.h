
#pragma once

#if !defined(MX_HEADER_RUN)
#define MX_HEADER_RUN

#include "thread.h"
#include "../version/version.h"
#include "../value/kind.h"
#include "../vm/vm.h"

#define MX_RUN_SLACK 64
#define MX_RUN_ERROR_ROOM 16384
#define MX_RUN_SMALL 16
#define MX_RUN_MAX_CHAIN 2000
#define MX_RUN_C_DEPTH 200
#define MX_RUN_KEY_HOOKED ((uintptr_t)1 << 63)
#define MX_RUN_KEY_RESUMED ((uintptr_t)1 << 62)

typedef uint8_t mx_run_status_t;
enum {
    MX_RUN_OK,
    MX_RUN_YIELD,
    MX_RUN_ERROR,
};

typedef uint8_t mx_run_start_t;
enum {
    MX_RUN_START_INIT,
    MX_RUN_START_CALL,
    MX_RUN_START_RESUME,
};

typedef struct mx_run_entry_t mx_run_entry_t;
typedef struct mx_run_result_t mx_run_result_t;
typedef struct mx_run_found_t mx_run_found_t;

struct mx_run_entry_t {
    mx_thread_t *thread;
    mx_version_t *rb;
    const mx_obj_t *args;
    uint32_t base;
    uint32_t nargs;
    mx_run_start_t start;
};

struct mx_run_result_t {
    mx_obj_t error;
    mx_run_status_t status;
};

struct mx_run_found_t {
    mx_obj_t value;
    bool is_found: 1;
};

static inline mx_obj_t mx_run_arg(const uintptr_t *p, const mx_val_t *base) {
    mx_val_t v;
    if ((p[1] & 1) != 0) {
        v.bits = p[0];
    } else {
        v = base[p[0]];
    }
    return mx_obj_of_raw((mx_kind_t)(p[1] >> 1), v);
}

static inline uintptr_t mx_run_key_flags(mx_t *vm, bool is_resumed) {
    uintptr_t flags = 0;
    if (vm->is_hooked) {
        flags |= MX_RUN_KEY_HOOKED;
    }
    if (is_resumed) {
        flags |= MX_RUN_KEY_RESUMED;
    }
    return flags;
}

static inline mx_val_t *mx_run_reserve(mx_t *vm, mx_val_t *base, uint32_t above) {
    mx_thread_t *th = vm->thread;
    uint32_t boff = (uint32_t)(base - th->slots);
    mx_thread_reserve(vm, th, boff + above);
    return th->slots + boff;
}

static inline uintptr_t mx_run_key1(mx_t *vm, mx_kind_t kind, bool is_resumed) {
    return mx_version_kinds_key(&kind, 1) | mx_run_key_flags(vm, is_resumed);
}

uintptr_t *mx_run_cache_find(const mx_resume_cache_t *cache, uintptr_t key);
void mx_run_cache_put(mx_t *vm, mx_resume_cache_t *cache, uintptr_t key, uintptr_t code);
mx_ffi_fast_t *mx_run_fast_of(const mx_t *vm, mx_ffi_t *fn);
bool mx_run_cells_stale(const mx_resume_t *rec, const mx_val_t *base);
void mx_run_cells_write(const mx_resume_t *rec, mx_val_t *base, const mx_kind_t *kinds, uint32_t n);
mx_run_found_t mx_run_index_chain(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key);
mx_run_found_t mx_run_get(mx_t *vm, mx_obj_t t, mx_obj_t key);
mx_obj_t mx_run_call_handler(mx_t *vm, mx_obj_t fn);
bool mx_run_set_is_raw(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key);
int32_t mx_run_set_str(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key, mx_obj_t value);
mx_obj_t mx_run_set_raw(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key, mx_obj_t value, uintptr_t *slot);
bool mx_run_eq_raw(mx_t *vm, mx_obj_t a, mx_obj_t b, bool *out);
mx_version_t *mx_run_push(mx_t *vm, mx_thread_t *th, uint32_t p, uintptr_t link, const mx_resume_t *rec, mx_obj_t fn, const mx_obj_t *args, uint32_t n);
void mx_run_spill(mx_val_t *base, uint32_t frame, const mx_obj_t *extra, uint32_t n);
bool mx_run_site_is_prim(const mx_resume_t *rec) __attribute__((noinline));
mx_obj_t mx_run_msgh(mx_t *vm, mx_obj_t handler, mx_obj_t err);
mx_obj_t mx_run_wrap_error(mx_t *vm, const mx_resume_t *rec, mx_obj_t err);
bool mx_run_collect(mx_t *vm);
mx_run_result_t mx_run_body(mx_t *vm, const mx_run_entry_t *entry) __attribute__((aligned(4096)));

mx_obj_t mx_call(mx_t *vm, mx_obj_t fn, uint32_t nargs, const mx_obj_t *args);
mx_obj_t mx_pcall(mx_t *vm, mx_obj_t fn, uint32_t nargs, const mx_obj_t *args, mx_obj_t handler);
mx_run_result_t mx_resume(mx_t *vm, mx_thread_t *co, uint32_t nargs, const mx_obj_t *args);
void mx_call_pend(mx_t *vm, mx_run_pending_kind_t kind, mx_obj_t fn, mx_obj_t extra, mx_thread_t *thread, uint32_t nargs, const mx_obj_t *args);
mx_obj_t mx_call_meta(mx_t *vm, mx_obj_t fn, uint32_t nargs, const mx_obj_t *args);
mx_obj_t *mx_call_args(mx_t *vm);
mx_obj_t *mx_call_results(mx_t *vm, uint32_t n);

#endif
