
#pragma once

#if !defined(MX_HEADER_LUA_RT_CONT)
#define MX_HEADER_LUA_RT_CONT

#include "../../vm/vm.h"

void mx_cont_start(mx_t *vm, size_t nargs, mx_obj_t *args, bool allow_table);
void mx_cont_callcc(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_cont_callcci(mx_t *vm, size_t nargs, mx_obj_t *args);
const char *mx_cont_fail(mx_t *vm, mx_thread_t *th, mx_thread_t *target, uint32_t p, int64_t state);
void mx_cont_invoke(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_cont_done(mx_t *vm, mx_obj_t k);

#endif
