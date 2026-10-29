
#include "chunk.h"
#include "parse.h"
#include "../../ir/ir.h"
#include "../../ir/scratch.h"

uint32_t mx_chunk_hash(const char *src, uint32_t len) {
    uint32_t h = MX_VM_FNV_OFFSET;
    h = (h ^ len) * MX_VM_FNV_PRIME;
    uint32_t stride = len / 1024 + 1;
    for (uint32_t i = 0; i < len; i += stride) {
        h = (h ^ (uint8_t)src[i]) * MX_VM_FNV_PRIME;
    }
    return h;
}

mx_chunk_compiled_t mx_chunk_compile(mx_t *vm, const char *src, uint32_t len, const char *file) {
    uint32_t hash = mx_chunk_hash(src, len);
    mx_chunk_cache_t *slot = &vm->chunk_cache[hash & 255];
    bool is_hit = slot->entry != NULL
        && slot->hash == hash
        && slot->len == len
        && memcmp(slot->src, src, len) == 0
        && strcmp(slot->file, file) == 0;
    if (is_hit) {
        return (mx_chunk_compiled_t) { .entry = slot->entry, .error = NULL };
    }
    char *src_copy = mx_ir_alloc_bytes(vm, (size_t)len + 1);
    memcpy(src_copy, src, len);
    src_copy[len] = '\0';
    vm->arenas.ir.sources += len + 1;
    const char *file_copy = mx_ir_strdup(vm, file);
    mx_scratch_push(vm, &vm->arenas.scratch);
    mx_parse_compiled_t got = mx_parse_chunk(vm, src_copy, len, file_copy);
    mx_scratch_pop(vm);
    mx_arena_free(&vm->arenas.scratch);
    if (got.entry != NULL) {
        *slot = (mx_chunk_cache_t) {
            .src = src_copy,
            .file = file_copy,
            .entry = got.entry,
            .len = len,
            .hash = hash,
        };
    }
    return (mx_chunk_compiled_t) { .entry = got.entry, .error = got.error };
}
