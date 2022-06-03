#pragma once

#include "lib.h"

typedef int64_t vm_int_t;
typedef double vm_float_t;

#if !VM_CONFIG_BIGINT
struct vm_value_t;
typedef struct vm_value_t vm_value_t;
struct vm_value_t {
  int64_t intval;
};

#define VM_VALUE_IS_BIGINT(n) (0)
#define VM_VALUE_GET_BIGINT(n) (__builtin_trap(), (vm_value_t) {.intval = 0})
#define VM_VALUE_SET_BIGINT(n) (__builtin_trap(), (vm_value_t) {.intval = 0})
#define VM_VALUE_GET_INT64(n) ((n).intval)

#define vm_value_from_int(gc, n) ((vm_value_t){.intval = (n)})
#define vm_value_from_func(n) ((vm_value_t){.intval = (n)})

#define vm_value_to_bool(gc, x) (vm_value_to_int(gc, x) != 0)
#define vm_value_to_int(gc, n) ((n).intval)
#define vm_value_to_func(n) ((n).intval)

#define vm_value_add(gc, x, y) vm_value_from_int(gc, vm_value_to_int(gc, x) + vm_value_to_int(gc, y))
#define vm_value_sub(gc, x, y) vm_value_from_int(gc, vm_value_to_int(gc, x) - vm_value_to_int(gc, y))
#define vm_value_mul(gc, x, y) vm_value_from_int(gc, vm_value_to_int(gc, x) * vm_value_to_int(gc, y))
#define vm_value_div(gc, x, y) vm_value_from_int(gc, vm_value_to_int(gc, x) / vm_value_to_int(gc, y))
#define vm_value_mod(gc, x, y) vm_value_from_int(gc, vm_value_to_int(gc, x) % vm_value_to_int(gc, y))

#define vm_value_is_equal(gc, x, y) (vm_value_to_int(gc, x) == vm_value_to_int(gc, y))
#define vm_value_is_less(gc, x, y) (vm_value_to_int(gc, x) < vm_value_to_int(gc, y))
#else
#include "int/gc.h"
#pragma GCC push_options
#pragma GCC optimize "no-ssa-phiopt"
union vm_value_t;
typedef union vm_value_t vm_value_t;

#define VM_VALUE_SHORT_OKAY(n_) ({ vm_int_t x_ = (n_) << 4; x_ == (int16_t)x_; })

#define VM_VALUE_GET_INT64(n_) ((n_).ival / (1 << 4))
#define VM_VALUE_GET_FUNC(n_) ((n_).ival >> 4)
#define VM_VALUE_GET_BIGINT(n_) (&gc->nums[(n_).ival >> 4])

#define VM_VALUE_SET_INT64(n_) ((vm_value_t){.ival = ((n_) * (1 << 4)) | 0})
#define VM_VALUE_SET_FUNC(f_) ((vm_value_t){.ival = ((f_) << 4) | 1})
#define VM_VALUE_SET_BIGINT(b_) ((vm_value_t){.ival = ((b_) << 4) | 2})

#define VM_VALUE_IS_INT64(n_) (((n_).ival & 0xF) == 0)
#define VM_VALUE_IS_FUNC(n_) (((n_).ival & 0xF) == 1)
#define VM_VALUE_IS_BIGINT(n_) (((n_).ival & 0xF) == 2)

union vm_value_t
{
  int64_t ival;
};

static inline vm_value_t vm_value_from_int(vm_gc_t *restrict gc, vm_int_t n)
{
  if (!VM_VALUE_SHORT_OKAY(n))
  {
    vm_value_t ret;
    ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
    mpz_set_si(VM_VALUE_GET_BIGINT(ret), n);
    return ret;
  }
  else
  {
    return VM_VALUE_SET_INT64(n);
  }
}

static inline vm_value_t vm_value_from_func(vm_int_t n)
{
  return VM_VALUE_SET_FUNC(n);
}

static inline bool vm_value_to_bool(vm_gc_t *restrict gc, vm_value_t val)
{
  if (VM_VALUE_IS_INT64(val))
  {
    return VM_VALUE_GET_INT64(val) != 0;
  }
  else
  {
    return mpz_sgn(VM_VALUE_GET_BIGINT(val)) != 0;
  }
}

static vm_int_t vm_value_to_int(vm_gc_t *restrict gc, vm_value_t val)
{
  if (VM_VALUE_IS_INT64(val))
  {
    return VM_VALUE_GET_INT64(val);
  }
  else
  {
    return mpz_get_si(VM_VALUE_GET_BIGINT(val));
  }
}

static vm_value_t vm_value_add(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (VM_VALUE_IS_INT64(lhs))
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(rhs)))
      {
        ret = VM_VALUE_SET_INT64(VM_VALUE_GET_INT64(lhs) + VM_VALUE_GET_INT64(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs) + VM_VALUE_GET_INT64(rhs));
      }
    }
    else
    {
      if (VM_VALUE_GET_INT64(lhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs), (uint64_t)-VM_VALUE_GET_INT64(lhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs), (uint64_t)VM_VALUE_GET_INT64(lhs));
      }
    }
  }
  else
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_GET_INT64(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)-VM_VALUE_GET_INT64(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)VM_VALUE_GET_INT64(rhs));
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
  if (VM_VALUE_IS_INT64(lhs))
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(rhs)))
      {
        ret = VM_VALUE_SET_INT64(VM_VALUE_GET_INT64(lhs) - VM_VALUE_GET_INT64(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs) - VM_VALUE_GET_INT64(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs));
      mpz_sub(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_GET_INT64(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_add_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)-VM_VALUE_GET_INT64(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_sub_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)VM_VALUE_GET_INT64(rhs));
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
  if (VM_VALUE_IS_INT64(lhs))
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(rhs)))
      {
        ret = VM_VALUE_SET_INT64(VM_VALUE_GET_INT64(lhs) * VM_VALUE_GET_INT64(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs) * VM_VALUE_GET_INT64(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs));
      mpz_mul(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_mul_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_INT64(rhs));
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
  if (VM_VALUE_IS_INT64(lhs))
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(rhs)))
      {
        ret = VM_VALUE_SET_INT64(VM_VALUE_GET_INT64(lhs) / VM_VALUE_GET_INT64(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs) / VM_VALUE_GET_INT64(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs));
      mpz_tdiv_q(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_GET_INT64(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_t rhsz;
        mpz_set_si(rhsz, VM_VALUE_GET_INT64(rhs));
        mpz_tdiv_q(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhsz);
        mpz_clear(rhsz);
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_tdiv_q_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)VM_VALUE_GET_INT64(rhs));
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
  if (VM_VALUE_IS_INT64(lhs))
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(lhs)) && VM_VALUE_SHORT_OKAY(VM_VALUE_GET_INT64(rhs)))
      {
        ret = VM_VALUE_SET_INT64(VM_VALUE_GET_INT64(lhs) % VM_VALUE_GET_INT64(rhs));
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs) % VM_VALUE_GET_INT64(rhs));
      }
    }
    else
    {
      ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
      mpz_set_si(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_INT64(lhs));
      mpz_mod(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(rhs));
    }
  }
  else
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      if (VM_VALUE_GET_INT64(rhs) < 0)
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_t rhsz;
        mpz_set_si(rhsz, VM_VALUE_GET_INT64(rhs));
        mpz_mod(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), rhsz);
        mpz_clear(rhsz);
      }
      else
      {
        ret = VM_VALUE_SET_BIGINT(vm_gc_num(gc));
        mpz_mod_ui(VM_VALUE_GET_BIGINT(ret), VM_VALUE_GET_BIGINT(lhs), (uint64_t)VM_VALUE_GET_INT64(rhs));
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
  if (VM_VALUE_IS_INT64(lhs))
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      return VM_VALUE_GET_INT64(lhs) == VM_VALUE_GET_INT64(rhs);
    }
    else
    {
      return mpz_cmp_si(VM_VALUE_GET_BIGINT(rhs), VM_VALUE_GET_INT64(lhs)) == 0;
    }
  }
  else
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      return mpz_cmp_si(VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_INT64(rhs)) == 0;
    }
    else
    {
      return mpz_cmp(VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_BIGINT(rhs)) == 0;
    }
  }
}

static bool vm_value_is_less(vm_gc_t *restrict gc, vm_value_t lhs, vm_value_t rhs)
{
  if (VM_VALUE_IS_INT64(lhs))
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      return VM_VALUE_GET_INT64(lhs) < VM_VALUE_GET_INT64(rhs);
    }
    else
    {
      return mpz_cmp_si(VM_VALUE_GET_BIGINT(rhs), VM_VALUE_GET_INT64(lhs)) > 0;
    }
  }
  else
  {
    if (VM_VALUE_IS_INT64(rhs))
    {
      return mpz_cmp_si(VM_VALUE_GET_BIGINT(lhs), VM_VALUE_GET_INT64(rhs)) < 0;
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
#pragma GCC pop_options
#endif