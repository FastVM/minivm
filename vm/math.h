#pragma once

#include "obj.h"
#include "gc.h"

static inline vm_obj_t vm_obj_num_add(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) + vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_addc(vm_obj_t lhs, int rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) + rhs);
}

static inline vm_obj_t vm_obj_num_sub(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) - vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_subc(vm_obj_t lhs, int rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) - rhs);
}

static inline vm_obj_t vm_obj_num_mul(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) * vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_mulc(vm_obj_t lhs, int rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) * rhs);
}

static inline vm_obj_t vm_obj_num_div(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) / vm_obj_to_num(rhs));
}

static inline vm_obj_t vm_obj_num_divc(vm_obj_t lhs, int rhs)
{
    return vm_obj_of_num(vm_obj_to_num(lhs) / rhs);
}

static inline vm_obj_t vm_obj_num_mod(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_of_num(fmod(vm_obj_to_num(lhs), vm_obj_to_num(rhs)));
}

static inline vm_obj_t vm_obj_num_modc(vm_obj_t lhs, int rhs)
{
    return vm_obj_of_num(fmod(vm_obj_to_num(lhs), rhs));
}

static inline bool vm_obj_lt(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_to_num(lhs) < vm_obj_to_num(rhs);
}

static inline bool vm_obj_ilt(vm_obj_t lhs, int rhs)
{
    return vm_obj_to_num(lhs) < rhs;
}

static inline bool vm_obj_gt(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_to_num(lhs) > vm_obj_to_num(rhs);
}

static inline bool vm_obj_igt(vm_obj_t lhs, int rhs)
{
    return vm_obj_to_num(lhs) > rhs;
}

static inline bool vm_obj_lte(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_to_num(lhs) <= vm_obj_to_num(rhs);
}

static inline bool vm_obj_ilte(vm_obj_t lhs, int rhs)
{
    return vm_obj_to_num(lhs) <= rhs;
}

static inline bool vm_obj_gte(vm_obj_t lhs, vm_obj_t rhs)
{
    return vm_obj_to_num(lhs) >= vm_obj_to_num(rhs);
}

static inline bool vm_obj_igte(vm_obj_t lhs, int rhs)
{
    return vm_obj_to_num(lhs) >= rhs;
}

static inline bool vm_obj_eq(vm_obj_t lhs, vm_obj_t rhs);

static inline bool vm_obj_eq_mem(vm_obj_t lhs, vm_obj_t rhs)
{
    if (!vm_obj_is_ptr(rhs) || !vm_obj_is_ptr(lhs))
    {
        return false;
    }
    void *lent = vm_obj_to_ptr(lhs);
    void *rent = vm_obj_to_ptr(rhs);
    int type = vm_gc_type(lent);
    if (type != vm_gc_type(rent))
    {
        return false;
    }
    switch (type)
    {
    case VM_TYPE_ARRAY:
    {
        vm_gc_entry_array_t *ra = (vm_gc_entry_array_t *)lent;
        vm_gc_entry_array_t *la = (vm_gc_entry_array_t *)lent;
        size_t len = la->len;
        if (len != ra->len)
        {
            return false;
        }
        for (size_t i = 0; i < len; i++)
        {
            vm_obj_t io = vm_obj_of_num(i);
            vm_obj_t cl = vm_gc_get_index(lent, io);
            vm_obj_t cr = vm_gc_get_index(rent, io);
            if (vm_obj_to_int(cl) != vm_obj_to_int(cr))
            {
                return false;
            }
        }
        return true;
    }
    case VM_TYPE_STRING:
    {
        vm_gc_entry_string_t *ra = (vm_gc_entry_string_t *)rent;
        vm_gc_entry_string_t *la = (vm_gc_entry_string_t *)lent;
        size_t len = la->len;
        if (len != ra->len)
        {
            return false;
        }
        for (size_t i = 0; i < len; i++)
        {
            vm_obj_t io = vm_obj_of_num(i);
            vm_obj_t cl = vm_gc_get_index(lent, io);
            vm_obj_t cr = vm_gc_get_index(rent, io);
            if (vm_obj_to_int(cl) != vm_obj_to_int(cr))
            {
                return false;
            }
        }
        return true;
    }
    case VM_TYPE_BOX:
    {
        return vm_obj_eq(vm_gc_get_box(lent), vm_gc_get_box(rent));
    }
    }
    return false;
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
    if (vm_obj_is_fun(lhs))
    {
        if (vm_obj_is_num(rhs))
        {
            return vm_obj_to_fun(lhs) == vm_obj_to_fun(rhs);
        }
        else
        {
            return false;
        }
    }
    return vm_obj_eq_mem(lhs, rhs);
}

static inline bool vm_obj_ieq(vm_obj_t lhs, int rhs)
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

static inline bool vm_obj_ineq(vm_obj_t lhs, int rhs)
{
    return !vm_obj_ieq(lhs, rhs);
}
