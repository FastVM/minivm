
#pragma once

#if !defined(MX_HEADER_LUA_RT_PRIM)
#define MX_HEADER_LUA_RT_PRIM

#include "../../vm/vm.h"

void mx_prim_table_append(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_prim_pack(mx_t *vm, size_t nargs, mx_obj_t *args);

mx_obj_t mx_prim_for_fail(mx_t *vm, const char *what, mx_obj_t bad);
int64_t mx_prim_for_stop_float(int64_t step, double limit);
int64_t mx_prim_for_stop(int64_t step, mx_obj_t limit);
int64_t mx_prim_for_last(int64_t init, int64_t step, mx_obj_t limit);
void mx_prim_forprep_initial(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_prim_forprep_limit(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_prim_forprep_step(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_prim_forprep_initial_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_prim_forprep_limit_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
bool mx_prim_forprep_step_fast(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_prim_global_fresh(mx_t *vm, size_t nargs, mx_obj_t *args);

mx_obj_t mx_prim_close_handler(mx_t *vm, mx_obj_t v);
void mx_prim_close_check(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_prim_close_call(mx_t *vm, size_t nargs, mx_obj_t *args);
mx_obj_t mx_prim_tbc_unwind(mx_t *vm, mx_thread_t *th, uint32_t from, mx_obj_t err);

bool mx_prim_is_direct(mx_t *vm, mx_obj_t fn);
void mx_prim_pcall(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_prim_xpcall(mx_t *vm, size_t nargs, mx_obj_t *args);
void mx_prim_protect_result(mx_t *vm, mx_obj_t got);

#endif
