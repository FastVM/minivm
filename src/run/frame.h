
#pragma once

#if !defined(MX_HEADER_RUN_FRAME)
#define MX_HEADER_RUN_FRAME

#include "thread.h"
#include "../ir/ir.h"
#include "../version/version.h"

typedef struct mx_frame_t mx_frame_t;
typedef struct mx_frame_iter_t mx_frame_iter_t;

struct mx_frame_t {
    mx_thread_t *th;
    const mx_resume_t *at;
    uint32_t p;
    uint32_t rec_at;
    mx_link_tag_t tag;
    bool is_lua: 1;
    bool is_valid: 1;
};

struct mx_frame_iter_t {
    mx_thread_t *th;
    uint32_t p;
    uint32_t rec_at;
};

mx_frame_iter_t mx_frame_iter(mx_thread_t *th);
mx_frame_t mx_frame_next(mx_frame_iter_t *it);
mx_frame_t mx_frame_at_level(mx_thread_t *th, uint32_t level);
uint32_t mx_frame_count(mx_thread_t *th);
mx_obj_t mx_frame_fn(const mx_frame_t *f);
mx_ir_block_t *mx_frame_func(const mx_frame_t *f);
mx_val_t *mx_frame_base(const mx_frame_t *f);
int64_t mx_frame_line(const mx_frame_t *f);
mx_obj_t mx_frame_reg(const mx_frame_t *f, uint32_t reg);
bool mx_frame_set_reg(mx_t *vm, const mx_frame_t *f, uint32_t reg, mx_obj_t v);
const mx_ir_local_t *mx_frame_local(const mx_frame_t *f, int64_t n);

#endif
