
#include "thread.h"
#include "run.h"
#include "../vm/gc.h"

mx_thread_t *mx_thread_new(mx_t *vm, mx_obj_t entry_fn) {
    mx_thread_t *th = mx_gc_alloc(vm, sizeof(mx_thread_t));
    *th = (mx_thread_t) {
        .head = (mx_obj_head_t) { .kind = vm->kinds.thread },
        .status = MX_THREAD_FRESH,
        .shape = MX_THREAD_SHAPE_STATUS,
        .is_c_resumed = false,
        .is_failed = false,
        .is_closing = false,
        .has_hook = false,
        .slots = NULL,
        .cap = 0,
        .top = 0,
        .frame = MX_THREAD_NONE,
        .nest = 0,
        .protects = 0,
        .limit = MX_THREAD_MAX_WORDS - MX_RUN_ERROR_ROOM,
        .open = NULL,
        .resumer = NULL,
        .entry_fn = entry_fn,
        .failure = mx_obj_of_nil(),
        .token = 0,
        .api = NULL,
    };
    mx_gc_note_final(vm, &th->head);
    mx_gc_note_thread(vm, th);
    return th;
}

bool mx_thread_grow(mx_t *vm, mx_thread_t *th, uint32_t need) {
    if (need > MX_THREAD_MAX_WORDS) {
        return false;
    }
    uint64_t want;
    if ((uint64_t)th->cap * 2 < MX_THREAD_MIN_WORDS) {
        want = MX_THREAD_MIN_WORDS;
    } else {
        want = (uint64_t)th->cap * 2;
    }
    while (want < need) {
        want *= 2;
    }
    uint64_t cap;
    if (want > MX_THREAD_MAX_WORDS) {
        cap = MX_THREAD_MAX_WORDS;
    } else {
        cap = want;
    }
    th->slots = mx_mem_realloc(th->slots, sizeof(mx_val_t) * cap);
    memset(th->slots + th->cap, 0, sizeof(mx_val_t) * (cap - th->cap));
    mx_gc_note_bytes(vm, sizeof(mx_val_t) * (cap - th->cap));
    th->cap = (uint32_t)cap;
    return true;
}

void mx_thread_finalize(mx_thread_t *th) {
    mx_mem_free(th->api);
    th->api = NULL;
    mx_mem_free(th->slots);
    th->slots = NULL;
    th->cap = 0;
    th->top = 0;
}

mx_obj_upval_t *mx_thread_upval_find(mx_t *vm, mx_thread_t *th, uint32_t at, uint32_t kind_at) {
    mx_obj_upval_t **link = &th->open;
    while (*link != NULL && (*link)->at > at) {
        link = &(*link)->next;
    }
    if (*link != NULL && (*link)->at == at) {
        return *link;
    }
    mx_obj_upval_t *uv = mx_gc_alloc(vm, sizeof(mx_obj_upval_t));
    *uv = (mx_obj_upval_t) {
        .head = (mx_obj_head_t) { .kind = vm->kinds.upval },
        .at = at,
        .thread = th,
        .next = *link,
        .kind_at = kind_at,
        .value = mx_obj_of_nil(),
    };
    *link = uv;
    return uv;
}

void mx_thread_close_upvals(mx_t *vm, mx_thread_t *th, uint32_t from) {
    while (th->open != NULL && th->open->at >= from) {
        mx_obj_upval_t *uv = th->open;
        th->open = uv->next;
        mx_obj_t v = mx_obj_of_raw((mx_kind_t)th->slots[uv->kind_at].bits, th->slots[uv->at]);
        mx_gc_write_barrier(vm, uv, v);
        uv->value = v;
        uv->thread = NULL;
        uv->next = NULL;
    }
}

void mx_upval_set(mx_t *vm, mx_obj_upval_t *uv, mx_obj_t v) {
    if (uv->thread != NULL) {
        uv->thread->slots[uv->at] = v.val;
        uv->thread->slots[uv->kind_at].bits = v.kind;
    } else {
        mx_gc_write_barrier(vm, uv, v);
        uv->value = v;
    }
}
