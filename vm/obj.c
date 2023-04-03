#include "obj.h"

vm_table_t *vm_table_new(void) {
    vm_table_t *ret = vm_malloc(sizeof(vm_table_t));
    *ret = (vm_table_t) {0};
    return ret;
}
