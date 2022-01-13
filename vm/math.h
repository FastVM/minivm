#pragma once

#include "gc.h"

static inline vm_obj_t vm_obj_num_add(vm_obj_t lhs, vm_obj_t rhs) {
  return (vm_obj_t){.num = lhs.num + rhs.num};
}

static inline vm_obj_t vm_obj_num_addc(vm_obj_t lhs, vm_int_t rhs) {
  return (vm_obj_t){.num = lhs.num + rhs};
}

static inline vm_obj_t vm_obj_num_sub(vm_obj_t lhs, vm_obj_t rhs) {
  return (vm_obj_t){.num = lhs.num - rhs.num};
}

static inline vm_obj_t vm_obj_num_subc(vm_obj_t lhs, vm_int_t rhs) {
  return (vm_obj_t){.num = lhs.num - rhs};
}

static inline vm_obj_t vm_obj_num_mul(vm_obj_t lhs, vm_obj_t rhs) {
  return (vm_obj_t){.num = lhs.num * rhs.num};
}

static inline vm_obj_t vm_obj_num_mulc(vm_obj_t lhs, vm_int_t rhs) {
  return (vm_obj_t){.num = lhs.num * rhs};
}

static inline vm_obj_t vm_obj_num_div(vm_obj_t lhs, vm_obj_t rhs) {
  return (vm_obj_t){.num = lhs.num / rhs.num};
}

static inline vm_obj_t vm_obj_num_divc(vm_obj_t lhs, vm_int_t rhs) {
  return (vm_obj_t){.num = lhs.num / rhs};
}

static inline vm_obj_t vm_obj_num_mod(vm_obj_t lhs, vm_obj_t rhs) {
  return (vm_obj_t){.num = vm_mod(lhs.num, rhs.num)};
}

static inline vm_obj_t vm_obj_num_modc(vm_obj_t lhs, vm_int_t rhs) {
  return (vm_obj_t){.num = vm_mod(lhs.num, rhs)};
}

static inline vm_obj_t vm_obj_num_pow(vm_obj_t lhs, vm_obj_t rhs) {
  return (vm_obj_t){.num = vm_pow(lhs.num, rhs.num)};
}

static inline vm_obj_t vm_obj_num_powc(vm_obj_t lhs, vm_int_t rhs) {
  return (vm_obj_t){.num = vm_pow(lhs.num, rhs)};
}

static inline bool vm_obj_lt(vm_obj_t lhs, vm_obj_t rhs) {
  return lhs.num < rhs.num;
}

static inline bool vm_obj_ilt(vm_obj_t lhs, vm_int_t rhs) {
  return lhs.num < rhs;
}

static inline bool vm_obj_gt(vm_obj_t lhs, vm_obj_t rhs) {
  return lhs.num > rhs.num;
}

static inline bool vm_obj_igt(vm_obj_t lhs, vm_int_t rhs) {
  return lhs.num > rhs;
}

static inline bool vm_obj_lte(vm_obj_t lhs, vm_obj_t rhs) {
  return lhs.num <= rhs.num;
}

static inline bool vm_obj_ilte(vm_obj_t lhs, vm_int_t rhs) {
  return lhs.num <= rhs;
}

static inline bool vm_obj_gte(vm_obj_t lhs, vm_obj_t rhs) {
  return lhs.num >= rhs.num;
}

static inline bool vm_obj_igte(vm_obj_t lhs, vm_int_t rhs) {
  return lhs.num >= rhs;
}

static inline bool vm_obj_eq(vm_obj_t lhs, vm_obj_t rhs) {
  return lhs.num == rhs.num;
}

static inline bool vm_obj_ieq(vm_obj_t lhs, vm_int_t rhs) {
  return lhs.num == rhs;
}

static inline bool vm_obj_neq(vm_obj_t lhs, vm_obj_t rhs) {
  return !vm_obj_eq(lhs, rhs);
}

static inline bool vm_obj_ineq(vm_obj_t lhs, vm_int_t rhs) {
  return !vm_obj_ieq(lhs, rhs);
}
