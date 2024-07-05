
#include "lib.h"

#if defined(__TINYC__)
void *end;
#endif

#if defined(VM_GC_MPS)
#include "../vendor/mps/code/mps.h"
#include "../vendor/mps/code/mpsavm.h"
#include "../vendor/mps/code/mpscmvff.h"
#include "../vendor/mps/code/mpstd.h"
#include "../vendor/mps/code/mpscmvt.h"


static mps_pool_t malloc_pool;

typedef union {
    size_t size;
    char alignment[MPS_PF_ALIGN]; /* see note below */
} header_u;

void vm_mem_init(void) {
    mps_arena_t arena;
    mps_res_t res;
    MPS_ARGS_BEGIN(args) {
        MPS_ARGS_ADD(args, MPS_KEY_ARENA_SIZE, 32 * 1024 * 1024);
        res = mps_arena_create_k(&arena, mps_arena_class_vm(), args);
    }
    MPS_ARGS_END(args);
    if (res != 0) {
        __builtin_trap();
    }
    MPS_ARGS_BEGIN(args) {
        res = mps_pool_create_k(&malloc_pool, arena, mps_class_mvff(), mps_args_none);
    }
    MPS_ARGS_END(args);
    if (res != 0) {
        __builtin_trap();
    }
}

void *vm_malloc(size_t size) {
    header_u *header;
    size += sizeof *header;
    mps_addr_t p;
    mps_res_t res = mps_alloc(&p, malloc_pool, size);
    if (res != MPS_RES_OK) {
        return NULL;
    }
    header = p;
    header->size = size;
    return header + 1;
}

void *vm_realloc(void *p, size_t size) {
    if (p == NULL) {
        return vm_malloc(size);
    }
    header_u *header = ((header_u *)p) - 1;
    void *ret = vm_malloc(size);
    memcpy(ret, p, header->size - sizeof(header_u));
    vm_free(p);
    return ret;
}

void vm_free(const void *p) {
    if (p) {
        header_u *header = ((header_u *)p) - 1;
        mps_free(malloc_pool, header, header->size);
    }
}

char *vm_strdup(const char *s) {
    size_t len = strlen(s);
    char *ret = vm_malloc(sizeof(char) * (len + 1));
    memcpy(ret, s, sizeof(char) * (len + 1));
    return ret;
}

#endif
