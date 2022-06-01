#pragma once

#include "lib.h"

typedef int64_t vm_int_t;
typedef double vm_float_t;

struct vm_value_t;
typedef struct vm_value_t vm_value_t;

#if 0
struct vm_value_t {
  int64_t intval;
};

#define vm_value_from_int(n) ((vm_value_t){.intval = (n)})
#define vm_value_from_func(n) ((vm_value_t){.intval = (n)})

#define vm_value_to_bool(x) (vm_value_to_int(x) != 0)
#define vm_value_to_int(n) ((n).intval)
#define vm_value_to_func(n) ((n).intval)

#define vm_value_add(x, y) vm_value_from_int(vm_value_to_int(x) + vm_value_to_int(y))
#define vm_value_sub(x, y) vm_value_from_int(vm_value_to_int(x) - vm_value_to_int(y))
#define vm_value_mul(x, y) vm_value_from_int(vm_value_to_int(x) * vm_value_to_int(y))
#define vm_value_div(x, y) vm_value_from_int(vm_value_to_int(x) / vm_value_to_int(y))
#define vm_value_mod(x, y) vm_value_from_int(vm_value_to_int(x) % vm_value_to_int(y))

#define vm_value_is_equal(x, y) (vm_value_to_int(x) == vm_value_to_int(y))
#define vm_value_is_less(x, y) (vm_value_to_int(x) < vm_value_to_int(y))
#else

// #define VM_VALUE_SHORT_OKAY(n_) ({vm_int_t x_=(n_); -(1L<<16)<x_&&x_<(1L<<16); })
#define VM_VALUE_SHORT_OKAY(n_) ({ vm_int_t x_ = (n_); -0x10000 < x_ && x_ < 0x10000; })

enum
{
  VM_VALUE_TYPE_INT64,
  VM_VALUE_TYPE_BIGINT,
  VM_VALUE_TYPE_FUNC,
};
struct vm_value_t
{
  union
  {
    vm_loc_t func;
    vm_int_t int64;
    mpz_t bigint;
  };
  uint8_t ty;
};

static inline vm_value_t vm_value_from_int(vm_int_t n)
{
  vm_value_t ret;
  ret.ty = VM_VALUE_TYPE_INT64;
  ret.int64 = n;
  return ret;
}

static inline vm_value_t vm_value_from_func(vm_loc_t n)
{
  return (vm_value_t){
      .ty = VM_VALUE_TYPE_BIGINT,
      .func = n,
  };
}

static inline bool vm_value_to_bool(vm_value_t val)
{
  switch (val.ty)
  {
  case VM_VALUE_TYPE_INT64:
  {
    return val.int64 != 0;
  }
  case VM_VALUE_TYPE_BIGINT:
  {
    return mpz_sgn(val.bigint) != 0;
  }
  case VM_VALUE_TYPE_FUNC:
  {
    return true;
  }
  default:
    __builtin_trap();
  }
}

static vm_int_t vm_value_to_int(vm_value_t val)
{
  switch (val.ty)
  {
  case VM_VALUE_TYPE_INT64:
  {
    return val.int64;
  }
  case VM_VALUE_TYPE_BIGINT:
  {
    return mpz_get_si(val.bigint);
  }
  default:
    __builtin_trap();
  }
}

static vm_value_t vm_value_add(vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (lhs.ty == VM_VALUE_TYPE_INT64)
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (VM_VALUE_SHORT_OKAY(lhs.int64) && VM_VALUE_SHORT_OKAY(rhs.int64))
      {
        ret.ty = VM_VALUE_TYPE_INT64;
        ret.int64 = lhs.int64 + rhs.int64;
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init_set_si(ret.bigint, lhs.int64 + rhs.int64);
      }
    }
    else
    {
      if (lhs.int64 < 0)
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_sub_ui(ret.bigint, rhs.bigint, (uint64_t) -lhs.int64);
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_add_ui(ret.bigint, rhs.bigint, (uint64_t) lhs.int64);
      }
    }
  }
  else
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (rhs.int64 < 0)
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_sub_ui(ret.bigint, lhs.bigint, (uint64_t) -rhs.int64);
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_add_ui(ret.bigint, lhs.bigint, (uint64_t) rhs.int64);
      }
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init(ret.bigint);
      mpz_add(ret.bigint, lhs.bigint, rhs.bigint);
    }
  }
  return ret;
}

static vm_value_t vm_value_sub(vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (lhs.ty == VM_VALUE_TYPE_INT64)
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (VM_VALUE_SHORT_OKAY(lhs.int64) && VM_VALUE_SHORT_OKAY(rhs.int64))
      {
        ret.ty = VM_VALUE_TYPE_INT64;
        ret.int64 = lhs.int64 - rhs.int64;
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init_set_si(ret.bigint, lhs.int64 - rhs.int64);
      }
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init_set_si(ret.bigint, lhs.int64);
      mpz_sub(ret.bigint, ret.bigint, rhs.bigint);
    }
  }
  else
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (rhs.int64 < 0)
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_add_ui(ret.bigint, lhs.bigint, (uint64_t) -rhs.int64);
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_sub_ui(ret.bigint, lhs.bigint, (uint64_t) rhs.int64);
      }
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init(ret.bigint);
      mpz_sub(ret.bigint, lhs.bigint, rhs.bigint);
    }
  }
  return ret;
}

static vm_value_t vm_value_mul(vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (lhs.ty == VM_VALUE_TYPE_INT64)
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (VM_VALUE_SHORT_OKAY(lhs.int64) && VM_VALUE_SHORT_OKAY(rhs.int64))
      {
        ret.ty = VM_VALUE_TYPE_INT64;
        ret.int64 = lhs.int64 * rhs.int64;
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init_set_si(ret.bigint, lhs.int64 * rhs.int64);
      }
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init_set_si(ret.bigint, lhs.int64);
      mpz_mul(ret.bigint, ret.bigint, rhs.bigint);
    }
  }
  else
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init(ret.bigint);
      mpz_mul_si(ret.bigint, lhs.bigint, rhs.int64);
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init(ret.bigint);
      mpz_mul(ret.bigint, lhs.bigint, rhs.bigint);
    }
  }
  return ret;
}

static vm_value_t vm_value_div(vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (lhs.ty == VM_VALUE_TYPE_INT64)
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (VM_VALUE_SHORT_OKAY(lhs.int64) && VM_VALUE_SHORT_OKAY(rhs.int64))
      {
        ret.ty = VM_VALUE_TYPE_INT64;
        ret.int64 = lhs.int64 / rhs.int64;
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init_set_si(ret.bigint, lhs.int64 / rhs.int64);
      }
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init_set_si(ret.bigint, lhs.int64);
      mpz_tdiv_q(ret.bigint, ret.bigint, rhs.bigint);
    }
  }
  else
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (rhs.int64 < 0) {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_t rhsz;
        mpz_init_set_si(rhsz, rhs.int64);
        mpz_tdiv_q(ret.bigint, lhs.bigint, rhsz);
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_tdiv_q_ui(ret.bigint, lhs.bigint, (uint64_t) rhs.int64);
      }
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init(ret.bigint);
      mpz_tdiv_q(ret.bigint, lhs.bigint, rhs.bigint);
    }
  }
  return ret;
}

static vm_value_t vm_value_mod(vm_value_t lhs, vm_value_t rhs)
{
  vm_value_t ret;
  if (lhs.ty == VM_VALUE_TYPE_INT64)
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (VM_VALUE_SHORT_OKAY(lhs.int64) && VM_VALUE_SHORT_OKAY(rhs.int64))
      {
        ret.ty = VM_VALUE_TYPE_INT64;
        ret.int64 = lhs.int64 % rhs.int64;
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init_set_si(ret.bigint, lhs.int64 % rhs.int64);
      }
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init_set_si(ret.bigint, lhs.int64);
      mpz_mod(ret.bigint, ret.bigint, rhs.bigint);
    }
  }
  else
  {
    if (rhs.ty == VM_VALUE_TYPE_INT64)
    {
      if (rhs.int64 < 0) {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_t rhsz;
        mpz_init_set_si(rhsz, rhs.int64);
        mpz_mod(ret.bigint, lhs.bigint, rhsz);
      }
      else
      {
        ret.ty = VM_VALUE_TYPE_BIGINT;
        mpz_init(ret.bigint);
        mpz_mod_ui(ret.bigint, lhs.bigint, (uint64_t) rhs.int64);
      }
    }
    else
    {
      ret.ty = VM_VALUE_TYPE_BIGINT;
      mpz_init(ret.bigint);
      mpz_mod(ret.bigint, lhs.bigint, rhs.bigint);
    }
  }
  return ret;
}

static bool vm_value_is_equal(vm_value_t lhs, vm_value_t rhs)
{
  if (lhs.ty == VM_VALUE_TYPE_INT64) {
    if (rhs.ty == VM_VALUE_TYPE_INT64) {
      return lhs.int64 == rhs.int64;
    } else {
      return mpz_cmp_si(rhs.bigint, lhs.int64) == 0;
    }
  } else {
    if (rhs.ty == VM_VALUE_TYPE_INT64) {
      return mpz_cmp_si(lhs.bigint, rhs.int64) == 0;
    } else {
      return mpz_cmp(lhs.bigint, rhs.bigint) == 0;
    }
  }
}

static bool vm_value_is_less(vm_value_t lhs, vm_value_t rhs)
{
  if (lhs.ty == VM_VALUE_TYPE_INT64) {
    if (rhs.ty == VM_VALUE_TYPE_INT64) {
      return lhs.int64 < rhs.int64;
    } else {
      return mpz_cmp_si(rhs.bigint, lhs.int64) > 0;
    }
  } else {
    if (rhs.ty == VM_VALUE_TYPE_INT64) {
      return mpz_cmp_si(lhs.bigint, rhs.int64) < 0;
    } else {
      return mpz_cmp(lhs.bigint, rhs.bigint) < 0;
    }
  }
}

static inline vm_loc_t vm_value_to_func(vm_value_t x)
{
  return x.func;
}

#endif