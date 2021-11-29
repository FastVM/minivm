#pragma once

#include "obj.h"
#include "gc.h"

static inline vm_obj_t vm_obj_num_add(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) + vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_addc(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) + rhs);
}

static inline vm_obj_t vm_obj_num_sub(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) - vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_subc(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) - rhs);
}

static inline vm_obj_t vm_obj_num_mul(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) * vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_mulc(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) * rhs);
}

static inline vm_obj_t vm_obj_num_div(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) / vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_divc(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) / rhs);
}

static inline vm_obj_t vm_obj_num_mod(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(fmod(vm_obj_to_num(lhs), vm_obj_to_num(rhs)));
}

static inline vm_obj_t vm_obj_num_modc(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_of_num(fmod(vm_obj_to_num(lhs), rhs));
}

static inline bool vm_obj_lt(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_to_num(lhs) < vm_obj_to_num(rhs);
}

static inline bool vm_obj_ilt(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_to_num(lhs) < rhs;
}

static inline bool vm_obj_gt(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_to_num(lhs) > vm_obj_to_num(rhs);
}

static inline bool vm_obj_igt(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_to_num(lhs) > rhs;
}

static inline bool vm_obj_lte(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_to_num(lhs) <= vm_obj_to_num(rhs);
}

static inline bool vm_obj_ilte(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_to_num(lhs) <= rhs;
}

static inline bool vm_obj_gte(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_to_num(lhs) >= vm_obj_to_num(rhs);
}

static inline bool vm_obj_igte(vm_obj_t lhs, vm_int_t rhs)
{
    return vm_obj_to_num(lhs) >= rhs;
}

static inline bool vm_obj_eq(vm_obj_t lhs, vm_obj_t rhs)
{
    if (vm_obj_is_num(lhs))
    {
        if (vm_obj_is_num(rhs))
        {
            return vm_obj_to_num(lhs) == vm_obj_to_num(rhs);
        }
        else
        {
            return false;
        }
    }
    if (vm_obj_is_none(lhs))
    {
        return vm_obj_is_none(rhs);
    }
    if (vm_obj_is_bool(lhs))
    {
        if (vm_obj_is_bool(rhs))
        {
            return vm_obj_to_bool(lhs) == vm_obj_to_bool(rhs);
        }
        else
        {
            return false;
        }
    }
    if (!vm_obj_is_ptr(rhs) || !vm_obj_is_ptr(lhs))
    {
        return false;
    }
    vm_gc_entry_t *lent = vm_obj_to_ptr(lhs);
    vm_gc_entry_t *rent = vm_obj_to_ptr(rhs);
    size_t len = vm_gc_sizeof(lent);
    if (len != vm_gc_sizeof(rent))
    {
        return false;
    }
    for (size_t i = 0; i < len; i++)
    {
        vm_obj_t cl = vm_gc_get_index(lent, i);
        vm_obj_t cr = vm_gc_get_index(rent, i);
        if (!vm_obj_eq(cl, cr))
        {
            return false;
        }
    }
    return true;
}

static inline bool vm_obj_ieq(vm_obj_t lhs, vm_int_t rhs)
{
    if (vm_obj_is_num(lhs))
    {
        return vm_obj_to_num(lhs) == rhs;
    }
    return false;
}

static inline bool vm_obj_neq(vm_obj_t lhs, vm_obj_t rhs)
{
    return !vm_obj_eq(lhs, rhs);
}

static inline bool vm_obj_ineq(vm_obj_t lhs, vm_int_t rhs)
{
    return !vm_obj_ieq(lhs, rhs);
}
