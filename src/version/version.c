
#include "version.h"
#include "lower.h"
#include "../value/obj.h"

mx_location_range_t mx_resume_range(const mx_resume_t *rec) {
    return mx_ir_block_range(rec->site, rec->site_at);
}

uint32_t mx_version_kinds_hash(const mx_kind_t *kinds, uint32_t n) {
    uint64_t h = ((uint64_t)n + 1) * UINT64_C(0x9e3779b97f4a7c15);
    uint32_t i = 0;
    while (i + 4 <= n) {
        uint64_t w;
        memcpy(&w, kinds + i, sizeof(uint64_t));
        h = (h ^ w) * UINT64_C(0x9e3779b97f4a7c15);
        h ^= h >> 29;
        i += 4;
    }
    uint64_t tail = 0;
    for (uint32_t j = i; j < n; j++) {
        tail |= (uint64_t)kinds[j] << (16 * (j - i));
    }
    h = (h ^ tail) * UINT64_C(0x9e3779b97f4a7c15);
    uint32_t folded = (uint32_t)(h ^ (h >> 32));
    if (folded == 0) {
        return 1;
    } else {
        return folded;
    }
}

const mx_kind_t *mx_version_kinds_intern(mx_t *vm, const mx_kind_t *kinds, uint32_t n) {
    if (n == 0) {
        return NULL;
    }
    if ((vm->kind_vecs.len + 1) * 2 > vm->kind_vecs.cap) {
        uint32_t ocap = vm->kind_vecs.cap;
        mx_kind_vec_t *old = vm->kind_vecs.ptr;
        if (ocap == 0) {
            vm->kind_vecs.cap = 256;
        } else {
            vm->kind_vecs.cap = ocap * 2;
        }
        vm->kind_vecs.ptr = mx_mem_calloc(sizeof(mx_kind_vec_t) * vm->kind_vecs.cap);
        for (uint32_t i = 0; i < ocap; i++) {
            if (old[i].ptr != NULL) {
                uint32_t at = old[i].hash & (vm->kind_vecs.cap - 1);
                while (vm->kind_vecs.ptr[at].ptr != NULL) {
                    at = (at + 1) & (vm->kind_vecs.cap - 1);
                }
                vm->kind_vecs.ptr[at] = old[i];
            }
        }
        mx_mem_free(old);
    }
    uint32_t hash = mx_version_kinds_hash(kinds, n);
    uint32_t at = hash & (vm->kind_vecs.cap - 1);
    while (vm->kind_vecs.ptr[at].ptr != NULL) {
        mx_kind_vec_t *e = &vm->kind_vecs.ptr[at];
        if (e->hash == hash && e->len == n && memcmp(e->ptr, kinds, sizeof(mx_kind_t) * n) == 0) {
            return e->ptr;
        }
        at = (at + 1) & (vm->kind_vecs.cap - 1);
    }
    mx_kind_t *canonical = mx_ir_alloc(vm, sizeof(mx_kind_t) * n);
    memcpy(canonical, kinds, sizeof(mx_kind_t) * n);
    vm->stats.version_bytes += (uint32_t)(sizeof(mx_kind_t) * n);
    vm->kind_vecs.ptr[at] = (mx_kind_vec_t) {
        .ptr = canonical,
        .len = n,
        .hash = hash,
    };
    vm->kind_vecs.len += 1;
    return canonical;
}

void mx_version_caches_free(mx_t *vm) {
    mx_version_cache_t *cache = vm->version_caches;
    while (cache != NULL) {
        mx_version_cache_t *next = cache->next;
        mx_mem_free(cache->list.ptr);
        mx_mem_free(cache->table.ptr);
        cache = next;
    }
    vm->version_caches = NULL;
    mx_mem_free(vm->kind_vecs.ptr);
    vm->kind_vecs = (mx_kind_vecs_t) {0};
}

uintptr_t *mx_version_slot(mx_t *vm, mx_ir_block_t *block) {
    mx_ir_func_t *func = block->func;
    if (func->versions == NULL) {
        func->versions = mx_ir_alloc(vm, sizeof(uintptr_t) * func->nblocks);
        memset(func->versions, 0, sizeof(uintptr_t) * func->nblocks);
        vm->stats.version_bytes += (uint32_t)(sizeof(uintptr_t) * func->nblocks);
    }
    return &func->versions[block->id];
}

void mx_version_slot_add(mx_t *vm, uintptr_t *slot, mx_version_t *rb) {
    if (*slot == 0) {
        *slot = (uintptr_t)rb;
        return;
    }
    mx_version_cache_t *cache;
    if ((*slot & 1) == 0) {
        cache = mx_ir_alloc(vm, sizeof(mx_version_cache_t));
        *cache = (mx_version_cache_t) {0};
        vm->stats.version_bytes += sizeof(mx_version_cache_t);
        cache->next = vm->version_caches;
        vm->version_caches = cache;
        mx_version_cache_add(cache, (mx_version_t *)*slot);
        *slot = (uintptr_t)cache | 1;
    } else {
        cache = (mx_version_cache_t *)(*slot & ~(uintptr_t)1);
    }
    mx_version_cache_add(cache, rb);
}

uint32_t mx_version_key_hash(const mx_kind_t *kinds, uint32_t nkinds, uint32_t start_at, bool is_hooked) {
    uint32_t h = mx_version_kinds_hash(kinds, nkinds);
    return (h ^ (start_at * 2 + (uint32_t)is_hooked)) * MX_VM_FNV_PRIME;
}

void mx_version_cache_add(mx_version_cache_t *cache, mx_version_t *rb) {
    if (cache->list.len == cache->list.cap) {
        cache->list.cap = cache->list.cap * 2 + 2;
        cache->list.ptr = mx_mem_realloc(cache->list.ptr, sizeof(mx_version_t *) * cache->list.cap);
    }
    cache->list.ptr[cache->list.len] = rb;
    cache->list.len += 1;
    if (cache->list.len >= 8 && cache->list.len * 4 > cache->table.cap * 3) {
        uint32_t ncap = 16;
        while (ncap < cache->list.len * 2) {
            ncap *= 2;
        }
        mx_mem_free(cache->table.ptr);
        cache->table.ptr = mx_mem_calloc(sizeof(mx_version_t *) * ncap);
        cache->table.cap = ncap;
        for (uint32_t i = 0; i < cache->list.len; i++) {
            mx_version_t *v = cache->list.ptr[i];
            uint32_t at = v->key_hash & (ncap - 1);
            while (cache->table.ptr[at] != NULL) {
                at = (at + 1) & (ncap - 1);
            }
            cache->table.ptr[at] = v;
        }
    } else if (cache->table.cap != 0) {
        uint32_t at = rb->key_hash & (cache->table.cap - 1);
        while (cache->table.ptr[at] != NULL) {
            at = (at + 1) & (cache->table.cap - 1);
        }
        cache->table.ptr[at] = rb;
    }
}

bool mx_version_matches(const mx_version_t *rb, const mx_kind_t *kinds, uint32_t nkinds, uint32_t start_at, bool is_hooked) {
    return rb->nkinds == nkinds
        && rb->start_at == start_at
        && rb->is_hooked == is_hooked
        && (nkinds == 0 || memcmp(rb->kinds, kinds, sizeof(mx_kind_t) * nkinds) == 0);
}

mx_version_t *mx_version_cache_find(const mx_version_cache_t *cache, uint32_t hash, const mx_kind_t *kinds, uint32_t nkinds, uint32_t start_at, bool is_hooked) {
    if (cache->table.cap != 0) {
        uint32_t at = hash & (cache->table.cap - 1);
        while (cache->table.ptr[at] != NULL) {
            mx_version_t *rb = cache->table.ptr[at];
            if (rb->key_hash == hash && mx_version_matches(rb, kinds, nkinds, start_at, is_hooked)) {
                return rb;
            }
            at = (at + 1) & (cache->table.cap - 1);
        }
    } else {
        for (uint32_t i = 0; i < cache->list.len; i++) {
            mx_version_t *rb = cache->list.ptr[i];
            if (rb->key_hash == hash && mx_version_matches(rb, kinds, nkinds, start_at, is_hooked)) {
                return rb;
            }
        }
    }
    return NULL;
}

mx_version_t *mx_version_for_at(mx_t *vm, mx_ir_block_t *block, const mx_kind_t *kinds, uint32_t nkinds, uint32_t start_at) {
    uintptr_t *slot = mx_version_slot(vm, block);
    while (nkinds > 0 && kinds[nkinds - 1] == MX_KIND_NIL) {
        nkinds -= 1;
    }
    bool is_hooked = vm->is_hooked;
    uint32_t hash = mx_version_key_hash(kinds, nkinds, start_at, is_hooked);
    mx_version_t *found = NULL;
    if ((*slot & 1) != 0) {
        found = mx_version_cache_find((const mx_version_cache_t *)(*slot - 1), hash, kinds, nkinds, start_at, is_hooked);
    } else if (*slot != 0) {
        found = (mx_version_t *)*slot;
        if (found->key_hash != hash || !mx_version_matches(found, kinds, nkinds, start_at, is_hooked)) {
            found = NULL;
        }
    }
    if (found != NULL) {
        return found;
    }
    const mx_kind_t *interned = mx_version_kinds_intern(vm, kinds, nkinds);
    mx_version_t *rb = mx_arena_alloc(&vm->arenas.code, sizeof(mx_version_t));
    *rb = (mx_version_t) {
        .block = block,
        .kinds = interned,
        .nkinds = nkinds,
        .start_at = start_at,
        .key_hash = hash,
        .frame = mx_ir_func_frame(block->func),
        .is_hooked = is_hooked,
        .is_vararg = block->func->is_vararg,
        .nparams = (uint16_t)block->func->nparams,
    };
    vm->stats.versions += 1;
    vm->stats.version_bytes += sizeof(mx_version_t);
    mx_version_slot_add(vm, slot, rb);
    mx_lower_version(vm, rb);
    return rb;
}

void mx_version_key_init(mx_version_key_t *key, uint32_t n) {
    if (n <= MX_VERSION_KINDS_INLINE) {
        key->kinds = key->inline_kinds;
    } else {
        key->kinds = mx_mem_alloc(sizeof(mx_kind_t) * n);
    }
    key->nkinds = n;
    for (uint32_t i = 0; i < n; i++) {
        key->kinds[i] = MX_KIND_NIL;
    }
}

void mx_version_key_free(mx_version_key_t *key) {
    if (key->kinds != key->inline_kinds) {
        mx_mem_free(key->kinds);
    }
}

void mx_version_key_copy(mx_version_key_t *key, const mx_kind_t *kinds, uint32_t nkinds, bool is_resumed) {
    for (uint32_t i = 0; i < key->nkinds && i < nkinds; i++) {
        mx_kind_t k = kinds[i];
        if (is_resumed && mx_kind_is_open(k)) {
            k = MX_KIND_CELL_ANY;
        }
        key->kinds[i] = k;
    }
}

void mx_version_key_live(mx_t *vm, mx_version_key_t *key, mx_ir_block_t *target) {
    if (target->func->live == NULL) {
        mx_ir_liveness(vm, mx_ir_func_head(target));
    }
    const uint64_t *live = mx_ir_block_live(target);
    for (uint32_t i = 0; i < key->nkinds; i++) {
        if (!mx_kind_is_cell(key->kinds[i]) && !mx_ir_live_test(live, i)) {
            key->kinds[i] = MX_KIND_NIL;
        }
    }
}

mx_version_t *mx_version_edge(mx_t *vm, const mx_kind_t *kinds, uint32_t nkinds, mx_ir_block_t *target) {
    mx_version_key_t key;
    mx_version_key_init(&key, target->func->nregs);
    mx_version_key_copy(&key, kinds, nkinds, false);
    mx_version_key_live(vm, &key, target);
    mx_version_t *rb = mx_version_for_at(vm, target, key.kinds, key.nkinds, 0);
    mx_version_key_free(&key);
    return rb;
}

void mx_version_key_outs(mx_version_key_t *key, uint32_t out, const mx_kind_t *outs, uint32_t nouts) {
    for (uint32_t i = 0; i < nouts && out + i < key->nkinds; i++) {
        if (mx_kind_is_open(key->kinds[out + i])) {
            key->kinds[out + i] = MX_KIND_CELL | outs[i];
        } else {
            key->kinds[out + i] = outs[i];
        }
    }
}

mx_version_t *mx_version_values(mx_t *vm, const mx_resume_t *rec, const mx_kind_t *outs, uint32_t nouts, bool is_resumed) {
    mx_version_key_t key;
    mx_version_key_init(&key, rec->target->func->nregs);
    mx_version_key_copy(&key, rec->kinds, rec->nkinds, is_resumed);
    mx_version_key_outs(&key, rec->out, outs, nouts);
    mx_version_key_live(vm, &key, rec->target);
    mx_version_t *rb = mx_version_for_at(vm, rec->target, key.kinds, key.nkinds, 0);
    mx_version_key_free(&key);
    return rb;
}

mx_version_t *mx_version_branch(mx_t *vm, const mx_resume_t *rec, bool is_true, bool is_resumed) {
    mx_ir_block_t *target;
    if (is_true) {
        target = rec->target;
    } else {
        target = rec->target2;
    }
    mx_version_key_t key;
    mx_version_key_init(&key, target->func->nregs);
    mx_version_key_copy(&key, rec->kinds, rec->nkinds, is_resumed);
    mx_version_key_live(vm, &key, target);
    mx_version_t *rb = mx_version_for_at(vm, target, key.kinds, key.nkinds, 0);
    mx_version_key_free(&key);
    return rb;
}

mx_version_t *mx_version_at(mx_t *vm, const mx_resume_t *rec, const mx_kind_t *outs, uint32_t nouts, bool is_resumed) {
    mx_version_key_t key;
    mx_version_key_init(&key, rec->target->func->nregs);
    mx_version_key_copy(&key, rec->kinds, rec->nkinds, is_resumed);
    mx_version_key_outs(&key, rec->out, outs, nouts);
    mx_version_t *rb = mx_version_for_at(vm, rec->target, key.kinds, key.nkinds, rec->resume_at);
    mx_version_key_free(&key);
    return rb;
}

mx_version_t *mx_version_again(mx_t *vm, mx_version_t *rb, bool is_resumed) {
    mx_version_key_t key;
    mx_version_key_init(&key, rb->block->func->nregs);
    mx_version_key_copy(&key, rb->kinds, rb->nkinds, is_resumed);
    mx_version_t *again = mx_version_for_at(vm, rb->block, key.kinds, key.nkinds, rb->start_at);
    mx_version_key_free(&key);
    return again;
}

// A function with at most three parameters remembers the version it last
// entered, keyed on the argument kinds, so a repeated call skips the lookup.
// The hit path is kept free of calls so it needs no stack frame; a miss tail
// calls the slow path, which also refreshes the remembered version.
mx_version_t *mx_version_entry(mx_t *vm, mx_ir_block_t *block, const mx_obj_t *args, uint32_t nargs) {
    mx_ir_func_t *func = block->func;
    uint64_t short_key = 0;
    if (func->nparams <= 3 && !vm->is_hooked) {
        short_key = (uint64_t)1 << 48;
        for (uint32_t i = 0; i < func->nparams && i < nargs; i++) {
            short_key |= (uint64_t)args[i].kind << (16 * i);
        }
        if (func->entry1 != NULL && func->entry1_key == short_key) {
            return func->entry1;
        }
    }
    return mx_version_entry_slow(vm, block, args, nargs, short_key);
}

mx_version_t *mx_version_entry_slow(mx_t *vm, mx_ir_block_t *block, const mx_obj_t *args, uint32_t nargs, uint64_t short_key) {
    mx_ir_func_t *func = block->func;
    mx_version_key_t key;
    mx_version_key_init(&key, func->nregs);
    key.kinds[0] = MX_KIND_CLOSURE;
    for (uint32_t i = 0; i < func->nparams && i < nargs; i++) {
        key.kinds[i + 1] = args[i].kind;
    }
    mx_version_t *rb = mx_version_for_at(vm, block, key.kinds, key.nkinds, 0);
    mx_version_key_free(&key);
    if (short_key != 0) {
        func->entry1 = rb;
        func->entry1_key = short_key;
    }
    return rb;
}

mx_version_t *mx_version_args(mx_t *vm, mx_ir_block_t *block, const mx_kind_t *kinds, uint32_t nargs) {
    mx_ir_func_t *func = block->func;
    uint64_t short_key = 0;
    if (func->nparams <= 3 && !vm->is_hooked) {
        short_key = (uint64_t)1 << 48;
        for (uint32_t i = 0; i < func->nparams && i < nargs; i++) {
            short_key |= (uint64_t)kinds[i] << (16 * i);
        }
        if (func->entry1 != NULL && func->entry1_key == short_key) {
            return func->entry1;
        }
    }
    return mx_version_args_slow(vm, block, kinds, nargs, short_key);
}

mx_version_t *mx_version_args_slow(mx_t *vm, mx_ir_block_t *block, const mx_kind_t *kinds, uint32_t nargs, uint64_t short_key) {
    mx_ir_func_t *func = block->func;
    mx_version_key_t key;
    mx_version_key_init(&key, func->nregs);
    key.kinds[0] = MX_KIND_CLOSURE;
    for (uint32_t i = 0; i < func->nparams && i < nargs; i++) {
        key.kinds[i + 1] = kinds[i];
    }
    mx_version_t *rb = mx_version_for_at(vm, block, key.kinds, key.nkinds, 0);
    mx_version_key_free(&key);
    if (short_key != 0) {
        func->entry1 = rb;
        func->entry1_key = short_key;
    }
    return rb;
}
