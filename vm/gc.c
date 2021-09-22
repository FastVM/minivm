#include <vm/gc.h>
#include <vm/obj.h>

#define VM_GC_MEM_GROW (2)

enum gc_mark_t;
typedef enum gc_mark_t gc_mark_t;

enum gc_mark_t
{
    GC_MARK_0 = 0,
    GC_MARK_1 = 1,
};

void vm_gc_run(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *useful, vm_obj_t *end)
{
    if (gc->state == GC_MARK_0)
    {
        vm_gc_mark_stack_even(gc, base, useful, end);
        vm_gc_sweep_even(gc);
        gc->state = GC_MARK_1;
    }
    else
    {
        vm_gc_mark_stack_odd(gc, base, useful, end);
        vm_gc_sweep_odd(gc);
        gc->state = GC_MARK_0;
    }
}

vm_gc_t *vm_gc_start(void)
{
    vm_gc_t *ret = vm_mem_alloc(sizeof(vm_gc_t));
    ret->maxlen = 16;
    ret->alloc = 16;
    ret->length = 0;
    ret->ptrs = vm_mem_alloc(ret->alloc * sizeof(uint8_t *));
    ret->state = 0;
    return ret;
}

void vm_gc_stop(vm_gc_t *gc)
{
    int max = gc->length;
    for (int i = 0; i < max; i++)
    {
        uint8_t *ptr = gc->ptrs[i];
        vm_mem_free(ptr);
    }
    vm_mem_free(gc->ptrs);
    vm_mem_free(gc);
}

vm_obj_t vm_gc_new(vm_gc_t *gc, int size)
{
    uint8_t *ptr = vm_mem_alloc(sizeof(vm_obj_t) * size + sizeof(uint8_t) * 2);
    gc->ptrs[gc->length++] = ptr;
    *ptr = gc->state;
    *(ptr + 1) = size;
    return vm_obj_of_ptr(ptr);
}

int vm_gc_sizeof(vm_gc_t *gc, vm_obj_t ptr)
{
    uint8_t *head = vm_obj_to_ptr(ptr);
    return *(head + 1);
}

vm_obj_t vm_gc_get(vm_gc_t *gc, vm_obj_t ptr, int nth)
{
    uint8_t *raw = vm_obj_to_ptr(ptr);
    vm_obj_t *head = (vm_obj_t *)(raw + 2);
    return head[nth];
}

void vm_gc_set(vm_gc_t *gc, vm_obj_t ptr, int nth, vm_obj_t val)
{
    uint8_t *raw = vm_obj_to_ptr(ptr);
    vm_obj_t *head = (vm_obj_t *)(raw + 2);
    head[nth] = val;
}

// all these functions happen twice, this is so the marks can cycle

// run when state is an even number

void vm_gc_mark_stack_even(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *useful, vm_obj_t *end)
{
    for (vm_obj_t *ptr = base; ptr < useful; ptr++)
    {
        if (vm_obj_is_ptr(*ptr))
        {
            uint8_t *sub = vm_obj_to_ptr(*ptr);
            if (*sub == GC_MARK_0)
            {
                *sub = GC_MARK_1;
                vm_gc_mark_even(gc, *(sub + 1), (vm_obj_t *)(sub + 2));
            }
        }
    }
    for (vm_obj_t *ptr = useful; !vm_obj_is_dead(*ptr) && ptr < end; ptr++)
    {
        *ptr = vm_obj_of_dead();
    }
}

void vm_gc_mark_even(vm_gc_t *gc, int len, vm_obj_t *ptrs)
{
    while (true)
    {
        vm_obj_t *nptrs;
        int nlen;
        int i = 0;
        while (i < len)
        {
            if (vm_obj_is_ptr(ptrs[i]))
            {
                uint8_t *sub = vm_obj_to_ptr(ptrs[i]);
                if (*sub == GC_MARK_0)
                {
                    *sub = GC_MARK_1;
                    nlen = *(sub + 1);
                    nptrs = (vm_obj_t *)(sub + 2);
                    goto next;
                }
            }
            i += 1;
        }
        return;
    next:
        while (i < len)
        {
            if (vm_obj_is_ptr(ptrs[i]))
            {
                uint8_t *sub = vm_obj_to_ptr(ptrs[i]);
                if (*sub == GC_MARK_0)
                {
                    *sub = GC_MARK_1;
                    vm_gc_mark_even(gc, *(sub + 1), (vm_obj_t *)(sub + 2));
                }
            }
            i += 1;
        }
        ptrs = nptrs;
        len = nlen;
    }
}

void vm_gc_sweep_even(vm_gc_t *gc)
{
    int out = 0;
    int max = gc->length;
    for (int i = 0; i < max; i++)
    {
        uint8_t *ptr = gc->ptrs[i];
        if (*ptr == GC_MARK_0)
        {
            vm_mem_free(ptr);
        }
        else
        {
            gc->ptrs[out++] = ptr;
        }
    }
    gc->length = out;
    int newlen = 16 + out * VM_GC_MEM_GROW;
    if (newlen > gc->maxlen)
    {
        gc->maxlen = newlen;
        if (gc->maxlen >= gc->alloc)
        {
            gc->alloc = newlen * 2;
            gc->ptrs = vm_mem_realloc(gc->ptrs, gc->alloc * sizeof(uint8_t *));
        }
    }
}

// duplicates of functions

// run when state is an odd number

void vm_gc_mark_stack_odd(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *useful, vm_obj_t *end)
{
    for (vm_obj_t *ptr = base; ptr < useful; ptr++)
    {
        if (vm_obj_is_ptr(*ptr))
        {
            uint8_t *sub = vm_obj_to_ptr(*ptr);
            if (*sub == GC_MARK_1)
            {
                *sub = GC_MARK_0;
                vm_gc_mark_odd(gc, *(sub + 1), (vm_obj_t *)(sub + 2));
            }
        }
    }
    for (vm_obj_t *ptr = useful; !vm_obj_is_dead(*ptr) && ptr < end; ptr++)
    {
        *ptr = vm_obj_of_dead();
    }
}

void vm_gc_mark_odd(vm_gc_t *gc, int len, vm_obj_t *ptrs)
{
    while (true)
    {
        vm_obj_t *nptrs;
        int nlen;
        int i = 0;
        while (i < len)
        {
            if (vm_obj_is_ptr(ptrs[i]))
            {
                uint8_t *sub = vm_obj_to_ptr(ptrs[i]);
                if (*sub == GC_MARK_1)
                {
                    *sub = GC_MARK_0;
                    nlen = *(sub + 1);
                    nptrs = (vm_obj_t *)(sub + 2);
                    goto next;
                }
            }
            i += 1;
        }
        return;
    next:
        while (i < len)
        {
            if (vm_obj_is_ptr(ptrs[i]))
            {
                uint8_t *sub = vm_obj_to_ptr(ptrs[i]);
                if (*sub == GC_MARK_1)
                {
                    *sub = GC_MARK_0;
                    vm_gc_mark_odd(gc, *(sub + 1), (vm_obj_t *)(sub + 2));
                }
            }
            i += 1;
        }
        ptrs = nptrs;
        len = nlen;
    }
}

void vm_gc_sweep_odd(vm_gc_t *gc)
{
    int out = 0;
    int max = gc->length;
    int last = max;
    for (int i = 0; i < max; i++)
    {
        uint8_t *ptr = gc->ptrs[i];
        if (*ptr == GC_MARK_1)
        {
            vm_mem_free(ptr);
        }
        else
        {
            gc->ptrs[out++] = ptr;
        }
    }
    gc->length = out;
    int newlen = 16 + out * VM_GC_MEM_GROW;
    if (newlen > gc->maxlen)
    {
        gc->maxlen = newlen;
        if (gc->maxlen >= gc->alloc)
        {
            gc->alloc = newlen * 2;
            gc->ptrs = vm_mem_realloc(gc->ptrs, gc->alloc * sizeof(uint8_t *));
        }
    }
}
