// Common is just a bunch of crap i want accessible to all projects in the Cuik repo
#include "arena.h"
#include "futex.h"
#include <stdatomic.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#define NL_MAP_IMPL
#include <hash_map.h>

#define NL_HASH_SET_IMPL
#include <new_hash_map.h>

#define NL_CHUNKED_ARRAY_IMPL
#include <chunked_array.h>

#define LOG_USE_COLOR
#include "log.c"

#include "perf.h"

uint64_t cuik__page_size = 0;
uint64_t cuik__page_mask = 0;

void cuik_init_terminal(void) {
    #if _WIN32
    // Raw input mode
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT);

    // Enable ANSI/VT sequences on windows
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output_handle != INVALID_HANDLE_VALUE) {
        DWORD old_mode;
        if (GetConsoleMode(output_handle, &old_mode)) {
            SetConsoleMode(output_handle, old_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
    #endif
}

void* cuik__valloc(size_t size) {
    #ifdef _WIN32
    if (cuik__page_size == 0) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);

        cuik__page_size = si.dwPageSize;
        cuik__page_mask = si.dwPageSize - 1;
    }

    // round size to page size
    size = (size + cuik__page_mask) & ~cuik__page_mask;

    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    #else
    cuik__page_size = 4096;
    cuik__page_mask = 4095;

    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    #endif
}

void cuik__vfree(void* ptr, size_t size) {
    #ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
    #else
    munmap(ptr, size);
    #endif
}

////////////////////////////////
// TB_Arenas
////////////////////////////////
void tb_arena_create(TB_Arena* restrict arena, size_t chunk_size) {
    if (chunk_size == 0) {
        chunk_size = TB_ARENA_LARGE_CHUNK_SIZE;
    }

    // allocate initial chunk
    TB_ArenaChunk* c = cuik__valloc(chunk_size);
    c->next = NULL;

    arena->chunk_size = chunk_size;
    arena->watermark  = c->data;
    arena->high_point = &c->data[chunk_size - sizeof(TB_ArenaChunk)];
    arena->base = arena->top = c;
}

void tb_arena_destroy(TB_Arena* restrict arena) {
    TB_ArenaChunk* c = arena->base;
    while (c != NULL) {
        TB_ArenaChunk* next = c->next;
        cuik__vfree(c, arena->chunk_size);
        c = next;
    }
}

void* tb_arena_unaligned_alloc(TB_Arena* restrict arena, size_t size) {
    if (LIKELY(arena->watermark + size < arena->high_point)) {
        char* ptr = arena->watermark;
        arena->watermark += size;
        return ptr;
    } else {
        // slow path, we need to allocate more
        TB_ArenaChunk* c = cuik__valloc(arena->chunk_size);
        c->next = NULL;

        arena->watermark  = c->data + size;
        arena->high_point = &c->data[arena->chunk_size - sizeof(TB_ArenaChunk)];

        // append to top
        arena->top->next = c;
        arena->top = c;

        return c->data;
    }
}

TB_API void* tb_arena_realloc(TB_Arena* restrict arena, void* old, size_t size) {
    char* p = old;
    if (p + size == arena->watermark) {
        // try to resize
        arena->watermark = old;
    }

    char* dst = tb_arena_unaligned_alloc(arena, size);
    if (dst != p && old) {
        memcpy(dst, old, size);
    }
    return dst;
}

void tb_arena_pop(TB_Arena* restrict arena, void* ptr, size_t size) {
    char* p = ptr;
    assert(p + size == arena->watermark); // cannot pop from arena if it's not at the top

    arena->watermark = p;
}

bool tb_arena_free(TB_Arena* restrict arena, void* ptr, size_t size) {
    size = (size + TB_ARENA_ALIGNMENT - 1) & ~(TB_ARENA_ALIGNMENT - 1);

    char* p = ptr;
    if (p + size == arena->watermark) {
        arena->watermark = p;
        return true;
    } else {
        return false;
    }
}

void tb_arena_realign(TB_Arena* restrict arena) {
    ptrdiff_t pos = arena->watermark - arena->top->data;
    pos = (pos + TB_ARENA_ALIGNMENT - 1) & ~(TB_ARENA_ALIGNMENT - 1);

    arena->watermark = &arena->top->data[pos];
}

TB_ArenaSavepoint tb_arena_save(TB_Arena* arena) {
    return (TB_ArenaSavepoint){ arena->top, arena->watermark };
}

void tb_arena_restore(TB_Arena* arena, TB_ArenaSavepoint sp) {
    // kill any chunks which are ahead of the top
    TB_ArenaChunk* c = sp.top->next;
    while (c != NULL) {
        TB_ArenaChunk* next = c->next;
        cuik__vfree(c, arena->chunk_size);
        c = next;
    }

    arena->top = sp.top;
    arena->top->next = NULL;
    arena->watermark = sp.watermark;
    arena->high_point = &sp.top->data[arena->chunk_size - sizeof(TB_ArenaChunk)];
}

void* tb_arena_alloc(TB_Arena* restrict arena, size_t size) {
    uintptr_t wm = (uintptr_t) arena->watermark;
    assert((wm & ~0xFull) == wm);

    size = (size + TB_ARENA_ALIGNMENT - 1) & ~(TB_ARENA_ALIGNMENT - 1);
    return tb_arena_unaligned_alloc(arena, size);
}

void tb_arena_clear(TB_Arena* arena) {
    TB_ArenaChunk* c = arena->base;
    if (c == NULL) return;

    arena->watermark = c->data;
    arena->high_point = &c->data[arena->chunk_size - sizeof(TB_ArenaChunk)];
    arena->base = arena->top = c;

    // remove extra chunks
    c = c->next;
    while (c != NULL) {
        TB_ArenaChunk* next = c->next;
        cuik__vfree(c, arena->chunk_size);
        c = next;
    }
}

bool tb_arena_is_empty(TB_Arena* arena) {
    return arena->base == NULL;
}

size_t tb_arena_current_size(TB_Arena* arena) {
    size_t total = 0;
    TB_ArenaChunk* c = arena->base;
    while (c != arena->top) {
        total += arena->chunk_size;
        c = c->next;
    }

    return total + (arena->watermark - (char*) arena->top);
}

////////////////////////////////
// Futex functions
////////////////////////////////
void futex_dec(Futex* f) {
    if (atomic_fetch_sub(f, 1) == 1) {
        futex_signal(f);
    }
}

#ifdef __linux__
#include <errno.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

// [https://man7.org/linux/man-pages/man2/futex.2.html]
//   glibc provides no wrapper for futex()
#define futex(...) syscall(SYS_futex, __VA_ARGS__)

void futex_signal(Futex* addr) {
    int ret = futex(addr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, NULL, NULL, 0);
    if (ret == -1) {
        __builtin_trap();
    }
}

void futex_broadcast(Futex* addr) {
    int ret = futex(addr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, INT32_MAX, NULL, NULL, 0);
    if (ret == -1) {
        __builtin_trap();
    }
}

void futex_wait(Futex* addr, Futex val) {
    for (;;) {
        int ret = futex(addr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, val, NULL, NULL, 0);

        if (ret == -1) {
            if (errno != EAGAIN) {
                __builtin_trap();
            }
        } else if (ret == 0) {
            if (*addr != val) {
                return;
            }
        }
    }
}

#undef futex
#elif defined(__APPLE__)

#include <errno.h>

enum {
    UL_COMPARE_AND_WAIT = 0x00000001,
    ULF_WAKE_ALL        = 0x00000100,
    ULF_NO_ERRNO        = 0x01000000
};

/* timeout is specified in microseconds */
int __ulock_wait(uint32_t operation, void *addr, uint64_t value, uint32_t timeout);
int __ulock_wake(uint32_t operation, void *addr, uint64_t wake_value);

void futex_signal(Futex* addr) {
    for (;;) {
        int ret = __ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, addr, 0);
        if (ret >= 0) {
            return;
        }
        ret = -ret;
        if (ret == EINTR || ret == EFAULT) {
            continue;
        }
        if (ret == ENOENT) {
            return;
        }
        printf("futex wake fail?\n");
        __builtin_trap();
    }
}

void _tpool_broadcast(Futex* addr) {
    for (;;) {
        int ret = __ulock_wake(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO | ULF_WAKE_ALL, addr, 0);
        if (ret >= 0) {
            return;
        }
        ret = -ret;
        if (ret == EINTR || ret == EFAULT) {
            continue;
        }
        if (ret == ENOENT) {
            return;
        }
        printf("futex wake fail?\n");
        __builtin_trap();
    }
}

void futex_wait(Futex* addr, Futex val) {
    for (;;) {
        int ret = __ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, addr, val, 0);
        if (ret >= 0) {
            if (*addr != val) {
                return;
            }
            continue;
        }
        ret = -ret;
        if (ret == EINTR || ret == EFAULT) {
            continue;
        }
        if (ret == ENOENT) {
            return;
        }

        printf("futex wait fail?\n");
    }
}

#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef __GNUC__
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Synchronization.lib")
#endif

void futex_signal(Futex* addr) {
    WakeByAddressSingle((void*) addr);
}

void futex_broadcast(Futex* addr) {
    WakeByAddressAll((void*) addr);
}

void futex_wait(Futex* addr, Futex val) {
    for (;;) {
        WaitOnAddress(addr, (void *)&val, sizeof(val), INFINITE);
        if (*addr != val) break;
    }
}
#endif

void futex_wait_eq(Futex* addr, Futex val) {
    while (*addr != val) {
        futex_wait(addr, *addr);
    }
}
