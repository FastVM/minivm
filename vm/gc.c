#include <vm/gc.h>
#include <vm/nanbox.h>
#include <time.h>

#define VM_GC_MEM_GROW (2)

enum gc_mark_t;
typedef enum gc_mark_t gc_mark_t;

enum gc_mark_t
{
    GC_MARK_DELETE,
    GC_MARK_KEEP,
};

void vm_gc_mark_stack(vm_gc_t *gc, nanbox_t *base, nanbox_t *useful, nanbox_t *end)
{
    nanbox_t *ptr = base;
    while (ptr < useful)
    {
        if (nanbox_is_pointer(*ptr))
        {
            int *sub = nanbox_to_pointer(*ptr);
            if (*sub == GC_MARK_DELETE)
            {
                *sub = GC_MARK_KEEP;
                vm_gc_mark(gc, *(sub + 1), (nanbox_t *)(sub + 2));
            }
        }
        ptr++;
    }
    while (!nanbox_is_empty(*ptr) && ptr < end)
    {
        *ptr = nanbox_empty();
        ptr++;
    }
}

int vm_gc_count = 0;
double vm_gc_time = 0;
double vm_gc_max_pause = 0;

void vm_gc_run(vm_gc_t *gc, nanbox_t *base, nanbox_t *useful, nanbox_t *end)
{
    struct timespec tstart;
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    vm_gc_mark_stack(gc, base, useful, end);
    vm_gc_sweep(gc);
    struct timespec tend;
    clock_gettime(CLOCK_MONOTONIC, &tend);
    double pause = ((double)tend.tv_sec + 1.0e-9 * tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9 * tstart.tv_nsec);
    vm_gc_count += 1;
    vm_gc_time += pause;
    if (pause > vm_gc_max_pause)
    {
        vm_gc_max_pause = pause;
    }
}

vm_gc_t *vm_gc_start(void)
{
    vm_gc_t *ret = vm_mem_alloc(sizeof(vm_gc_t));
    ret->maxlen = 256;
    ret->alloc = 256;
    ret->length = 0;
    ret->ptrs = vm_mem_alloc(ret->alloc * sizeof(int *));
    return ret;
}

void vm_gc_stop(vm_gc_t *gc)
{
    vm_gc_sweep(gc);
    vm_mem_free(gc->ptrs);
    vm_mem_free(gc);
    printf("gc stats: runs: %i\n", vm_gc_count);
    printf("gc stats: total: %.3fs\n", vm_gc_time);
    if (vm_gc_count != 0)
    {
        printf("gc stats: per run: %.3fms\n", (vm_gc_time / (double)vm_gc_count * 1000));
        printf("gc stats: pause max: %.3fms\n", vm_gc_max_pause * 1000);
    }
}

nanbox_t vm_gc_new(vm_gc_t *gc, int size)
{
    int *ptr = vm_mem_alloc(sizeof(nanbox_t) * size + sizeof(int) * 2);
    gc->ptrs[gc->length++] = ptr;
    *ptr = GC_MARK_DELETE;
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

void vm_gc_mark(vm_gc_t *gc, int len, nanbox_t *ptrs)
{
    for (int i = 0; i < len; i++)
    {
        if (nanbox_is_pointer(ptrs[i]))
        {
            int *sub = nanbox_to_pointer(ptrs[i]);
            if (*sub != GC_MARK_KEEP)
            {
                *sub = GC_MARK_KEEP;
                vm_gc_mark(gc, *(sub + 1), (nanbox_t *)(sub + 2));
            }
        }
    }
}

void vm_gc_sweep(vm_gc_t *gc)
{
    int out = 0;
    int max = gc->length;
    for (int i = 0; i < max; i++)
    {
        int *ptr = gc->ptrs[i];
        if (*ptr == GC_MARK_DELETE)
        {
            vm_mem_free(ptr);
        }
        else
        {
            *ptr = GC_MARK_DELETE;
            gc->ptrs[out++] = gc->ptrs[i];
        }
    }
    gc->length = out;
    gc->maxlen = 256 + out * VM_GC_MEM_GROW;
    if (gc->maxlen >= gc->alloc)
    {
        gc->alloc = gc->maxlen * 2;
        gc->ptrs = vm_mem_realloc(gc->ptrs, gc->alloc * sizeof(int *));
    }
}
