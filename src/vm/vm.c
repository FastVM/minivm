
#include "vm.h"
#include "gc.h"
#include "../ir/ir.h"
#include "../ir/scratch.h"
#include "../run/run.h"
#include "../run/thread.h"
#include "../value/kind.h"
#include "../version/version.h"

void mx_fatal(const char *msg) {
    fprintf(stderr, "minivm: %s\n", msg);
    fflush(stderr);
    exit(EXIT_FAILURE);
}

void *mx_mem_alloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL && size != 0) {
        mx_fatal("not enough memory");
    }
    return ptr;
}

void *mx_mem_calloc(size_t size) {
    void *ptr = calloc(1, size);
    if (ptr == NULL && size != 0) {
        mx_fatal("not enough memory");
    }
    return ptr;
}

void *mx_mem_realloc(void *ptr, size_t size) {
    void *out = realloc(ptr, size);
    if (out == NULL && size != 0) {
        mx_fatal("not enough memory");
    }
    return out;
}

bool mx_mem_fits(size_t size) {
    if (size < ((size_t)1 << 20)) {
        return true;
    }
    void *ptr = malloc(size);
    free(ptr);
    return ptr != NULL;
}

void mx_mem_free(void *ptr) {
    free(ptr);
}

char *mx_mem_strdup(const char *str) {
    size_t len = strlen(str);
    char *out = mx_mem_alloc(len + 1);
    memcpy(out, str, len + 1);
    return out;
}

mx_arena_t mx_arena_new(bool is_tracking) {
    return (mx_arena_t) {
        .is_tracking = is_tracking,
    };
}

void *mx_arena_take(mx_arena_t *arena, size_t size, size_t align) {
    size_t header = (sizeof(mx_arena_chunk_t) + MX_ARENA_ALIGN - 1) & ~(size_t)(MX_ARENA_ALIGN - 1);
    mx_arena_chunk_t *cur = arena->chunks;
    size_t aligned;
    if (cur != NULL) {
        aligned = (cur->used + align - 1) & ~(align - 1);
    } else {
        aligned = 0;
    }
    mx_arena_chunk_t *chunk;
    size_t at;
    if (cur == NULL || aligned + size > cur->size) {
        mx_arena_chunk_t *fresh;
        if (arena->spare != NULL && arena->spare->size >= size + header) {
            fresh = arena->spare;
            arena->spare = fresh->next;
        } else {
            size_t bytes = MX_ARENA_CHUNK_BYTES;
            while (bytes < size + header) {
                bytes *= 2;
            }
            fresh = mx_mem_alloc(bytes);
            fresh->size = bytes;
        }
        fresh->next = arena->chunks;
        fresh->used = header;
        arena->chunks = fresh;
        chunk = fresh;
        at = header;
    } else {
        chunk = cur;
        at = aligned;
    }
    void *ret = (uint8_t *)chunk + at;
    if (arena->is_tracking) {
        arena->total += (uint32_t)(at + size - chunk->used);
        arena->pad += (uint32_t)(at - chunk->used);
        arena->allocs += 1;
    }
    chunk->used = at + size;
    return ret;
}

void *mx_arena_alloc(mx_arena_t *arena, size_t size) {
    return mx_arena_take(arena, size, MX_ARENA_ALIGN);
}

void *mx_arena_alloc_bytes(mx_arena_t *arena, size_t size) {
    return mx_arena_take(arena, size, 1);
}

const char *mx_arena_strdup(mx_arena_t *arena, const char *str) {
    size_t len = strlen(str);
    char *out = mx_arena_alloc(arena, len + 1);
    memcpy(out, str, len + 1);
    return out;
}

mx_arena_mark_t mx_arena_mark(const mx_arena_t *arena) {
    if (arena->chunks == NULL) {
        return (mx_arena_mark_t) { .chunk = NULL, .used = 0 };
    } else {
        return (mx_arena_mark_t) { .chunk = arena->chunks, .used = arena->chunks->used };
    }
}

void mx_arena_release(mx_arena_t *arena, mx_arena_mark_t mark) {
    while (arena->chunks != mark.chunk) {
        mx_arena_chunk_t *chunk = arena->chunks;
        arena->chunks = chunk->next;
        chunk->next = arena->spare;
        arena->spare = chunk;
    }
    if (mark.chunk != NULL) {
        mark.chunk->used = mark.used;
    }
}

void mx_arena_free(mx_arena_t *arena) {
    mx_arena_release(arena, (mx_arena_mark_t) { .chunk = NULL, .used = 0 });
    mx_arena_chunk_t *chunk = arena->spare;
    while (chunk != NULL) {
        mx_arena_chunk_t *next = chunk->next;
        mx_mem_free(chunk);
        chunk = next;
    }
    arena->spare = NULL;
}

mx_t *mx_vm_new(void) {
    mx_t *vm = mx_mem_calloc(sizeof(mx_t));
    vm->arenas.ir = mx_arena_new(true);
    vm->arenas.code = mx_arena_new(false);
    vm->arenas.scratch = mx_arena_new(true);
    mx_scratch_init(vm);
    vm->gc = mx_gc_new();
    vm->gc->vm = vm;
    mx_kind_init(vm);
    mx_run_body(vm, &(mx_run_entry_t) { .start = MX_RUN_START_INIT });
    vm->main = mx_thread_new(vm, mx_obj_of_raw(MX_KIND_NIL, (mx_val_t) { .bits = 0 }));
    vm->main->status = MX_THREAD_RUNNING;
    vm->thread = vm->main;
    mx_vm_rets_reserve(vm, 1);
    mx_vm_pending_args(vm, 1);
    return vm;
}

void mx_vm_free(mx_t *vm) {
    mx_kind_free(vm);
    mx_gc_free(vm, vm->gc);
    mx_version_caches_free(vm);
    for (size_t i = 0; i < sizeof(vm->chunk_cache) / sizeof(vm->chunk_cache[0]); i++) {
        mx_mem_free(vm->chunk_cache[i].src);
    }
    mx_mem_free(vm->interns.slots);
    mx_mem_free(vm->interns.young.ptr);
    mx_mem_free(vm->code_scratch.ptr);
    mx_mem_free(vm->lower_scratch.types);
    mx_mem_free(vm->lower_scratch.fresh);
    mx_mem_free(vm->lower_scratch.recs.ptr);
    mx_mem_free(vm->lower_scratch.refs.ptr);
    mx_mem_free(vm->tbcs.ptr);
    mx_mem_free(vm->pending.args);
    mx_mem_free(vm->rets.vals);
    for (uint32_t i = 0; i < vm->rets_saved.len; i++) {
        mx_mem_free(vm->rets_saved.ptr[i].vals);
    }
    mx_mem_free(vm->rets_saved.ptr);
    mx_mem_free(vm->tmp.vals);
    mx_mem_free(vm->holds.ptr);
    mx_mem_free(vm->consts.ptr);
    mx_scratch_free(vm);
    mx_arena_free(&vm->arenas.scratch);
    mx_arena_free(&vm->arenas.code);
    mx_arena_free(&vm->arenas.ir);
    mx_mem_free(vm);
}

uint32_t mx_vm_grown_cap(uint32_t cap, uint32_t n) {
    if (cap * 2 + 16 < n) {
        return n;
    } else {
        return cap * 2 + 16;
    }
}

mx_obj_t *mx_vm_rets_reserve(mx_t *vm, uint32_t n) {
    if (n > vm->rets.cap) {
        uint32_t cap = mx_vm_grown_cap(vm->rets.cap, n);
        vm->rets.vals = mx_mem_realloc(vm->rets.vals, sizeof(mx_obj_t) * cap);
        vm->rets.cap = cap;
    }
    return vm->rets.vals;
}

void mx_vm_rets_set(mx_t *vm, const mx_obj_t *vals, uint32_t n) {
    if (vals != vm->rets.vals && n != 0) {
        mx_obj_t *out = mx_vm_rets_reserve(vm, n);
        memcpy(out, vals, sizeof(mx_obj_t) * n);
    }
    vm->rets.len = n;
}

void mx_vm_rets_prepend(mx_t *vm, mx_obj_t first, const mx_obj_t *vals, uint32_t n) {
    if (vals == vm->rets.vals) {
        mx_obj_t *out = mx_vm_rets_reserve(vm, n + 1);
        memmove(out + 1, out, sizeof(mx_obj_t) * n);
        out[0] = first;
    } else {
        mx_obj_t *out = mx_vm_rets_reserve(vm, n + 1);
        if (n != 0) {
            memcpy(out + 1, vals, sizeof(mx_obj_t) * n);
        }
        out[0] = first;
    }
    vm->rets.len = n + 1;
}

void mx_vm_rets_push(mx_t *vm) {
    if (vm->rets_saved.len == vm->rets_saved.cap) {
        vm->rets_saved.cap = vm->rets_saved.cap * 2 + 4;
        vm->rets_saved.ptr = mx_mem_realloc(vm->rets_saved.ptr, sizeof(mx_rets_t) * vm->rets_saved.cap);
    }
    vm->rets_saved.ptr[vm->rets_saved.len] = vm->rets;
    vm->rets_saved.len += 1;
    vm->rets = (mx_rets_t) {0};
    mx_vm_rets_reserve(vm, 1);
}

void mx_vm_rets_pop(mx_t *vm) {
    mx_mem_free(vm->rets.vals);
    vm->rets_saved.len -= 1;
    vm->rets = vm->rets_saved.ptr[vm->rets_saved.len];
}

mx_obj_t *mx_vm_tmp_reserve(mx_t *vm, uint32_t n) {
    if (n > vm->tmp.cap) {
        uint32_t cap = mx_vm_grown_cap(vm->tmp.cap, n);
        vm->tmp.vals = mx_mem_realloc(vm->tmp.vals, sizeof(mx_obj_t) * cap);
        vm->tmp.cap = cap;
    }
    vm->tmp.len = n;
    return vm->tmp.vals;
}

void mx_vm_hold(mx_t *vm, mx_obj_t v) {
    if (vm->holds.len == vm->holds.cap) {
        vm->holds.cap = vm->holds.cap * 2 + 8;
        vm->holds.ptr = mx_mem_realloc(vm->holds.ptr, sizeof(mx_obj_t) * vm->holds.cap);
    }
    vm->holds.ptr[vm->holds.len] = v;
    vm->holds.len += 1;
}

mx_obj_t mx_vm_release(mx_t *vm) {
    vm->holds.len -= 1;
    return vm->holds.ptr[vm->holds.len];
}

void mx_vm_keep(mx_t *vm, mx_obj_t v) {
    if (v.kind == MX_KIND_STRING) {
        mx_str_t *str = mx_ref_obj(v.val.ref);
        if (str->is_kept) {
            return;
        }
        str->is_kept = true;
    }
    if (vm->consts.len == vm->consts.cap) {
        vm->consts.cap = vm->consts.cap * 2 + 64;
        vm->consts.ptr = mx_mem_realloc(vm->consts.ptr, sizeof(mx_obj_t) * vm->consts.cap);
    }
    vm->consts.ptr[vm->consts.len] = v;
    vm->consts.len += 1;
}

mx_obj_t *mx_vm_pending_args(mx_t *vm, uint32_t n) {
    if (n > vm->pending.cap) {
        uint32_t cap = mx_vm_grown_cap(vm->pending.cap, n);
        vm->pending.args = mx_mem_realloc(vm->pending.args, sizeof(mx_obj_t) * cap);
        vm->pending.cap = cap;
    }
    return vm->pending.args;
}
