
#if !defined(VM_HEADER_INT_GC)
#define VM_HEADER_INT_GC

#include "lib.h"

#define NANBOX_PREFIX vm_box

#include "nanbox.h"

typedef int64_t vm_int_t;
typedef int64_t vm_loc_t;
typedef int64_t vm_reg_t;

typedef double vm_number_t;

struct vm_value_array_t;
typedef struct vm_value_array_t vm_value_array_t;

typedef vm_box_t vm_value_t;

struct vm_value_array_t {
    vm_value_t *data;
    size_t len : 32;
    size_t alloc : 32;
};

enum {
    VM_TYPE_UNKNOWN,
    VM_TYPE_NIL,
    VM_TYPE_BOOL,
    VM_TYPE_INT,
    VM_TYPE_FLOAT,
    VM_TYPE_PTR,
    VM_TYPE_MAX,
};

vm_value_t vm_gc_new(vm_int_t slots);
vm_value_t vm_gc_get(vm_value_t obj, vm_value_t index);
void vm_gc_set(vm_value_t obj, vm_value_t index, vm_value_t value);
vm_int_t vm_gc_len(vm_value_t obj);

#define vm_gc_get_v(obj_, nth_) vm_gc_get(obj_, (nth_))
#define vm_gc_get_i(obj_, nth_) vm_gc_get(obj_, vm_value_from_int(nth_))

#define vm_gc_set_vv(obj_, nth_, val_) vm_gc_set(obj_, nth_, val_)
#define vm_gc_set_vi(obj_, nth_, val_) vm_gc_set(obj_, nth_, vm_value_from_int(val_))
#define vm_gc_set_iv(obj_, nth_, val_) vm_gc_set(obj_, vm_value_from_int(nth_), val_)
#define vm_gc_set_ii(obj_, nth_, val_) vm_gc_set(obj_, vm_value_from_int(nth_), vm_value_from_int(val_))

#define vm_value_from_int(n_) (vm_box_from_int(n_))
#define vm_value_from_float(n_) (vm_box_from_double(n_))
#define vm_value_from_block(n_) (vm_box_from_pointer(n_))
#define vm_value_from_static(n_) (vm_box_from_pointer(n_))

#define vm_value_to_int(v_) (vm_box_to_int(v_))
#define vm_value_to_float(v_) (vm_box_to_double(v_))
#define vm_value_to_block(v_) ((void *)vm_box_to_pointer(v_))
#define vm_value_to_array(v_) ((vm_value_array_t *)vm_box_to_pointer(v_))

static inline uint8_t vm_typeof(vm_value_t val) {
    if (vm_box_is_int(val)) {
        return VM_TYPE_INT;
    }
    if (vm_box_is_double(val)) {
        return VM_TYPE_FLOAT;
    }
    if (vm_box_is_empty(val)) {
        return VM_TYPE_NIL;
    }
    if (vm_box_is_boolean(val)) {
        return VM_TYPE_BOOL;
    }
    if (vm_box_is_pointer(val)) {
        return VM_TYPE_PTR;
    }
    __builtin_unreachable();
}

#endif
