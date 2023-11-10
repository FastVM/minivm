
#if !defined(VM_HEADER_BE_TB)
#define VM_HEADER_BE_TB

#include "../ir.h"
#include "../lib.h"
#include "../obj.h"
#include "../type.h"
#include "../std/libs/io.h"
#include "../std/std.h"

struct vm_tb_state_t;
struct vm_tb_comp_state_t;

typedef struct vm_tb_state_t vm_tb_state_t;
typedef struct vm_tb_comp_state_t vm_tb_comp_state_t;

typedef vm_std_value_t VM_CDECL vm_tb_comp_t(vm_tb_comp_state_t *comp, vm_value_t *args);

struct vm_tb_state_t {
    vm_table_t *std;
    void *module;
    size_t faults;
    vm_config_t *config;

    // externals
    void *state_self;
    void *vm_tb_rfunc_comp;
    void *vm_table_get_pair;
    void *vm_tb_print;
    void *vm_tb_report_err;
};

struct vm_tb_comp_state_t {
    // func must be first
    vm_tb_comp_t *func;
    vm_rblock_t *rblock;
};

void *vm_tb_rfunc_comp(vm_rblock_t *rblock);
vm_std_value_t vm_tb_run(vm_config_t *config, vm_block_t *block, vm_table_t *std);
vm_std_value_t vm_tb_comp_call(vm_tb_comp_state_t *comp, vm_value_t *args);

#endif
