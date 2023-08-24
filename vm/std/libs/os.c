
#include "../util.h"

vm_std_value_t VM_STD_EXPORT vm_std_os_system(vm_std_value_t *args) {
    const char *str = NULL;
    if (vm_std_parse_args(args, "s", &str)) {
        return (vm_std_value_t){
            .tag = VM_TAG_F64,
            .value.f64 = (double)system(str),
        };
    }
    return (vm_std_value_t){
        .tag = VM_TAG_UNK,
    };
}
