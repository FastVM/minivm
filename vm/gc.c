#include <vm/gc.h>
#include <vm/nanbox.h>

#define VM_GC_MEM_GROW (3)

enum gc_mark_t;
typedef enum gc_mark_t gc_mark_t;

enum gc_mark_t
{
    GC_MARK_0 = 0,
    GC_MARK_1 = 1,
};

void vm_gc_run(vm_gc_t *gc, nanbox_t *base, nanbox_t *useful, nanbox_t *end)
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
    ret->ptrs = vm_mem_alloc(ret->alloc * sizeof(int *));
    ret->state = 0;
    return ret;
}

void vm_gc_stop(vm_gc_t *gc)
{
    int max = gc->length;
    for (int i = 0; i < max; i++)
    {
        int *ptr = gc->ptrs[i];
        vm_mem_free(ptr);
    }
    vm_mem_free(gc->ptrs);
    vm_mem_free(gc);
}

nanbox_t vm_gc_new(vm_gc_t *gc, int size)
{
    int *ptr = vm_mem_alloc(sizeof(nanbox_t) * size + sizeof(int) * 2);
    gc->ptrs[gc->length++] = ptr;
    *ptr = gc->state;
    *(ptr + 1) = size;
    return nanbox_from_pointer(ptr);
}

int vm_gc_sizeof(vm_gc_t *gc, nanbox_t ptr)
{
    int *head = nanbox_to_pointer(ptr);
    return *(head + 1);
}

nanbox_t vm_gc_get(vm_gc_t *gc, nanbox_t ptr, int nth)
{
    int *raw = nanbox_to_pointer(ptr);
    nanbox_t *head = (nanbox_t *)(raw + 2);
    return head[nth];
}

void vm_gc_set(vm_gc_t *gc, nanbox_t ptr, int nth, nanbox_t val)
{
    int *raw = nanbox_to_pointer(ptr);
    nanbox_t *head = (nanbox_t *)(raw + 2);
    head[nth] = val;
}

// all these functions happen twice, this is so the marks can cycle

// run when state is an even number

void vm_gc_mark_stack_even(vm_gc_t *gc, nanbox_t *base, nanbox_t *useful, nanbox_t *end)
{
    for (nanbox_t *ptr = base; ptr < useful; ptr++)
    {
        if (nanbox_is_pointer(*ptr))
        {
            int *sub = nanbox_to_pointer(*ptr);
            if (*sub == GC_MARK_0)
            {
                *sub = GC_MARK_1;
                vm_gc_mark_even(gc, *(sub + 1), (nanbox_t *)(sub + 2));
            }
        }
    }
    for (nanbox_t *ptr = useful; !nanbox_is_empty(*ptr) && ptr < end; ptr++)
    {
        *ptr = nanbox_empty();
    }
}

void vm_gc_mark_even(vm_gc_t *gc, int len, nanbox_t *ptrs)
{
    while (true)
    {
        nanbox_t *nptrs;
        int nlen;
        int i = 0;
        while (i < len)
        {
            if (nanbox_is_pointer(ptrs[i]))
            {
                int *sub = nanbox_to_pointer(ptrs[i]);
                if (*sub == GC_MARK_0)
                {
                    *sub = GC_MARK_1;
                    nlen = *(sub + 1);
                    nptrs = (nanbox_t *)(sub + 2);
                    goto next;
                }
            }
            i += 1;
        }
        return;
    next:
        while (i < len)
        {
            if (nanbox_is_pointer(ptrs[i]))
            {
                int *sub = nanbox_to_pointer(ptrs[i]);
                if (*sub == GC_MARK_0)
                {
                    *sub = GC_MARK_1;
                    vm_gc_mark_even(gc, *(sub + 1), (nanbox_t *)(sub + 2));
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
        int *ptr = gc->ptrs[i];
        if (*ptr == GC_MARK_0)
        {
            vm_mem_free(ptr);
        }
        else
        {
            gc->ptrs[out++] = gc->ptrs[i];
        }
    }
    gc->length = out;
    int newlen = 16 + out * VM_GC_MEM_GROW;
    gc->maxlen = newlen;
    if (gc->maxlen >= gc->alloc)
    {
        gc->alloc = newlen * 2;
        gc->ptrs = vm_mem_realloc(gc->ptrs, gc->alloc * sizeof(int *));
    }
}

// duplicates of functions

// run when state is an odd number

void vm_gc_mark_stack_odd(vm_gc_t *gc, nanbox_t *base, nanbox_t *useful, nanbox_t *end)
{
    for (nanbox_t *ptr = base; ptr < useful; ptr++)
    {
        if (nanbox_is_pointer(*ptr))
        {
            int *sub = nanbox_to_pointer(*ptr);
            if (*sub == GC_MARK_1)
            {
                *sub = GC_MARK_0;
                vm_gc_mark_odd(gc, *(sub + 1), (nanbox_t *)(sub + 2));
            }
        }
    }
    for (nanbox_t *ptr = useful; !nanbox_is_empty(*ptr) && ptr < end; ptr++)
    {
        *ptr = nanbox_empty();
    }
}

void vm_gc_mark_odd(vm_gc_t *gc, int len, nanbox_t *ptrs)
{
    while (true)
    {
        nanbox_t *nptrs;
        int nlen;
        int i = 0;
        while (i < len)
        {
            if (nanbox_is_pointer(ptrs[i]))
            {
                int *sub = nanbox_to_pointer(ptrs[i]);
                if (*sub == GC_MARK_1)
                {
                    *sub = GC_MARK_0;
                    nlen = *(sub + 1);
                    nptrs = (nanbox_t *)(sub + 2);
                    goto next;
                }
            }
            i += 1;
        }
        return;
    next:
        while (i < len)
        {
            if (nanbox_is_pointer(ptrs[i]))
            {
                int *sub = nanbox_to_pointer(ptrs[i]);
                if (*sub == GC_MARK_1)
                {
                    *sub = GC_MARK_0;
                    vm_gc_mark_odd(gc, *(sub + 1), (nanbox_t *)(sub + 2));
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
    for (int i = 0; i < max; i++)
    {
        int *ptr = gc->ptrs[i];
        if (*ptr == GC_MARK_1)
        {
            vm_mem_free(ptr);
        }
        else
        {
            gc->ptrs[out++] = gc->ptrs[i];
        }
    }
    gc->length = out;
    int newlen = 16 + out * VM_GC_MEM_GROW;
    gc->maxlen = newlen;
    if (gc->maxlen >= gc->alloc)
    {
        gc->alloc = newlen * 2;
        gc->ptrs = vm_mem_realloc(gc->ptrs, gc->alloc * sizeof(int *));
    }
}
