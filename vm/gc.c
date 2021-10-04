#include <vm/gc.h>
#include <vm/obj.h>

#define VM_MEM_MAX ((VM_FRAME_BYTES) + (VM_LOCALS_BYTES))

#define VM_GC_MEM_GROW (1)

#define VM_GC_DELETE (0)
#define VM_GC_KEEP (1)

size_t vm_stats_memsize = VM_MEM_MAX;

size_t vm_mem_top = 0;
uint8_t vm_mem[VM_MEM_MAX];
#if defined(VM_GC_NO_MALLOC)
vm_gc_entry_t vm_gc_objs1[VM_MEM_UNITS];
vm_gc_entry_t vm_gc_objs2[VM_MEM_UNITS];
vm_gc_entry_t vm_gc_objs3[VM_MEM_UNITS];
#endif

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

uint64_t vm_gc_hash(uint64_t ptr)
{
    ptr ^= ptr * 2654435761;
    ptr >>= 8;
    return ptr;
}

vm_gc_entry_t vm_gc_find_in(vm_gc_entry_t *elems, uint64_t ptr)
{
    uint64_t head = vm_gc_hash(ptr);
    do
    {
        vm_gc_entry_t ent = elems[head % VM_MEM_UNITS];
        if (ent.ptr == ptr)
        {
            return ent;
        }
        if (ent.ptr == 0 || ent.ptr == 1)
        {
            return ent;
        }
        head++;
    } while (true);
}

vm_gc_entry_t vm_gc_get(vm_gc_t *gc, uint64_t ptr)
{
    vm_gc_entry_t e1 = vm_gc_find_in(gc->objs1, ptr);
    if (e1.ptr != 0)
    {
        return e1;
    }
    vm_gc_entry_t e2 = vm_gc_find_in(gc->objs2, ptr);
    if (e2.ptr != 0)
    {
        return e2;
    }
    return vm_gc_find_in(gc->objs3, ptr);
}

int vm_gc_mark_entry_yes(vm_gc_t *gc, vm_gc_entry_t *elems, uint64_t ptr)
{
    uint64_t head = vm_gc_hash(ptr) % VM_MEM_UNITS;
    do
    {
        vm_gc_entry_t *ent = &elems[head];
        if (ent->ptr == ptr)
        {
            bool ret = ent->keep;
            ent->keep = true;
            return ret;
        }
        if (ent->ptr == 0)
        {
            return 2;
        }
        head++;
        if (head == VM_MEM_UNITS)
        {
            head = 0;
        }
    } while (true);
}

int vm_gc_mark_val_yes(vm_gc_t *gc, uint64_t ptr)
{
    int e1 = vm_gc_mark_entry_yes(gc, gc->objs1, ptr);
    if (e1 != 2)
    {
        return e1;
    }
    int e2 = vm_gc_mark_entry_yes(gc, gc->objs2, ptr);
    if (e2 != 2)
    {
        return e2;
    }
    int e3 = vm_gc_mark_entry_yes(gc, gc->objs3, ptr);
    if (e3 != 2)
    {
        return e3;
    }
    return 2;
}

void vm_gc_mark_ptr_yes(vm_gc_t *gc, uint64_t ptr)
{
    if (ptr == 0 || ptr == 1)
    {
        return;
    }
more:;
    vm_gc_entry_t ent = vm_gc_get(gc, ptr);
    if (ent.root)
    {
        return;
    }
    size_t len = ent.len;
    for (size_t i = 0; i < len; i++)
    {
        vm_gc_entry_t ent = vm_gc_get(gc, ptr + i);
        int res = vm_gc_mark_val_yes(gc, ptr + i);
        if (res != VM_GC_KEEP && vm_obj_is_ptr(ent.obj))
        {
            vm_gc_mark_ptr_yes(gc, vm_obj_to_ptr(ent.obj));
        }
    }
}

void vm_gc_move(vm_gc_entry_t *to, vm_gc_entry_t value)
{
    uint64_t ind = vm_gc_hash(value.ptr) % VM_MEM_UNITS;
    while (to[ind].ptr != 0)
    {
        ind++;
        if (ind == VM_MEM_UNITS)
        {
            ind = 0;
        }
    }
    to[ind] = value;
}

void vm_gc_delete(vm_gc_entry_t *to, uint64_t ptr)
{
    uint64_t ind = vm_gc_hash(ptr) % VM_MEM_UNITS;
    while (to[ind].ptr != ptr)
    {
        ind++;
        if (ind == VM_MEM_UNITS)
        {
            ind = 0;
        }
    }
    to[ind].ptr = 0;
    to[ind].obj = vm_obj_of_dead();
}

size_t vm_gc_count(vm_gc_entry_t *ents)
{
    size_t ret = 0;
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        if (ents[index].ptr != 0)
        {
            ret += 1;
        }
    }
    return ret;
}

void vm_gc_run1(vm_gc_t *gc)
{
    printf("-> %zu : %zu : %zu\n", vm_gc_count(gc->objs1), vm_gc_count(gc->objs2), vm_gc_count(gc->objs3));
    for (vm_obj_t *cur = gc->base; cur < gc->end; cur++)
    {
        if (vm_obj_is_ptr(*cur))
        {
            vm_gc_mark_ptr_yes(gc, vm_obj_to_ptr(*cur));
        }
    }
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        gc->objs1[index].root = false;
    }
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        vm_gc_entry_t *ent = &gc->objs2[index];
        if (ent->ptr > 1)
        {
            if (ent->keep)
            {
                vm_gc_move(gc->objs3, *ent);
            }
            else
            {
                vm_gc_delete(gc->objs3, ent->ptr);
            }
            ent->ptr = 0;
        }
    }
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        vm_gc_entry_t *ent = &gc->objs3[index];
        if (ent->ptr > 1)
        {
            if (!ent->keep)
            {
                ent->ptr = 0;
            }
            else
            {
                ent->keep = false;
            }
        }
    }
    void *new1 = gc->objs2;
    gc->objs2 = gc->objs1;
    gc->objs1 = new1;
    // printf("-> %zu : %zu : %zu\n", vm_gc_count(gc->objs1), vm_gc_count(gc->objs2), vm_gc_count(gc->objs3));
}

#if defined(VM_GC_THREADS)
void *vm_gc_run_thread(void *gc_arg)
{
    vm_gc_t *gc = gc_arg;
    while (!gc->die)
    {
        vm_gc_run1(gc);
    }
    return NULL;
}
#endif

void vm_gc_start(vm_gc_t *gc, vm_obj_t *base, vm_obj_t *end)
{
    gc->base = base;
    gc->end = end;
    gc->last = 2;
#if defined(VM_GC_NO_MALLOC)
    gc->objs1 = &vm_gc_objs1[0];
    gc->objs2 = &vm_gc_objs2[0];
    gc->objs3 = &vm_gc_objs3[0];
    for (size_t i = 0; i < VM_MEM_UNITS; i++)
    {
        vm_gc_objs1[i].ptr = 0;
        vm_gc_objs2[i].ptr = 0;
        vm_gc_objs3[i].ptr = 0;
    }
#else
    gc->objs1 = calloc(1, sizeof(vm_gc_entry_t) * VM_MEM_UNITS);
    gc->objs2 = calloc(1, sizeof(vm_gc_entry_t) * VM_MEM_UNITS);
    gc->objs3 = calloc(1, sizeof(vm_gc_entry_t) * VM_MEM_UNITS);
#endif
    gc->max1 = 0;
#if defined(VM_GC_THREADS)
    gc->die = false;
    // pthread_create(&gc->thread, NULL, &vm_gc_run_thread, gc);
#endif
}

void vm_gc_stop(vm_gc_t *gc)
{
#if !defined(VM_GC_NO_MALLOC)
    free(gc->objs1);
    free(gc->objs2);
    free(gc->objs3);
#endif
#if defined(VM_GC_THREADS)
    gc->die = true;
    // pthread_join(gc->thread, NULL);
#endif
}

vm_obj_t vm_gc_new(vm_gc_t *gc, size_t size, vm_obj_t *values)
{
    if (size >= (1 << 15))
    {
        __builtin_trap();
    }
    else if (size > 0)
    {
        uint64_t where = gc->last;
        for (int i = size; i > 0; i--)
        {
            if (gc->last % 10 == 0)
            {
                vm_gc_run1(gc);
            }
            uint64_t ptr = gc->last;
            vm_gc_entry_t entry = (vm_gc_entry_t){
                .keep = true,
                .root = true,
                .len = i,
                .ptr = ptr,
                .obj = values[size - i],
            };
            gc->last += 1;
            vm_gc_move(gc->objs1, entry);
        }
        return vm_obj_of_ptr(where);
    }
    else
    {
        return vm_obj_of_ptr(1);
    }
}

vm_obj_t vm_gc_index(vm_gc_t *gc, uint64_t ptr, size_t index)
{
    return vm_gc_get(gc, ptr + index).obj;
}

size_t vm_gc_sizeof(vm_gc_t *gc, uint64_t ptr)
{
    if (ptr == 1)
    {
        return 0;
    }
    else
    {
        return vm_gc_get(gc, ptr).len;
    }
}