
#pragma once

#if !defined(MX_HEADER_LUA_RT_TRACE)
#define MX_HEADER_LUA_RT_TRACE

#include "../../run/frame.h"
#include "../../value/error.h"
#include "../../value/string.h"
#include "../../vm/vm.h"

#define MX_TRACE_LEVELS_HEAD 10
#define MX_TRACE_LEVELS_TAIL 11

typedef struct mx_trace_name_t mx_trace_name_t;

struct mx_trace_name_t {
    const char *what;
    const char *name;
};

bool mx_trace_is_close_call(const mx_resume_t *site);
mx_trace_name_t mx_trace_namewhat(const mx_frame_t *f);
bool mx_trace_global_name(mx_t *vm, mx_obj_t fn, mx_str_t *out);
void mx_trace_short_src(const mx_frame_t *f, mx_str_t *out);
void mx_trace_funcname(mx_t *vm, mx_str_t *out, const mx_frame_t *f);
void mx_trace_line(mx_t *vm, mx_str_t *out, const mx_frame_t *f);
void mx_trace_append(mx_t *vm, mx_str_t *out, mx_thread_t *th, uint32_t level);

#endif
