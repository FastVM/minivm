// Arena-backed arrays without fully contiguous memory
#ifndef NL_CHUNKED_ARRAY_H
#define NL_CHUNKED_ARRAY_H

typedef struct NL_ArrChunk NL_ArrChunk;
struct NL_ArrChunk {
    NL_ArrChunk* next;
    size_t cap;
    size_t count;
    void* elems[];
};

typedef struct NL_ChunkedArr {
    TB_Arena* arena;
    TB_ArenaSavepoint sp;

    NL_ArrChunk* first;
    NL_ArrChunk* last;
} NL_ChunkedArr;

NL_ChunkedArr nl_chunked_arr_alloc(TB_Arena* arena);
void nl_chunked_arr_put(NL_ChunkedArr* arr, void* v);
void nl_chunked_arr_reset(NL_ChunkedArr* arr);

// we're done with growing it, so we don't wanna take up any more reserved space
void nl_chunked_arr_trim(NL_ChunkedArr* arr);

#endif // NL_CHUNKED_ARRAY_H

#ifdef NL_CHUNKED_ARRAY_IMPL
#include <common.h>

NL_ChunkedArr nl_chunked_arr_alloc(TB_Arena* arena) {
    TB_ArenaSavepoint sp = tb_arena_save(arena);
    tb_arena_realign(arena);

    ptrdiff_t leftovers = arena->high_point - (arena->watermark + sizeof(NL_ChunkedArr));
    if (leftovers < 64) {
        size_t chunk_size = tb_arena_chunk_size(arena);
        leftovers = chunk_size - (sizeof(NL_ArrChunk) + sizeof(TB_Arena));
    }

    ptrdiff_t num_elems = leftovers / sizeof(void*);
    NL_ArrChunk* arr = tb_arena_unaligned_alloc(arena, sizeof(NL_ArrChunk) + num_elems*sizeof(void*));
    arr->next = NULL;
    arr->cap = num_elems;
    arr->count = 0;

    return (NL_ChunkedArr){ arena, sp, arr, arr };
}

void nl_chunked_arr_put(NL_ChunkedArr* arr, void* v) {
    NL_ArrChunk* last = arr->last;
    if (last->cap == last->count) {
        // allocate new chunk
        size_t chunk_size = tb_arena_chunk_size(arr->arena);
        ptrdiff_t leftovers = chunk_size - (sizeof(NL_ArrChunk) + sizeof(TB_Arena));
        ptrdiff_t num_elems = leftovers / sizeof(void*);
        NL_ArrChunk* new_chk = tb_arena_alloc(arr->arena, sizeof(NL_ChunkedArr) + num_elems*sizeof(void*));

        // append
        arr->last->next = new_chk;
        arr->last = new_chk;
    }
    last->elems[last->count++] = v;
}

void* nl_chunked_arr_pop(NL_ChunkedArr* arr) {
    NL_ArrChunk* last = arr->last;
    return last->elems[--last->count];
}

void nl_chunked_arr_reset(NL_ChunkedArr* arr) {
    tb_arena_restore(arr->arena, arr->sp);
}

void nl_chunked_arr_trim(NL_ChunkedArr* arr) {
    TB_Arena* arena = arr->arena;

    void* top = &arr->last->elems[arr->last->count];
    tb_arena_pop(arena, top, (arr->last->cap - arr->last->count) * sizeof(void*));
    arr->last->cap = arr->last->count;

    tb_arena_realign(arena);
}

#endif // NL_CHUNKED_ARRAY_IMPL
