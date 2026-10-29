
#include "thread.h"
#include "run.h"
#include "../vm/gc.h"

mx_thread_t *mx_thread_new(mx_t *vm, mx_obj_t entry_fn) {
    mx_thread_t *th = mx_gc_alloc(vm, sizeof(mx_thread_t));
    th->head.kind = vm->kinds.thread;
    th->status = MX_THREAD_FRESH;
    th->shape = MX_THREAD_SHAPE_STATUS;
    th->is_c_resumed = false;
    th->is_failed = false;
    th->is_closing = false;
    th->has_hook = false;
    th->slots = NULL;
    th->cap = 0;
    th->top = 0;
    th->frame = MX_THREAD_NONE;
    th->nest = 0;
    th->protects = 0;
    th->limit = MX_THREAD_MAX_WORDS - MX_RUN_ERROR_ROOM;
    th->open = NULL;
    th->resumer = NULL;
    th->entry_fn = entry_fn;
    th->failure = mx_obj_of_nil();
    th->token = 0;
    th->api = NULL;
    mx_gc_note_final(vm, &th->head);
    mx_gc_note_thread(vm, th);
    return th;
}

bool mx_thread_grow(mx_t *vm, mx_thread_t *th, uint32_t need) {
    if (need > MX_THREAD_MAX_WORDS) {
        return false;
    }
    uint64_t cap = (uint64_t)th->cap * 2;
    if (cap < MX_THREAD_MIN_WORDS) {
        cap = MX_THREAD_MIN_WORDS;
    }
    while (cap < need) {
        cap *= 2;
    }
    if (cap > MX_THREAD_MAX_WORDS) {
        cap = MX_THREAD_MAX_WORDS;
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
    uv->head.kind = vm->kinds.upval;
    uv->at = at;
    uv->kind_at = kind_at;
    uv->thread = th;
    uv->value = mx_obj_of_nil();
    uv->next = *link;
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
