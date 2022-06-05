#pragma once

#include "../lib.h"
#include <gmp.h>

typedef int64_t vm_int_t;
typedef double vm_float_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

struct vm_value_t
{
  uint8_t type: 4;
  int64_t value: 60;
};

enum {
  VM_TYPE_INT = 0,
  VM_TYPE_FUN = 1,
  VM_TYPE_BIG = 2,
  VM_TYPE_STR = 3,
  VM_TYPE_ARR = 4,
  VM_TYPE_MAP = 5,
};

struct vm_gc_t;
typedef struct vm_gc_t vm_gc_t;

struct vm_gc_t {
    MP_INT *num_buf;
    uint8_t *num_marks;
    uint32_t num_used;
    uint32_t num_alloc;
    
    uint32_t str_root;
    char **str_buf;
    uint32_t *str_lens;
    uint8_t *str_marks;
    uint32_t str_used;
    uint32_t str_alloc;
    
    vm_value_t **arr_buf;
    uint32_t *arr_lens;
    uint8_t *arr_marks;
    uint32_t arr_used;
    uint32_t arr_alloc;

    uint32_t *move_buf;
    uint32_t move_alloc;

    uint32_t count;
    uint32_t max;

    bool running;
};

void vm_gc_init(vm_gc_t *restrict out);
void vm_gc_stop(vm_gc_t gc);
void vm_gc_run(vm_gc_t *restrict gc, size_t nregs, void *regs);

uint32_t vm_gc_num(vm_gc_t *restrict gc);
uint32_t vm_gc_str(vm_gc_t *restrict gc, size_t size);
uint32_t vm_gc_arr(vm_gc_t *restrict gc, size_t size);
void vm_gc_set_char(vm_gc_t *restrict gc, uint32_t ptr, vm_int_t index, char chr);

uint32_t vm_gc_len(vm_gc_t *restrict gc, vm_value_t ptr);
vm_value_t vm_gc_get_v(vm_gc_t *restrict gc, vm_value_t ptr, vm_value_t key);
vm_value_t vm_gc_get_i(vm_gc_t *restrict gc, vm_value_t ptr, vm_int_t key);

void vm_gc_set_vv(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t key, vm_value_t value);
void vm_gc_set_vi(vm_gc_t *restrict gc, vm_value_t obj, vm_value_t key, vm_int_t value);
void vm_gc_set_iv(vm_gc_t *restrict gc, vm_value_t obj, vm_int_t key, vm_value_t value);
void vm_gc_set_ii(vm_gc_t *restrict gc, vm_value_t obj, vm_int_t key, vm_int_t value);

mpz_ptr vm_gc_num_get(vm_gc_t *restrict gc, uint32_t n);

#define VM_VALUE_SHORT_OKAY(n_) ({ vm_int_t x_ = (n_); -(1L<<30)<x_&&x_<(1L<<30); })

#define VM_VALUE_GET_INT(n_) ((n_).value)
#define VM_VALUE_GET_FUN(n_) ((n_).value)
#define VM_VALUE_GET_BIG(n_) (&gc->num_buf[(n_).value])

#define VM_VALUE_SET_INT(n_) ((vm_value_t) {.type = VM_TYPE_INT, .value = (n_)})
#define VM_VALUE_SET_FUN(n_) ((vm_value_t) {.type = VM_TYPE_FUN, .value = (n_)})
#define VM_VALUE_SET_BIG(n_) ((vm_value_t) {.type = VM_TYPE_BIG, .value = (n_)})
#define VM_VALUE_SET_STR(n_) ((vm_value_t) {.type = VM_TYPE_STR, .value = (n_)})
#define VM_VALUE_SET_ARR(n_) ((vm_value_t) {.type = VM_TYPE_ARR, .value = (n_)})

#define VM_VALUE_IS_INT(n_) ((n_).type == VM_TYPE_INT)
#define VM_VALUE_IS_FUN(n_) ((n_).type == VM_TYPE_FUN)
#define VM_VALUE_IS_BIG(n_) ((n_).type == VM_TYPE_BIG)
#define VM_VALUE_IS_STR(n_) ((n_).type == VM_TYPE_STR)
#define VM_VALUE_IS_ARR(n_) ((n_).type == VM_TYPE_ARR)

uint32_t vm_gc_num(vm_gc_t *restrict gc);

static inline vm_value_t vm_value_from_int(vm_gc_t *restrict gc, vm_int_t n)
{
  vm_value_t ret;
  if (VM_VALUE_SHORT_OKAY(n)) {
    ret = VM_VALUE_SET_INT(n);  
  } else {
    ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_set_si(VM_VALUE_GET_BIG(ret), n);
  }
  return ret;
}

static inline vm_value_t vm_value_from_func(vm_int_t n)
{
  return VM_VALUE_SET_FUN(n);
}

static inline bool vm_value_to_bool(vm_gc_t *restrict gc, vm_value_t val)
{
  if (VM_VALUE_IS_INT(val))
  {
    return VM_VALUE_GET_INT(val) != 0;
  }
  else
  {
    return mpz_sgn(VM_VALUE_GET_BIG(val)) != 0;
  }
}

static vm_int_t vm_value_to_int(vm_gc_t *restrict gc, vm_value_t val)
{
  if (VM_VALUE_IS_INT(val))
  {
    return VM_VALUE_GET_INT(val);
  }
  else
  {
    return mpz_get_si(VM_VALUE_GET_BIG(val));
  }
}

static vm_value_t vm_value_add(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT(lhs))
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(rhs)))
      {
        ret = VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) + VM_VALUE_GET_INT(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs) + VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      if (VM_VALUE_GET_INT(lhs) < 0)
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs), (uint64_t)-VM_VALUE_GET_INT(lhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs), (uint64_t)VM_VALUE_GET_INT(lhs));
      }
    }
  }
  else
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_GET_INT(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), (uint64_t)-VM_VALUE_GET_INT(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), (uint64_t)VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_add(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_BIG(rhs));
    }
  }
  return ret;
}

static vm_value_t vm_value_sub(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT(lhs))
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(rhs)))
      {
        ret = VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) - VM_VALUE_GET_INT(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs) - VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs));
      mpz_sub(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_GET_INT(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), (uint64_t)-VM_VALUE_GET_INT(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), (uint64_t)VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_sub(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_BIG(rhs));
    }
  }
  return ret;
}

static vm_value_t vm_value_mul(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT(lhs))
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(rhs)))
      {
        ret = VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) * VM_VALUE_GET_INT(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs) * VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs));
      mpz_mul(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_mul_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_INT(rhs));
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_mul(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_BIG(rhs));
    }
  }
  return ret;
}

static vm_value_t vm_value_div(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT(lhs))
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(rhs)))
      {
        ret = VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) / VM_VALUE_GET_INT(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs) / VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs));
      mpz_tdiv_q(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_GET_INT(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_t rhsz;
        mpz_set_si(rhsz, VM_VALUE_GET_INT(rhs));
        mpz_tdiv_q(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), rhsz);
        mpz_clear(rhsz);
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_tdiv_q_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), (uint64_t)VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_tdiv_q(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_BIG(rhs));
    }
  }
  return ret;
}

static vm_value_t vm_value_mod(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT(lhs))
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT(rhs)))
      {
        ret = VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) % VM_VALUE_GET_INT(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs) % VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_INT(lhs));
      mpz_mod(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      if (VM_VALUE_GET_INT(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_t rhsz;
        mpz_set_si(rhsz, VM_VALUE_GET_INT(rhs));
        mpz_mod(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), rhsz);
        mpz_clear(rhsz);
      }
      else
      {
        ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
        mpz_mod_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), (uint64_t)VM_VALUE_GET_INT(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
      mpz_mod(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_BIG(rhs));
    }
  }
  return ret;
}

static bool vm_value_is_equal(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      return VM_VALUE_GET_INT(lhs) == VM_VALUE_GET_INT(rhs);
    }
    else
    {
      return mpz_cmp_si(VM_VALUE_GET_BIG(rhs), VM_VALUE_GET_INT(lhs)) == 0;
    }
  }
  else
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      return mpz_cmp_si(VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_INT(rhs)) == 0;
    }
    else
    {
      return mpz_cmp(VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_BIG(rhs)) == 0;
    }
  }
}

static bool vm_value_is_less(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      return VM_VALUE_GET_INT(lhs) < VM_VALUE_GET_INT(rhs);
    }
    else
    {
      return mpz_cmp_si(VM_VALUE_GET_BIG(rhs), VM_VALUE_GET_INT(lhs)) > 0;
    }
  }
  else
  {
    if (VM_VALUE_IS_INT(rhs))
    {
      return mpz_cmp_si(VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_INT(rhs)) < 0;
    }
    else
    {
      return mpz_cmp(VM_VALUE_GET_BIG(lhs), VM_VALUE_GET_BIG(rhs)) < 0;
    }
  }
}

static inline vm_int_t vm_value_to_func(vm_value_t x)
{
  return VM_VALUE_GET_FUN(x);
}

// with an int argument

static inline vm_value_t vm_value_addi(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    return VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) + rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_add_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_subi(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    return VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) - rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_sub_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_isub(vm_gc_t *restrict gc, vm_int_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT(rhs))
  {
    return VM_VALUE_SET_INT(lhs - VM_VALUE_GET_INT(rhs));
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_sub_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs), lhs);
    mpz_neg(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(ret));
    return ret;
  }
}

static inline vm_value_t vm_value_muli(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    return VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) * rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_mul_si(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_divi(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    return VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) / rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_div_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_idiv(vm_gc_t *restrict gc, vm_int_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT(rhs))
  {
    return VM_VALUE_SET_INT(lhs / VM_VALUE_GET_INT(rhs));
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_init_set_si(VM_VALUE_GET_BIG(ret), lhs);
    mpz_div(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs));
    return ret;
  }
}

static inline vm_value_t vm_value_modi(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    return VM_VALUE_SET_INT(VM_VALUE_GET_INT(lhs) % rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_mod_ui(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_imod(vm_gc_t *restrict gc, vm_int_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT(rhs))
  {
    return VM_VALUE_SET_INT(lhs % VM_VALUE_GET_INT(rhs));
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIG(vm_gc_num(gc));
    mpz_init_set_si(VM_VALUE_GET_BIG(ret), lhs);
    mpz_mod(VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(ret), VM_VALUE_GET_BIG(rhs));
    return ret;
  }
}

static bool vm_value_is_equal_int(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    return VM_VALUE_GET_INT(lhs) == rhs;
  }
  else
  {
    return mpz_cmp_si(VM_VALUE_GET_BIG(lhs), rhs) == 0;
  }
}

static bool vm_value_is_less_int(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT(lhs))
  {
    return VM_VALUE_GET_INT(lhs) < rhs;
  }
  else
  {
    return mpz_cmp_si(VM_VALUE_GET_BIG(lhs), rhs) < 0;
  }
}


static bool vm_value_is_int_less(vm_gc_t *restrict gc, vm_int_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT(rhs))
  {
    return VM_VALUE_GET_INT(rhs) > lhs;
  }
  else
  {
    return mpz_cmp_si(VM_VALUE_GET_BIG(rhs), lhs) > 0;
  }
}
