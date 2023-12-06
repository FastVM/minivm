
#if !defined(VM_HEADER_BE_TB)
#define VM_HEADER_BE_TB

#include "../ir/ir.h"
#include "../lib.h"
#include "../obj.h"
#include "../std/libs/io.h"
#include "../std/std.h"
#include "../ir/type.h"

struct vm_tb_state_t;
struct vm_tb_comp_state_t;

typedef struct vm_tb_state_t vm_tb_state_t;
typedef struct vm_tb_comp_state_t vm_tb_comp_state_t;

typedef vm_std_value_t VM_CDECL vm_tb_comp_t(vm_tb_comp_state_t *comp, vm_value_t *args);

struct vm_tb_state_t {
    void *module;
    size_t faults;
    vm_config_t *config;
    size_t nblocks;
    vm_block_t **blocks;

    // externals
    void *vm_tb_rfunc_comp;
    void *vm_table_new;
    void *vm_table_set;
    void *vm_table_get_pair;
    void *vm_tb_print;
    void *std;
};

struct vm_tb_comp_state_t {
    // func must be first
    vm_tb_comp_t *func;
    vm_rblock_t *rblock;
};


void *vm_tb_rfunc_comp(vm_rblock_t *rblock);
vm_std_value_t vm_tb_run_main(vm_config_t *config, vm_block_t *entry, size_t nblocks, vm_block_t **blocks, vm_table_t *std);
vm_std_value_t vm_tb_run_repl(vm_config_t *config, vm_block_t *entry, size_t nblocks, vm_block_t **blocks, vm_table_t *std);
vm_std_value_t vm_tb_comp_call(vm_tb_comp_state_t *comp, vm_value_t *args);

#endif
