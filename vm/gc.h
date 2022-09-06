
#if !defined(VM_HEADER_INT_GC)
#define VM_HEADER_INT_GC

#include "lib.h"

typedef int64_t vm_int_t;
typedef int64_t vm_loc_t;
typedef int64_t vm_reg_t;

struct vm_value_array_t;
typedef struct vm_value_array_t vm_value_array_t;

union vm_value_data_t;
typedef union vm_value_data_t vm_value_data_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

struct vm_value_array_t {
  vm_value_data_t *datas;
  uint8_t *types;
  size_t len: 32;
  size_t alloc: 32;
};

enum {
  VM_TYPE_VOID,
  VM_TYPE_INT,
  VM_TYPE_BLOCK,
  VM_TYPE_ARRAY,
  VM_TYPE_STATIC,
};

union vm_value_data_t {
  void *block;
  vm_value_array_t *arr;
  vm_int_t ival;
};

struct vm_value_t {
  vm_value_data_t data;
  uint8_t type;
};

struct vm_gc_t {
  size_t alloc;
};

vm_value_t vm_gc_new(vm_gc_t *gc, vm_int_t slots);
vm_value_t vm_gc_get(vm_gc_t *gc, vm_value_t obj, vm_value_t index);
void vm_gc_set(vm_gc_t *gc, vm_value_t obj, vm_value_t index, vm_value_t value);
vm_int_t vm_gc_len(vm_gc_t *gc, vm_value_t obj);

#define vm_gc_get_v(gc_, obj_, nth_) vm_gc_get(gc_, obj_, (nth_))
#define vm_gc_get_i(gc_, obj_, nth_)                                           \
  vm_gc_get(gc_, obj_, vm_value_from_int(nth_))

#define vm_gc_set_vv(gc_, obj_, nth_, val_) vm_gc_set(gc_, obj_, nth_, val_)
#define vm_gc_set_vi(gc_, obj_, nth_, val_)                                    \
  vm_gc_set(gc_, obj_, nth_, vm_value_from_int(val_))
#define vm_gc_set_iv(gc_, obj_, nth_, val_)                                    \
  vm_gc_set(gc_, obj_, vm_value_from_int(nth_), val_)
#define vm_gc_set_ii(gc_, obj_, nth_, val_)                                    \
  vm_gc_set(gc_, obj_, vm_value_from_int(nth_), vm_value_from_int(val_))

#define vm_value_from_int(n_) ((vm_value_t){.data.ival = (n_), .type = VM_TYPE_INT})
#define vm_value_from_block(n_) ((vm_value_t){.data.block = (n_), .type = VM_TYPE_BLOCK})
#define vm_value_from_array(n_) ((vm_value_t){.data.arr = (n_), .type = VM_TYPE_ARRAY})
#define vm_value_from_static(n_) ((vm_value_t){.data.arr = (n_), .type = VM_TYPE_STATIC})

#define vm_value_to_int(v_) ((v_).data.ival)
#define vm_value_to_block(v_) ((v_).data.block)
#define vm_value_to_array(v_) ((v_).data.arr)
#define vm_value_to_static(n_) ((v_).data.arr)

#define vm_value_add(a_, b_)                                              \
  (vm_value_from_int(vm_value_to_int(a_) + vm_value_to_int(b_)))
#define vm_value_addi(a_, b_)                                             \
  (vm_value_from_int(vm_value_to_int(a_) + (b_)))

#define vm_value_sub(a_, b_)                                              \
  (vm_value_from_int(vm_value_to_int(a_) - vm_value_to_int(b_)))
#define vm_value_subi(a_, b_)                                             \
  (vm_value_from_int(vm_value_to_int(a_) - (b_)))
#define vm_value_isub(a_, b_) (vm_value_from_int((a_)-vm_value_to_int(b_)))

#define vm_value_mul(a_, b_)                                              \
  (vm_value_from_int(vm_value_to_int(a_) * vm_value_to_int(b_)))
#define vm_value_muli(a_, b_)                                             \
  (vm_value_from_int(vm_value_to_int(a_) * (b_)))

#define vm_value_div(a_, b_)                                              \
  (vm_value_from_int(vm_value_to_int(a_) / vm_value_to_int(b_)))
#define vm_value_divi(a_, b_)                                             \
  (vm_value_from_int(vm_value_to_int(a_) / (b_)))
#define vm_value_idiv(a_, b_)                                             \
  (vm_value_from_int((a_) / vm_value_to_int(b_)))

#define vm_value_mod(a_, b_)                                              \
  vm_value_from_int(fmod(vm_value_to_int(a_), vm_value_to_int(b_)))
#define vm_value_modi(a_, b_)                                             \
  vm_value_from_int(fmod(vm_value_to_int(a_), (b_)))
#define vm_value_imod(a_, b_)                                             \
  vm_value_from_int(fmod((a_), vm_value_to_int(b_)))

#define vm_value_to_bool(v_) (vm_value_to_int(v_) != 0)

#define vm_value_is_equal(a_, b_)                                         \
  (vm_value_to_int(a_) == vm_value_to_int(b_))
#define vm_value_is_equal_int(a_, b_) (vm_value_to_int(a_) == (b_))

#define vm_value_is_less(a_, b_)                                          \
  (vm_value_to_int(a_) < vm_value_to_int(b_))
#define vm_value_is_less_int(a_, b_) (vm_value_to_int(a_) < (b_))
#define vm_value_is_int_less(a_, b_) ((a_) < vm_value_to_int(b_))

#endif
