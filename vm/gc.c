#include <vm/gc.h>
#include <vm/obj.h>

#define VM_MEM_MAX ((VM_FRAME_BYTES) + (VM_LOCALS_BYTES) + (VM_MEM_BYTES)*4)

#define VM_GC_MEM_GROW (2)

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

uint32_t *vm_objs_len(vm_gc_entry_t *objs)
{
    return &objs[-1].len;
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
    size_t len = vm_gc_sizeof(gc, ptr);

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
    while (true)
    {
        if (place2 >= *vm_objs_len(gc->objs2))
        {
            while (place3 < *vm_objs_len(gc->objs3))
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
            break;
        }
        if (place3 >= *vm_objs_len(gc->objs3))
        {
            while (place2 < *vm_objs_len(gc->objs2))
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
}

void *vm_gc_run(void *gc_arg)
{
    vm_gc_t *gc = gc_arg;
    while (!gc->die)
    {
        vm_gc_run1(gc);
    }
    return NULL;
}

void vm_gc_start(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *end)
{
    gc->base = base;
    gc->end = end;
    gc->last = VM_OBJ_PTR_BASE;
    gc->objs1 = vm_mem_grow(VM_MEM_BYTES);
    gc->objs2 = vm_mem_grow(VM_MEM_BYTES);
    gc->objs3 = vm_mem_grow(VM_MEM_BYTES);
    gc->swap = vm_mem_grow(VM_MEM_BYTES);
    gc->objs1 += 1;
    gc->objs2 += 1;
    gc->objs3 += 1;
    gc->swap += 1;
    gc->die = false;
    gc->objs1[-1].len = 0;
    gc->objs2[-1].len = 0;
    gc->objs3[-1].len = 0;
    pthread_create(&gc->thread, NULL, &vm_gc_run, gc);
}

void vm_gc_stop(vm_gc_t *gc) {
    gc->die = true;
    pthread_join(gc->thread, NULL);
}

vm_obj_t vm_gc_new(vm_gc_t *gc, int size, vm_obj_t *values)
{
    uint64_t where = gc->last;
    gc->objs1[(*vm_objs_len(gc->objs1))++] = (vm_gc_entry_t){
        .ptr = gc->last++,
        .tag = VM_GC_DELETE,
        .len = size,
    };
    for (int i = 0; i < size; i++)
    {
        gc->objs1[(*vm_objs_len(gc->objs1))++] = (vm_gc_entry_t){
            .ptr = gc->last++,
            .tag = VM_GC_DELETE,
            .obj = values[i],
        };
    }
    return vm_obj_of_ptr(where);
}

vm_gc_entry_t vm_gc_find_in(size_t elems_len, vm_gc_entry_t *elems, uint64_t ptr)
{
    while (true) {
        if (elems_len > 1)
        {
            size_t mid = elems_len >> 1;
            vm_gc_entry_t mid_entry = elems[mid];
            if (mid_entry.ptr > ptr)
            {
                elems_len = mid;
                continue;
            }
            if (mid_entry.ptr < ptr)
            {
                elems_len -= mid;
                elems += mid;
                continue;
            }
            return mid_entry;
        }
        else
        {
            vm_gc_entry_t first_entry = elems[0];
            if (first_entry.ptr == ptr)
            {
                return first_entry;
            }
            return (vm_gc_entry_t){
                .ptr = 0,
            };
        }
    }
}

vm_gc_entry_t vm_gc_get(vm_gc_t *gc, uint64_t ptr)
{
    uint32_t len;
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

int vm_gc_sizeof(vm_gc_t *gc, uint64_t ptr)
{
    return vm_gc_get(gc, ptr).len;
}