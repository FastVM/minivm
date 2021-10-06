#include <vm/gc.h>
#include <vm/obj.h>
#include <vm/io.h>

#define VM_MEM_MAX ((VM_FRAMES_UNITS * sizeof(vm_stack_frame_t)) + (VM_LOCALS_UNITS * sizeof(vm_obj_t)))

#define VM_GC_MEM_GROW (1)

size_t vm_stats_memsize = VM_MEM_MAX;
size_t vm_stats_memunits = VM_MEM_UNITS;

size_t vm_mem_top = 0;
uint8_t vm_mem[VM_MEM_MAX];
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
    h ^= h * 2654435761;
    h >>= 30;
    return h % VM_MEM_UNITS;
}

vm_gc_entry_t vm_gc_find_in(vm_gc_entry_t *elems, uint64_t ptr)
{
    size_t head = vm_gc_hash(ptr);
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
    vm_gc_entry_t e1 = vm_gc_find_in(gc->objs[0], ptr);
    if (e1.ptr != 0)
    {
        return e1;
    }
    vm_gc_entry_t e2 = vm_gc_find_in(gc->objs[1], ptr);
    if (e2.ptr != 0)
    {
        return e2;
    }
    return vm_gc_find_in(gc->objs[2], ptr);
}

enum
{
    VM_GC_KEEP_IS_FALSE,
    VM_GC_KEEP_IS_TRUE,
    VM_GC_NOT_FOUND,
};

bool vm_gc_mark_entry_yes(vm_gc_t *gc, vm_gc_entry_t *elems, uint64_t ptr)
{
    size_t head = vm_gc_hash(ptr);
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
    bool okay = vm_gc_mark_entry_yes(gc, gc->objs[0], ptr);
    if (!okay)
    {
        okay = vm_gc_mark_entry_yes(gc, gc->objs[1], ptr);
    }
    if (!okay)
    {
        vm_gc_mark_entry_yes(gc, gc->objs[2], ptr);
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

void vm_gc_move(vm_gc_entry_t *to, vm_gc_entry_t value)
{
    size_t ind = vm_gc_hash(value.ptr);
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
        vm_gc_entry_t *ent = &gc->objs[2][index];
        if (ent->ptr != 0)
        {
            if (ent->keep)
            {
                vm_gc_move(gc->objs[1], *ent);
            }
            ent->ptr = 0;
        }
    }
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        vm_gc_entry_t *ent = &gc->objs[1][index];
        if (ent->ptr != 0)
        {
            if (ent->keep)
            {
                ent->keep = false;
                vm_gc_move(gc->objs[2], *ent);
            }
            ent->ptr = 0;
        }
    }
    for (size_t index = 0; index < VM_MEM_UNITS; index++)
    {
        gc->objs[2][index].keep = false;
    }
    void *new1 = gc->objs[1];
    gc->objs[1] = gc->objs[0];
    gc->objs[0] = new1;
    // vm_gc_entry_stats_t stats = vm_gc_stats(gc->objs[2]);
    // printf("usage: %.2lf%%\n", stats.count * 100 / (double) VM_MEM_UNITS);
    // printf("avg find: %lf\n", (stats.findall + 1) / (double) (stats.count + 1));
    // printf("max find: %zu\n", stats.maxfind);
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

void vm_gc_start(vm_gc_t *gc, vm_obj_t *base, size_t nlocals)
{
    gc->base = base;
    gc->nlocals = nlocals;
    gc->last = 2;
    gc->objs[0] = &vm_gc_objs1[0];
    gc->objs[1] = &vm_gc_objs2[0];
    gc->objs[2] = &vm_gc_objs3[0];
    for (long i = 0; i < VM_MEM_UNITS; i++)
    {
        gc->objs[0][i].ptr = 0;
        gc->objs[1][i].ptr = 0;
        gc->objs[2][i].ptr = 0;
    }
#if defined(VM_GC_THREADS)
    gc->die = false;
    pthread_create(&gc->thread, NULL, &vm_gc_run_thread, gc);
#endif
}

void vm_gc_stop(vm_gc_t *gc)
{
#if defined(VM_GC_THREADS)
    gc->die = true;
    pthread_join(gc->thread, NULL);
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
            uint64_t ptr = gc->last;
            vm_gc_entry_t entry = (vm_gc_entry_t){
                .keep = true,
                .len = i,
                .ptr = ptr,
                .obj = values[size - i],
            };
            gc->last += 1;
            vm_gc_move(gc->objs[0], entry);
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