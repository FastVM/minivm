
#if !defined(VM_HEADER_BE_TB)
#define VM_HEADER_BE_TB

#include "../ir.h"
#include "../lib.h"
#include "../obj.h"
#include "../type.h"
#include "../std/std.h"

struct vm_tb_state_t;
struct vm_tb_cache_t;

typedef struct vm_tb_state_t vm_tb_state_t;
typedef struct vm_tb_cache_t vm_tb_cache_t;

struct vm_tb_state_t {
    vm_table_t *std;
};

struct vm_tb_cache_t {
    vm_block_t *block;
};

vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args);

#endif
