#pragma once

#include "obj.h"
#include "opcode.h"

int vm_run(size_t nops, vm_opcode_t *ops);
vm_obj_t vm_run_ext(size_t func, vm_obj_t obj);
