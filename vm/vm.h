#pragma once

#include "obj.h"
#include "opcode.h"

enum {
    VM_RUN_OKAY = 0,
    VM_RUN_CAR_ON_NUM = 1,
    VM_RUN_CDR_ON_NUM = 2,
    VM_RUN_UNK_OPCODE = 3,
    VM_RUN_UNK_ARG = 4,
    VM_RUN_BAD_LOCAL = 5,
    VM_RUN_DIV_ZERO = 6,
    VM_RUN_MOD_ZERO = 7,
    VM_RUN_CALL_ZERO = 8,
};

int vm_run(size_t nops, const vm_opcode_t *ops);
vm_obj_t vm_run_ext(size_t func, vm_obj_t obj);
