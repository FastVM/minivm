
#if !defined(VM_HEADER_INT_GC)
#define VM_HEADER_INT_GC

#include "../lib.h"

typedef int64_t vm_int_t;
typedef int64_t vm_loc_t;
typedef int64_t vm_reg_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

struct vm_gc_header_t;
typedef struct vm_gc_header_t vm_gc_header_t;

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

struct vm_value_t
{
  void *ptr;
  vm_int_t ival;
};

struct vm_gc_header_t
{
  uint16_t mark;
  uint16_t type;
  uint32_t len;
};

typedef union {
  vm_value_t value;
  vm_gc_header_t header;
} vm_gc_data_t;

struct vm_gc_t
{
  vm_gc_data_t *buf;
  vm_int_t buf_used;
  vm_int_t buf_alloc;

  vm_int_t *restrict move_buf;
  vm_int_t move_alloc;

  vm_int_t nregs;
  vm_value_t *regs;

  vm_int_t count;
  vm_int_t max;

  bool running;
};

void vm_gc_init(vm_gc_t *restrict out);
void vm_gc_stop(vm_gc_t gc);
void vm_gc_run(vm_gc_t *restrict gc);

vm_int_t vm_gc_arr(vm_gc_t *restrict gc, vm_int_t size);

#define VM_VALUE_GET_INT(n_) ((n_).ival >> 1)
#define VM_VALUE_GET_ARR(n_) ((n_).ival >> 1)

#define VM_VALUE_SET_INT(n_) ((vm_value_t) {.ival = (n_) * 2})
#define VM_VALUE_SET_ARR(n_) ((vm_value_t) {.ival = ((n_) * 2) | 1})

#define VM_VALUE_IS_INT(n_) (((n_).ival & 1) == 0)
#define VM_VALUE_IS_ARR(n_) (((n_).ival & 1) == 1)

#define vm_gc_new(gc_, len_) VM_VALUE_SET_ARR(vm_gc_arr(gc_, len_))
#define vm_gc_len(gc_, obj_) ((gc_)->buf[VM_VALUE_GET_ARR(obj_) - 1].header.len)
#define vm_gc_get_v(gc_, obj_, nth_) ((gc_)->buf[VM_VALUE_GET_ARR(obj_) + VM_VALUE_GET_INT(nth_)].value)
#define vm_gc_get_i(gc_, obj_, nth_) ((gc_)->buf[VM_VALUE_GET_ARR(obj_) + (nth_)].value)

#define vm_gc_set_vv(gc_, obj_, nth_, val_) ((gc_)->buf[VM_VALUE_GET_ARR(obj_) + VM_VALUE_GET_INT(nth_)].value = (val_))
#define vm_gc_set_vi(gc_, obj_, nth_, val_) ((gc_)->buf[VM_VALUE_GET_ARR(obj_) + VM_VALUE_GET_INT(nth_)].value = VM_VALUE_SET_INT(val_))
#define vm_gc_set_iv(gc_, obj_, nth_, val_) ((gc_)->buf[VM_VALUE_GET_ARR(obj_) + (nth_)].value = (val_))
#define vm_gc_set_ii(gc_, obj_, nth_, val_) ((gc_)->buf[VM_VALUE_GET_ARR(obj_) + (nth_)].value = VM_VALUE_SET_INT(val_))

#define vm_value_from_func(gc_, n_) (VM_VALUE_SET_INT(n_))
#define vm_value_to_func(gc_, n_) (VM_VALUE_GET_INT(n_))

#define vm_value_from_int(gc_, n_) (VM_VALUE_SET_INT(n_))
#define vm_value_to_int(gc_, n_) (VM_VALUE_GET_INT(n_))

#define vm_value_add(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) + VM_VALUE_GET_INT(b_)))
#define vm_value_addi(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) + (b_)))
#define vm_value_sub(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) - VM_VALUE_GET_INT(b_)))
#define vm_value_subi(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) - (b_)))
#define vm_value_isub(gc_, a_, b_) (VM_VALUE_SET_INT((a_) - VM_VALUE_GET_INT(b_)))
#define vm_value_mul(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) * VM_VALUE_GET_INT(b_)))
#define vm_value_muli(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) * (b_)))
#define vm_value_div(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) / VM_VALUE_GET_INT(b_)))
#define vm_value_divi(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) / (b_)))
#define vm_value_idiv(gc_, a_, b_) (VM_VALUE_SET_INT ((a_) / VM_VALUE_GET_INT(b_)))
#define vm_value_mod(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) % VM_VALUE_GET_INT(b_)))
#define vm_value_modi(gc_, a_, b_) (VM_VALUE_SET_INT(VM_VALUE_GET_INT(a_) % (b_)))
#define vm_value_imod(gc_, a_,b_) (VM_VALUE_SET_INT((a_) % VM_VALUE_GET_INT(b_)))
#define vm_value_to_bool(gc_, v_) (VM_VALUE_GET_INT(v_) != 0)
#define vm_value_is_equal(gc_, a_, b_) (VM_VALUE_GET_INT(a_) == VM_VALUE_GET_INT(b_))
#define vm_value_is_equal_int(gc_, a_, b_) (VM_VALUE_GET_INT(a_) == (b_))
#define vm_value_is_less(gc_, a_, b_) (VM_VALUE_GET_INT(a_) < VM_VALUE_GET_INT(b_))
#define vm_value_is_less_int(gc_, a_, b_) (VM_VALUE_GET_INT(a_) < (b_))
#define vm_value_is_int_less(gc_, a_, b_) ((a_) < VM_VALUE_GET_INT(b_))
#define vm_value_typeof(gc_, v_) ({vm_value_t t_ = (v_); VM_VALUE_IS_INT(t_) ? 0 : VM_VALUE_IS_ARR(t_) ? 1 : 2;})

#endif
