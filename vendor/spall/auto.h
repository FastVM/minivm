// SPDX-FileCopyrightText: © 2024 Colin Davidson <colrdavidson@gmail.com>
// SPDX-License-Identifier: MIT

#ifndef SPALL_AUTO_H
#define SPALL_AUTO_H

// THIS IS EXPERIMENTAL, BUT VERY HANDY
// *should* work on clang and gcc on Windows, Mac, and Linux

#define SPALL_IS_WINDOWS 0
#define SPALL_IS_DARWIN  0
#define SPALL_IS_LINUX   0
#define SPALL_IS_GCC     0
#define SPALL_IS_CLANG   0
#define SPALL_IS_CPP     0
#define SPALL_IS_X64     0
#define SPALL_IS_ARM64   0

#ifdef __cplusplus
    #undef SPALL_IS_CPP
    #define SPALL_IS_CPP 1
#endif

#if defined(__clang__)
    #undef SPALL_IS_CLANG
    #define SPALL_IS_CLANG 1
#endif
#if defined(_WIN32)
    #undef SPALL_IS_WINDOWS
    #define SPALL_IS_WINDOWS 1
#elif defined(__APPLE__)
    #undef SPALL_IS_DARWIN
    #define SPALL_IS_DARWIN 1
#elif defined(__linux__)
    #undef SPALL_IS_LINUX
    #define SPALL_IS_LINUX 1
#endif
#ifdef __GNUC__
    #undef SPALL_IS_GCC
    #define SPALL_IS_GCC 1
#endif
#if defined(__x86_64__) || defined(_M_AMD64)
    #undef SPALL_IS_X64
    #define SPALL_IS_X64 1
#elif defined(__aarch64__)
    #undef SPALL_IS_ARM64
    #define SPALL_IS_ARM64 1
#endif

#if (!SPALL_IS_CLANG && !SPALL_IS_GCC)
#error "Compiler not supported!"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool spall_auto_init(char *filename);
void spall_auto_quit(void);
bool spall_auto_thread_init(uint32_t thread_id, size_t buffer_size);
void spall_auto_thread_quit(void);

bool spall_auto_buffer_begin(const char *name, signed long name_len, const char *args, signed long args_len);
bool spall_auto_buffer_end(void);
bool spall_auto_buffer_flush(void);

void spall_auto_set_thread_instrumenting(bool on);

#if SPALL_IS_GCC && SPALL_IS_CPP
    #define _Thread_local thread_local
#endif

#define SPALL_DEFAULT_BUFFER_SIZE (32 * 1024 * 1024)
#define SPALL_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define SPALL_MAX(a, b) (((a) > (b)) ? (a) : (b))

#ifdef __cplusplus
}
#endif
#endif // endif SPALL_AUTO_H

#ifdef SPALL_AUTO_IMPLEMENTATION
#ifndef SPALL_AUTO_IMPLEMENTED_H
#define SPALL_AUTO_IMPLEMENTED_H

#if !SPALL_IS_WINDOWS
    #if SPALL_IS_CPP
        #include <atomic>
    #else
        #include <stdatomic.h>
    #endif
#else
    #pragma comment(lib, "Synchronization")
    #include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if !SPALL_IS_WINDOWS
    #include <time.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <errno.h>
#endif

#if SPALL_IS_WINDOWS
    #include <windows.h>
    #include <process.h>

    typedef ptrdiff_t ssize_t;
    typedef HANDLE Spall_ThreadHandle;

    #define spall_thread_start(t) ((t)->writer.thread = (HANDLE) _beginthread(spall_writer, 0, t))
    #define spall_thread_end(t)   WaitForSingleObject((t)->writer.thread, INFINITE)
#else
    typedef pthread_t Spall_ThreadHandle;
    #define spall_thread_start(t) pthread_create(&(t)->writer.thread, NULL, spall_writer, (void *) (t))
    #define spall_thread_end(t)   pthread_join((t)->writer.thread, NULL)
#endif

#define SPALL_NOINSTRUMENT __attribute__((no_instrument_function))
#define SPALL_FORCE_INLINE __attribute__((always_inline)) inline
#define SPALL_FORCE_NOINLINE __attribute__((noinline))
#define __debugbreak() __builtin_trap()

#if SPALL_IS_CPP
    #define Spall_Atomic(X) std::atomic<X>
#else
    #define Spall_Atomic(X) _Atomic (X)
#endif

#define SPALL_FN static SPALL_NOINSTRUMENT

#if SPALL_IS_X64
#include <x86intrin.h>
SPALL_FN uint64_t spall_get_clock(void) {
    return __rdtsc();
}
SPALL_FN void spall_pause(void) {
    _mm_pause();
}
#elif SPALL_IS_ARM64
SPALL_FN uint64_t spall_get_clock(void) {
    int64_t timer_val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(timer_val));
    return (uint64_t)timer_val;
}
SPALL_FN void spall_pause(void) {
    asm volatile("yield");
}
#endif

#pragma pack(push, 1)

typedef struct SpallHeader {
    uint64_t magic_header; // = 0xABADF00D
    uint64_t version; // = 3
    double   timestamp_unit;
    uint64_t base_address;
    uint16_t program_path_len;
} SpallHeader;

enum {
    SpallAutoEventType_Invalid = 0,
    SpallAutoEventType_Begin   = 1,
};

typedef struct SpallMicroBeginEventMax {
    uint8_t type;
    uint64_t ts;
    uint64_t caller;
} SpallMicroBeginEventMax;

typedef struct SpallMicroEndEventMax {
    uint8_t type;
    uint64_t ts;
} SpallMicroEndEventMax;

typedef struct SpallAutoBeginEvent {
    uint8_t type;
    uint64_t when;
} SpallAutoBeginEvent;

typedef struct SpallBufferHeader {
    uint32_t size;
    uint32_t tid;
    uint64_t first_ts;
    uint32_t max_depth;
} SpallBufferHeader;

#pragma pack(pop)

SPALL_FN SPALL_FORCE_INLINE uint64_t spall_delta_to_bits(uint64_t dt) {
	uint32_t count = 0;
	count += (dt >= 0x100);
	count += (dt >= 0x10000);
	count += (dt >= 0x100000000);
	return count;
}

typedef struct SpallProfile {
    double stamp_scale;
    FILE *file;
} SpallProfile;

typedef Spall_Atomic(uint64_t) Spall_Futex;
typedef struct SpallBuffer {
    uint8_t *data;
    size_t   length;

    // if true, write to upper-half, else lower-half
    size_t sub_length;
    bool   write_half;

    struct {
        Spall_Atomic(bool)     is_running;
        Spall_ThreadHandle     thread;
        Spall_Atomic(uint64_t) ptr;
        Spall_Atomic(size_t)   size;
    } writer;

    size_t   head;
    uint32_t thread_id;

    uint64_t previous_ts;
    uint64_t first_ts;

    uint64_t previous_addr;
    uint64_t previous_caller;

    uint32_t current_depth;
    uint32_t max_depth;
} SpallBuffer;


// Cross-platform wrappers
#if SPALL_IS_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <asm/unistd.h>
#include <linux/futex.h>
#include <linux/limits.h>
#include <linux/perf_event.h>
#include <sys/auxv.h>
#include <elf.h>

SPALL_FN uint64_t spall_auto_get_base_address(void) {
	uint64_t hdr_size = 0;
	if (sizeof(void *) == 8) {
		hdr_size = sizeof(Elf64_Ehdr);
	} else {
		hdr_size = sizeof(Elf32_Ehdr);
	}

	uint64_t phdr = getauxval(AT_PHDR);
	uint64_t addr = phdr - hdr_size;
	return addr;
}

SPALL_FN bool get_program_path(char **out_path) {
    char path[PATH_MAX+1] = {0};
    uint32_t size = sizeof(path);

    ssize_t buff_len = (ssize_t)readlink("/proc/self/exe", path, size - 1);
    if (buff_len == -1) {
        *out_path = NULL;
        return false;
    }

    char *post_path = (char *)calloc(PATH_MAX+1, 1);
    if (realpath(path, post_path) == NULL) {
        free(post_path);
        *out_path = NULL;
        return false;
    }

    *out_path = post_path;
    return true;
}

SPALL_FN uint64_t mul_u64_u32_shr(uint64_t cyc, uint32_t mult, uint32_t shift) {
    __uint128_t x = cyc;
    x *= mult;
    x >>= shift;
    return (uint64_t)x;
}

SPALL_FN long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

#if SPALL_IS_X64
SPALL_FN double spall_get_clock_multiplier(void) {
    struct perf_event_attr pe = {
        .type = PERF_TYPE_HARDWARE,
        .size = sizeof(struct perf_event_attr),
        .config = PERF_COUNT_HW_INSTRUCTIONS,
        .disabled = 1,
        .exclude_kernel = 1,
        .exclude_hv = 1
    };

    int fd = (int)perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        perror("perf_event_open failed");
        return 1;
    }
    void *addr = mmap(NULL, 4*1024, PROT_READ, MAP_SHARED, fd, 0);
    if (!addr) {
        perror("mmap failed");
        return 1;
    }
    struct perf_event_mmap_page *pc = (struct perf_event_mmap_page *)addr;
    if (pc->cap_user_time != 1) {
        fprintf(stderr, "Perf system doesn't support user time\n");
        return 1;
    }
    double nanos = (double)mul_u64_u32_shr(1000000000000000ull, pc->time_mult, pc->time_shift);
    double multiplier = nanos / 1000000000000000.0;
    return multiplier;
}
#endif


SPALL_FN SPALL_FORCE_INLINE void spall_signal(Spall_Futex *addr) {
    long ret = syscall(SYS_futex, addr, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, 1, NULL, NULL, 0);
    if (ret == -1) {
        perror("Futex wake");
        __debugbreak();
    }
}

SPALL_FN SPALL_FORCE_INLINE void spall_wait(Spall_Futex *addr, uint64_t val) {
    for (;;) {
        long ret = syscall(SYS_futex, addr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, val, NULL, NULL, 0);
        if (ret == -1) {
            if (errno != EAGAIN) {
                perror("Futex wait");
                __debugbreak();
            } else {
                return;
            }
        } else if (ret == 0) {
            return;
        }
    }
}

#elif SPALL_IS_DARWIN

#include <mach-o/ldsyms.h>
#include <mach-o/dyld.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <dlfcn.h>

SPALL_FN uint64_t spall_auto_get_base_address(void) {
	return (uint64_t)&_mh_execute_header;
}

#if SPALL_IS_X64
SPALL_FN double spall_get_clock_multiplier(void) {
    uint64_t freq;
    size_t size = sizeof(freq);

    sysctlbyname("machdep.tsc.frequency", &freq, &size, NULL, 0);
    return 1000000000.0 / (double)freq;
}
#elif SPALL_IS_ARM64
SPALL_FN double spall_get_clock_multiplier(void) {
    uint64_t freq_val;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(freq_val));

    double multiplier = 1000000000.0 / (double)freq_val;
    return multiplier;
}
#endif

SPALL_FN bool get_program_path(char **out_path) {
    char pre_path[1025];
    uint32_t size = sizeof(pre_path);
    if (_NSGetExecutablePath(pre_path, &size) == -1) {
        *out_path = NULL;
        return false;
    }

    char *post_path = (char *)malloc(1025);
    if (realpath(pre_path, post_path) == NULL) {
        free(post_path);
        *out_path = NULL;
        return false;
    }

    *out_path = post_path;
    return true;
}

#define UL_COMPARE_AND_WAIT 0x00000001
#define ULF_WAKE_ALL        0x00000100
#define ULF_NO_ERRNO        0x01000000

/* timeout is specified in microseconds */
int __ulock_wait(uint32_t operation, void *addr, uint64_t value, uint32_t timeout);
int __ulock_wake(uint32_t operation, void *addr, uint64_t wake_value);

SPALL_FN SPALL_FORCE_INLINE void spall_signal(Spall_Futex *addr) {
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
        printf("futex signal fail?\n");
        __debugbreak();
    }
}

SPALL_FN SPALL_FORCE_INLINE void spall_wait(Spall_Futex *addr, uint64_t val) {
    for (;;) {
        int ret = __ulock_wait(UL_COMPARE_AND_WAIT | ULF_NO_ERRNO, addr, val, 0);
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

        printf("futex wait fail? %d\n", ret);
        __debugbreak();
    }
}

#elif SPALL_IS_WINDOWS

SPALL_FN uint64_t spall_auto_get_base_address(void) {
	return (uint64_t)GetModuleHandleW(NULL);
}

SPALL_FN bool get_program_path(char **out_path) {
    char *post_path = (char *)calloc(MAX_PATH, 1);
    if (GetModuleFileNameA(NULL, post_path, MAX_PATH) == 0) {
        *out_path = NULL;
        return false;
    }

    *out_path = post_path;
    return true;
}

SPALL_FN SPALL_FORCE_INLINE double spall_get_clock_multiplier(void) {

    // Cache the answer so that multiple calls never take the slow path more than once
    static double multiplier = 0;
    if (multiplier) {
        return multiplier;
    }

    uint64_t tsc_freq = 0;

    // Get time before sleep
    uint64_t qpc_begin = 0; QueryPerformanceCounter((LARGE_INTEGER *)&qpc_begin);
    uint64_t tsc_begin = spall_get_clock();

    Sleep(2);

    // Get time after sleep
    uint64_t qpc_end = qpc_begin + 1; QueryPerformanceCounter((LARGE_INTEGER *)&qpc_end);
    uint64_t tsc_end = spall_get_clock();

    // Do the math to extrapolate the RDTSC ticks elapsed in 1 second
    uint64_t qpc_freq = 0; QueryPerformanceFrequency((LARGE_INTEGER *)&qpc_freq);
    tsc_freq = (tsc_end - tsc_begin) * qpc_freq / (qpc_end - qpc_begin);

    multiplier = 1000000000.0 / (double)tsc_freq;
    return multiplier;
}

SPALL_FN SPALL_FORCE_INLINE void spall_signal(Spall_Futex *addr) {
    WakeByAddressSingle((void *)addr);
}

SPALL_FN SPALL_FORCE_INLINE void spall_wait(Spall_Futex *addr, uint64_t val) {
    WaitOnAddress(addr, (void *)&val, sizeof(val), INFINITE);
}

#endif

// Auto-tracing impl
SpallProfile spall_ctx;
_Thread_local SpallBuffer *spall_buffer = NULL;
_Thread_local bool spall_thread_running = false;

SPALL_NOINSTRUMENT void spall_auto_set_thread_instrumenting(bool on) {
    spall_thread_running = on;
}

#if SPALL_IS_WINDOWS
SPALL_FN void spall_writer(void *arg) {
#else
SPALL_FN void *spall_writer(void *arg) {
#endif

    SpallBuffer *buffer = (SpallBuffer *)arg;
    while (buffer->writer.is_running) {
        spall_wait(&buffer->writer.ptr, 0);
        if (!buffer->writer.is_running) { break; }

        void *buffer_ptr = (void *)atomic_load(&buffer->writer.ptr);
        if (buffer_ptr == 0) { continue; }

        size_t size = (size_t)atomic_load(&buffer->writer.size);
        atomic_store(&buffer->writer.ptr, 0);

        fwrite(buffer_ptr, size, 1, spall_ctx.file);
    }

#if !SPALL_IS_WINDOWS
    return NULL;
#endif
}

SPALL_NOINSTRUMENT SPALL_FORCE_INLINE bool spall__file_write(void *p, size_t n) {
    atomic_store(&spall_buffer->writer.size, n);
    atomic_store(&spall_buffer->writer.ptr, (uint64_t)p);
    spall_signal(&spall_buffer->writer.ptr);

    for (;;) {
        void *ptr = (void *)atomic_load(&spall_buffer->writer.ptr);
        if (ptr == 0) { break; }
        spall_pause();
    }

    return true;
}

SPALL_NOINSTRUMENT SPALL_FORCE_INLINE bool spall_auto_buffer_flush(void) {
    if (!spall_buffer) return false;

    size_t data_start = spall_buffer->write_half ? spall_buffer->sub_length : 0;

    SpallBufferHeader *sbp = (SpallBufferHeader *)(spall_buffer->data + data_start);
    if (spall_buffer->head > 0) {
        sbp->size = (uint32_t)(spall_buffer->head - sizeof(SpallBufferHeader));
        sbp->first_ts = spall_buffer->first_ts;
        sbp->max_depth = spall_buffer->max_depth;
        if (!spall__file_write(spall_buffer->data + data_start, spall_buffer->head)) return false;

        spall_buffer->write_half = !spall_buffer->write_half;
    }

    data_start = spall_buffer->write_half ? spall_buffer->sub_length : 0;
    sbp = (SpallBufferHeader *)(spall_buffer->data + data_start);
    sbp->size = 0;
    sbp->first_ts = 0;
    sbp->tid = spall_buffer->thread_id;

    spall_buffer->head = sizeof(SpallBufferHeader);
    spall_buffer->first_ts = 0;
    spall_buffer->previous_ts = 0;
    spall_buffer->previous_addr = 0;
    spall_buffer->previous_caller = 0;
    return true;
}

SPALL_FN SPALL_FORCE_INLINE bool spall_buffer_micro_begin(uint64_t addr, uint64_t caller) {
    spall_buffer->current_depth += 1;
    spall_buffer->max_depth = SPALL_MAX(spall_buffer->max_depth, spall_buffer->current_depth);

    size_t ev_size = sizeof(SpallMicroBeginEventMax);
    if ((spall_buffer->head + ev_size) > spall_buffer->sub_length) {
        if (!spall_auto_buffer_flush()) {
            return false;
        }
    }

    size_t data_start = spall_buffer->write_half ? spall_buffer->sub_length : 0;
    uint8_t *ev_buffer = (spall_buffer->data + data_start) + spall_buffer->head;

    uint64_t now = spall_get_clock();
    if (spall_buffer->first_ts == 0) {
        spall_buffer->first_ts = now;
        spall_buffer->previous_ts = now;
    }

    uint64_t dt       = now    - spall_buffer->previous_ts;
    uint64_t d_addr   = addr   ^ spall_buffer->previous_addr;
    uint64_t d_caller = caller ^ spall_buffer->previous_caller;

    uint64_t dt_bits     = spall_delta_to_bits(dt);
    uint64_t addr_bits   = spall_delta_to_bits(d_addr);
    uint64_t caller_bits = spall_delta_to_bits(d_caller);

    uint64_t dt_size     = 1 << dt_bits;
    uint64_t addr_size   = 1 << addr_bits;
    uint64_t caller_size = 1 << caller_bits;

    // [begin event tag | size of ts | size of addr | size of caller]
    uint8_t type_byte = (0 << 6) | (dt_bits << 4) | (addr_bits << 2) | caller_bits;

    int i = 0;
    *(ev_buffer + i) = type_byte; i += 1;
    memcpy(ev_buffer + i, &dt,       8);     i += dt_size;
    memcpy(ev_buffer + i, &d_addr,   8);   i += addr_size;
    memcpy(ev_buffer + i, &d_caller, 8); i += caller_size;

    spall_buffer->previous_ts = now;
    spall_buffer->previous_addr = addr;
    spall_buffer->previous_caller = caller;
    spall_buffer->head += i;

    return true;
}

SPALL_FN SPALL_FORCE_INLINE bool spall_buffer_micro_end(void) {
    uint64_t now = spall_get_clock();
    spall_buffer->current_depth -= 1;

    size_t ev_size = sizeof(SpallMicroEndEventMax);
    if ((spall_buffer->head + ev_size) > spall_buffer->sub_length) {
        if (!spall_auto_buffer_flush()) {
            return false;
        }
    }
    if (spall_buffer->first_ts == 0) {
        spall_buffer->first_ts = now;
        spall_buffer->previous_ts = now;
    }

    size_t data_start = spall_buffer->write_half ? spall_buffer->sub_length : 0;
    uint8_t *ev_buffer = (spall_buffer->data + data_start) + spall_buffer->head;

    uint64_t dt = now - spall_buffer->previous_ts;
    uint64_t dt_bits = spall_delta_to_bits(dt);
    uint64_t dt_size = 1 << dt_bits;

    // [end event tag | size of ts]
    uint8_t type_byte = (1 << 6) | (dt_bits << 4);

    int i = 0;
    *(ev_buffer + i) = type_byte; i += 1;
    memcpy(ev_buffer + i, &dt, 8); i += dt_size;

    spall_buffer->previous_ts = now;
    spall_buffer->head += i;
    return true;
}

SPALL_NOINSTRUMENT bool spall_auto_buffer_begin(const char *name, signed long name_len, const char *args, signed long args_len) {

    spall_buffer->current_depth += 1;
    spall_buffer->max_depth = SPALL_MAX(spall_buffer->max_depth, spall_buffer->current_depth);

    uint16_t trunc_name_len = (uint16_t)SPALL_MIN(name_len, UINT16_MAX);
    uint16_t trunc_args_len = (uint16_t)SPALL_MIN(args_len, UINT16_MAX);
    uint64_t name_len_size = (trunc_name_len > 255) ? 2 : 1;
    uint64_t args_len_size = (trunc_args_len > 255) ? 2 : 1;

    uint64_t event_tail = trunc_name_len + name_len_size + trunc_args_len + args_len_size;
    if ((spall_buffer->head + sizeof(SpallAutoBeginEvent) + event_tail) > spall_buffer->sub_length) {
        if (!spall_auto_buffer_flush()) {
            return false;
        }
    }

    size_t data_start = spall_buffer->write_half ? spall_buffer->sub_length : 0;
    uint8_t *ev_buffer = (spall_buffer->data + data_start) + spall_buffer->head;

    uint64_t now = spall_get_clock();
    if (spall_buffer->first_ts == 0) {
        spall_buffer->first_ts = now;
        spall_buffer->previous_ts = now;
    }
    uint64_t dt = now - spall_buffer->previous_ts;
    uint64_t dt_bits = spall_delta_to_bits(dt);
    uint64_t dt_size = 1 << dt_bits;

    // [extended tag | begin type | delta size | field lengths]
    uint8_t name_args_lens = ((name_len_size >> 1) << 1) | (args_len_size >> 1);
    uint8_t type_byte = (2 << 6) | (SpallAutoEventType_Begin << 4) | (dt_bits << 2) | name_args_lens;

    int i = 0;
    *(ev_buffer + i) = type_byte;              i += 1;
    memcpy(ev_buffer + i, &dt, 8);             i += dt_size;
    memcpy(ev_buffer + i, &trunc_name_len, name_len_size); i += name_len_size;
    memcpy(ev_buffer + i, &trunc_args_len, args_len_size); i += args_len_size;
    memcpy(ev_buffer + i, name, trunc_name_len); i += trunc_name_len;
    memcpy(ev_buffer + i, args, trunc_args_len); i += trunc_args_len;

    spall_buffer->previous_ts = now;
    spall_buffer->head += i;

    return true;
}

SPALL_NOINSTRUMENT bool spall_auto_buffer_end(void) {
    return spall_buffer_micro_end();
}

SPALL_NOINSTRUMENT SPALL_FORCE_INLINE bool spall_auto_thread_init(uint32_t thread_id, size_t buffer_size) {
    if (buffer_size < 512) { return false; }
    if (spall_buffer != NULL) { return false; }

    spall_buffer = (SpallBuffer *)calloc(sizeof(SpallBuffer), 1);
    spall_buffer->data = (uint8_t *)malloc(buffer_size);
    spall_buffer->length = buffer_size;
    spall_buffer->thread_id = thread_id;
    spall_buffer->sub_length = buffer_size / 2;

    // removing initial page-fault bubbles to make the data a little more accurate, at the cost of thread spin-up time
    memset(spall_buffer->data, 1, spall_buffer->length);

    spall_buffer->writer.is_running = true;
    spall_thread_start(spall_buffer);

    spall_auto_buffer_flush();
    spall_thread_running = true;
    return true;
}

SPALL_NOINSTRUMENT SPALL_FORCE_INLINE void spall_auto_thread_quit(void) {
    spall_thread_running = false;
    spall_auto_buffer_flush();

    spall_buffer->writer.is_running = false;
    spall_buffer->writer.ptr = 1;
    spall_signal(&spall_buffer->writer.ptr);
    spall_thread_end(spall_buffer);

    free(spall_buffer->data);
    free(spall_buffer);
    spall_buffer = NULL;
}

SPALL_FN void *spall_canonical_addr(void* fn) {
    // sometimes the pointer we get back is to a jump table; walk past that first layer.

    void *ret = fn;
#if SPALL_IS_X64
    unsigned char *fn_data = (unsigned char *)fn;
    if (fn_data[0] == 0xE9) {
        // JMP rel32
        int32_t target = *(int32_t*) &fn_data[1];

        int jump_inst_size = 5;
        ret = (void *)(fn_data + jump_inst_size + target);
    }
#endif

    return ret;
}


SPALL_NOINSTRUMENT SPALL_FORCE_NOINLINE bool spall_auto_init(char *filename) {
    if (!filename) return false;
    memset(&spall_ctx, 0, sizeof(spall_ctx));

    spall_ctx.file = fopen(filename, "wb"); // TODO: handle utf8 and long paths on windows
    if (spall_ctx.file) { // basically freopen() but we don't want to force users to lug along another macro define
        fclose(spall_ctx.file);
        spall_ctx.file = fopen(filename, "ab");
    }
    if (!spall_ctx.file) { return false; }

    spall_ctx.stamp_scale = spall_get_clock_multiplier();
    SpallHeader header = {0};
    header.magic_header = 0xABADF00D;
    header.version = 3;
    header.timestamp_unit = spall_ctx.stamp_scale;
    header.base_address = spall_auto_get_base_address();

    char *program_path;
    if (!get_program_path(&program_path)) { return false; }
    uint16_t program_path_len = (uint16_t)strlen(program_path);

    header.program_path_len = program_path_len;

    size_t full_header_size = sizeof(SpallHeader) + (size_t)program_path_len;
    uint8_t *full_header = (uint8_t *)malloc(full_header_size);
    memcpy(full_header, &header, sizeof(SpallHeader));
    memcpy(full_header + sizeof(SpallHeader), program_path, program_path_len);

    size_t write_ret = fwrite(full_header, 1, full_header_size, spall_ctx.file);
    if (write_ret < full_header_size) { return false; }

    free(full_header);
    return true;
}

SPALL_NOINSTRUMENT void spall_auto_quit(void) {}

SPALL_NOINSTRUMENT void __cyg_profile_func_enter(void *fn, void *caller) {
    if (!spall_thread_running) {
        return;
    }
    fn = spall_canonical_addr(fn);

    spall_thread_running = false;
    spall_buffer_micro_begin((uint64_t)fn, (uint64_t)caller);
    spall_thread_running = true;
}

SPALL_NOINSTRUMENT void __cyg_profile_func_exit(void *fn, void *caller) {
    if (!spall_thread_running) {
        return;
    }

    spall_thread_running = false;
    spall_buffer_micro_end();
    spall_thread_running = true;
}

#ifdef __cplusplus
}
#endif

#endif
#endif
