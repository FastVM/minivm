#ifndef TGC_H
#define TGC_H

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    TGC_MARK = 0x01,
    TGC_ROOT = 0x02,
    TGC_LEAF = 0x04
};

typedef struct {
    void *ptr;
    int flags;
    size_t size, hash;
    void (*dtor)(void *);
} tgc_ptr_t;

typedef struct {
    void *bottom;
    uintptr_t minptr, maxptr;
    tgc_ptr_t *items, *frees;
    double loadfactor, sweepfactor;
    size_t nitems, nslots, mitems, nfrees;
    bool paused : 1;
} tgc_t;

void tgc_start(tgc_t *gc, void *stk);
void tgc_stop(tgc_t *gc);
void tgc_pause(tgc_t *gc);
void tgc_resume(tgc_t *gc);
void tgc_run(tgc_t *gc);

void *tgc_alloc(tgc_t *gc, size_t size);
void *tgc_calloc(tgc_t *gc, size_t num, size_t size);
void *tgc_realloc(tgc_t *gc, void *ptr, size_t size);
void tgc_free(tgc_t *gc, void *ptr);

void *tgc_alloc_opt(tgc_t *gc, size_t size, int flags, void (*dtor)(void *));
void *tgc_calloc_opt(tgc_t *gc, size_t num, size_t size, int flags, void (*dtor)(void *));

void tgc_set_dtor(tgc_t *gc, void *ptr, void (*dtor)(void *));
void tgc_set_flags(tgc_t *gc, void *ptr, int flags);
int tgc_get_flags(tgc_t *gc, void *ptr);
void (*tgc_get_dtor(tgc_t *gc, void *ptr))(void *);
size_t tgc_get_size(tgc_t *gc, void *ptr);

#endif