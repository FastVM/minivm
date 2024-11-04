
#include "math.h"
#include "obj.h"
#include "errors.h"

bool vm_obj_unsafe_eq(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_nil(v1) && vm_obj_is_nil(v2)) {
        return true;
    } else if (vm_obj_is_boolean(v1) && vm_obj_is_boolean(v2)) {
        return vm_obj_get_boolean(v1) == vm_obj_get_boolean(v2);
    } else if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_get_number(v1) == vm_obj_get_number(v2);
    } else if (vm_obj_is_buffer(v1) && vm_obj_is_buffer(v2)) {
        return strcmp(vm_obj_get_buffer(v1)->buf, vm_obj_get_buffer(v2)->buf) == 0;
    } else if (vm_obj_is_table(v1) && vm_obj_is_table(v2)) {
        return vm_obj_get_table(v1) == vm_obj_get_table(v2);
    } else if (vm_obj_is_closure(v1) && vm_obj_is_closure(v2)) {
        return vm_obj_get_closure(v1) == vm_obj_get_closure(v2);
    } else if (vm_obj_is_ffi(v1) && vm_obj_is_ffi(v2)) {
        return vm_obj_get_ffi(v1) == vm_obj_get_ffi(v2);
    } else {
        return false;
    }
}

bool vm_obj_unsafe_lt(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_get_number(v1) < vm_obj_get_number(v2);
    } else if (vm_obj_is_buffer(v1) && vm_obj_is_buffer(v2)) {
        return strcmp(vm_obj_get_buffer(v1)->buf, vm_obj_get_buffer(v2)->buf) < 0;
    } else {
        return false;
    }
}

bool vm_obj_unsafe_le(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_get_number(v1) <= vm_obj_get_number(v2);
    } else if (vm_obj_is_buffer(v1) && vm_obj_is_buffer(v2)) {
        return strcmp(vm_obj_get_buffer(v1)->buf, vm_obj_get_buffer(v2)->buf) <= 0;
    } else {
        return false;
    }
}

vm_obj_t vm_obj_eq(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    (void) vm;
    if (vm_obj_is_nil(v1) && vm_obj_is_nil(v2)) {
        return vm_obj_of_boolean(true);
    } else if (vm_obj_is_boolean(v1) && vm_obj_is_boolean(v2)) {
        return vm_obj_of_boolean(vm_obj_get_boolean(v1) == vm_obj_get_boolean(v2));
    } else if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_boolean(vm_obj_get_number(v1) == vm_obj_get_number(v2));
    } else if (vm_obj_is_buffer(v1) && vm_obj_is_buffer(v2)) {
        return vm_obj_of_boolean(strcmp(vm_obj_get_buffer(v1)->buf, vm_obj_get_buffer(v2)->buf) == 0);
    } else if (vm_obj_is_table(v1) && vm_obj_is_table(v2)) {
        // TODO: use metamethod
        return vm_obj_of_boolean(vm_obj_get_table(v1) == vm_obj_get_table(v2));
    } else if (vm_obj_is_closure(v1) && vm_obj_is_closure(v2)) {
        return vm_obj_of_boolean(vm_obj_get_closure(v1) == vm_obj_get_closure(v2));
    } else if (vm_obj_is_ffi(v1) && vm_obj_is_ffi(v2)) {
        return vm_obj_of_boolean(vm_obj_get_ffi(v1) == vm_obj_get_ffi(v2));
    } else {
        return vm_obj_of_boolean(false);
    }
}

vm_obj_t vm_obj_lt(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    (void) vm;
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_boolean(vm_obj_get_number(v1) < vm_obj_get_number(v2));
    } else if (vm_obj_is_buffer(v1) && vm_obj_is_buffer(v2)) {
        return vm_obj_of_boolean(strcmp(vm_obj_get_buffer(v1)->buf, vm_obj_get_buffer(v2)->buf) < 0);
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "not comparable"));
    }
}

vm_obj_t vm_obj_le(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    (void) vm;
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_boolean(vm_obj_get_number(v1) < vm_obj_get_number(v2));
    } else if (vm_obj_is_buffer(v1) && vm_obj_is_buffer(v2)) {
        return vm_obj_of_boolean(strcmp(vm_obj_get_buffer(v1)->buf, vm_obj_get_buffer(v2)->buf) < 0);
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_FUNC, "not comparable"));
    }
}
