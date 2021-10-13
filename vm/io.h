#pragma once

#include "gc.h"
#include "vm.h"

void vm_puts(const char *src);
void vm_print(vm_gc_t *gc, vm_obj_t mem);
