#pragma once

#include "lib.h"
#include "int/gc.h"

typedef int64_t vm_int_t;
typedef double vm_float_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

enum {
  VM_TYPE_INT32 = 0,
  VM_TYPE_FUNC = 1,
  VM_TYPE_BIGINT = 2,
};

#define VM_VALUE_SHORT_OKAY(n_) ({ vm_int_t x_ = (n_); -(1L<<30)<x_&&x_<(1L<<30); })

#define VM_VALUE_GET_INT32(n_) ((n_).value)
#define VM_VALUE_GET_FUNC(n_) ((n_).value)
#define VM_VALUE_GET_BIGINT(n_) (&gc->nums[(n_).value])

#define VM_VALUE_SET_INT32(n_) ((vm_value_t) {.type = VM_TYPE_INT32, .value = (n_)})
#define VM_VALUE_SET_FUNC(n_) ((vm_value_t) {.type = VM_TYPE_FUNC, .value = (n_)})
#define VM_VALUE_SET_BIGINT(n_) ((vm_value_t) {.type = VM_TYPE_BIGINT, .value = (n_)})

#define VM_VALUE_IS_INT32(n_) ((n_).type == VM_TYPE_INT32)
#define VM_VALUE_IS_FUNC(n_) ((n_).type == VM_TYPE_FUNC)
#define VM_VALUE_IS_BIGINT(n_) ((n_).type == VM_TYPE_BIGINT)

#define VM_VALUE_POSSIBLE_CINT()

struct vm_value_t
{
  uint8_t type: 4;
  int64_t value: 60;
};

static inline vm_value_t vm_value_from_int(vm_gc_t *restrict gc, vm_int_t n)
{
  vm_value_t ret;
  if (VM_VALUE_SHORT_OKAY(n)) {
    ret = VM_VALUE_SET_INT32(n);  
  } else {
    ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_set_si(VM_VALUE_GET_BIGINT(ret), n);
  }
  return ret;
}

static inline vm_value_t vm_value_from_func(vm_int_t n)
{
  return VM_VALUE_SET_FUNC(n);
}

static inline bool vm_value_to_bool(vm_gc_t *restrict gc, vm_value_t val)
{
  if (VM_VALUE_IS_INT32(val))
  {
    return VM_VALUE_GET_INT32(val) != 0;
  }
  else
  {
    return mpz_sgn(VM_VALUE_GET_BIGINT(val)) != 0;
  }
}

static vm_int_t vm_value_to_int(vm_gc_t *restrict gc, vm_value_t val)
{
  if (VM_VALUE_IS_INT32(val))
  {
    return VM_VALUE_GET_INT32(val);
  }
  else
  {
    return mpz_get_si(VM_VALUE_GET_BIGINT(val));
  }
}

static vm_value_t vm_value_add(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT32(lhs))
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(rhs)))
      {
        ret = VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) + VM_VALUE_GET_INT32(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs) + VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      if (VM_VALUE_GET_INT32(lhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs), (uint64_t)-VM_VALUE_GET_INT32(lhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs), (uint64_t)VM_VALUE_GET_INT32(lhs));
      }
    }
  }
  else
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_GET_INT32(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)-VM_VALUE_GET_INT32(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_add(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  return ret;
}

static vm_value_t vm_value_sub(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT32(lhs))
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(rhs)))
      {
        ret = VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) - VM_VALUE_GET_INT32(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs) - VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs));
      mpz_sub(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_GET_INT32(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)-VM_VALUE_GET_INT32(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_sub(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  return ret;
}

static vm_value_t vm_value_mul(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT32(lhs))
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(rhs)))
      {
        ret = VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) * VM_VALUE_GET_INT32(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs) * VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs));
      mpz_mul(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_mul_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_INT32(rhs));
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_mul(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  return ret;
}

static vm_value_t vm_value_div(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT32(lhs))
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(rhs)))
      {
        ret = VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) / VM_VALUE_GET_INT32(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs) / VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs));
      mpz_tdiv_q(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_GET_INT32(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_t rhsz;
        mpz_set_si(rhsz, VM_VALUE_GET_INT32(rhs));
        mpz_tdiv_q(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhsz);
        mpz_clear(rhsz);
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_tdiv_q_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_tdiv_q(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  return ret;
}

static vm_value_t vm_value_mod(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT32(lhs))
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT32(rhs)))
      {
        ret = VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) % VM_VALUE_GET_INT32(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs) % VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT32(lhs));
      mpz_mod(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      if (VM_VALUE_GET_INT32(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_t rhsz;
        mpz_set_si(rhsz, VM_VALUE_GET_INT32(rhs));
        mpz_mod(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhsz);
        mpz_clear(rhsz);
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_mod_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)VM_VALUE_GET_INT32(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_mod(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  return ret;
}

static bool vm_value_is_equal(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      return VM_VALUE_GET_INT32(lhs) == VM_VALUE_GET_INT32(rhs);
    }
    else
    {
      return mpz_cmp_si(VM_VALUE_GET_BIGINT(rhs), VM_VALUE_GET_INT32(lhs)) == 0;
    }
  }
  else
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      return mpz_cmp_si(VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_INT32(rhs)) == 0;
    }
    else
    {
      return mpz_cmp(VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_BIGINT(rhs)) == 0;
    }
  }
}

static bool vm_value_is_less(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      return VM_VALUE_GET_INT32(lhs) < VM_VALUE_GET_INT32(rhs);
    }
    else
    {
      return mpz_cmp_si(VM_VALUE_GET_BIGINT(rhs), VM_VALUE_GET_INT32(lhs)) > 0;
    }
  }
  else
  {
    if (VM_VALUE_IS_INT32(rhs))
    {
      return mpz_cmp_si(VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_INT32(rhs)) < 0;
    }
    else
    {
      return mpz_cmp(VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_BIGINT(rhs)) < 0;
    }
  }
}

static inline vm_int_t vm_value_to_func(vm_value_t x)
{
  return VM_VALUE_GET_FUNC(x);
}

// with an int argument

static inline vm_value_t vm_value_addi(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    return VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) + rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_add_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_subi(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    return VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) - rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_sub_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_isub(vm_gc_t *restrict gc, vm_int_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT32(rhs))
  {
    return VM_VALUE_SET_INT32(lhs - VM_VALUE_GET_INT32(rhs));
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_sub_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs), lhs);
    mpz_neg(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret));
    return ret;
  }
}

static inline vm_value_t vm_value_muli(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    return VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) * rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_mul_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_divi(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    return VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) / rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_div_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_idiv(vm_gc_t *restrict gc, vm_int_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT32(rhs))
  {
    return VM_VALUE_SET_INT32(lhs / VM_VALUE_GET_INT32(rhs));
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_init_set_si(VM_VALUE_GET_BIGINT(ret), lhs);
    mpz_div(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    return ret;
  }
}

static inline vm_value_t vm_value_modi(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    return VM_VALUE_SET_INT32(VM_VALUE_GET_INT32(lhs) % rhs);
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_mod_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhs);
    return ret;
  }
}

static inline vm_value_t vm_value_imod(vm_gc_t *restrict gc, vm_int_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT32(rhs))
  {
    return VM_VALUE_SET_INT32(lhs % VM_VALUE_GET_INT32(rhs));
  }
  else
  {
    vm_value_t ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_init_set_si(VM_VALUE_GET_BIGINT(ret), lhs);
    mpz_mod(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    return ret;
  }
}

static bool vm_value_is_equal_int(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    return VM_VALUE_GET_INT32(lhs) == rhs;
  }
  else
  {
    return mpz_cmp_si(VM_VALUE_GET_BIGINT(lhs), rhs) == 0;
  }
}

static bool vm_value_is_less_int(vm_gc_t *restrict gc, vm_value_t lhs, vm_int_t rhs)
{
  if (VM_VALUE_IS_INT32(lhs))
  {
    return VM_VALUE_GET_INT32(lhs) < rhs;
  }
  else
  {
    return mpz_cmp_si(VM_VALUE_GET_BIGINT(lhs), rhs) < 0;
  }
}


static bool vm_value_is_int_less(vm_gc_t *restrict gc, vm_int_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT32(rhs))
  {
    return VM_VALUE_GET_INT32(rhs) > lhs;
  }
  else
  {
    return mpz_cmp_si(VM_VALUE_GET_BIGINT(rhs), lhs) > 0;
  }
}
