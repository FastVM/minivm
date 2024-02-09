#include <common.h>
#include <perf.h>
#include <threads.h>
#include <stdarg.h>
#include <stdatomic.h>

#ifdef CUIK_USE_SPALL_AUTO
#define SPALL_BUFFER_PROFILING
#define SPALL_BUFFER_PROFILING_GET_TIME() __rdtsc()
#define SPALL_AUTO_IMPLEMENTATION
#include "spall_native_auto.h"
#else
#define SPALL_BUFFER_PROFILING
#define SPALL_BUFFER_PROFILING_GET_TIME() cuik_time_in_nanos()
#include "spall.h"
#endif

#if defined(_AMD64_) || defined(__amd64__)
static double rdtsc_freq;
static uint64_t timer_start;
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#endif

static _Atomic bool profiling;
static SpallProfile ctx;
static _Thread_local SpallBuffer muh_buffer;

#ifdef CUIK__IS_X64
#ifdef _WIN32
static uint64_t get_rdtsc_freq(void) {
    // Get time before sleep
    uint64_t qpc_begin = 0; QueryPerformanceCounter((LARGE_INTEGER *)&qpc_begin);
    uint64_t tsc_begin = __rdtsc();

    Sleep(2);

    // Get time after sleep
    uint64_t qpc_end = qpc_begin + 1; QueryPerformanceCounter((LARGE_INTEGER *)&qpc_end);
    uint64_t tsc_end = __rdtsc();

    // Do the math to extrapolate the RDTSC ticks elapsed in 1 second
    uint64_t qpc_freq = 0; QueryPerformanceFrequency((LARGE_INTEGER *)&qpc_freq);
    uint64_t tsc_freq = (tsc_end - tsc_begin) * qpc_freq / (qpc_end - qpc_begin);

    // Failure case
    if (!tsc_freq) {
        tsc_freq = 1000000000;
    }

    return tsc_freq;
}
#else
#include <sys/mman.h>
#include <linux/perf_event.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>

static uint64_t get_rdtsc_freq(void) {
    // Fast path: Load kernel-mapped memory page
    struct perf_event_attr pe = {0};
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    uint64_t tsc_freq = 0;

    // __NR_perf_event_open == 298 (on x86_64)
    int fd = syscall(298, &pe, 0, -1, -1, 0);
    if (fd != -1) {

        struct perf_event_mmap_page *pc = (struct perf_event_mmap_page *)mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
        if (pc) {

            // success
            if (pc->cap_user_time == 1) {
                // docs say nanoseconds = (tsc * time_mult) >> time_shift
                //      set nanoseconds = 1000000000 = 1 second in nanoseconds, solve for tsc
                //       =>         tsc = 1000000000 / (time_mult >> time_shift)
                tsc_freq = (1000000000ull << (pc->time_shift / 2)) / (pc->time_mult >> (pc->time_shift - pc->time_shift / 2));
                // If your build configuration supports 128 bit arithmetic, do this:
                // tsc_freq = ((__uint128_t)1000000000ull << (__uint128_t)pc->time_shift) / pc->time_mult;
            }
            munmap(pc, 4096);
        }
        close(fd);
    }

    // Slow path
    if (!tsc_freq) {

        // Get time before sleep
        uint64_t nsc_begin = 0; { struct timespec t; if (!clock_gettime(CLOCK_MONOTONIC_RAW, &t)) nsc_begin = (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }
        uint64_t tsc_begin = __rdtsc();

        usleep(10000); // 10ms gives ~4.5 digits of precision - the longer you sleep, the more precise you get

        // Get time after sleep
        uint64_t nsc_end = nsc_begin + 1; { struct timespec t; if (!clock_gettime(CLOCK_MONOTONIC_RAW, &t)) nsc_end = (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }
        uint64_t tsc_end = __rdtsc();

        // Do the math to extrapolate the RDTSC ticks elapsed in 1 second
        tsc_freq = (tsc_end - tsc_begin) * 1000000000 / (nsc_end - nsc_begin);
    }

    // Failure case
    if (!tsc_freq) {
        tsc_freq = 1000000000;
    }

    return tsc_freq;
}
#endif
#endif

void cuik_init_timer_system(void) {
    #if defined(_AMD64_) || defined(__amd64__)
    rdtsc_freq = 1000000000.0 / get_rdtsc_freq();
    timer_start = __rdtsc();
    #endif
}

void cuikperf_start(const char* path) {
    #ifndef CUIK_USE_SPALL_AUTO
    profiling = true;
    ctx = spall_init_file(path, 1.0 / 1000.0);
    cuikperf_thread_start();
    #endif
}

void cuikperf_stop(void) {
    #ifndef CUIK_USE_SPALL_AUTO
    cuikperf_thread_stop();
    spall_quit(&ctx);
    profiling = false;
    #endif
}

bool cuikperf_is_active(void) {
    return profiling;
}

uint64_t cuik_time_in_nanos(void) {
    #if defined(_AMD64_) || defined(__amd64__)
    return (__rdtsc() - timer_start) * rdtsc_freq;
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((long long)ts.tv_sec * 1000000000LL) + ts.tv_nsec;
    #endif
}

void cuikperf_thread_start(void) {
    #if _WIN32
    uint32_t tid = GetCurrentThreadId();
    #else
    uint32_t tid = (uint32_t) pthread_self();
    #endif

    if (profiling) {
        #ifdef CUIK_USE_SPALL_AUTO
        spall_auto_thread_init(tid, SPALL_DEFAULT_BUFFER_SIZE);
        #else
        if (cuikperf_is_active()) {
            size_t size = 4 * 1024 * 1024;
            muh_buffer = (SpallBuffer){ cuik_malloc(size), size };
            spall_buffer_init(&ctx, &muh_buffer);
        }
        #endif
    }
}

void cuikperf_thread_stop(void) {
    if (profiling) {
        #ifdef CUIK_USE_SPALL_AUTO
        spall_auto_thread_quit();
        #else
        if (cuikperf_is_active()) {
            spall_buffer_quit(&ctx, &muh_buffer);
        }
        #endif
    }
}

void cuikperf_region_start(const char* label, const char* extra) {
    if (profiling) {
        uint64_t nanos = cuik_time_in_nanos();

        #ifndef CUIK_USE_SPALL_AUTO
        #if _WIN32
        uint32_t tid = GetCurrentThreadId();
        #else
        uint32_t tid = (uint32_t) pthread_self();
        #endif

        spall_buffer_begin_args(&ctx, &muh_buffer, label, strlen(label), extra, extra ? strlen(extra) : 0, nanos, tid, 0);
        #endif
    }
}

void cuikperf_region_end(void) {
    if (profiling) {
        uint64_t nanos = cuik_time_in_nanos();

        #ifndef CUIK_USE_SPALL_AUTO
        #if _WIN32
        uint32_t tid = GetCurrentThreadId();
        #else
        uint32_t tid = (uint32_t) pthread_self();
        #endif

        spall_buffer_end_ex(&ctx, &muh_buffer, nanos, tid, 0);
        #endif
    }
}
