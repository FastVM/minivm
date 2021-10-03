#include <vm/gc.h>
#include <vm/obj.h>

#define VM_MEM_MAX ((VM_FRAME_BYTES) + (VM_LOCALS_BYTES) + (VM_MEM_BYTES)*4)

#define VM_GC_MEM_GROW (1)

#define VM_GC_DELETE (0)
#define VM_GC_KEEP (1)

size_t vm_stats_memsize = VM_MEM_MAX;

size_t vm_mem_top = 0;
uint8_t vm_mem[VM_MEM_MAX];

void *vm_mem_grow(size_t size)
{
    void *ret = &vm_mem[vm_mem_top];
    vm_mem_top += size;
    return ret;
}

void vm_mem_reset(void)
{
    vm_mem_top = 0;
}

size_t *vm_objs_len(vm_gc_entry_t *objs)
{
    return &objs[-1].xlen;
}

vm_gc_entry_t vm_gc_find_in(size_t elems_len, vm_gc_entry_t *elems, uint64_t ptr)
{
    while (true)
    {
        size_t mid = elems_len >> 1;
        vm_gc_entry_t mid_entry = elems[mid];
        if (mid_entry.ptr > ptr)
        {
            if (mid == 0)
            {
                return (vm_gc_entry_t){
                    .ptr = 0,
                };
            }
            elems_len = mid;
            continue;
        }
        if (mid_entry.ptr < ptr)
        {
            if (mid == 0)
            {
                return (vm_gc_entry_t){
                    .ptr = 0,
                };
            }
            elems_len -= mid;
            elems += mid;
            continue;
        }
        return mid_entry;
    }
}

vm_gc_entry_t vm_gc_get(vm_gc_t *gc, uint64_t ptr)
{
    vm_gc_entry_t e1 = vm_gc_find_in(*vm_objs_len(gc->objs1), gc->objs1, ptr);
    if (e1.ptr != 0)
    {
        return e1;
    }
    vm_gc_entry_t e2 = vm_gc_find_in(*vm_objs_len(gc->objs2), gc->objs2, ptr);
    if (e2.ptr != 0)
    {
        return e2;
    }
    return vm_gc_find_in(*vm_objs_len(gc->objs3), gc->objs3, ptr);
}

int vm_gc_mark_entry_yes(vm_gc_t *gc, size_t elems_len, vm_gc_entry_t *elems, uint64_t ptr)
{
    if (elems_len > 1)
    {
        size_t mid = elems_len >> 1;
        vm_gc_entry_t mid_entry = elems[mid];
        if (mid_entry.ptr > ptr)
        {
            return vm_gc_mark_entry_yes(gc, mid, elems, ptr);
        }
        if (mid_entry.ptr < ptr)
        {
            return vm_gc_mark_entry_yes(gc, elems_len - mid, elems + mid, ptr);
        }
        int ret = elems[mid].tag;
        elems[mid].tag = VM_GC_KEEP;
        return ret;
    }
    else
    {
        if (elems[0].ptr == ptr)
        {
            int ret = elems[0].tag;
            elems[0].tag = VM_GC_KEEP;
            return ret;
        }
        return 2;
    }
}

int vm_gc_mark_val_yes(vm_gc_t *gc, uint64_t ptr)
{
    int e1 = vm_gc_mark_entry_yes(gc, *vm_objs_len(gc->objs1), gc->objs1, ptr);
    if (e1 != 2)
    {
        return e1;
    }
    int e2 = vm_gc_mark_entry_yes(gc, *vm_objs_len(gc->objs2), gc->objs2, ptr);
    if (e2 != 2)
    {
        return e2;
    }
    int e3 = vm_gc_mark_entry_yes(gc, *vm_objs_len(gc->objs3), gc->objs3, ptr);
    if (e3 != 2)
    {
        return e3;
    }
    return 2;
}

void vm_gc_mark_ptr_yes(vm_gc_t *gc, uint64_t ptr)
{
    int res = vm_gc_mark_val_yes(gc, ptr);
    if (res == 2)
    {
        return;
    }
more:;
    vm_gc_entry_t ent = vm_gc_get(gc, ptr);
    if (vm_obj_is_ptr(ent.obj))
    {
        vm_gc_mark_ptr_yes(gc, vm_obj_to_ptr(ent.obj));
    }
    size_t len = ent.len;

    for (int i = 1; i <= len; i++)
    {
        vm_gc_entry_t ent = vm_gc_get(gc, ptr + i);
        vm_gc_mark_val_yes(gc, ptr + i);
        if (vm_obj_is_ptr(ent.obj))
        {
            vm_gc_mark_ptr_yes(gc, vm_obj_to_ptr(ent.obj));
        }
    }
}

void vm_gc_run1(vm_gc_t *gc)
{
    for (vm_obj_t *cur = gc->base; cur < gc->end; cur++)
    {
        if (vm_obj_is_ptr(*cur))
        {
            vm_gc_mark_ptr_yes(gc, vm_obj_to_ptr(*cur));
        }
    }
    size_t swaplen = 0;
    size_t place2 = 0;
    size_t place3 = 0;
    size_t len2 = *vm_objs_len(gc->objs2);
    size_t len3 = *vm_objs_len(gc->objs3);
    while (true)
    {
        if (place2 >= len2)
        {
            for (size_t cur = place3; cur < len3; cur++)
            {
                if (gc->objs3[cur].tag != VM_GC_DELETE)
                {
                    gc->swap[swaplen++] = gc->objs3[cur];
                }
            }
            break;
        }
        if (place3 >= len3)
        {
            for (size_t cur = place2; cur < len2; cur++)
            {
                if (gc->objs2[cur].tag != VM_GC_DELETE)
                {
                    gc->swap[swaplen++] = gc->objs2[cur];
                }
            }
            break;
        }
        if (gc->objs2[place2].ptr < gc->objs3[place3].ptr)
        {
            if (gc->objs2[place2].tag != VM_GC_DELETE)
            {
                gc->swap[swaplen] = gc->objs2[place2];
                swaplen++;
                place2++;
            }
            else
            {
                place2++;
            }
        }
        else
        {
            if (gc->objs3[place3].tag != VM_GC_DELETE)
            {
                gc->swap[swaplen] = gc->objs3[place3];
                swaplen++;
                place3++;
            }
            else
            {
                place3++;
            }
        }
    }
    void *newswap = gc->objs3;
    gc->objs3 = gc->swap;
    *vm_objs_len(gc->objs3) = swaplen;
    gc->swap = newswap;
    void *new1 = gc->objs2;
    gc->objs2 = gc->objs1;
    *vm_objs_len(gc->objs2) = *vm_objs_len(gc->objs1);
    *vm_objs_len(new1) = 0;
    gc->objs1 = new1;
    for (int i = 0; i < *vm_objs_len(gc->objs2); i++)
    {
        gc->objs2[i].tag = VM_GC_DELETE;
    }
    for (int i = 0; i < *vm_objs_len(gc->objs3); i++)
    {
        gc->objs3[i].tag = VM_GC_DELETE;
    }
    gc->max1 = *vm_objs_len(gc->objs3) * VM_GC_MEM_GROW;
}

#if defined(VM_GC_THREADS)
void *vm_gc_run_thread(void *gc_arg)
{
    vm_gc_t *gc = gc_arg;
    while (!gc->die)
    {
        pthread_mutex_lock(&gc->lock);
        while (gc->not_collecting)
        {
            pthread_cond_wait(&gc->cond, &gc->lock);
        }
        vm_gc_run1(gc);
        gc->not_collecting = true;
        pthread_mutex_unlock(&gc->lock);
    }
    return NULL;
}
#endif

void vm_gc_start(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *end)
{
    gc->base = base;
    gc->end = end;
    gc->last = 1;
    gc->objs1 = vm_mem_grow(VM_MEM_BYTES);
    gc->objs2 = vm_mem_grow(VM_MEM_BYTES);
    gc->objs3 = vm_mem_grow(VM_MEM_BYTES);
    gc->swap = vm_mem_grow(VM_MEM_BYTES);
    gc->objs1 += 1;
    gc->objs2 += 1;
    gc->objs3 += 1;
    gc->swap += 1;
    gc->objs1[-1].xlen = 0;
    gc->objs2[-1].xlen = 0;
    gc->objs3[-1].xlen = 0;
    gc->max1 = 0;
#if defined(VM_GC_THREADS)
    gc->die = false;
    gc->not_collecting = false;
    pthread_cond_init(&gc->cond, NULL);
    pthread_mutex_init(&gc->lock, NULL);
    pthread_create(&gc->thread, NULL, &vm_gc_run_thread, gc);
#endif
}

void vm_gc_stop(vm_gc_t *gc)
{
#if defined(VM_GC_THREADS)
    gc->die = true;
    pthread_join(gc->thread, NULL);
    pthread_mutex_destroy(&gc->lock);
    pthread_cond_destroy(&gc->cond);
#endif
}

vm_obj_t vm_gc_new(vm_gc_t *gc, size_t size, vm_obj_t *values)
{
    if (*vm_objs_len(gc->objs1) >= gc->max1)
    {
#if defined(VM_GC_THREADS)
        if (gc->not_collecting)
        {
            gc->not_collecting = false;
            pthread_mutex_lock(&gc->lock);
            pthread_cond_broadcast(&gc->cond);
            pthread_mutex_unlock(&gc->lock);
        }
#else
        vm_gc_run1(gc);
#endif
    }
    if (size >= (1 << 15))
    {
        __builtin_trap();
    }
    else if (size > 0)
    {
        uint64_t where = gc->last;
        for (int i = size; i > 0; i--)
        {
            gc->objs1[(*vm_objs_len(gc->objs1))++] = (vm_gc_entry_t){
                .tag = VM_GC_DELETE,
                .len = i,
                .ptr = gc->last++,
                .obj = values[size - i],
            };
        }
        return vm_obj_of_ptr(where);
    }
    else
    {
        return vm_obj_of_ptr((uint64_t) 0);
    }
}

vm_obj_t vm_gc_index(vm_gc_t *gc, uint64_t ptr, size_t index)
{
    return vm_gc_get(gc, ptr + index).obj;
}

size_t vm_gc_sizeof(vm_gc_t *gc, uint64_t ptr)
{
    if (ptr == 0)
    {
        return 0;
    }
    else
    {
        return vm_gc_get(gc, ptr).len;
    }
}