
#if !defined(VM_HEADER_BC)
#define VM_HEADER_BC

#include "opcode.h"

struct vm_bc_buf_t;
typedef struct vm_bc_buf_t vm_bc_buf_t;

struct vm_bc_buf_t
{
    vm_opcode_t *ops;
    size_t nops;
};

#endif
