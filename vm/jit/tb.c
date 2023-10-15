
#include "../obj.h"
#include "../std/std.h"
#include "../type.h"
#include "../../tb/include/tb.h"

typedef void __attribute__((cdecl)) callable_t(double, double);

vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args) {
    
    return (vm_std_value_t) {
        .tag = VM_TAG_NIL,
    };
}
