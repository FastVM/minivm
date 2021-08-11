#include <vm/gc.h>
#include <vm/nanbox.h>
#include <vm/vector.h>
#include <pthread.h>
#include <unistd.h>

enum gc_mark_t;
typedef enum gc_mark_t gc_mark_t;

enum gc_mark_t
{
    GC_MARK_DELETE,
    GC_MARK_KEEP,
};
void vm_gc_stack_end_set(vm_gc_t *gc, nanbox_t *stackptr)
{
    gc->stackptr = stackptr;
}

void vm_gc_mark_stack(vm_gc_t *gc)
{
    for (nanbox_t *ptr = gc->baseptr; ptr < gc->stackptr; ptr++)
    {
        if (nanbox_is_pointer(*ptr))
        {
            int *sub = nanbox_to_pointer(*ptr);
            int mark = *(sub - 2);
            if (mark != GC_MARK_KEEP)
            {
                *(sub - 2) = GC_MARK_KEEP;
                vm_gc_mark(gc, *(sub - 1), (nanbox_t *)sub);
            }
        }
    }
}

void vm_gc_run(vm_gc_t *gc)
{
    vm_gc_mark_stack(gc);
    vm_gc_sweep(gc);
}

vm_gc_t *vm_gc_start(nanbox_t *baseptr)
{
    vm_gc_t *ret = vm_mem_alloc(sizeof(vm_gc_t));
    ret->ptrs = vec_new(int *);
    ret->baseptr = baseptr;
    ret->maxlen = 256;
    return ret;
}

void vm_gc_stop(vm_gc_t *gc)
{
    vm_gc_sweep(gc);
    vec_del(gc->ptrs);
    vm_mem_free(gc);
}

nanbox_t vm_gc_new(vm_gc_t *gc, int size)
{
    int *ptr = vm_mem_alloc(sizeof(nanbox_t) * size + sizeof(int) * 2);
    vec_push(gc->ptrs, ptr);
    *ptr = GC_MARK_DELETE;
    ptr += 1;
    *ptr = size;
    ptr += 1;
    return nanbox_from_pointer(ptr);
}

int vm_gc_sizeof(vm_gc_t *gc, nanbox_t ptr)
{
    int *head = nanbox_to_pointer(ptr);
    return *(head - 1);
}

nanbox_t vm_gc_get(vm_gc_t *gc, nanbox_t ptr, int nth)
{
    nanbox_t *head = nanbox_to_pointer(ptr);
    return head[nth];
}

void vm_gc_set(vm_gc_t *gc, nanbox_t ptr, int nth, nanbox_t val)
{
    nanbox_t *head = nanbox_to_pointer(ptr);
    head[nth] = val;
}

void vm_gc_mark(vm_gc_t *gc, int len, nanbox_t *ptrs)
{
    for (int i = 0; i < len; i++)
    {
        if (nanbox_is_pointer(ptrs[i]))
        {
            int *sub = nanbox_to_pointer(ptrs[i]);
            if (*(sub - 2) != GC_MARK_KEEP)
            {
                *(sub - 2) = GC_MARK_KEEP;
                vm_gc_mark(gc, *(sub - 1), (nanbox_t *)sub);
            }
        }
    }
}

void vm_gc_sweep(vm_gc_t *gc)
{
    int out = 0;
    int max = vec_size(gc->ptrs);
    for (int i = 0; i < max; i++)
    {
        int *ptr = *(int **)vec_get(gc->ptrs, i);
        if (*ptr == GC_MARK_DELETE)
        {
            vm_mem_free(ptr);
        }
        else
        {
            *ptr = GC_MARK_DELETE;
            *(int **)vec_get(gc->ptrs, out) = ptr;
            out += 1;
        }
    }
    vec_set_size(gc->ptrs, out);
}
