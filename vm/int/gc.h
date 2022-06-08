#pragma once

#include "../lib.h"

typedef int64_t vm_int_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

#if 0
struct vm_value_t
{
  uint8_t type: 3;
  int64_t ival: 61;
};
#else
struct vm_value_t
{
  vm_int_t ival;
};
#endif

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

struct vm_gc_t
{
  vm_value_t **restrict arr_buf;
  vm_int_t *restrict arr_lens;
  uint8_t *restrict arr_marks;
  vm_int_t arr_used;
  vm_int_t arr_alloc;

  vm_int_t *restrict move_buf;
  vm_int_t move_alloc;

  vm_int_t nregs;
  vm_value_t *restrict regs;

  vm_int_t count;
  vm_int_t max;

  bool running;
};

void vm_gc_init(vm_gc_t *restrict out);
void vm_gc_stop(vm_gc_t gc);
void vm_gc_run(vm_gc_t *restrict gc);

vm_int_t vm_gc_arr(vm_gc_t *restrict gc, vm_int_t size);
void vm_gc_set_char(vm_gc_t *restrict gc, vm_int_t ptr, vm_int_t index, char chr);

vm_int_t vm_gc_len(vm_gc_t *restrict gc, vm_value_t ptr);
vm_value_t vm_gc_get_v(vm_gc_t *restrict gc, vm_value_t ptr, vm_value_t key);

void vm_gc_set_vv(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t key, vm_value_t value);

#if 0
#define VM_VALUE_GET_INT(n_) ((n_).ival)
#define VM_VALUE_GET_ARR(n_) ((n_).ival)

#define VM_VALUE_SET_INT(n_) ((vm_value_t){.type = 0, .ival = (n_)})
#define VM_VALUE_SET_ARR(n_) ((vm_value_t){.type = 1, .ival = (n_)})

#define VM_VALUE_IS_INT(n_) ((n_).type == 0)
#define VM_VALUE_IS_ARR(n_) ((n_).type == 1)
#else
#define VM_VALUE_GET_INT(n_) ((n_).ival >> 1)
#define VM_VALUE_GET_ARR(n_) ((n_).ival >> 1)

#define VM_VALUE_SET_INT(n_) ((vm_value_t) {.ival = (n_) << 1})
#define VM_VALUE_SET_ARR(n_) ((vm_value_t) {.ival = ((n_) << 1) | 1})

#define VM_VALUE_IS_INT(n_) (((n_).ival & 1) == 0)
#define VM_VALUE_IS_ARR(n_) (((n_).ival & 1) == 1)
#endif

#define vm_value_from_func(n_) (VM_VALUE_SET_INT(n_))
#define vm_value_to_func(n_) (VM_VALUE_GET_INT(n_))

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
