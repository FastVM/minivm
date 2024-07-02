
#include "errors.h"

vm_error_t *vm_error_from_msg(const char *msg) {
    vm_error_t *err = vm_malloc(sizeof(vm_error_t));
    *err = (vm_error_t) {
        .msg = vm_strdup(msg),
        .child = NULL,
    };
    return err;
}

vm_error_t *vm_error_from_error(vm_location_range_t range, vm_error_t *child) {
    vm_error_t *err = vm_malloc(sizeof(vm_error_t));
    *err = (vm_error_t) {
        .range = range,
        .child = child,
    };
    return err;
}
