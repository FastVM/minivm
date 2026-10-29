
#include "gc.h"
#include "../ir/ir.h"
#include "../run/thread.h"
#include "../value/error.h"
#include "../value/kind.h"
#include "../value/obj.h"
#include "../value/string.h"
#include "../value/table.h"
#include "../version/version.h"

mx_gc_t *mx_gc_new(void) {
    mx_gc_t *gc = mx_mem_calloc(sizeof(mx_gc_t));
    gc->mark_value = 1;
    gc->cycle.threshold = MX_GC_NURSERY_MIN;
    gc->cycle.major_growth_factor = 2;
    gc->is_generational = true;
    gc->params[MX_GC_PARAM_PAUSE] = 250;
    gc->params[MX_GC_PARAM_STEPMUL] = 200;
    gc->params[MX_GC_PARAM_STEPSIZE] = 9600;
    gc->params[MX_GC_PARAM_MINORMUL] = 20;
    gc->params[MX_GC_PARAM_MAJORMINOR] = 50;
    gc->params[MX_GC_PARAM_MINORMAJOR] = 68;
    return gc;
}

#if MX_GC_STAT

void mx_gc_stat_dump(mx_t *vm) {
    mx_gc_t *gc = vm->gc;
    size_t nlarge = 0;
    size_t nfree = 0;
    for (mx_gc_large_t *l = gc->larges.list; l != NULL; l = l->next) {
        nlarge += 1;
    }
    for (mx_gc_block_t *b = gc->blocks.free; b != NULL; b = b->next) {
        nfree += 1;
    }
    fprintf(
        stderr,
        "gc: collections=%zu blocks=%zu free=%zu large=%zu peak_blocks=%zu peak_live=%zu pool=%zu threshold=%zu threads=%zu code=%u ir_total=%u ir_pad=%u ir_allocs=%u ir_blocks=%zu versions=%zu version_bytes=%zu block_structs=%u sealed=%u branches=%u sources=%u names=%u block_struct=%zu scratch=%u consts=%u\n",
        gc->stats.collections,
        gc->blocks.count,
        nfree,
        nlarge,
        gc->stats.peak_blocks,
        gc->stats.peak_live_bytes,
        gc->pool.bytes,
        gc->cycle.threshold,
        gc->threads.len,
        vm->arenas.code.hops,
        vm->arenas.ir.total,
        vm->arenas.ir.pad,
        vm->arenas.ir.allocs,
        (size_t)vm->next_block_id,
        (size_t)vm->stats.versions,
        (size_t)vm->stats.version_bytes,
        vm->arenas.ir.blocks,
        vm->arenas.ir.sealed,
        vm->arenas.ir.branches,
        vm->arenas.ir.sources,
        vm->arenas.ir.names,
        sizeof(mx_ir_block_t),
        vm->arenas.scratch.total,
        vm->consts.len
    );
    fprintf(
        stderr,
        "run: deliver_fast=%" PRIu64 " deliver_resumed_fast=%" PRIu64 " deliver_found=%" PRIu64 " deliver_new=%" PRIu64 " deliver_void=%" PRIu64 " sk_hit=%" PRIu64 " sk_miss=%" PRIu64 " call_lua=%" PRIu64 " call_ffi=%" PRIu64 " call_fast=%" PRIu64 "\n",
        vm->stats.deliver_fast,
        vm->stats.deliver_resumed_fast,
        vm->stats.deliver_found,
        vm->stats.deliver_new,
        vm->stats.deliver_void,
        vm->stats.sk_hit,
        vm->stats.sk_miss,
        vm->stats.call_lua,
        vm->stats.call_ffi,
        vm->stats.call_fast
    );
    size_t hist[6] = {0};
    const mx_version_cache_t *top[8] = {NULL};
    for (const mx_version_cache_t *c = vm->version_caches; c != NULL; c = c->next) {
        uint32_t n = c->list.len;
        size_t at;
        if (n <= 2) {
            at = n - 1;
        } else if (n <= 4) {
            at = 2;
        } else if (n <= 8) {
            at = 3;
        } else if (n <= 16) {
            at = 4;
        } else {
            at = 5;
        }
        if (n != 0) {
            hist[at] += 1;
        }
        bool is_placed = n == 0;
        for (size_t i = 0; i < 8 && !is_placed; i++) {
            if (top[i] == NULL || top[i]->list.len < n) {
                memmove(&top[i + 1], &top[i], sizeof(top[0]) * (7 - i));
                top[i] = c;
                is_placed = true;
            }
        }
    }
    fprintf(stderr, "versions: per-block 1=%zu 2=%zu 3-4=%zu 5-8=%zu 9-16=%zu 17+=%zu\n", hist[0], hist[1], hist[2], hist[3], hist[4], hist[5]);
    for (size_t i = 0; i < 8 && top[i] != NULL; i++) {
        const mx_versions_t *list = &top[i]->list;
        const mx_ir_block_t *block = list->ptr[0]->block;
        uint32_t nregs = block->func->nregs;
        uint32_t varying = 0;
        for (uint32_t r = 0; r < nregs; r++) {
            mx_kind_t first = MX_KIND_NIL;
            if (r < list->ptr[0]->nkinds) {
                first = list->ptr[0]->kinds[r];
            }
            bool is_varying = false;
            for (uint32_t v = 1; v < list->len && !is_varying; v++) {
                mx_kind_t k = MX_KIND_NIL;
                if (r < list->ptr[v]->nkinds) {
                    k = list->ptr[v]->kinds[r];
                }
                is_varying = k != first;
            }
            varying += is_varying;
        }
        mx_location_range_t range = mx_ir_block_range(block, 0);
        fprintf(stderr, "  %s:%u block %u: %u versions, %u of %u registers vary\n", block->func->source->file, mx_source_line(block->func->source, range.start), block->id, list->len, varying, nregs);
    }
}

#endif

void mx_gc_finalize_all(mx_t *vm, mx_gc_t *gc) {
    for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
        for (size_t s = 0; s < MX_GC_OBJ_BITMAP_BYTES; s++) {
            uint8_t bits = b->obj_start[s];
            for (size_t k = 0; k < 8; k++) {
                if ((bits & (uint8_t)((uint32_t)1 << k)) != 0) {
                    mx_obj_head_t *h = (mx_obj_head_t *)((uint8_t *)b + (s * 8 + k) * 8);
                    if (h->kind == MX_KIND_STRING) {
                        mx_str_t *buf = (mx_str_t *)h;
                        buf->is_interned = false;
                        buf->is_pooled = false;
                    }
                    mx_gc_finalize(vm, h);
                }
            }
        }
    }
    size_t header = MX_GC_LARGE_HEADER;
    for (mx_gc_large_t *lg = gc->larges.list; lg != NULL; lg = lg->next) {
        mx_obj_head_t *h = (mx_obj_head_t *)((uint8_t *)lg + header);
        if (h->kind == MX_KIND_STRING) {
            mx_str_t *buf = (mx_str_t *)h;
            buf->is_interned = false;
            buf->is_pooled = false;
        }
        mx_gc_finalize(vm, h);
    }
    for (int32_t c = MX_GC_POOL_SLAB_CLASSES; c < MX_GC_POOL_CLASSES; c++) {
        void *p = gc->pool.free[c];
        while (p != NULL) {
            void *next = *(void **)p;
            mx_mem_free(p);
            p = next;
        }
        gc->pool.free[c] = NULL;
    }
    gc->pool.free_bytes = 0;
    void *slab = gc->pool.slabs;
    while (slab != NULL) {
        void *next = *(void **)slab;
        mx_mem_free(slab);
        slab = next;
    }
    gc->pool.slabs = NULL;
    for (int32_t c = 0; c < MX_GC_POOL_SLAB_CLASSES; c++) {
        gc->pool.free[c] = NULL;
    }
}

void mx_gc_free(mx_t *vm, mx_gc_t *gc) {
#if MX_GC_STAT
    mx_gc_stat_dump(vm);
#endif
    mx_gc_finalize_all(vm, gc);
    mx_gc_large_t *lg = gc->larges.list;
    while (lg != NULL) {
        mx_gc_large_t *next = lg->next;
        mx_mem_free(lg);
        lg = next;
    }
    for (size_t i = 0; i < gc->slabs.len; i++) {
        mx_mem_free(gc->slabs.ptr[i]);
    }
    mx_mem_free(gc->slabs.ptr);
    mx_mem_free(gc->block_index.ptr);
    mx_mem_free(gc->larges.index.ptr);
    mx_mem_free(gc->gray.ptr);
    mx_mem_free(gc->dirty.ptr);
    mx_mem_free(gc->dirty.scan);
    mx_mem_free(gc->finobjs.ptr);
    mx_mem_free(gc->tobefnz.ptr);
    mx_mem_free(gc->weaks.ptr);
    mx_mem_free(gc->threads.ptr);
    mx_mem_free(gc->ident_keys.ptr);
    mx_mem_free(gc->pinned.ptr);
    mx_mem_free(gc->roots.ptr);
    mx_mem_free(gc);
}

size_t mx_gc_block_index_slot(mx_gc_t *gc, uintptr_t base) {
    uint64_t h = (uint64_t)(base / MX_GC_BLOCK_SIZE) * MX_GC_HASH_MULT;
    return (size_t)(h >> 32) & (gc->block_index.len - 1);
}

void mx_gc_block_index_rebuild(mx_gc_t *gc) {
    size_t cap = 64;
    while (cap < gc->blocks.count * 2) {
        cap *= 2;
    }
    if (cap > gc->block_index.cap) {
        gc->block_index.cap = cap;
        gc->block_index.ptr = mx_mem_realloc(gc->block_index.ptr, sizeof(uintptr_t) * gc->block_index.cap);
    }
    gc->block_index.len = cap;
    memset(gc->block_index.ptr, 0, sizeof(uintptr_t) * cap);
    for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
        size_t i = mx_gc_block_index_slot(gc, (uintptr_t)b);
        while (gc->block_index.ptr[i] != 0) {
            i = (i + 1) & (cap - 1);
        }
        gc->block_index.ptr[i] = (uintptr_t)b;
    }
    gc->block_index.is_dirty = false;
    for (size_t i = 0; i < MX_GC_OWNED_CACHE_SLOTS; i++) {
        gc->owned_cache[i] = 0;
    }
}

bool mx_gc_block_owned(mx_gc_t *gc, void *ptr) {
    uintptr_t base = (uintptr_t)ptr & ~(MX_GC_BLOCK_SIZE - 1);
    if (gc->block_index.is_dirty) {
        mx_gc_block_index_rebuild(gc);
    }
    size_t slot = (base / MX_GC_BLOCK_SIZE) & (MX_GC_OWNED_CACHE_SLOTS - 1);
    if (base != 0 && gc->owned_cache[slot] == base) {
        return true;
    } else if (base == 0 || gc->block_index.len == 0) {
        return false;
    }
    size_t mask = gc->block_index.len - 1;
    size_t i = mx_gc_block_index_slot(gc, base);
    while (true) {
        uintptr_t got = gc->block_index.ptr[i];
        if (got == 0) {
            return false;
        } else if (got == base) {
            gc->owned_cache[slot] = base;
            return true;
        }
        i = (i + 1) & mask;
    }
}

void mx_gc_obj_start_set(mx_gc_block_t *block, void *ptr) {
    size_t slot = ((uintptr_t)ptr - (uintptr_t)block) / 8;
    block->obj_start[slot >> 3] |= (uint8_t)((uint32_t)1 << (slot & 7));
}

bool mx_gc_obj_start_test(mx_gc_block_t *block, void *ptr) {
    size_t slot = ((uintptr_t)ptr - (uintptr_t)block) / 8;
    return (block->obj_start[slot >> 3] & (uint8_t)((uint32_t)1 << (slot & 7))) != 0;
}

uint8_t *mx_gc_block_data(mx_gc_block_t *block) {
    return (uint8_t *)block + MX_GC_BLOCK_HEADER_LINES * MX_GC_LINE_SIZE;
}

size_t mx_gc_line_of(mx_gc_block_t *block, void *ptr) {
    return ((uint8_t *)ptr - (uint8_t *)block) / MX_GC_LINE_SIZE;
}

mx_gc_block_t *mx_gc_map_aligned_block(mx_gc_t *gc) {
    size_t bs = MX_GC_BLOCK_SIZE;
    if (gc->slabs.cursor == NULL || gc->slabs.cursor + bs > gc->slabs.end) {
        size_t slab = MX_GC_SLAB_BYTES;
        uint8_t *raw = mx_mem_calloc(slab + bs + sizeof(void *));
        uintptr_t at = (uintptr_t)raw + sizeof(void *);
        uintptr_t aligned = (at + (bs - 1)) & ~(uintptr_t)(bs - 1);
        if (gc->slabs.len >= gc->slabs.cap) {
            gc->slabs.cap = (gc->slabs.len + 1) * 2;
            gc->slabs.ptr = mx_mem_realloc(gc->slabs.ptr, sizeof(void *) * gc->slabs.cap);
        }
        gc->slabs.ptr[gc->slabs.len] = raw;
        gc->slabs.len += 1;
        gc->slabs.cursor = (uint8_t *)aligned;
        gc->slabs.end = (uint8_t *)aligned + slab;
    }
    mx_gc_block_t *block = (mx_gc_block_t *)gc->slabs.cursor;
    gc->slabs.cursor += bs;
    return block;
}

mx_gc_block_t *mx_gc_new_block(mx_gc_t *gc) {
    mx_gc_block_t *block;
    if (gc->blocks.free != NULL) {
        block = gc->blocks.free;
        gc->blocks.free = block->next;
    } else {
        block = mx_gc_map_aligned_block(gc);
    }
    block->magic = MX_GC_BLOCK_MAGIC;
    block->final_count = 0;
    block->old_head = NULL;
    block->is_evac = false;
    block->is_old = false;
    block->is_dirty = false;
    block->has_young = true;
    memset(block->line_mark, 0, MX_GC_LINES_PER_BLOCK);
    memset(block->obj_start, 0, MX_GC_OBJ_BITMAP_BYTES);
    block->next = gc->blocks.live;
    gc->blocks.live = block;
    gc->blocks.count += 1;
    if (gc->blocks.count > gc->stats.peak_blocks) {
        gc->stats.peak_blocks = gc->blocks.count;
    }
    if (!gc->block_index.is_dirty && gc->block_index.len != 0 && gc->block_index.len >= gc->blocks.count * 2) {
        size_t mask = gc->block_index.len - 1;
        size_t i = mx_gc_block_index_slot(gc, (uintptr_t)block);
        while (gc->block_index.ptr[i] != 0) {
            i = (i + 1) & mask;
        }
        gc->block_index.ptr[i] = (uintptr_t)block;
    } else {
        gc->block_index.is_dirty = true;
    }
    return block;
}

bool mx_gc_next_hole_in_block(mx_gc_t *gc, mx_gc_block_t *block, size_t from_line) {
    if (from_line < MX_GC_BLOCK_HEADER_LINES) {
        from_line = MX_GC_BLOCK_HEADER_LINES;
    }
    for (size_t i = from_line; i < MX_GC_LINES_PER_BLOCK; i++) {
        if (block->line_mark[i] == 0) {
            size_t end = i + 1;
            while (end < MX_GC_LINES_PER_BLOCK && block->line_mark[end] == 0) {
                end += 1;
            }
            block->has_young = true;
            gc->small.block = block;
            gc->small.cursor = (uint8_t *)block + i * MX_GC_LINE_SIZE;
            gc->small.limit = (uint8_t *)block + end * MX_GC_LINE_SIZE;
            return true;
        }
    }
    return false;
}

void mx_gc_find_hole(mx_gc_t *gc) {
    bool found = false;
    if (gc->small.block != NULL && gc->small.limit != NULL) {
        size_t next_line = mx_gc_line_of(gc->small.block, gc->small.limit);
        found = mx_gc_next_hole_in_block(gc, gc->small.block, next_line);
    }
    while (!found && gc->blocks.recycle != NULL) {
        mx_gc_block_t *block = gc->blocks.recycle;
        gc->blocks.recycle = block->recycle_next;
        found = mx_gc_next_hole_in_block(gc, block, MX_GC_BLOCK_HEADER_LINES);
    }
    if (!found) {
        mx_gc_block_t *block = mx_gc_new_block(gc);
        gc->small.block = block;
        gc->small.cursor = mx_gc_block_data(block);
        gc->small.limit = (uint8_t *)block + MX_GC_BLOCK_SIZE;
    }
}

void mx_gc_note_alloc(mx_t *vm, size_t bytes) {
    mx_gc_t *gc = vm->gc;
    gc->cycle.bytes_since_gc += bytes;
#if MX_GC_STRESS
    vm->gc_pending = 1;
    vm->interrupt = 1;
#else
    if (gc->cycle.bytes_since_gc >= gc->cycle.threshold) {
        vm->gc_pending = 1;
        vm->interrupt = 1;
    }
#endif
}

void *mx_gc_alloc_large(mx_t *vm, size_t size) {
    mx_gc_t *gc = vm->gc;
    size_t header = MX_GC_LARGE_HEADER;
    size_t total = header + size;
    mx_gc_large_t *lg = mx_mem_calloc(total);
    lg->size = total;
    lg->mark = 0;
    lg->next = gc->larges.list;
    gc->larges.list = lg;
    gc->larges.index.is_dirty = true;
    void *payload = (uint8_t *)lg + header;
    mx_gc_note_alloc(vm, total);
    return payload;
}

mx_gc_large_t *mx_gc_large_of(void *payload) {
    size_t header = MX_GC_LARGE_HEADER;
    return (mx_gc_large_t *)((uint8_t *)payload - header);
}

void *mx_gc_alloc(mx_t *vm, size_t size) {
    mx_gc_t *gc = vm->gc;
    size = (size + 7) & ~(size_t)7;
    if (size > MX_GC_MEDIUM_MAX) {
        return mx_gc_alloc_large(vm, size);
    } else if (size > MX_GC_LINE_SIZE) {
        while (gc->overflow.cursor == NULL || gc->overflow.cursor + size > gc->overflow.limit) {
            mx_gc_block_t *block = mx_gc_new_block(gc);
            gc->overflow.block = block;
            gc->overflow.cursor = mx_gc_block_data(block);
            gc->overflow.limit = (uint8_t *)block + MX_GC_BLOCK_SIZE;
        }
        void *ret = gc->overflow.cursor;
        gc->overflow.cursor += size;
        mx_gc_obj_start_set(gc->overflow.block, ret);
        mx_gc_head_clear_flags(ret);
        mx_gc_note_alloc(vm, size);
        return ret;
    } else {
        while (gc->small.cursor == NULL || gc->small.cursor + size > gc->small.limit) {
            mx_gc_find_hole(gc);
        }
        void *ret = gc->small.cursor;
        gc->small.cursor += size;
        mx_gc_obj_start_set(gc->small.block, ret);
        mx_gc_head_clear_flags(ret);
        mx_gc_note_alloc(vm, size);
        return ret;
    }
}

void mx_gc_note_bytes(mx_t *vm, size_t bytes) {
    mx_gc_t *gc = vm->gc;
    gc->cycle.bytes_since_gc += bytes;
    if (gc->cycle.bytes_since_gc >= gc->cycle.threshold) {
        vm->gc_pending = 1;
        vm->interrupt = 1;
    }
}

void mx_gc_note_final(mx_t *vm, mx_obj_head_t *head) {
    if (!head->is_final && mx_gc_block_owned(vm->gc, head)) {
        mx_gc_note_final_owned(head);
    } else {
        head->is_final = true;
    }
}

void mx_gc_note_final_owned(mx_obj_head_t *head) {
    if (!head->is_final) {
        head->is_final = true;
        mx_gc_block_t *b = (mx_gc_block_t *)((uintptr_t)head & ~(MX_GC_BLOCK_SIZE - 1));
        b->final_count += 1;
    }
}

void mx_gc_note_thread(mx_t *vm, mx_thread_t *th) {
    mx_gc_t *gc = vm->gc;
    if (gc->threads.len >= gc->threads.cap) {
        gc->threads.cap = gc->threads.cap * 2 + 8;
        gc->threads.ptr = mx_mem_realloc(gc->threads.ptr, sizeof(mx_thread_t *) * gc->threads.cap);
    }
    gc->threads.ptr[gc->threads.len] = th;
    gc->threads.len += 1;
}

bool mx_gc_is_heap(mx_gc_t *gc, void *ptr) {
    return mx_gc_block_owned(gc, ptr) || mx_gc_large_owns(gc, ptr);
}

void mx_gc_dirty_block(mx_gc_t *gc, mx_gc_block_t *b) {
    if (!b->is_dirty) {
        b->is_dirty = true;
        if (gc->dirty.len >= gc->dirty.cap) {
            gc->dirty.cap = (gc->dirty.len + 1) * 2;
            gc->dirty.ptr = mx_mem_realloc(gc->dirty.ptr, sizeof(void *) * gc->dirty.cap);
        }
        gc->dirty.ptr[gc->dirty.len] = b;
        gc->dirty.len += 1;
    }
}

void mx_gc_write_barrier_slow(mx_t *vm, void *container) {
    mx_gc_t *gc = vm->gc;
    uintptr_t base = (uintptr_t)container & ~(MX_GC_BLOCK_SIZE - 1);
    if (base != gc->dirty.last_base && mx_gc_block_owned(gc, container)) {
        mx_gc_dirty_block(gc, (mx_gc_block_t *)base);
        gc->dirty.last_base = base;
    }
}

int32_t mx_gc_pool_class(size_t bytes) {
    if (bytes <= ((size_t)1 << MX_GC_POOL_MIN_SHIFT)) {
        return 0;
    } else {
        uint64_t v = (uint64_t)(bytes - 1) >> MX_GC_POOL_MIN_SHIFT;
        int32_t cls = 0;
        while (v != 0 && cls < MX_GC_POOL_CLASSES) {
            v >>= 1;
            cls += 1;
        }
        if (cls >= MX_GC_POOL_CLASSES) {
            return -1;
        } else {
            return cls;
        }
    }
}

void mx_gc_pool_slab_refill(mx_gc_t *gc, int32_t cls) {
    uint8_t *slab = mx_mem_alloc(MX_GC_POOL_SLAB_BYTES);
    *(void **)slab = gc->pool.slabs;
    gc->pool.slabs = slab;
    size_t chunk = (size_t)1 << (cls + MX_GC_POOL_MIN_SHIFT);
    for (size_t at = MX_GC_POOL_SLAB_HEADER; at + chunk <= MX_GC_POOL_SLAB_BYTES; at += chunk) {
        *(void **)(slab + at) = gc->pool.free[cls];
        gc->pool.free[cls] = slab + at;
    }
}

void *mx_gc_pool_alloc(mx_t *vm, size_t bytes) {
    mx_gc_t *gc = vm->gc;
    gc->pool.bytes += bytes;
    int32_t cls = mx_gc_pool_class(bytes);
    if (cls < 0) {
        return mx_mem_alloc(bytes);
    } else if (gc->pool.free[cls] != NULL) {
        void *ret = gc->pool.free[cls];
        gc->pool.free[cls] = *(void **)ret;
        if (cls >= MX_GC_POOL_SLAB_CLASSES) {
            gc->pool.free_bytes -= (size_t)1 << (cls + MX_GC_POOL_MIN_SHIFT);
        }
        return ret;
    } else if (cls < MX_GC_POOL_SLAB_CLASSES) {
        mx_gc_pool_slab_refill(gc, cls);
        void *ret = gc->pool.free[cls];
        gc->pool.free[cls] = *(void **)ret;
        return ret;
    } else {
        return mx_mem_alloc((size_t)1 << (cls + MX_GC_POOL_MIN_SHIFT));
    }
}

void *mx_gc_pool_realloc(mx_t *vm, void *ptr, size_t old_bytes, size_t bytes) {
    mx_gc_t *gc = vm->gc;
    int32_t old_cls = mx_gc_pool_class(old_bytes);
    int32_t cls = mx_gc_pool_class(bytes);
    bool old_is_slab = old_cls >= 0 && old_cls < MX_GC_POOL_SLAB_CLASSES;
    bool is_slab = cls >= 0 && cls < MX_GC_POOL_SLAB_CLASSES;
    if (ptr == NULL || old_is_slab || is_slab) {
        void *ret = mx_gc_pool_alloc(vm, bytes);
        if (ptr != NULL) {
            memcpy(ret, ptr, old_bytes);
            mx_gc_pool_free(vm, ptr, old_bytes);
        }
        return ret;
    } else {
        gc->pool.bytes = gc->pool.bytes - old_bytes + bytes;
        if (old_cls == cls && cls >= 0) {
            return ptr;
        } else {
            size_t want;
            if (cls >= 0) {
                want = (size_t)1 << (cls + MX_GC_POOL_MIN_SHIFT);
            } else {
                want = bytes;
            }
            return mx_mem_realloc(ptr, want);
        }
    }
}

void mx_gc_pool_free(mx_t *vm, void *ptr, size_t bytes) {
    mx_gc_t *gc = vm->gc;
    if (ptr != NULL) {
        gc->pool.bytes -= bytes;
        int32_t cls = mx_gc_pool_class(bytes);
        if (cls < 0) {
            mx_mem_free(ptr);
        } else {
            *(void **)ptr = gc->pool.free[cls];
            gc->pool.free[cls] = ptr;
            if (cls >= MX_GC_POOL_SLAB_CLASSES) {
                gc->pool.free_bytes += (size_t)1 << (cls + MX_GC_POOL_MIN_SHIFT);
            }
        }
    }
}

size_t mx_gc_bytes(mx_t *vm) {
    mx_gc_t *gc = vm->gc;
    return gc->cycle.live_bytes + gc->cycle.bytes_since_gc + gc->pool.bytes;
}

size_t mx_gc_budget(const mx_gc_t *gc, size_t live_bytes) {
    size_t budget;
    if (gc->is_generational) {
        size_t nursery = live_bytes * 2 + gc->pool.bytes / 4;
        if (nursery < MX_GC_NURSERY_MIN) {
            nursery = MX_GC_NURSERY_MIN;
        } else if (nursery > MX_GC_NURSERY) {
            nursery = MX_GC_NURSERY;
        }
        budget = (live_bytes + nursery + gc->cycle.old_live_after_major / 16) / 20 * gc->params[MX_GC_PARAM_MINORMUL];
    } else if (gc->params[MX_GC_PARAM_PAUSE] > 100) {
        budget = live_bytes / 100 * (gc->params[MX_GC_PARAM_PAUSE] - 100);
    } else {
        budget = 0;
    }
    if (budget < MX_GC_NURSERY_MIN) {
        return MX_GC_NURSERY_MIN;
    } else {
        return budget;
    }
}

bool mx_gc_step(mx_t *vm, size_t kilobytes) {
    mx_gc_t *gc = vm->gc;
    gc->cycle.bytes_since_gc += kilobytes * 1024;
    if (kilobytes == 0 || mx_gc_over_budget(gc)) {
        mx_gc_collect(vm);
        return !gc->is_generational;
    } else {
        return false;
    }
}

size_t mx_gc_large_index_slot(mx_gc_t *gc, uintptr_t payload) {
    uint64_t h = (uint64_t)(payload >> 4) * MX_GC_HASH_MULT;
    return (size_t)(h >> 32) & (gc->larges.index.len - 1);
}

void mx_gc_large_index_rebuild(mx_gc_t *gc) {
    size_t header = MX_GC_LARGE_HEADER;
    size_t n = 0;
    for (mx_gc_large_t *lg = gc->larges.list; lg != NULL; lg = lg->next) {
        n += 1;
    }
    size_t cap = 64;
    while (cap < n * 2) {
        cap *= 2;
    }
    if (cap > gc->larges.index.cap) {
        gc->larges.index.cap = cap;
        gc->larges.index.ptr = mx_mem_realloc(gc->larges.index.ptr, sizeof(uintptr_t) * gc->larges.index.cap);
    }
    gc->larges.index.len = cap;
    memset(gc->larges.index.ptr, 0, sizeof(uintptr_t) * cap);
    for (mx_gc_large_t *lg = gc->larges.list; lg != NULL; lg = lg->next) {
        uintptr_t payload = (uintptr_t)((uint8_t *)lg + header);
        size_t i = mx_gc_large_index_slot(gc, payload);
        while (gc->larges.index.ptr[i] != 0) {
            i = (i + 1) & (cap - 1);
        }
        gc->larges.index.ptr[i] = payload;
    }
    gc->larges.index.is_dirty = false;
}

bool mx_gc_large_owns(mx_gc_t *gc, void *ptr) {
    if (gc->larges.index.is_dirty || gc->larges.index.len == 0) {
        mx_gc_large_index_rebuild(gc);
    }
    uintptr_t want = (uintptr_t)ptr;
    size_t mask = gc->larges.index.len - 1;
    size_t i = mx_gc_large_index_slot(gc, want);
    while (true) {
        uintptr_t got = gc->larges.index.ptr[i];
        if (got == 0) {
            return false;
        } else if (got == want) {
            return true;
        }
        i = (i + 1) & mask;
    }
}

size_t mx_gc_obj_size(mx_t *vm, mx_obj_head_t *head) {
    if (head->kind == MX_KIND_CLOSURE) {
        mx_obj_closure_t *c = (mx_obj_closure_t *)head;
        return sizeof(mx_obj_closure_t) + sizeof(mx_obj_upval_t *) * c->len;
    } else if (head->kind == MX_KIND_STRING) {
        mx_str_t *buf = (mx_str_t *)head;
        if (buf->is_tail) {
            return MX_STR_TAIL_HEADER + buf->cap;
        } else {
            return sizeof(mx_str_t);
        }
    } else if (head->kind == MX_KIND_ERROR) {
        return sizeof(mx_error_t);
    } else if (head->kind >= MX_KIND_FIRST_DATA && head->is_container) {
        if (((mx_obj_table_t *)head)->is_short) {
            return MX_TABLE_SHORT_SIZE;
        } else {
            return sizeof(mx_obj_table_t);
        }
    } else if (head->kind == vm->kinds.cfn) {
        mx_obj_cfn_t *f = (mx_obj_cfn_t *)head;
        return sizeof(mx_obj_cfn_t) + sizeof(mx_obj_t) * f->len;
    } else if (head->kind == vm->kinds.upval) {
        return sizeof(mx_obj_upval_t);
    } else if (head->kind == vm->kinds.thread) {
        return sizeof(mx_thread_t);
    }
    mx_fatal("gc: object with unexpected kind");
    return 0;
}

void *mx_gc_forward_get(mx_obj_head_t *head) {
    return *(void **)((uint8_t *)head + 8);
}

void mx_gc_forward_set(mx_obj_head_t *head, void *to) {
    *(void **)((uint8_t *)head + 8) = to;
}

void *mx_gc_evac_alloc(mx_gc_t *gc, size_t size) {
    while (gc->evac.cursor == NULL || gc->evac.cursor + size > gc->evac.limit) {
        mx_gc_block_t *block = mx_gc_new_block(gc);
        block->is_evac = false;
        gc->evac.block = block;
        gc->evac.cursor = mx_gc_block_data(block);
        gc->evac.limit = (uint8_t *)block + MX_GC_BLOCK_SIZE;
    }
    void *ret = gc->evac.cursor;
    gc->evac.cursor += size;
    mx_gc_obj_start_set(gc->evac.block, ret);
    gc->evac_bytes += size;
    return ret;
}

size_t mx_gc_ident_key_slot(const mx_gc_t *gc, uintptr_t p) {
    uint64_t h = (uint64_t)(p >> 4) * MX_GC_HASH_MULT;
    return (size_t)(h >> 32) & (gc->ident_keys.cap - 1);
}

bool mx_gc_ident_key_has(const mx_gc_t *gc, void *obj) {
    if (gc->ident_keys.len == 0) {
        return false;
    }
    size_t i = mx_gc_ident_key_slot(gc, (uintptr_t)obj);
    while (gc->ident_keys.ptr[i] != 0) {
        if (gc->ident_keys.ptr[i] == (uintptr_t)obj) {
            return true;
        }
        i = (i + 1) & (gc->ident_keys.cap - 1);
    }
    return false;
}

void mx_gc_note_ident_key(mx_t *vm, void *obj) {
    mx_gc_t *gc = vm->gc;
    if ((uintptr_t)obj != gc->ident_keys.last) {
        gc->ident_keys.last = (uintptr_t)obj;
        if (gc->ident_keys.len * 2 >= gc->ident_keys.cap) {
            size_t ocap = gc->ident_keys.cap;
            uintptr_t *old = gc->ident_keys.ptr;
            gc->ident_keys.cap = ocap * 2;
            if (ocap == 0) {
                gc->ident_keys.cap = MX_GC_IDENT_KEYS_INITIAL;
            }
            gc->ident_keys.ptr = mx_mem_calloc(sizeof(uintptr_t) * gc->ident_keys.cap);
            gc->ident_keys.len = 0;
            for (size_t i = 0; i < ocap; i++) {
                if (old[i] != 0) {
                    size_t at = mx_gc_ident_key_slot(gc, old[i]);
                    while (gc->ident_keys.ptr[at] != 0) {
                        at = (at + 1) & (gc->ident_keys.cap - 1);
                    }
                    gc->ident_keys.ptr[at] = old[i];
                    gc->ident_keys.len += 1;
                }
            }
            mx_mem_free(old);
        }
        size_t i = mx_gc_ident_key_slot(gc, (uintptr_t)obj);
        bool present = false;
        while (gc->ident_keys.ptr[i] != 0 && !present) {
            present = gc->ident_keys.ptr[i] == (uintptr_t)obj;
            if (!present) {
                i = (i + 1) & (gc->ident_keys.cap - 1);
            }
        }
        if (!present) {
            gc->ident_keys.ptr[i] = (uintptr_t)obj;
            gc->ident_keys.len += 1;
        }
    }
}

void *mx_gc_maybe_evacuate(mx_t *vm, mx_gc_t *gc, void *obj) {
    if (!gc->is_evacuating || !mx_gc_block_owned(gc, obj)) {
        return obj;
    }
    mx_obj_head_t *head = obj;
    if (head->is_forwarded) {
        return mx_gc_forward_get(head);
    } else if (head->kind >= MX_KIND_FIRST_DATA && !head->is_container) {
        return obj;
    }
    mx_gc_block_t *block = (mx_gc_block_t *)((uintptr_t)obj & ~(MX_GC_BLOCK_SIZE - 1));
    if (!block->is_evac || head->is_pinned || mx_gc_ident_key_has(gc, obj) || head->mark == gc->mark_value) {
        return obj;
    }
    size_t size = mx_gc_obj_size(vm, head);
    void *to = mx_gc_evac_alloc(gc, size);
    memcpy(to, obj, size);
    if (head->kind >= MX_KIND_FIRST_DATA && head->is_container) {
        mx_obj_table_t *from_table = obj;
        mx_obj_table_t *to_table = to;
        if (from_table->array == from_table->inline_array) {
            to_table->array = to_table->inline_array;
        }
    }
    head->is_forwarded = true;
    mx_gc_forward_set(head, to);
    mx_gc_mark_lines_block(block, obj, size, gc->mark_value);
    return to;
}

mx_obj_head_t **mx_gc_next_slot(mx_obj_head_t *head) {
    return (mx_obj_head_t **)((uint8_t *)head + offsetof(mx_obj_table_t, gc_next));
}

void mx_gc_old_record_block(mx_gc_block_t *block, mx_obj_head_t *head) {
    *mx_gc_next_slot(head) = block->old_head;
    block->old_head = head;
}

void mx_gc_promote_block(mx_gc_t *gc, mx_obj_head_t *head, mx_gc_block_t *block) {
    if (head->is_old) {
        if (!gc->is_minor && block != NULL) {
            block->is_old = true;
            mx_gc_dirty_block(gc, block);
            mx_gc_old_record_block(block, head);
        }
    } else if (head->age < MX_GC_TENURE_AGE) {
        head->age += 1;
    } else {
        head->is_old = true;
        if (block != NULL) {
            block->is_old = true;
            mx_gc_dirty_block(gc, block);
            mx_gc_old_record_block(block, head);
        }
        gc->cycle.promoted_bytes += mx_gc_obj_size(gc->vm, head);
    }
}

void mx_gc_gray_push(mx_gc_t *gc, mx_obj_head_t *head) {
    if (gc->gray.len >= gc->gray.cap) {
        gc->gray.cap = (gc->gray.len + 1) * 2;
        gc->gray.ptr = mx_mem_realloc(gc->gray.ptr, sizeof(mx_obj_head_t *) * gc->gray.cap);
    }
    gc->gray.ptr[gc->gray.len] = head;
    gc->gray.len += 1;
}

bool mx_gc_kind_is_ref(mx_t *vm, mx_kind_t kind) {
    return kind == MX_KIND_STRING || kind == MX_KIND_CLOSURE || kind == MX_KIND_ERROR || (kind >= MX_KIND_FIRST_DATA && kind < vm->kinds.len);
}

void mx_gc_pin(mx_gc_t *gc, mx_obj_head_t *head) {
    if (!head->is_pinned) {
        head->is_pinned = true;
        if (gc->pinned.len >= gc->pinned.cap) {
            gc->pinned.cap = (gc->pinned.len + 1) * 2;
            gc->pinned.ptr = mx_mem_realloc(gc->pinned.ptr, sizeof(mx_obj_head_t *) * gc->pinned.cap);
        }
        gc->pinned.ptr[gc->pinned.len] = head;
        gc->pinned.len += 1;
    }
}

void mx_gc_mark_obj(mx_gc_t *gc, mx_obj_head_t *head) {
    if (head->mark == gc->mark_value) {
        return;
    }
    head->mark = gc->mark_value;
    if (gc->is_minor && head->is_old) {
        return;
    }
    size_t size = mx_gc_obj_size(gc->vm, head);
    mx_gc_block_t *block = NULL;
    if (mx_gc_block_owned(gc, head)) {
        block = (mx_gc_block_t *)((uintptr_t)head & ~(MX_GC_BLOCK_SIZE - 1));
#if MX_GC_VERIFY
        if (!mx_gc_obj_start_test(block, head)) {
            fprintf(stderr, "gc: marking %p kind=%u which is not an object start, collection=%zu\n", (void *)head, head->kind, gc->stats.collections);
            mx_fatal("gc: stale reference");
        }
#endif
    } else if (mx_gc_large_owns(gc, head)) {
        mx_gc_large_of(head)->mark = gc->mark_value;
    }
    if (head->kind == MX_KIND_STRING) {
        if (block != NULL) {
            if (head->is_old) {
                block->is_old = true;
            } else if (head->age < MX_GC_TENURE_AGE) {
                head->age += 1;
            } else if (!((mx_str_t *)head)->is_rope) {
                head->is_old = true;
                block->is_old = true;
                gc->cycle.promoted_bytes += size;
            }
        }
        if (((mx_str_t *)head)->is_rope) {
            mx_gc_gray_push(gc, head);
        }
    } else {
        mx_gc_promote_block(gc, head, block);
        mx_gc_gray_push(gc, head);
    }
    if (block != NULL && head->is_old) {
        mx_gc_mark_lines_block(block, head, size, MX_GC_MARK_OLD);
    } else if (block != NULL) {
        block->has_young = true;
        mx_gc_mark_lines_block(block, head, size, gc->mark_value);
    }
}

void mx_gc_drain(mx_gc_t *gc) {
    if (!gc->is_tracing) {
        gc->is_tracing = true;
        while (gc->gray.len > 0) {
            gc->gray.len -= 1;
            mx_gc_scan_obj(gc, gc->gray.ptr[gc->gray.len], false);
        }
        gc->is_tracing = false;
    }
}

void mx_gc_visit(mx_gc_t *gc, mx_obj_t *slot, bool is_old_scan) {
    if (is_old_scan) {
        mx_gc_scan_old_child(gc, slot);
    } else {
        mx_gc_mark_ref(gc, slot);
    }
}

void mx_gc_visit_ptr(mx_gc_t *gc, mx_kind_t kind, void **ptr, bool is_old_scan) {
    if (*ptr != NULL) {
        mx_obj_t slot = mx_obj_of_raw(kind, mx_val_ref(mx_ref_pin(*ptr)));
        mx_gc_visit(gc, &slot, is_old_scan);
        *ptr = mx_ref_obj(slot.val.ref);
    }
}

void mx_gc_stack_value(mx_gc_t *gc, mx_kind_t kind, mx_val_t *val, bool is_pin) {
    if (!is_pin) {
        mx_gc_mark_entry(gc, kind, val);
        return;
    }
    mx_obj_head_t *head = mx_gc_heap_head(gc->vm, mx_obj_of_raw(kind, *val));
    if (head != NULL) {
        mx_gc_pin(gc, head);
    }
}

void mx_gc_stack_objs(mx_gc_t *gc, mx_val_t *from, mx_val_t *to, bool is_pin) {
    for (mx_val_t *at = from; at + 1 < to; at += 2) {
        mx_obj_t *o = (mx_obj_t *)at;
        mx_gc_stack_value(gc, o->kind, &o->val, is_pin);
    }
}

void mx_gc_scan_lua_frame(mx_gc_t *gc, mx_val_t *base, const mx_kind_t *kinds, uint32_t nkinds, const mx_version_t *rb, bool is_pin) {
    mx_gc_stack_value(gc, MX_KIND_CLOSURE, &base[0], is_pin);
    const mx_ir_func_t *func = ((mx_obj_closure_t *)mx_ref_obj(base[0].ref))->block->func;
    for (uint32_t r = 1; r < nkinds && r < func->nregs; r++) {
        mx_kind_t kind = kinds[r];
        if (mx_kind_is_cell(kind)) {
            kind = (mx_kind_t)base[func->nregs + r].bits;
        }
        mx_gc_stack_value(gc, kind, &base[r], is_pin);
    }
    if (rb != NULL && func->is_vararg && rb->block->is_func && rb->start_at == 0) {
        uint32_t frame = mx_ir_func_frame(func);
        mx_val_t *spill = base + frame + 2;
        mx_gc_stack_objs(gc, spill, spill + 2 * base[frame].bits, is_pin);
    }
}

void mx_gc_scan_stack(mx_gc_t *gc, mx_thread_t *th, bool is_pin) {
    mx_val_t *slots = th->slots;
    uint32_t upper = th->top;
    uint32_t p = th->frame;
    const mx_kind_t *kinds = NULL;
    uint32_t nkinds = 0;
    const mx_version_t *rb = NULL;
    if (p == MX_THREAD_NONE && slots != NULL) {
        mx_gc_stack_objs(gc, slots, slots + upper, is_pin);
    }
    while (p != MX_THREAD_NONE && slots != NULL) {
        uintptr_t link = slots[p].bits;
        mx_link_tag_t tag = mx_link_tag(link);
        if (tag == MX_LINK_SAFEPOINT) {
            rb = (const mx_version_t *)slots[p + 1].bits;
            kinds = rb->kinds;
            nkinds = rb->nkinds;
        } else {
            if (mx_link_is_lua(tag)) {
#if MX_GC_VERIFY
                if (kinds == NULL) {
                    fprintf(stderr, "gc: Lua frame %u has no kinds, collection=%zu\n", p, gc->stats.collections);
                    mx_fatal("gc: Lua frame without kinds");
                }
#endif
                mx_gc_scan_lua_frame(gc, slots + p + 2, kinds, nkinds, rb, is_pin);
                if (tag == MX_LINK_ENTRY) {
                    mx_gc_stack_objs(gc, slots + p - 2, slots + p, is_pin);
                }
            } else {
                mx_gc_stack_objs(gc, slots + p + 2, slots + upper, is_pin);
            }
            const mx_resume_t *rec = (const mx_resume_t *)slots[p + 1].bits;
            if (rec != NULL) {
                kinds = rec->kinds;
                nkinds = rec->nkinds;
            } else {
                kinds = NULL;
                nkinds = 0;
            }
            rb = NULL;
        }
        upper = p;
        p = mx_link_caller(link);
    }
}

void mx_gc_scan_obj(mx_gc_t *gc, mx_obj_head_t *head, bool is_old_scan) {
    mx_t *vm = gc->vm;
    if (head->kind == MX_KIND_STRING) {
        mx_str_t *buf = (mx_str_t *)head;
        if (buf->is_rope) {
            mx_gc_visit_ptr(gc, MX_KIND_STRING, (void **)&buf->rope.left, is_old_scan);
            mx_gc_visit_ptr(gc, MX_KIND_STRING, (void **)&buf->rope.right, is_old_scan);
        }
    } else if (head->kind == MX_KIND_CLOSURE) {
        mx_obj_closure_t *c = (mx_obj_closure_t *)head;
        for (uint32_t i = 0; i < c->len; i++) {
            mx_gc_visit_ptr(gc, vm->kinds.upval, (void **)&c->upvals[i], is_old_scan);
        }
    } else if (head->kind == MX_KIND_ERROR) {
        mx_error_t *e = (mx_error_t *)head;
        mx_gc_mark_entry(gc, e->payload_kind, &e->payload);
    } else if (head->kind >= MX_KIND_FIRST_DATA && head->is_container) {
        mx_gc_scan_table(gc, (mx_obj_table_t *)head, is_old_scan);
    } else if (head->kind == vm->kinds.cfn) {
        mx_obj_cfn_t *c = (mx_obj_cfn_t *)head;
        for (uint32_t i = 0; i < c->len; i++) {
            mx_gc_visit(gc, &c->values[i], is_old_scan);
        }
    } else if (head->kind == vm->kinds.upval) {
        mx_obj_upval_t *uv = (mx_obj_upval_t *)head;
        if (uv->thread != NULL) {
            mx_gc_visit_ptr(gc, vm->kinds.thread, (void **)&uv->thread, is_old_scan);
        } else {
            mx_gc_visit(gc, &uv->value, is_old_scan);
        }
    } else if (head->kind == vm->kinds.thread) {
        mx_thread_t *th = (mx_thread_t *)head;
        mx_gc_scan_stack(gc, th, false);
        mx_gc_visit(gc, &th->entry_fn, is_old_scan);
        mx_gc_visit(gc, &th->failure, is_old_scan);
        mx_gc_visit_ptr(gc, vm->kinds.thread, (void **)&th->resumer, is_old_scan);
        for (mx_obj_upval_t *uv = th->open; uv != NULL; uv = uv->next) {
            mx_gc_mark_obj(gc, &uv->head);
        }
    }
}

void mx_gc_mark_entry(mx_gc_t *gc, mx_kind_t kind, mx_val_t *val) {
    mx_obj_t tmp = mx_obj_of_raw(kind, *val);
    mx_gc_mark_ref(gc, &tmp);
    *val = tmp.val;
}

void mx_gc_mark_ref(mx_gc_t *gc, mx_obj_t *slot) {
    mx_kind_t kind = slot->kind;
    void *ptr = mx_ref_obj(slot->val.ref);
    if (ptr == NULL || !mx_gc_kind_is_ref(gc->vm, kind) || !mx_gc_is_heap(gc, ptr)) {
        return;
    }
    if (gc->is_evacuating && (kind == MX_KIND_CLOSURE || kind >= MX_KIND_FIRST_DATA)) {
        void *to = mx_gc_maybe_evacuate(gc->vm, gc, ptr);
        if (to != ptr) {
            slot->val.ref = mx_ref_pin(to);
            ptr = to;
        }
    } else if (((mx_obj_head_t *)ptr)->is_forwarded) {
        ptr = mx_gc_forward_get(ptr);
        slot->val.ref = mx_ref_pin(ptr);
    }
    if (gc->is_scanning_dirty && !((mx_obj_head_t *)ptr)->is_old) {
        gc->has_found_young = true;
    }
    mx_gc_mark_obj(gc, ptr);
    mx_gc_drain(gc);
}

void mx_gc_add_root(mx_t *vm, mx_obj_t *slot) {
    mx_gc_t *gc = vm->gc;
    if (gc->roots.len >= gc->roots.cap) {
        gc->roots.cap = (gc->roots.len + 1) * 2;
        gc->roots.ptr = mx_mem_realloc(gc->roots.ptr, sizeof(mx_obj_t *) * gc->roots.cap);
    }
    gc->roots.ptr[gc->roots.len] = slot;
    gc->roots.len += 1;
}

void mx_gc_mark_holds(mx_gc_t *gc, mx_holds_t *holds) {
    for (uint32_t i = 0; i < holds->len; i++) {
        mx_gc_mark_ref(gc, &holds->ptr[i]);
    }
}

void mx_gc_mark_rets(mx_gc_t *gc, mx_rets_t *rets) {
    for (uint32_t i = 0; i < rets->cap; i++) {
        if (i < rets->len) {
            mx_gc_mark_ref(gc, &rets->vals[i]);
        } else {
            rets->vals[i] = mx_obj_of_nil();
        }
    }
}

mx_obj_head_t *mx_gc_heap_head(mx_t *vm, mx_obj_t v) {
    void *ptr = mx_ref_obj(v.val.ref);
    if (mx_gc_kind_is_ref(vm, v.kind) && ptr != NULL && mx_gc_is_heap(vm->gc, ptr)) {
        return ptr;
    } else {
        return NULL;
    }
}

void mx_gc_pin_roots(mx_t *vm, mx_gc_t *gc) {
    for (size_t t = 0; t < gc->threads.len; t++) {
        mx_gc_scan_stack(gc, gc->threads.ptr[t], true);
    }
    for (uint32_t i = 0; i < vm->consts.len; i++) {
        mx_obj_head_t *head = mx_gc_heap_head(vm, vm->consts.ptr[i]);
        if (head != NULL) {
            mx_gc_pin(gc, head);
            mx_gc_mark_obj(gc, head);
        }
    }
}

void mx_gc_mark_threads(mx_t *vm, mx_gc_t *gc) {
    if (gc->is_minor) {
        for (size_t t = 0; t < gc->threads.len; t++) {
            mx_thread_t *th = gc->threads.ptr[t];
            mx_gc_mark_obj(gc, &th->head);
            mx_gc_scan_obj(gc, &th->head, false);
        }
    }
    mx_gc_mark_obj(gc, &vm->main->head);
    for (mx_thread_t *th = vm->thread; th != NULL; th = th->resumer) {
        mx_gc_mark_obj(gc, &th->head);
    }
    mx_gc_drain(gc);
}

void mx_gc_mark_precise_roots(mx_t *vm) {
    mx_gc_t *gc = vm->gc;
    for (size_t i = 0; i < gc->roots.len; i++) {
        mx_gc_mark_ref(gc, gc->roots.ptr[i]);
    }
    if (gc->is_minor) {
        uint32_t w = 0;
        for (uint32_t i = 0; i < vm->interns.young.len; i++) {
            mx_str_t *buf = vm->interns.young.ptr[i];
            mx_gc_mark_obj(gc, &buf->head);
            if (!buf->head.is_old) {
                vm->interns.young.ptr[w] = buf;
                w += 1;
            }
        }
        vm->interns.young.len = w;
    }
    for (uint32_t i = 0; i < vm->kinds.len; i++) {
        for (mx_op_t op = 0; op < MX_OP_COUNT; op++) {
            mx_gc_mark_ref(gc, &vm->kinds.ptr[i].ops[op]);
        }
        mx_gc_mark_ref(gc, &vm->kinds.ptr[i].get_methods);
    }
    for (uint32_t i = 0; i < 256; i++) {
        mx_gc_mark_ref(gc, &vm->char_strs[i]);
    }
    mx_gc_mark_ref(gc, &vm->meta_call.key);
    mx_gc_mark_ref(gc, &vm->meta_get.key);
    mx_gc_mark_ref(gc, &vm->meta_set.key);
    mx_gc_mark_ref(gc, &vm->meta_eq.key);
    mx_gc_mark_ref(gc, &vm->weak_mode_key);
    mx_gc_mark_ref(gc, &vm->std_error.value);
    for (uint32_t i = 0; i < vm->tbcs.len; i++) {
        mx_gc_mark_ref(gc, &vm->tbcs.ptr[i].value);
        mx_gc_visit_ptr(gc, vm->kinds.thread, (void **)&vm->tbcs.ptr[i].thread, false);
    }
    for (size_t i = 0; i < gc->tobefnz.len; i++) {
        mx_gc_mark_ref(gc, &gc->tobefnz.ptr[i]);
    }
    mx_gc_mark_rets(gc, &vm->rets);
    for (uint32_t i = 0; i < vm->rets_saved.len; i++) {
        mx_gc_mark_rets(gc, &vm->rets_saved.ptr[i]);
    }
    mx_gc_mark_holds(gc, &vm->holds);
    for (uint32_t i = 0; i < vm->pending.cap && vm->pending.is_set && i < vm->pending.call.nargs; i++) {
        mx_gc_mark_ref(gc, &vm->pending.args[i]);
    }
}

void mx_gc_scan_old_child(mx_gc_t *gc, mx_obj_t *slot) {
    mx_kind_t kind = slot->kind;
    if (!mx_gc_kind_is_ref(gc->vm, kind)) {
        return;
    }
    void *ptr = mx_ref_obj(slot->val.ref);
    if (ptr == NULL || !mx_gc_block_owned(gc, ptr)) {
        mx_gc_mark_ref(gc, slot);
        return;
    }
    mx_gc_block_t *cb = (mx_gc_block_t *)((uintptr_t)ptr & ~(MX_GC_BLOCK_SIZE - 1));
#if MX_GC_VERIFY
    if (!mx_gc_obj_start_test(cb, ptr)) {
        fprintf(stderr, "gc: dirty scan child=%p kind=%u is not an object start collection=%zu\n", ptr, kind, gc->stats.collections);
        mx_fatal("gc: stale child in old object");
    }
#endif
    if (mx_gc_obj_start_test(cb, ptr)) {
        mx_gc_mark_ref(gc, slot);
    }
}

void mx_gc_scan_dirty_cards(mx_gc_t *gc) {
    if (gc->dirty.len != 0) {
        gc->is_scanning_dirty = true;
        size_t n = gc->dirty.len;
        mx_gc_block_t **pending = gc->dirty.ptr;
        size_t apending = gc->dirty.cap;
        gc->dirty.ptr = gc->dirty.scan;
        gc->dirty.cap = gc->dirty.scan_cap;
        gc->dirty.scan = pending;
        gc->dirty.scan_cap = apending;
        gc->dirty.len = 0;
        gc->dirty.last_base = 0;
        for (size_t bi = 0; bi < n; bi++) {
            mx_gc_block_t *b = pending[bi];
            if (b->is_old && b->is_dirty) {
                b->is_dirty = false;
                gc->has_found_young = false;
                for (mx_obj_head_t *h = b->old_head; h != NULL; h = *mx_gc_next_slot(h)) {
                    if (h->is_old && !h->is_forwarded && h->kind != gc->vm->kinds.thread) {
                        mx_gc_scan_obj(gc, h, true);
                    }
                }
                if (gc->has_found_young) {
                    mx_gc_dirty_block(gc, b);
                }
            }
        }
        size_t header = MX_GC_LARGE_HEADER;
        for (mx_gc_large_t *lg = gc->larges.list; lg != NULL; lg = lg->next) {
            mx_obj_head_t *h = (mx_obj_head_t *)((uint8_t *)lg + header);
            if (h->is_old && !h->is_forwarded && h->kind != gc->vm->kinds.thread) {
                mx_gc_scan_obj(gc, h, true);
            }
        }
        gc->is_scanning_dirty = false;
    }
}

void mx_gc_interns_sweep(mx_t *vm, mx_gc_t *gc) {
    if (vm->interns.slots != NULL) {
        for (uint32_t i = 0; i < vm->interns.cap; i++) {
            mx_str_t *b = vm->interns.slots[i];
            if (b != NULL && b != mx_intern_tomb()) {
                if (b->head.is_forwarded) {
                    vm->interns.slots[i] = mx_gc_forward_get(&b->head);
                } else if (b->head.mark != gc->mark_value && !(gc->is_minor && b->head.is_old)) {
                    b->is_interned = false;
                    vm->interns.slots[i] = mx_intern_tomb();
                    vm->interns.used -= 1;
                    vm->interns.tombs += 1;
                }
            }
        }
        uint32_t w = 0;
        for (uint32_t i = 0; i < vm->interns.young.len; i++) {
            mx_str_t *b = vm->interns.young.ptr[i];
            if (b->head.is_forwarded) {
                b = mx_gc_forward_get(&b->head);
            } else if (b->head.mark != gc->mark_value) {
                b = NULL;
            }
            if (b != NULL) {
                vm->interns.young.ptr[w] = b;
                w += 1;
            }
        }
        vm->interns.young.len = w;
    }
}

void mx_gc_mark_roots(mx_t *vm) {
    mx_gc_t *gc = vm->gc;
    bool want_evac = gc->is_evacuating;
    gc->is_evacuating = false;
    gc->pinned.len = 0;
    mx_gc_pin_roots(vm, gc);
    mx_gc_drain(gc);
    gc->is_evacuating = want_evac;
    if (gc->is_minor) {
        mx_gc_scan_dirty_cards(gc);
    }
    mx_gc_mark_threads(vm, gc);
    mx_gc_mark_precise_roots(vm);
    for (size_t i = 0; i < gc->pinned.len; i++) {
        gc->pinned.ptr[i]->is_pinned = false;
    }
}

mx_gc_weak_t mx_gc_table_weak_mode(mx_gc_t *gc, mx_obj_table_t *table) {
    mx_gc_weak_t got = { .is_weak_keys = false, .is_weak_vals = false };
    if (table->meta == NULL || mx_obj_is_nil(gc->vm->weak_mode_key)) {
        return got;
    }
    mx_obj_t mode = mx_table_get(table->meta, gc->vm->weak_mode_key);
    if (!mx_obj_is_string(mode)) {
        return got;
    }
    mx_str_t *buf = mx_obj_get_string(mode);
    for (uint32_t i = 0; i < buf->len; i++) {
        if (buf->buf[i] == 'k') {
            got.is_weak_keys = true;
        } else if (buf->buf[i] == 'v') {
            got.is_weak_vals = true;
        }
    }
    return got;
}

void mx_gc_weak_note(mx_gc_t *gc, mx_obj_table_t *table) {
    if (!table->is_weak) {
        table->is_weak = true;
        if (gc->weaks.len >= gc->weaks.cap) {
            gc->weaks.cap = (gc->weaks.len + 1) * 2;
            gc->weaks.ptr = mx_mem_realloc(gc->weaks.ptr, sizeof(mx_obj_table_t *) * gc->weaks.cap);
        }
        gc->weaks.ptr[gc->weaks.len] = table;
        gc->weaks.len += 1;
    }
}

void mx_gc_scan_table(mx_gc_t *gc, mx_obj_table_t *table, bool is_old_scan) {
    if (table->meta != NULL) {
        mx_gc_visit_ptr(gc, table->meta->head.kind, (void **)&table->meta, is_old_scan);
    }
    mx_gc_weak_t weak = mx_gc_table_weak_mode(gc, table);
    bool weak_keys = weak.is_weak_keys;
    bool weak_vals = weak.is_weak_vals;
    if (weak_keys || weak_vals) {
        mx_gc_weak_note(gc, table);
    }
    if (table->array != NULL) {
        for (uint32_t i = 0; i < table->array_len; i++) {
            if (!weak_vals || !mx_gc_slot_collectable(gc, table->array[i])) {
                mx_gc_visit(gc, &table->array[i], is_old_scan);
            }
        }
    }
    if (table->entries != NULL) {
        size_t len = mx_table_hash_cap(table);
        for (size_t i = 0; i < len; i++) {
            mx_table_entry_t *e = &table->entries[i];
            mx_obj_t key = mx_table_entry_key(e);
            bool is_key_live;
            if (!weak_keys || !mx_gc_slot_collectable(gc, key)) {
                mx_gc_visit(gc, &key, is_old_scan);
                is_key_live = true;
            } else {
                is_key_live = !mx_gc_slot_is_dead(gc, &key);
            }
            e->key = key.val;
            mx_obj_t val = mx_table_entry_val(e);
            if (is_key_live && (!weak_vals || !mx_gc_slot_collectable(gc, val))) {
                mx_gc_visit(gc, &val, is_old_scan);
                e->val = val.val;
            }
        }
    }
}

bool mx_gc_ephemeron_pass(mx_gc_t *gc) {
    bool is_changed = false;
    for (size_t i = 0; i < gc->weaks.len; i++) {
        mx_obj_table_t *table = gc->weaks.ptr[i];
        if (table->head.is_forwarded) {
            table = mx_gc_forward_get(&table->head);
        }
        mx_gc_weak_t weak = mx_gc_table_weak_mode(gc, table);
        if (!weak.is_weak_keys || weak.is_weak_vals || table->entries == NULL) {
            continue;
        }
        bool is_old_scan = gc->is_minor && table->head.is_old;
        size_t len = mx_table_hash_cap(table);
        for (size_t j = 0; j < len; j++) {
            mx_table_entry_t *e = &table->entries[j];
            mx_obj_t key = mx_table_entry_key(e);
            mx_obj_t val = mx_table_entry_val(e);
            mx_obj_head_t *h = mx_gc_heap_head(gc->vm, val);
            if (mx_gc_slot_collectable(gc, key) && mx_gc_slot_is_dead(gc, &key)) {
                continue;
            } else if (h != NULL && !h->is_forwarded && h->mark != gc->mark_value && !(gc->is_minor && h->is_old)) {
                mx_gc_visit(gc, &val, is_old_scan);
                e->val = val.val;
                is_changed = true;
            }
        }
    }
    if (is_changed) {
        mx_gc_drain(gc);
    }
    return is_changed;
}

bool mx_gc_slot_collectable(mx_gc_t *gc, mx_obj_t slot) {
    mx_kind_t kind = slot.kind;
    if (kind != MX_KIND_CLOSURE && (kind < MX_KIND_FIRST_DATA || kind == MX_KIND_ERROR)) {
        return false;
    }
    void *ptr = mx_ref_obj(slot.val.ref);
    if (ptr == NULL) {
        return false;
    }
    return mx_gc_block_owned(gc, ptr) || mx_gc_large_owns(gc, ptr);
}

bool mx_gc_slot_is_dead(mx_gc_t *gc, mx_obj_t *slot) {
    mx_obj_head_t *head = mx_ref_obj(slot->val.ref);
    if (head->is_forwarded) {
        head = mx_gc_forward_get(head);
        slot->val.ref = mx_ref_pin(head);
    }
    return !(gc->is_minor && head->is_old) && head->mark != gc->mark_value;
}

void mx_gc_weak_clear_table(mx_gc_t *gc, mx_obj_table_t *table, bool weak_keys, bool weak_vals) {
    if (weak_vals && table->array != NULL) {
        for (uint32_t i = 0; i < table->array_len; i++) {
            mx_obj_t *slot = &table->array[i];
            if (mx_gc_slot_collectable(gc, *slot) && mx_gc_slot_is_dead(gc, slot)) {
                *slot = mx_obj_of_nil();
                if (i < table->len) {
                    table->len = i;
                }
            }
        }
    }
    if (table->entries != NULL) {
        size_t len = mx_table_hash_cap(table);
        for (size_t i = 0; i < len; i++) {
            mx_table_entry_t *e = &table->entries[i];
            mx_obj_t key = mx_table_entry_key(e);
            if (mx_obj_is_nil(key)) {
                continue;
            }
            if (weak_keys && mx_gc_slot_collectable(gc, key)) {
                bool dead = mx_gc_slot_is_dead(gc, &key);
                e->key = key.val;
                if (dead) {
                    mx_table_entry_set_key(e, mx_obj_of_boolean(false), mx_table_key_hash(mx_obj_of_boolean(false)));
                    mx_table_entry_set_val(e, mx_obj_of_nil());
                    continue;
                }
            }
            mx_obj_t val = mx_table_entry_val(e);
            if (weak_vals && mx_gc_slot_collectable(gc, val)) {
                bool dead = mx_gc_slot_is_dead(gc, &val);
                e->val = val.val;
                if (dead) {
                    mx_table_entry_set_val(e, mx_obj_of_nil());
                }
            }
        }
    }
}

void mx_gc_weak_clear(mx_gc_t *gc) {
    size_t w = 0;
    for (size_t i = 0; i < gc->weaks.len; i++) {
        mx_obj_table_t *table = gc->weaks.ptr[i];
        if (table->head.is_forwarded) {
            table = mx_gc_forward_get(&table->head);
        }
        bool table_dead = !(gc->is_minor && table->head.is_old) && table->head.mark != gc->mark_value;
        if (table_dead) {
            continue;
        }
        mx_gc_weak_t weak = mx_gc_table_weak_mode(gc, table);
        if (!weak.is_weak_keys && !weak.is_weak_vals) {
            table->is_weak = false;
        } else {
            mx_gc_weak_clear_table(gc, table, weak.is_weak_keys, weak.is_weak_vals);
            gc->weaks.ptr[w] = table;
            w += 1;
        }
    }
    gc->weaks.len = w;
}

void mx_gc_threads_sweep(mx_gc_t *gc) {
    size_t w = 0;
    for (size_t i = 0; i < gc->threads.len; i++) {
        mx_thread_t *th = gc->threads.ptr[i];
        if ((gc->is_minor && th->head.is_old) || th->head.mark == gc->mark_value) {
            gc->threads.ptr[w] = th;
            w += 1;
        }
    }
    gc->threads.len = w;
}

void mx_gc_note_finalizer(mx_t *vm, mx_obj_table_t *table) {
    mx_gc_t *gc = vm->gc;
    if (!table->is_finalizable) {
        table->is_finalizable = true;
        if (gc->finobjs.len >= gc->finobjs.cap) {
            gc->finobjs.cap = (gc->finobjs.len + 1) * 2;
            gc->finobjs.ptr = mx_mem_realloc(gc->finobjs.ptr, sizeof(mx_obj_t) * gc->finobjs.cap);
        }
        gc->finobjs.ptr[gc->finobjs.len] = mx_obj_of_table(table);
        gc->finobjs.len += 1;
    }
}

void mx_gc_finalizers_separate(mx_gc_t *gc) {
    size_t w = 0;
    for (size_t i = 0; i < gc->finobjs.len; i++) {
        mx_obj_t *slot = &gc->finobjs.ptr[i];
        if (!mx_gc_slot_is_dead(gc, slot)) {
            gc->finobjs.ptr[w] = *slot;
            w += 1;
        } else {
            mx_gc_mark_ref(gc, slot);
            mx_obj_table_t *table = mx_obj_get_table(*slot);
            table->is_finalizable = false;
            if (gc->tobefnz.len >= gc->tobefnz.cap) {
                gc->tobefnz.cap = (gc->tobefnz.len + 1) * 2;
                gc->tobefnz.ptr = mx_mem_realloc(gc->tobefnz.ptr, sizeof(mx_obj_t) * gc->tobefnz.cap);
            }
            gc->tobefnz.ptr[gc->tobefnz.len] = *slot;
            gc->tobefnz.len += 1;
        }
    }
    gc->finobjs.len = w;
}

bool mx_gc_run_finalizers(mx_t *vm) {
    mx_gc_t *gc = vm->gc;
    if (gc->tobefnz.len != 0 && !gc->is_finalizing && vm->hooks.gc_finalizer != NULL) {
        gc->is_finalizing = true;
        bool ran = vm->hooks.gc_finalizer(vm);
        gc->is_finalizing = false;
        return ran;
    } else {
        return false;
    }
}

void mx_gc_finalize(mx_t *vm, mx_obj_head_t *head) {
    if (head->kind == MX_KIND_STRING) {
        mx_str_t *buf = (mx_str_t *)head;
        if (!buf->is_rope && !buf->is_interned && !buf->is_pooled && !buf->is_sso && !buf->is_tail && buf->buf != NULL) {
            mx_str_payload_free(buf->buf);
            buf->buf = NULL;
        }
    } else if (head->kind >= MX_KIND_FIRST_DATA && head->is_container) {
        mx_obj_table_t *table = (mx_obj_table_t *)head;
        if (head->kind == vm->kinds.udata && table->hidden != 0) {
            mx_mem_free((void *)(uintptr_t)table->hidden);
            table->hidden = 0;
        }
        if (table->entries != NULL) {
            mx_table_entries_free(vm, table->entries, table->size);
            table->entries = NULL;
        }
        if (table->array != NULL && table->array != table->inline_array) {
            mx_table_array_free(vm, table->array, table->array_cap);
            table->array = NULL;
        }
    } else if (head->kind == vm->kinds.thread) {
        mx_thread_finalize((mx_thread_t *)head);
    }
}

void mx_gc_retire_obj_starts(mx_gc_t *gc, mx_gc_block_t *block, size_t line) {
    uint32_t bits = block->obj_start[line * 2] | ((uint32_t)block->obj_start[line * 2 + 1] << 8);
    uint32_t keep = bits;
    uint8_t mark = gc->mark_value;
    bool is_minor = gc->is_minor;
    uint8_t *first = (uint8_t *)block + line * MX_GC_LINE_SIZE;
    while (bits != 0) {
        uint32_t k = (uint32_t)__builtin_ctz(bits);
        bits &= bits - 1;
        mx_obj_head_t *head = (mx_obj_head_t *)(first + k * 8);
        if (head->mark != mark && !(is_minor && head->is_old)) {
            if (head->is_final && !head->is_forwarded) {
                head->is_final = false;
                block->final_count -= 1;
                mx_gc_finalize(gc->vm, head);
            }
            keep &= ~((uint32_t)1 << k);
        }
    }
    block->obj_start[line * 2] = (uint8_t)keep;
    block->obj_start[line * 2 + 1] = (uint8_t)(keep >> 8);
}

void mx_gc_finalize_dead_line(mx_gc_t *gc, mx_gc_block_t *block, size_t line) {
    uint32_t bits = block->obj_start[line * 2] | ((uint32_t)block->obj_start[line * 2 + 1] << 8);
    uint8_t *first = (uint8_t *)block + line * MX_GC_LINE_SIZE;
    while (bits != 0) {
        uint32_t k = (uint32_t)__builtin_ctz(bits);
        bits &= bits - 1;
        mx_obj_head_t *head = (mx_obj_head_t *)(first + k * 8);
        if (head->is_final && !head->is_forwarded) {
            head->is_final = false;
            block->final_count -= 1;
            mx_gc_finalize(gc->vm, head);
        }
    }
}

void mx_gc_sweep(mx_gc_t *gc) {
    gc->blocks.recycle = NULL;
    mx_gc_block_t **prev = &gc->blocks.live;
    mx_gc_block_t *block = gc->blocks.live;
    bool freed_any = false;
    size_t total_live_lines = 0;
    size_t young_lines = 0;
    while (block != NULL) {
        mx_gc_block_t *next = block->next;
        if (gc->is_minor && block->is_old && !block->has_young) {
            total_live_lines += block->live_lines;
            if (block->live_lines < MX_GC_USABLE_LINES) {
                block->recycle_next = gc->blocks.recycle;
                gc->blocks.recycle = block;
            }
            prev = &block->next;
            block = next;
            continue;
        }
        size_t live_lines = 0;
        size_t block_young = 0;
        for (size_t i = MX_GC_BLOCK_HEADER_LINES; i < MX_GC_LINES_PER_BLOCK; i++) {
            if (mx_gc_line_is_live(gc, block->line_mark[i])) {
                live_lines += 1;
                if (block->line_mark[i] == gc->mark_value) {
                    block_young += 1;
                }
                mx_gc_retire_obj_starts(gc, block, i);
            } else {
#if MX_GC_VERIFY
                for (size_t s = i * 2; s < i * 2 + 2; s++) {
                    for (size_t k = 0; k < 8; k++) {
                        mx_obj_head_t *h = (mx_obj_head_t *)((uint8_t *)block + (s * 8 + k) * 8);
                        if ((block->obj_start[s] & (1u << k)) != 0 && gc->is_minor && h->is_old && !h->is_forwarded) {
                            fprintf(stderr, "gc: old object %p kind=%u on dead line %zu mark=%u collection=%zu\n", (void *)h, h->kind, i, block->line_mark[i], gc->stats.collections);
                            mx_fatal("gc: old object on a dead line");
                        }
                    }
                }
#endif
                if (block->final_count != 0) {
                    mx_gc_finalize_dead_line(gc, block, i);
                }
                block->line_mark[i] = 0;
                block->obj_start[i * 2] = 0;
                block->obj_start[i * 2 + 1] = 0;
            }
        }
        total_live_lines += live_lines;
        young_lines += block_young;
        block->live_lines = (uint32_t)live_lines;
        block->has_young = block_young != 0;
        if (live_lines == 0) {
            *prev = next;
            gc->blocks.count -= 1;
            block->next = gc->blocks.free;
            gc->blocks.free = block;
            freed_any = true;
        } else {
            *prev = block;
            prev = &block->next;
            if (live_lines < MX_GC_USABLE_LINES) {
                block->recycle_next = gc->blocks.recycle;
                gc->blocks.recycle = block;
            }
        }
        block = next;
    }
    gc->cycle.live_line_bytes = total_live_lines * MX_GC_LINE_SIZE;
    gc->cycle.young_line_bytes = young_lines * MX_GC_LINE_SIZE;
#if MX_GC_FRAG
    size_t live_blocks = 0;
    for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
        live_blocks += 1;
    }
    fprintf(stderr, "gc: live_lines=%zu cap_lines=%zu live_blocks=%zu\n", total_live_lines, live_blocks * (size_t)MX_GC_USABLE_LINES, live_blocks);
#endif
    if (freed_any) {
        gc->block_index.is_dirty = true;
    }
}

void mx_gc_sweep_large(mx_gc_t *gc) {
    size_t header = MX_GC_LARGE_HEADER;
    mx_gc_large_t **prev = &gc->larges.list;
    mx_gc_large_t *lg = gc->larges.list;
    while (lg != NULL) {
        mx_gc_large_t *next = lg->next;
        mx_obj_head_t *payload = (mx_obj_head_t *)((uint8_t *)lg + header);
        if (lg->mark == gc->mark_value || (gc->is_minor && payload->is_old)) {
            prev = &lg->next;
        } else {
            *prev = next;
            if (!payload->is_forwarded) {
                mx_gc_finalize(gc->vm, payload);
            }
            mx_mem_free(lg);
            gc->larges.index.is_dirty = true;
        }
        lg = next;
    }
}

void mx_gc_select_evac(mx_gc_t *gc) {
    gc->is_evacuating = false;
    for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
        size_t live = 0;
        for (size_t i = MX_GC_BLOCK_HEADER_LINES; i < MX_GC_LINES_PER_BLOCK; i++) {
            if (b->line_mark[i] != 0) {
                live += 1;
            }
        }
        b->is_evac = false;
#if MX_GC_EVAC_ENABLED
        if (live > 0 && live * 2 < MX_GC_USABLE_LINES) {
            b->is_evac = true;
            gc->is_evacuating = true;
        }
#endif
    }
}

void mx_gc_verify_child(mx_gc_t *gc, mx_obj_head_t *parent, const char *what, size_t index, mx_obj_t child) {
    void *ptr = mx_ref_obj(child.val.ref);
    if (!mx_gc_kind_is_ref(gc->vm, child.kind) || ptr == NULL || !mx_gc_block_owned(gc, ptr)) {
        return;
    }
    mx_gc_block_t *b = (mx_gc_block_t *)((uintptr_t)ptr & ~(MX_GC_BLOCK_SIZE - 1));
    if (!mx_gc_obj_start_test(b, ptr)) {
        fprintf(
            stderr,
            "gc: parent=%p kind=%u old=%u age=%u mark=%u field=%s index=%zu child=%p child_kind=%u collection=%zu minor=%u\n",
            (void *)parent,
            parent->kind,
            (unsigned)parent->is_old,
            (unsigned)parent->age,
            (unsigned)parent->mark,
            what,
            index,
            ptr,
            child.kind,
            gc->stats.collections,
            (unsigned)gc->is_minor
        );
        mx_fatal("gc: child pointer is not an object start");
    }
}

void mx_gc_verify_obj(mx_gc_t *gc, mx_obj_head_t *h) {
    mx_t *vm = gc->vm;
    if (h->kind == MX_KIND_CLOSURE) {
        mx_obj_closure_t *c = (mx_obj_closure_t *)h;
        for (uint32_t i = 0; i < c->len; i++) {
            if (c->upvals[i] != NULL) {
                mx_gc_verify_child(gc, h, "upv", i, mx_obj_of_raw(vm->kinds.upval, mx_val_ref(mx_ref_pin(c->upvals[i]))));
            }
        }
    } else if (h->kind >= MX_KIND_FIRST_DATA && h->is_container) {
        mx_obj_table_t *t = (mx_obj_table_t *)h;
        if (t->array != NULL) {
            for (uint32_t i = 0; i < t->array_len; i++) {
                mx_gc_verify_child(gc, h, "arr", i, t->array[i]);
            }
        }
        if (t->entries != NULL) {
            size_t len = mx_table_hash_cap(t);
            for (size_t i = 0; i < len; i++) {
                mx_gc_verify_child(gc, h, "ent", i, mx_table_entry_key(&t->entries[i]));
                mx_gc_verify_child(gc, h, "ent", i, mx_table_entry_val(&t->entries[i]));
            }
        }
    } else if (h->kind == vm->kinds.cfn) {
        mx_obj_cfn_t *c = (mx_obj_cfn_t *)h;
        for (uint32_t i = 0; i < c->len; i++) {
            mx_gc_verify_child(gc, h, "cfn", i, c->values[i]);
        }
    } else if (h->kind == vm->kinds.upval) {
        mx_obj_upval_t *uv = (mx_obj_upval_t *)h;
        if (uv->thread == NULL) {
            mx_gc_verify_child(gc, h, "val", 0, uv->value);
        }
    } else if (h->kind == MX_KIND_ERROR) {
        mx_error_t *e = (mx_error_t *)h;
        mx_gc_verify_child(gc, h, "err", 0, mx_obj_of_raw(e->payload_kind, e->payload));
    }
}

void mx_gc_verify(mx_t *vm) {
    mx_gc_t *gc = vm->gc;
    for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
        for (mx_obj_head_t *h = b->old_head; h != NULL; h = *mx_gc_next_slot(h)) {
            bool is_inside = (uintptr_t)h - (uintptr_t)b < MX_GC_BLOCK_SIZE;
            if (!is_inside || !mx_gc_obj_start_test(b, h) || !h->is_old) {
                fprintf(stderr, "gc: block=%p old_head entry=%p kind=%u collection=%zu minor=%u\n", (void *)b, (void *)h, h->kind, gc->stats.collections, (unsigned)gc->is_minor);
                mx_fatal("gc: bad old_head entry");
            }
        }
    }
    for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
        for (size_t s = 0; s < MX_GC_OBJ_BITMAP_BYTES; s++) {
            uint8_t bits = b->obj_start[s];
            for (size_t k = 0; k < 8; k++) {
                if ((bits & (uint8_t)((uint32_t)1 << k)) != 0) {
                    mx_obj_head_t *h = (mx_obj_head_t *)((uint8_t *)b + (s * 8 + k) * 8);
                    if (h->mark == gc->mark_value && !h->is_forwarded) {
                        mx_gc_verify_obj(gc, h);
                    }
                }
            }
        }
    }
}

bool mx_gc_collect(mx_t *vm) {
    mx_gc_t *gc = vm->gc;
    memset(vm->str_cache, 0, sizeof(vm->str_cache));
    size_t grow_limit = gc->cycle.old_live_after_major * 2;
    if (grow_limit < MX_GC_INITIAL_THRESHOLD) {
        grow_limit = MX_GC_INITIAL_THRESHOLD;
    }
    grow_limit = grow_limit / 68 * gc->params[MX_GC_PARAM_MINORMAJOR];
    bool need_major = gc->is_major_forced
        || !gc->is_generational
        || gc->is_major_mode
        || gc->cycle.promoted_bytes > grow_limit
        || gc->cycle.minors_since_major >= MX_GC_MINORS_PER_MAJOR
        || gc->cycle.old_live_after_major == 0
        || gc->blocks.count > gc->cycle.blocks_after_major * gc->cycle.major_growth_factor + 64;
#if !MX_GC_MINOR_ENABLED
    need_major = true;
#endif
    gc->is_major_forced = false;
    gc->is_minor = !need_major;
    if (need_major) {
        gc->cycle.blocks_after_major = gc->blocks.count;
        for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
            b->is_old = false;
            b->is_dirty = false;
            b->old_head = NULL;
        }
        gc->dirty.len = 0;
        gc->dirty.last_base = 0;
        mx_gc_select_evac(gc);
        for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
            for (size_t i = MX_GC_BLOCK_HEADER_LINES; i < MX_GC_LINES_PER_BLOCK; i++) {
                if (b->line_mark[i] == MX_GC_MARK_OLD) {
                    b->line_mark[i] = 0;
                }
            }
        }
    } else {
        gc->is_evacuating = false;
    }
    gc->evac.cursor = NULL;
    gc->evac.limit = NULL;
    gc->evac.block = NULL;
    gc->evac_bytes = 0;
    if (gc->mark_value >= MX_GC_MARK_WRAP) {
        for (mx_gc_block_t *b = gc->blocks.live; b != NULL; b = b->next) {
            for (size_t i = MX_GC_BLOCK_HEADER_LINES; i < MX_GC_LINES_PER_BLOCK; i++) {
                if (b->line_mark[i] != 0 && b->line_mark[i] != MX_GC_MARK_OLD) {
                    b->line_mark[i] = MX_GC_MARK_STALE;
                }
            }
            for (size_t s = 0; s < MX_GC_OBJ_BITMAP_BYTES; s++) {
                uint8_t bits = b->obj_start[s];
                for (size_t k = 0; k < 8; k++) {
                    if ((bits & (uint8_t)((uint32_t)1 << k)) != 0) {
                        mx_obj_head_t *h = (mx_obj_head_t *)((uint8_t *)b + (s * 8 + k) * 8);
                        h->mark = MX_GC_MARK_STALE;
                    }
                }
            }
        }
        size_t header = MX_GC_LARGE_HEADER;
        for (mx_gc_large_t *lg = gc->larges.list; lg != NULL; lg = lg->next) {
            lg->mark = MX_GC_MARK_STALE;
            mx_obj_head_t *h = (mx_obj_head_t *)((uint8_t *)lg + header);
            h->mark = MX_GC_MARK_STALE;
        }
        gc->mark_value = 1;
    } else {
        gc->mark_value += 1;
    }
    if (gc->block_index.is_dirty) {
        mx_gc_block_index_rebuild(gc);
    }
    mx_gc_mark_roots(vm);
    while (mx_gc_ephemeron_pass(gc)) {
    }
    mx_gc_finalizers_separate(gc);
    while (mx_gc_ephemeron_pass(gc)) {
    }
    gc->is_evacuating = false;
    mx_gc_weak_clear(gc);
    mx_gc_threads_sweep(gc);
    mx_gc_interns_sweep(vm, gc);
    mx_gc_sweep(gc);
    if (gc->cycle.live_line_bytes > gc->evac_bytes) {
        gc->cycle.live_line_bytes -= gc->evac_bytes;
    } else {
        gc->cycle.live_line_bytes = 0;
    }
    mx_gc_sweep_large(gc);
    gc->small.block = NULL;
    gc->small.cursor = NULL;
    gc->small.limit = NULL;
    gc->overflow.block = NULL;
    gc->overflow.cursor = NULL;
    gc->overflow.limit = NULL;
    gc->cycle.bytes_since_major += gc->cycle.bytes_since_gc;
    gc->cycle.bytes_since_gc = 0;
    vm->gc_pending = 0;
    gc->stats.collections += 1;
    if (gc->is_minor) {
        gc->cycle.minors_since_major += 1;
    } else {
        size_t before = gc->cycle.old_live_after_major + gc->cycle.bytes_since_major;
        size_t freed = 0;
        if (before > gc->cycle.live_line_bytes) {
            freed = before - gc->cycle.live_line_bytes;
        }
        gc->is_major_mode = gc->is_generational
            && freed * 100 < gc->cycle.bytes_since_major * gc->params[MX_GC_PARAM_MAJORMINOR];
        gc->cycle.bytes_since_major = 0;
        gc->cycle.old_live_after_major = gc->cycle.live_line_bytes;
        gc->cycle.promoted_bytes = 0;
        gc->cycle.minors_since_major = 0;
        if (gc->blocks.count * 4 > gc->cycle.blocks_after_major * 3) {
            if (gc->cycle.major_growth_factor < MX_GC_MAJOR_GROWTH_MAX) {
                gc->cycle.major_growth_factor *= 2;
            }
        } else {
            gc->cycle.major_growth_factor = 2;
        }
    }
    size_t large_bytes = 0;
    for (mx_gc_large_t *l = gc->larges.list; l != NULL; l = l->next) {
        large_bytes += l->size;
    }
    size_t live_bytes = gc->cycle.live_line_bytes + large_bytes;
    if (live_bytes > gc->stats.peak_live_bytes) {
        gc->stats.peak_live_bytes = live_bytes;
    }
    gc->cycle.live_bytes = live_bytes;
    if (gc->is_minor) {
        gc->cycle.threshold = mx_gc_budget(gc, gc->cycle.young_line_bytes + large_bytes);
    } else {
        gc->cycle.threshold = mx_gc_budget(gc, live_bytes);
    }
    if (!gc->is_minor || gc->pool.free_bytes > MX_GC_POOL_FREE_FLUSH_BYTES) {
        for (int32_t c = MX_GC_POOL_SLAB_CLASSES; c < MX_GC_POOL_CLASSES; c++) {
            void *p = gc->pool.free[c];
            while (p != NULL) {
                void *next = *(void **)p;
                mx_mem_free(p);
                p = next;
            }
            gc->pool.free[c] = NULL;
        }
        gc->pool.free_bytes = 0;
    }
#if MX_GC_VERIFY
    mx_gc_verify(vm);
#endif
    return mx_gc_run_finalizers(vm);
}
