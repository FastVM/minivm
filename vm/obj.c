
#include <math.h>

#include "vm.h"
#include "io.h"
#include "gc.h"

bool vm_obj_is_nil(vm_obj_t o) {
    return nanbox_is_empty(o);
}
bool vm_obj_is_boolean(vm_obj_t o) {
    return nanbox_is_boolean(o);
}
bool vm_obj_is_number(vm_obj_t o) {
    return nanbox_is_number(o);
}
bool vm_obj_is_buffer(vm_obj_t o) {
    return nanbox_is_aux1(o);
}
bool vm_obj_is_table(vm_obj_t o) {
    return nanbox_is_aux2(o);
}
bool vm_obj_is_closure(vm_obj_t o) {
    return nanbox_is_aux3(o);
}
bool vm_obj_is_ffi(vm_obj_t o) {
    return nanbox_is_aux4(o);
}
bool vm_obj_is_block(vm_obj_t o) {
    return nanbox_is_aux5(o);
}
bool vm_obj_is_error(vm_obj_t o) {
    return nanbox_is_pointer(o);
}

bool vm_obj_get_boolean(vm_obj_t o) {
    assert(vm_obj_is_boolean(o));
    return nanbox_to_boolean(o);
}
double vm_obj_get_number(vm_obj_t o) {
    assert(vm_obj_is_number(o));
    return nanbox_to_double(o);
}
vm_io_buffer_t *vm_obj_get_buffer(vm_obj_t o) {
    assert(vm_obj_is_buffer(o));
    return ((vm_io_buffer_t *) nanbox_to_aux(o));
}
vm_obj_table_t *vm_obj_get_table(vm_obj_t o) {
    assert(vm_obj_is_table(o));
    return ((vm_obj_table_t *) nanbox_to_aux(o));
}
vm_obj_closure_t *vm_obj_get_closure(vm_obj_t o) {
    assert(vm_obj_is_closure(o));
    return ((vm_obj_closure_t *) nanbox_to_aux(o));
}
vm_ffi_t *vm_obj_get_ffi(vm_obj_t o) {
    assert(vm_obj_is_ffi(o));
    return ((vm_ffi_t *) nanbox_to_aux(o));
}
vm_ir_block_t *vm_obj_get_block(vm_obj_t o) {
    assert(vm_obj_is_block(o));
    return ((vm_ir_block_t *) nanbox_to_aux(o));
}
vm_error_t *vm_obj_get_error(vm_obj_t o) {
    assert(vm_obj_is_error(o));
    return ((vm_error_t *) nanbox_to_pointer(o));
}

vm_obj_t vm_obj_of_nil(void) {
    return nanbox_empty(); 
}
vm_obj_t vm_obj_of_boolean(bool b) {
    return nanbox_from_boolean(b);
} 
vm_obj_t vm_obj_of_number(double n) {
    return nanbox_from_double(n);
}
vm_obj_t vm_obj_of_buffer(vm_io_buffer_t *o) {
    return nanbox_from_aux1(o);
}
vm_obj_t vm_obj_of_table(vm_obj_table_t *o) {
    return nanbox_from_aux2(o);
}
vm_obj_t vm_obj_of_closure(vm_obj_closure_t *o) {
    return nanbox_from_aux3(o);
}
vm_obj_t vm_obj_of_ffi(vm_ffi_t *o) {
    return nanbox_from_aux4(o);
}
vm_obj_t vm_obj_of_block(vm_ir_block_t *o) {
    return nanbox_from_aux5(o);
}
vm_obj_t vm_obj_of_error(vm_error_t *o) {
    return nanbox_from_pointer(o);
}

// extras
uint32_t vm_obj_hash(vm_obj_t value) {
    if (vm_obj_is_number(value)) {
        double n = vm_obj_get_number(value);
        if (n == floor(n) && INT32_MIN <= n && n <= INT32_MAX) {
            return (uint32_t) (int32_t) n * 31;
        }
        return (uint32_t) (*(uint64_t *)&n >> 12) * 37;
    }
    if (vm_obj_is_buffer(value)) {
        vm_io_buffer_t *restrict buf = vm_obj_get_buffer(value);
        if (buf->hash == 0) {
            uint32_t ret = 0x811c9dc5;
            for (size_t i = 0; i < buf->len; i++) {
                char c = buf->buf[i];
                if (c == '\0') {
                    break;
                }
                ret *= 0x01000193;
                ret ^= c;
            }
            buf->hash = ret;
        }
        return buf->hash;
    }
    if (vm_obj_is_boolean(value)) {
        return UINT32_MAX - (uint32_t)vm_obj_get_boolean(value);
    }
    if (vm_obj_is_ffi(value)) {
        return (uint32_t)(size_t)vm_obj_get_ffi(value) >> 4;
    }
    if (vm_obj_is_closure(value)) {
        return (uint32_t)(size_t)vm_obj_get_closure(value) >> 4;
    }
    if (vm_obj_is_table(value)) {
        return (uint32_t)(size_t)vm_obj_get_table(value) >> 4;
    }
    return 0;
}

vm_obj_t vm_obj_of_string(vm_t *vm, const char *str) {
    vm_obj_t ret = vm_obj_of_buffer(vm_io_buffer_from_str(str));
    vm_gc_add(vm, ret);
    return ret;
}
