#pragma once

#include "gc.h"
#include "obj.h"

static inline vm_obj_t vm_obj_num_add(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_of_num(vm_obj_to_num(lhs) + vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_addc(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_of_num(vm_obj_to_num(lhs) + rhs);
}

static inline vm_obj_t vm_obj_num_sub(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_of_num(vm_obj_to_num(lhs) - vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_subc(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_of_num(vm_obj_to_num(lhs) - rhs);
}

static inline vm_obj_t vm_obj_num_mul(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_of_num(vm_obj_to_num(lhs) * vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_mulc(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_of_num(vm_obj_to_num(lhs) * rhs);
}

static inline vm_obj_t vm_obj_num_div(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_of_num(vm_obj_to_num(lhs) / vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_divc(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_of_num(vm_obj_to_num(lhs) / rhs);
}

static inline vm_obj_t vm_obj_num_mod(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_of_num(fmod(vm_obj_to_num(lhs), vm_obj_to_num(rhs)));
}

static inline vm_obj_t vm_obj_num_modc(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_of_num(fmod(vm_obj_to_num(lhs), rhs));
}


static inline vm_obj_t vm_obj_num_pow(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_of_num(pow(vm_obj_to_num(lhs), vm_obj_to_num(rhs)));
}

static inline vm_obj_t vm_obj_num_powc(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_of_num(pow(vm_obj_to_num(lhs), rhs));
}

static inline bool vm_obj_lt(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_to_num(lhs) < vm_obj_to_num(rhs);
}

static inline bool vm_obj_ilt(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_to_num(lhs) < rhs;
}

static inline bool vm_obj_gt(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_to_num(lhs) > vm_obj_to_num(rhs);
}

static inline bool vm_obj_igt(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_to_num(lhs) > rhs;
}

static inline bool vm_obj_lte(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_to_num(lhs) <= vm_obj_to_num(rhs);
}

static inline bool vm_obj_ilte(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_to_num(lhs) <= rhs;
}

static inline bool vm_obj_gte(vm_obj_t lhs, vm_obj_t rhs) {
  return vm_obj_to_num(lhs) >= vm_obj_to_num(rhs);
}

static inline bool vm_obj_igte(vm_obj_t lhs, vm_int_t rhs) {
  return vm_obj_to_num(lhs) >= rhs;
}

int printf(const char *fmt, ...);

static inline bool vm_obj_eq(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs) {
  if (vm_obj_is_num(lhs)) {
    if (vm_obj_is_num(rhs)) {
      return vm_obj_to_num(lhs) == vm_obj_to_num(rhs);
    } else {
      // __builtin_trap();
      return false;
    }
  }
  if (vm_obj_is_none(lhs)) {
    return vm_obj_is_none(rhs);
  }
  if (vm_obj_is_bool(lhs)) {
    if (vm_obj_is_bool(rhs)) {
      return vm_obj_to_bool(lhs) == vm_obj_to_bool(rhs);
    } else {
      return false;
      // __builtin_trap();
    }
  }
  if (!vm_obj_is_ptr(rhs) || !vm_obj_is_ptr(lhs)) {
      // __builtin_trap();
    return false;
  }
  vm_gc_entry_t *lent = vm_obj_to_ptr(gc, lhs);
  vm_gc_entry_t *rent = vm_obj_to_ptr(gc, rhs);
  size_t len = vm_gc_sizeof(gc, lent);
  if (len != vm_gc_sizeof(gc, rent)) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    vm_obj_t cl = vm_gc_get_index(gc, lent, i);
    vm_obj_t cr = vm_gc_get_index(gc, rent, i);
    if (!vm_obj_eq(gc, cl, cr)) {
      return false;
    }
  }
  return true;
}

static inline bool vm_obj_ieq(vm_obj_t lhs, vm_int_t rhs) {
  if (vm_obj_is_num(lhs)) {
    return vm_obj_to_num(lhs) == rhs;
  }
  return false;
}

static inline bool vm_obj_neq(vm_gc_t *gc, vm_obj_t lhs, vm_obj_t rhs) {
  return !vm_obj_eq(gc, lhs, rhs);
}

static inline bool vm_obj_ineq(vm_obj_t lhs, vm_int_t rhs) {
  return !vm_obj_ieq(lhs, rhs);
}
