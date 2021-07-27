#pragma once

#include <vm/vm.h>
#include <vm/debug.h>
#include <vm/vector.h>

struct vm_asm_result_t;
typedef struct vm_asm_result_t vm_asm_result_t;

#define vm_asm_result_fail ((vm_asm_result_t){ \
    .bytecode = NULL,                          \
    .len = 0})

struct vm_asm_result_t
{
    opcode_t *bytecode;
    int len;
};

vm_asm_result_t vm_assemble(const char *src);