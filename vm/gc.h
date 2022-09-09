
#if !defined(VM_HEADER_INT_GC)
#define VM_HEADER_INT_GC

#include "lib.h"

typedef int64_t vm_int_t;
typedef int64_t vm_loc_t;
typedef int64_t vm_reg_t;

struct vm_value_array_t;
typedef struct vm_value_array_t vm_value_array_t;

struct vm_value_meta_t;
typedef struct vm_value_meta_t vm_value_meta_t;

union vm_value_data_t;
typedef union vm_value_data_t vm_value_data_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

struct vm_value_array_t {
  vm_value_data_t *datas;
  uint8_t *types;
  size_t len: 32;
  size_t alloc: 32;
};

enum {
  VM_META_ADD,
  VM_META_SUB,
  VM_META_MUL,
  VM_META_DIV,
  VM_META_MOD,
  VM_META_GET,
  VM_META_SET,
  VM_META_MAX,
};

struct vm_value_meta_t {
  void *cases[VM_META_MAX];
  vm_value_t *value;
};

enum {
  VM_TYPE_VOID,
  VM_TYPE_FLOAT,
  VM_TYPE_BLOCK,
  VM_TYPE_ARRAY,
  VM_TYPE_STATIC,
  VM_TYPE_META,
};

union vm_value_data_t {
  void *block;
  vm_value_array_t *arr;
  double ival;
  vm_value_meta_t *meta;
};

struct vm_value_t {
  vm_value_data_t data;
  uint8_t type;
};

vm_value_t vm_gc_new(vm_int_t slots);
vm_value_t vm_gc_get(vm_value_t obj, vm_value_t index);
void vm_gc_set(vm_value_t obj, vm_value_t index, vm_value_t value);
vm_int_t vm_gc_len(vm_value_t obj);

#define vm_gc_get_v(obj_, nth_) vm_gc_get(obj_, (nth_))
#define vm_gc_get_i(obj_, nth_)                                           \
  vm_gc_get(obj_, vm_value_from_int(nth_))

#define vm_gc_set_vv(obj_, nth_, val_) vm_gc_set(obj_, nth_, val_)
#define vm_gc_set_vi(obj_, nth_, val_)                                    \
  vm_gc_set(obj_, nth_, vm_value_from_int(val_))
#define vm_gc_set_iv(obj_, nth_, val_)                                    \
  vm_gc_set(obj_, vm_value_from_int(nth_), val_)
#define vm_gc_set_ii(obj_, nth_, val_)                                    \
  vm_gc_set(obj_, vm_value_from_int(nth_), vm_value_from_int(val_))

#define vm_value_from_int(n_) ((vm_value_t){.data.ival = (n_), .type = VM_TYPE_FLOAT})
#define vm_value_from_block(n_) ((vm_value_t){.data.block = (n_), .type = VM_TYPE_BLOCK})
#define vm_value_from_array(n_) ((vm_value_t){.data.arr = (n_), .type = VM_TYPE_ARRAY})
#define vm_value_from_static(n_) ((vm_value_t){.data.arr = (n_), .type = VM_TYPE_STATIC})

#define vm_value_to_int(v_) ((v_).data.ival)
#define vm_value_to_block(v_) ((v_).data.block)
#define vm_value_to_array(v_) ((v_).data.arr)
#define vm_value_to_static(n_) ((v_).data.arr)

#endif
