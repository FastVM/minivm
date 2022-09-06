
#if !defined(VM_HEADER_INT_GC)
#define VM_HEADER_INT_GC

#include "lib.h"

typedef int64_t vm_int_t;
typedef int64_t vm_loc_t;
typedef int64_t vm_reg_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

struct vm_value_t {
  union {
    void *ptr;
    double ival;
  };
};


vm_value_t vm_gc_arr(vm_int_t size);
vm_value_t vm_gc_get(vm_value_t obj, vm_value_t index);
void vm_gc_set(vm_value_t obj, vm_value_t index,
               vm_value_t value);
vm_int_t vm_gc_len(vm_value_t obj);

#define VM_VALUE_GET_INT(n_) ((n_).ival)

#define VM_VALUE_SET_INT(n_) ((vm_value_t){.ival = (n_)})

#define vm_gc_arr_new(len_) vm_gc_arr(len_)

#define vm_gc_get_v(obj_, nth_) vm_gc_get(obj_, (nth_))
#define vm_gc_get_i(obj_, nth_)                                           \
  vm_gc_get(obj_, VM_VALUE_SET_INT(nth_))

#define vm_gc_set_vv(obj_, nth_, val_) vm_gc_set(obj_, nth_, val_)
#define vm_gc_set_vi(obj_, nth_, val_)                                    \
  vm_gc_set(obj_, nth_, VM_VALUE_SET_INT(val_))
#define vm_gc_set_iv(obj_, nth_, val_)                                    \
  vm_gc_set(obj_, VM_VALUE_SET_INT(nth_), val_)
#define vm_gc_set_ii(obj_, nth_, val_)                                    \
  vm_gc_set(obj_, VM_VALUE_SET_INT(nth_), VM_VALUE_SET_INT(val_))

#define vm_value_from_func(n_) (VM_VALUE_SET_INT(n_))
#define vm_value_to_func(n_) (VM_VALUE_GET_INT(n_))

#define vm_value_from_int(n_) (VM_VALUE_SET_INT(n_))
#define vm_value_to_int(n_) (VM_VALUE_GET_INT(n_))

#define vm_value_add(a_, b_)                                              \
  (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) + VM_VALUE_GET_INT(b_)))
#define vm_value_addi(a_, b_)                                             \
  (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) + (b_)))

#define vm_value_sub(a_, b_)                                              \
  (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) - VM_VALUE_GET_INT(b_)))
#define vm_value_subi(a_, b_)                                             \
  (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) - (b_)))
#define vm_value_isub(a_, b_) (VM_VALUE_SET_INT((a_)-VM_VALUE_GET_INT(b_)))

#define vm_value_mul(a_, b_)                                              \
  (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) * VM_VALUE_GET_INT(b_)))
#define vm_value_muli(a_, b_)                                             \
  (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) * (b_)))

#define vm_value_div(a_, b_)                                              \
  (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) / VM_VALUE_GET_INT(b_)))
#define vm_value_divi(a_, b_)                                             \
  (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) / (b_)))
#define vm_value_idiv(a_, b_)                                             \
  (VM_VALUE_SET_INT((a_) / VM_VALUE_GET_INT(b_)))

#define vm_value_mod(a_, b_)                                              \
  VM_VALUE_SET_INT(fmod(VM_VALUE_GET_INT(a_), VM_VALUE_GET_INT(b_)))
#define vm_value_modi(a_, b_)                                             \
  VM_VALUE_SET_INT(fmod(VM_VALUE_GET_INT(a_), (b_)))
#define vm_value_imod(a_, b_)                                             \
  VM_VALUE_SET_INT(fmod((a_), VM_VALUE_GET_INT(b_)))

#define vm_value_to_bool(v_) (VM_VALUE_GET_INT(v_) != 0)

#define vm_value_is_equal(a_, b_)                                         \
  (VM_VALUE_GET_INT(a_) == VM_VALUE_GET_INT(b_))
#define vm_value_is_equal_int(a_, b_) (VM_VALUE_GET_INT(a_) == (b_))

#define vm_value_is_less(a_, b_)                                          \
  (VM_VALUE_GET_INT(a_) < VM_VALUE_GET_INT(b_))
#define vm_value_is_less_int(a_, b_) (VM_VALUE_GET_INT(a_) < (b_))
#define vm_value_is_int_less(a_, b_) ((a_) < VM_VALUE_GET_INT(b_))

#endif
