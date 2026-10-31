
#pragma once

#if !defined(MX_HEADER_RUN_THREAD)
#define MX_HEADER_RUN_THREAD

#include "../value/obj.h"
#include "../vm/vm.h"

#define MX_THREAD_MIN_WORDS 1024
#define MX_THREAD_MAX_WORDS ((uint32_t)1 << 22)
#define MX_THREAD_NONE UINT32_MAX

typedef uint8_t mx_link_tag_t;
enum {
    MX_LINK_LUA,
    MX_LINK_C,
    MX_LINK_META,
    MX_LINK_HOOK,
    MX_LINK_SAFEPOINT,
    MX_LINK_ENTRY,
    MX_LINK_PROTECT,
    MX_LINK_PROTECT_MSGH,
    MX_LINK_CALLCC,
    MX_LINK_BOTTOM,
    MX_LINK_PROTECT_C,
    MX_LINK_PROTECT_MSGH_C,
    MX_LINK_BOTTOM_C,
    MX_LINK_K,
    MX_LINK_K_PROTECT,
};

typedef uint8_t mx_thread_status_t;
enum {
    MX_THREAD_FRESH,
    MX_THREAD_RUNNING,
    MX_THREAD_NORMAL,
    MX_THREAD_SUSPENDED,
    MX_THREAD_DEAD,
};

typedef uint8_t mx_thread_shape_t;
enum {
    MX_THREAD_SHAPE_STATUS,
    MX_THREAD_SHAPE_WRAP,
};

struct mx_thread_t {
    mx_obj_head_t head;
    mx_thread_status_t status;
    mx_thread_shape_t shape;
    bool is_c_resumed: 1;
    bool is_failed: 1;
    bool is_closing: 1;
    bool has_hook: 1;
    mx_val_t *slots;
    void *gc_next;
    uint32_t cap;
    uint32_t top;
    uint32_t frame;
    uint32_t nest;
    uint32_t protects;
    uint32_t limit;
    mx_obj_upval_t *open;
    mx_thread_t *resumer;
    mx_obj_t entry_fn;
    mx_obj_t failure;
    uint64_t token;
    void *api;
};

static inline uintptr_t mx_link_make(uint32_t caller, mx_link_tag_t tag) {
    return (uintptr_t)caller | ((uintptr_t)tag << 32);
}

static inline uint32_t mx_link_caller(uintptr_t link) {
    return (uint32_t)link;
}

static inline mx_link_tag_t mx_link_tag(uintptr_t link) {
    return (mx_link_tag_t)(link >> 32);
}

static inline uintptr_t mx_link_with_extra(uintptr_t link, uint32_t extra) {
    return (link & (((uintptr_t)1 << 40) - 1)) | ((uintptr_t)extra << 40);
}

static inline uint32_t mx_link_extra(uintptr_t link) {
    return (uint32_t)(link >> 40) & 0x7fffff;
}

static inline uintptr_t mx_link_with_tail(uintptr_t link) {
    return mx_link_with_extra(link, (uint32_t)1 << 23);
}

static inline bool mx_link_is_tail(uintptr_t link) {
    return (link >> 63) != 0;
}

static inline bool mx_link_has_aux(mx_link_tag_t tag) {
    return tag >= MX_LINK_ENTRY;
}

static inline bool mx_link_is_lua(mx_link_tag_t tag) {
    return tag != MX_LINK_C && tag != MX_LINK_META && tag != MX_LINK_HOOK && tag != MX_LINK_SAFEPOINT && tag != MX_LINK_PROTECT_C && tag != MX_LINK_PROTECT_MSGH_C && tag != MX_LINK_BOTTOM_C;
}

static inline mx_obj_t *mx_thread_objs(mx_thread_t *th, uint32_t at) {
    return (mx_obj_t *)(th->slots + at);
}

static inline mx_thread_t *mx_obj_get_thread(mx_obj_t o) {
    return (mx_thread_t *)mx_ref_obj(o.val.ref);
}

static inline mx_obj_t mx_obj_of_thread(mx_thread_t *th) {
    return mx_obj_of_data(th);
}

mx_thread_t *mx_thread_new(mx_t *vm, mx_obj_t entry_fn);
bool mx_thread_grow(mx_t *vm, mx_thread_t *th, uint32_t need);
void mx_thread_finalize(mx_thread_t *th);
mx_obj_upval_t *mx_thread_upval_find(mx_t *vm, mx_thread_t *th, uint32_t at, uint32_t kind_at);
void mx_thread_close_upvals(mx_t *vm, mx_thread_t *th, uint32_t from);
void mx_upval_set(mx_t *vm, mx_obj_upval_t *uv, mx_obj_t v);

static inline bool mx_thread_reserve(mx_t *vm, mx_thread_t *th, uint32_t need) {
    if (need <= th->cap) {
        return true;
    } else {
        return mx_thread_grow(vm, th, need);
    }
}

static inline mx_obj_t mx_upval_get(const mx_obj_upval_t *uv) {
    if (uv->thread != NULL) {
        return mx_obj_of_raw((mx_kind_t)uv->thread->slots[uv->kind_at].bits, uv->thread->slots[uv->at]);
    } else {
        return uv->value;
    }
}

#endif
