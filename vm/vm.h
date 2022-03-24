#pragma once

#include "gc.h"
#include "opcode.h"

vm_obj_t vm_global_from(vm_gc_t *gc, size_t len, const char **args);
int vm_table_opt(size_t nops, vm_opcode_t *ops, void *const *const ptrs);
int vm_run_from(vm_gc_t *gc, size_t nops, vm_opcode_t *ops, vm_obj_t globals);
int vm_run(vm_config_t config, size_t nops, vm_opcode_t *ops, size_t nargs, const char **args);
vm_obj_t vm_run_ext(vm_gc_t *gc, size_t func, vm_obj_t obj);
