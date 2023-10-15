////////////////////////////////////////////
// Profiler
////////////////////////////////////////////
// The callbacks are global and a user may even hook in using the cuikperf_start
// and cuikperf_stop, or by using CUIK_TIMED_BLOCK
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Cuik_IProfiler {
    void (*start)(void* user_data);
    void (*stop)(void* user_data);

    void (*begin_plot)(void* user_data, uint64_t nanos, const char* label, const char* extra);
    void (*end_plot)(void* user_data, uint64_t nanos);
} Cuik_IProfiler;

// DONT USE THIS :(((
void cuik_init_timer_system(void);

void cuikperf_start(void* ud, const Cuik_IProfiler* profiler, bool lock_on_plot);
void cuikperf_stop(void);
bool cuikperf_is_active(void);

// the absolute values here don't have to mean anything, it's just about being able
// to measure between two points.
uint64_t cuik_time_in_nanos(void);

// Reports a region of time to the profiler callback
void cuikperf_region_start(const char* fmt, const char* extra);
void cuikperf_region_end(void);

// Usage:
// CUIK_TIMED_BLOCK("Beans %d", 5) {
//   ...
// }
#define CUIK_TIMED_BLOCK(label) for (uint64_t __i = (cuikperf_region_start(label, NULL), 0); __i < 1; __i++, cuikperf_region_end())
#define CUIK_TIMED_BLOCK_ARGS(label, extra) for (uint64_t __i = (cuikperf_region_start(label, extra), 0); __i < 1; __i++, cuikperf_region_end())
