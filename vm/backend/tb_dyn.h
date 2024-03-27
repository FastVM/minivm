
#if !defined(VM_BE_TB_DYN)
#define VM_BE_TB_DYN

#include "../../vendor/cuik/common/arena.h"
#include "../../vendor/tcc/libtcc.h"
#include "../ir/check.h"
#include "./exec.h"

struct vm_tb_dyn_state_t;

typedef struct vm_tb_dyn_state_t vm_tb_dyn_state_t;

struct vm_tb_dyn_state_t {
    void *data;
};

#endif
