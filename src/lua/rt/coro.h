
#pragma once

#if !defined(MX_HEADER_LUA_RT_CORO)
#define MX_HEADER_LUA_RT_CORO

#include "api/std.h"

const char *mx_coro_status_name(mx_t *vm, mx_thread_t *co);
const char *mx_coro_resume_check(mx_t *vm, mx_thread_t *co);
mx_thread_t *mx_coro_check(mx_std_state_t *ls, mx_std_slot_t arg);
void mx_coro_push_resumed(mx_std_state_t *ls, mx_run_result_t res, bool is_wrap, mx_thread_t *co);
int32_t mx_coro_create(mx_std_state_t *ls);
int32_t mx_coro_resume(mx_std_state_t *ls);
int32_t mx_coro_yield(mx_std_state_t *ls);
int32_t mx_coro_status(mx_std_state_t *ls);
int32_t mx_coro_running(mx_std_state_t *ls);
int32_t mx_coro_isyieldable(mx_std_state_t *ls);
int32_t mx_coro_close(mx_std_state_t *ls);
mx_obj_t mx_coro_kill(mx_t *vm, mx_thread_t *co);
int32_t mx_coro_wrap(mx_std_state_t *ls);
int32_t mx_coro_wrap_call(mx_std_state_t *ls);

#endif
