#pragma once

#include "../lib.h"
#include <gmp.h>

typedef int64_t vm_int_t;
typedef double vm_float_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

struct vm_value_t
{
  union {
    ptrdiff_t ival;
    size_t uval;
  };
};

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

struct vm_gc_t
{
  size_t str_root;
  char **restrict str_buf;
  size_t *restrict str_lens;
  uint8_t *restrict str_marks;
  size_t str_used;
  size_t str_alloc;

  vm_value_t **restrict arr_buf;
  size_t *restrict arr_lens;
  uint8_t *restrict arr_marks;
  size_t arr_used;
  size_t arr_alloc;

  size_t *restrict move_buf;
  size_t move_alloc;

  size_t nregs;
  vm_value_t *restrict regs;

  size_t count;
  size_t max;

  bool running;
};

void vm_gc_init(vm_gc_t *restrict out);
void vm_gc_stop(vm_gc_t gc);
void vm_gc_run(vm_gc_t *restrict gc);

size_t vm_gc_arr(vm_gc_t *restrict gc, size_t size);
void vm_gc_set_char(vm_gc_t *restrict gc, size_t ptr, vm_int_t index, char chr);

size_t vm_gc_len(vm_gc_t *restrict gc, vm_value_t ptr);
vm_value_t vm_gc_get_v(vm_gc_t *restrict gc, vm_value_t ptr, vm_value_t key);

void vm_gc_set_vv(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t key, vm_value_t value);

#define VM_VALUE_SHORT_OKAY(n_) ({ vm_int_t x_ = (n_); -(1L<<28)<x_&&x_<(1L<<28); })

#define VM_VALUE_GET_INT(n_) ((n_).ival >> 1)
#define VM_VALUE_GET_ARR(n_) ((size_t)((n_).uval >> 1))

#define VM_VALUE_SET_INT(n_) ((vm_value_t){.ival = (n_) << 1 })
#define VM_VALUE_SET_ARR(n_) ((vm_value_t){.uval = ((n_) << 1) | 1})

#define VM_VALUE_IS_INT(n_) ((n_).ival & 0x1 == 0)
#define VM_VALUE_IS_ARR(n_) ((n_).uval & 0x1 == 1)

static inline vm_value_t vm_value_from_func(vm_int_t n)
{
  return VM_VALUE_SET_INT(n);
}

static inline vm_int_t vm_value_to_func(vm_value_t x)
{
  return VM_VALUE_GET_INT(x);
}

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
