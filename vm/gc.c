#include <vm/gc.h>
#include <vm/obj.h>

#define VM_GC_MEM_GROW (4)

#define VM_GC_DELETE (0)
#define VM_GC_KEEP (1)

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
    int e1 = vm_gc_mark_entry_yes(gc, gc->len1, gc->objs1, ptr);
    if (e1 != 2)
    {
        return e1;
    }
    int e2 = vm_gc_mark_entry_yes(gc, gc->len2, gc->objs2, ptr);
    if (e2 != 2)
    {
        return e2;
    }
    int e3 = vm_gc_mark_entry_yes(gc, gc->len3, gc->objs3, ptr);
    if (e3 != 2)
    {
        return e3;
    }
    return 2;
}
void vm_print(vm_gc_t *gc, vm_obj_t obj);

void vm_gc_mark_ptr_yes(vm_gc_t *gc, uint64_t ptr)
{
    int res = vm_gc_mark_val_yes(gc, ptr);
    if (res == 2)
    {
        exit(1);
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


void vm_gc_run(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *end)
{
    if (gc->len1 < gc->big)
    {
        return;
    }
    for (vm_obj_t *cur = base; cur < end; cur++)
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
        if (place2 >= gc->len2)
        {
            while (place3 < gc->len3)
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
        if (place3 >= gc->len3)
        {
            while (place2 < gc->len2)
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
    gc->len3 = swaplen;
    gc->objs3 = gc->swap;
    gc->swap = newswap;
    gc->len2 = gc->len1;
    void *new1 = gc->objs2;
    gc->objs2 = gc->objs1;
    gc->len1 = 0;
    gc->objs1 = new1;
    gc->big = swaplen * VM_GC_MEM_GROW;
    for (int i = 0; i < gc->len2; i++)
    {
        gc->objs2[i].tag = VM_GC_DELETE;
    }
    for (int i = 0; i < gc->len3; i++)
    {
        gc->objs3[i].tag = VM_GC_DELETE;
    }
}

vm_gc_t *vm_gc_start(void)
{
    vm_gc_t *ret = vm_mem_alloc(sizeof(vm_gc_t));
    ret->last = 1;
    ret->len1 = 0;
    ret->len2 = 0;
    ret->len3 = 0;
    ret->objs1 = vm_mem_alloc(sizeof(vm_gc_entry_t) * (1 << 29));
    ret->objs2 = vm_mem_alloc(sizeof(vm_gc_entry_t) * (1 << 29));
    ret->objs3 = vm_mem_alloc(sizeof(vm_gc_entry_t) * (1 << 29));
    ret->swap = vm_mem_alloc(sizeof(vm_gc_entry_t) * (1 << 29));
    ret->big = 0;
    return ret;
}

void vm_gc_stop(vm_gc_t *gc)
{
    vm_mem_free(gc->objs1);
    vm_mem_free(gc->objs2);
    vm_mem_free(gc->objs3);
    vm_mem_free(gc->swap);
    vm_mem_free(gc);
}

vm_obj_t vm_gc_new(vm_gc_t *gc, int size, vm_obj_t *values)
{
    uint64_t where = gc->last;
    gc->objs1[gc->len1++] = (vm_gc_entry_t){
        .ptr = gc->last++,
        .tag = VM_GC_DELETE,
        .type = VM_GC_ENTRY_TYPE_PTR,
        .len = size,
    };
    for (int i = 0; i < size; i++)
    {
        gc->objs1[gc->len1++] = (vm_gc_entry_t){
            .ptr = gc->last++,
            .tag = VM_GC_DELETE,
            .type = VM_GC_ENTRY_TYPE_OBJ,
            .obj = values[i],
        };
    }
    return vm_obj_of_ptr(where);
}

vm_gc_entry_t vm_gc_find_in(size_t elems_len, vm_gc_entry_t *elems, uint64_t ptr)
{
    if (elems_len > 1)
    {
        size_t mid = elems_len >> 1;
        vm_gc_entry_t mid_entry = elems[mid];
        if (mid_entry.ptr > ptr)
        {
            return vm_gc_find_in(mid, elems, ptr);
        }
        if (mid_entry.ptr < ptr)
        {
            return vm_gc_find_in(elems_len - mid, elems + mid, ptr);
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

vm_gc_entry_t vm_gc_get(vm_gc_t *gc, uint64_t ptr)
{
    uint32_t len;
    size_t nelems = gc->nelems;
    vm_gc_entry_t e1 = vm_gc_find_in(gc->len1, gc->objs1, ptr);
    if (e1.ptr != 0)
    {
        return e1;
    }
    vm_gc_entry_t e2 = vm_gc_find_in(gc->len2, gc->objs2, ptr);
    if (e2.ptr != 0)
    {
        return e2;
    }
    return vm_gc_find_in(gc->len3, gc->objs3, ptr);
}

int vm_gc_sizeof(vm_gc_t *gc, uint64_t ptr)
{
    return vm_gc_get(gc, ptr).len;
}