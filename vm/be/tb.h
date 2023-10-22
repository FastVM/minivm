
#if !defined(VM_HEADER_BE_TB)
#define VM_HEADER_BE_TB

#include "../ir.h"
#include "../lib.h"
#include "../obj.h"
#include "../type.h"
#include "../std/libs/io.h"
#include "../std/std.h"

struct vm_tb_state_t;

typedef struct vm_tb_state_t vm_tb_state_t;

struct vm_tb_state_t {
    vm_table_t *std;
    void *module;

    // externals
    void *state_self;
    void *vm_tb_rfunc_comp;
    void *vm_table_get_pair;
    void *vm_tb_print;
};
void GC_add_roots(void *low, void *high);
void *vm_tb_rfunc_comp(vm_tb_state_t *state, vm_rblock_t *rblock);
vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args);

#endif
