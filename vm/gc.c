#include <vm/gc.h>
#include <vm/obj.h>
#include <vm/io.h>

#define VM_MEM_MAX ((VM_FRAMES_UNITS * sizeof(vm_stack_frame_t)) + (VM_LOCALS_UNITS * sizeof(vm_obj_t)))

#define VM_GC_MEM_GROW (1)

size_t vm_stats_memsize = VM_MEM_MAX;
size_t vm_stats_memunits = VM_MEM_UNITS;

size_t vm_mem_top = 0;
uint8_t vm_mem[VM_MEM_MAX];
vm_gc_entry_t vm_gc_objs0[VM_MEM_UNITS];
vm_gc_entry_t vm_gc_objs1[VM_MEM_UNITS];
vm_gc_entry_t vm_gc_objs2[VM_MEM_UNITS];
vm_gc_entry_t vm_gc_objs3[VM_MEM_UNITS];

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

size_t vm_gc_hash(uint64_t h)
{
    return ((h * 2654435761) >> 29) % VM_MEM_UNITS;
}

size_t vm_gc_litehash(uint64_t h)
{
    return (h * 3) % VM_MEM_UNITS;
}

vm_gc_entry_t vm_gc_find_in(vm_gc_entry_t *elems, uint64_t ptr, size_t head)
{
    do
    {
        vm_gc_entry_t ent = elems[head];
        if (ent.ptr == ptr)
        {
            return ent;
        }
        if (ent.ptr == 0)
        {
            return (vm_gc_entry_t){
                .ptr = 0,
            };
        }
        head++;
        if (head == VM_MEM_UNITS)
        {
            head = 0;
        }
    } while (true);
}

vm_gc_entry_t vm_gc_get(vm_gc_t *gc, uint64_t ptr)
{
    vm_gc_entry_t e0 = vm_gc_find_in(gc->objs0, ptr, vm_gc_litehash(ptr));
    if (e0.ptr != 0)
    {
        return e0;
    }
    vm_gc_entry_t e1 = vm_gc_find_in(gc->objs1, ptr, vm_gc_litehash(ptr));
    if (e1.ptr != 0)
    {
        return e1;
    }
    vm_gc_entry_t e2 = vm_gc_find_in(gc->objs2, ptr, vm_gc_hash(ptr));
    if (e2.ptr != 0)
    {
        return e2;
    }
    return vm_gc_find_in(gc->objs3, ptr, vm_gc_hash(ptr));
}

bool vm_gc_mark_entry_yes(vm_gc_entry_t *elems, uint64_t ptr, size_t head)
{
    do
    {
        vm_gc_entry_t *ent = &elems[head];
        if (ent->ptr == ptr)
        {
            ent->keep = true;
            return true;
        }
        if (ent->ptr == 0)
        {
            return false;
        }
        head++;
        if (head == VM_MEM_UNITS)
        {
            head = 0;
        }
    } while (true);
}

void vm_gc_mark_val_yes(vm_gc_t *gc, uint64_t ptr)
{
    bool okay = vm_gc_mark_entry_yes(gc->objs0, ptr, vm_gc_litehash(ptr));
    if (!okay)
    {
        okay = vm_gc_mark_entry_yes(gc->objs1, ptr, vm_gc_litehash(ptr));
        if (!okay)
        {
            okay = vm_gc_mark_entry_yes(gc->objs2, ptr, vm_gc_hash(ptr));
            if (!okay)
            {
                vm_gc_mark_entry_yes(gc->objs3, ptr, vm_gc_hash(ptr));
            }
        }
    }
}

void vm_gc_mark_ptr_yes(vm_gc_t *gc, uint64_t ptr)
{
    vm_gc_entry_t ent = vm_gc_get(gc, ptr);
    vm_gc_mark_val_yes(gc, ptr);
    size_t len = ent.len;
    while (len > 0)
    {
        len -= 1;
        vm_gc_mark_val_yes(gc, ptr + len);
        if (vm_obj_is_ptr(ent.obj))
        {
            vm_gc_mark_ptr_yes(gc, vm_obj_to_ptr(ent.obj));
        }
        ent = vm_gc_get(gc, ptr + len);
    }
}

void vm_gc_move(vm_gc_entry_t *to, vm_gc_entry_t value, size_t head)
{
    while (to[head].ptr != 0)
    {
        head++;
        if (head == VM_MEM_UNITS)
        {
            head = 0;
        }
    }
    to[head] = value;
}

vm_gc_entry_stats_t vm_gc_stats(vm_gc_entry_t *ents)
{
    size_t findall = 0;
    size_t maxfind = 0;
    size_t count = 0;
    size_t index = 0;
    while (index < VM_MEM_UNITS)
    {
        if (ents[index].ptr != 0)
        {
            size_t lookups = 0;
            while (ents[index].ptr != 0)
            {
                count += 1;
                index += 1;
                lookups += 1;
                findall += lookups;
            }
            if (lookups >= maxfind)
            {
                maxfind = lookups;
            }
        }
        else
        {
            index += 1;
        }
    }
    return (vm_gc_entry_stats_t){
        .count = count,
        .findall = findall,
        .maxfind = maxfind,
    };
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
    size_t num_dead = 0;
    for (size_t index = 0; index < gc->nlocals; index++)
    {
        vm_obj_t cur = gc->base[index];
        if (vm_obj_is_ptr(cur))
        {
            uint64_t ptr = vm_obj_to_ptr(cur);
            vm_gc_mark_ptr_yes(gc, ptr);
            num_dead = 0;
        }
        else if (vm_obj_is_dead(cur))
        {
            if (num_dead == 256)
            {
                break;
            }
            num_dead += 1;
        }
        else
        {
            num_dead += 1;
        }
    }
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        vm_gc_entry_t *ent = &gc->objs1[index];
        if (ent->ptr != 0)
        {
            if (ent->keep)
            {
                vm_gc_move(gc->objs3, *ent, vm_gc_hash(ent->ptr));
            }
            ent->ptr = 0;
        }
    }
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        vm_gc_entry_t *ent = &gc->objs3[index];
        if (ent->ptr != 0)
        {
            if (ent->keep)
            {
                vm_gc_move(gc->objs2, *ent, vm_gc_hash(ent->ptr));
            }
            ent->ptr = 0;
        }
    }
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        vm_gc_entry_t *ent = &gc->objs2[index];
        if (ent->ptr != 0)
        {
            if (ent->keep)
            {
                ent->keep = false;
                vm_gc_move(gc->objs3, *ent, vm_gc_hash(ent->ptr));
            }
            ent->ptr = 0;
        }
    }
    vm_gc_entry_t *old1 = gc->objs1;
    gc->objs1 = gc->objs0;
    gc->objs0 = old1;
}

#if defined(VM_GC_THREADS)
void *vm_gc_run_thread(void *gc_arg)
{
    vm_gc_t *gc = gc_arg;
    while (true)
    {
        vm_gc_run1(gc);
    }
    return NULL;
}
#endif

void vm_gc_start(vm_gc_t *gc, vm_obj_t *base, size_t nlocals)
{
    gc->base = base;
    gc->nlocals = nlocals;
    gc->last = 2;
    gc->objs0 = &vm_gc_objs0[0];
    gc->objs1 = &vm_gc_objs1[0];
    gc->objs2 = &vm_gc_objs2[0];
    gc->objs3 = &vm_gc_objs3[0];
    for (long i = 0; i < VM_MEM_UNITS; i++)
    {
        gc->objs0[i].ptr = 0;
        gc->objs1[i].ptr = 0;
        gc->objs2[i].ptr = 0;
        gc->objs3[i].ptr = 0;
    }
#if defined(VM_GC_THREADS)
    pthread_create(&gc->thread, NULL, &vm_gc_run_thread, gc);
#endif
}

void vm_gc_stop(vm_gc_t *gc)
{
#if defined(VM_GC_THREADS)
    pthread_cancel(gc->thread);
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
        for (size_t i = size; i > 0; i--)
        {
#if !defined(VM_GC_THREADS)
            if (gc->last % (VM_MEM_UNITS / 3) == 0)
            {
                vm_gc_run1(gc);
            }
#endif
            uint64_t ptr = gc->last;
            vm_gc_entry_t entry = (vm_gc_entry_t){
                .keep = false,
                .len = i,
                .ptr = ptr,
                .obj = values[size - i],
            };
            gc->last += 1;
            vm_gc_move(gc->objs0, entry, vm_gc_litehash(ptr));
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