#include <common.h>
#include <perf.h>
#include <threads.h>
#include <stdarg.h>
#include <stdatomic.h>

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
#endif

static mtx_t timer_mutex;

static bool should_lock_profiler;

static const Cuik_IProfiler* profiler;
static void* profiler_userdata;

#ifdef CUIK__IS_X64
#ifdef _WIN32
static double get_rdtsc_freq(void) {
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

    return 1000000.0 / (double)tsc_freq;
}
#else
#include <sys/mman.h>
#include <linux/perf_event.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>

static double get_rdtsc_freq(void) {
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

    return 1000000.0 / (double)tsc_freq;
}
#endif
#endif

void cuik_init_timer_system(void) {
    #if defined(_AMD64_) || defined(__amd64__)
    rdtsc_freq = get_rdtsc_freq() / 1000000.0;
    timer_start = __rdtsc();
    #endif
}

void cuikperf_start(void* ud, const Cuik_IProfiler* p, bool lock_on_plot) {
    assert(profiler == NULL);

    profiler = p;
    profiler_userdata = ud;
    should_lock_profiler = lock_on_plot;

    if (lock_on_plot) {
        mtx_init(&timer_mutex, mtx_plain);
    }

    profiler->start(profiler_userdata);
    profiler->begin_plot(profiler_userdata, cuik_time_in_nanos(), "main thread", "");
}

void cuikperf_stop(void) {
    assert(profiler != NULL);
    profiler->end_plot(profiler_userdata, cuik_time_in_nanos());
    profiler->stop(profiler_userdata);

    if (should_lock_profiler) {
        mtx_destroy(&timer_mutex);
    }
    profiler = NULL;
}

bool cuikperf_is_active(void) {
    return (profiler != NULL);
}

uint64_t cuik_time_in_nanos(void) {
    #if defined(_AMD64_) || defined(__amd64__)
    return (__rdtsc() - timer_start) * (rdtsc_freq * 1000000000.0);
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((long long)ts.tv_sec * 1000000000LL) + ts.tv_nsec;
    #endif
}

void cuikperf_region_start(const char* fmt, const char* extra) {
    if (profiler == NULL) return;
    uint64_t nanos = cuik_time_in_nanos();

    // lock if necessary
    if (should_lock_profiler) mtx_lock(&timer_mutex);

    profiler->begin_plot(profiler_userdata, nanos, fmt, extra ? extra : "");
    if (should_lock_profiler) mtx_unlock(&timer_mutex);
}

void cuikperf_region_end(void) {
    if (profiler == NULL) return;
    uint64_t nanos = cuik_time_in_nanos();

    if (should_lock_profiler) mtx_lock(&timer_mutex);
    profiler->end_plot(profiler_userdata, nanos);
    if (should_lock_profiler) mtx_unlock(&timer_mutex);
}
