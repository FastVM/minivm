#include "gc.h"
#include "obj.h"
#include "io.h"

#if defined(VM_USE_MIMALLOC)
void *mi_malloc(size_t size);
void mi_free(void *ptr);
#define vm_malloc mi_malloc
#define vm_free mi_free
#else
void *malloc(size_t size);
void free(void *ptr);
#define vm_malloc malloc
#define vm_free free
#endif

#define VM_MEM_UNITS (1 << 26)

#define VM_MEM_MAX ((VM_FRAMES_UNITS * sizeof(vm_stack_frame_t)) + (VM_LOCALS_UNITS * sizeof(vm_obj_t)))

#define VM_GC_MEM_GROW (1)

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

void vm_gc_mark_ptr_yes(vm_gc_t *gc, vm_gc_entry_t *ent)
{
    ent->keep += 1;
    for (size_t cur = 0; cur < ent->len; cur++)
    {
        vm_obj_t obj = ent->obj[cur];
        if (vm_obj_is_ptr(obj))
        {
            vm_gc_mark_ptr_yes(gc, vm_obj_to_ptr(obj));
        }
    }
}

void vm_gc_run1(vm_gc_t *gc)
{
    for (size_t index = 0; index < gc->len; index++)
    {
        gc->objs2[index]->keep = 0;
    }
    gc->calls++;
    size_t num_dead = 0;
    for (size_t index = 0; index < gc->nlocals; index++)
    {
        vm_obj_t cur = gc->base[index];
        if (vm_obj_is_ptr(cur))
        {
            vm_gc_entry_t *ptr = vm_obj_to_ptr(cur);
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
            num_dead = 0;
        }
    }
    size_t begin = 0;
    for (size_t index = 0; index < gc->len; index++)
    {
        vm_gc_entry_t *ent = gc->objs2[index];
        if (ent->keep)
        {
            gc->objs2[begin++] = ent;
        }
        else {
            vm_free(ent);
            gc->objs2[index] = NULL;
        }
    }
    gc->len = begin;
    gc->max = 16 + begin * 2;
}

void vm_gc_start(vm_gc_t *gc, vm_obj_t *base, size_t nlocals)
{
    gc->base = base;
    gc->nlocals = nlocals;
    gc->len = 0;
    gc->max = 16;
    gc->objs2 = vm_malloc(sizeof(vm_gc_entry_t *) * VM_MEM_UNITS);
    gc->calls = 0;
}

void vm_gc_stop(vm_gc_t *gc)
{
    // printf("calls: %zu\n", gc->calls);
    for (size_t index = 0; index < gc->len; index++)
    {
        vm_free(gc->objs2[index]);
    }
    vm_free(gc->objs2);
}

vm_obj_t vm_gc_new(vm_gc_t *gc, size_t size, vm_obj_t *values)
{
    if (gc->len == gc->max)
    {
        vm_gc_run1(gc);
    }
    vm_gc_entry_t *entry = vm_malloc(sizeof(vm_gc_entry_t) + sizeof(vm_obj_t[size]));
    *entry = (vm_gc_entry_t){
        .len = size,
        .keep = false,
    };
    for (size_t i = 0; i < size; i++)
    {
        entry->obj[i] = values[i];
    }
    gc->objs2[gc->len++] = entry;
    return vm_obj_of_ptr(entry);
}

vm_obj_t vm_gc_get_index(vm_gc_t *gc, vm_gc_entry_t* ptr, size_t index)
{
    return ptr->obj[index];
}

size_t vm_gc_sizeof(vm_gc_t *gc, vm_gc_entry_t *ptr)
{
    return ptr->len;
}