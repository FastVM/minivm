#include "gc.h"
#include "obj.h"
#include "sys.h"
#include "io.h"

#if defined(VM_USE_MIMALLOC)
#include <mimalloc.h>
#define vm_malloc(size) (mi_heap_malloc(gc->heap (size)))
#define vm_free(ptr) (mi_heap_free(gc->heap, (ptr)))
#define vm_realloc(ptr, size) (mi_heap_realloc(gc->heap, (ptr), (size)))
#else
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
#define vm_malloc malloc
#define vm_free free
#define vm_realloc realloc
#endif

#define VM_MEM_UNITS (1 << 26)

#define VM_MEM_MAX ((VM_FRAMES_UNITS * sizeof(vm_stack_frame_t)) + (VM_LOCALS_UNITS * sizeof(vm_obj_t)))

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

void vm_gc_mark_ptr(vm_gc_t *gc, vm_gc_entry_t *ent)
{
    if (ent->keep) {
        return;
    }
    ent->keep = true;
    for (size_t cur = 0; cur < ent->len; cur++)
    {
        vm_obj_t obj = ent->obj[cur];
        if (vm_obj_is_ptr(obj))
        {
            vm_gc_mark_ptr(gc, vm_obj_to_ptr(obj));
        }
    }
}

void vm_gc_run1(vm_gc_t *gc, vm_obj_t *low, vm_obj_t *high)
{
    for (vm_obj_t *base = low; base < high; base++)
    {
        vm_obj_t cur = *base;
        if (vm_obj_is_ptr(cur))
        {
            vm_gc_entry_t *ptr = vm_obj_to_ptr(cur);
            vm_gc_mark_ptr(gc, ptr);
        }
    }
    size_t begin = 0;
    for (size_t index = 0; index < gc->len; index++)
    {
        vm_gc_entry_t *ent = gc->objs[index];
        if (ent->keep)
        {
            ent->keep = false;
            gc->objs[begin++] = ent;
        }
        else {
            vm_free(ent);
            gc->objs[index] = NULL;
        }
    }
    gc->len = begin;
    size_t newmax = 4 + begin * 1.2;
    if (gc->max < newmax)
    {
        gc->max = newmax;
    }
    if (gc->max + 4 >= gc->alloc) {
        gc->alloc = 4 + gc->alloc * 2;
        gc->objs = vm_realloc(gc->objs, sizeof(vm_gc_entry_t *) * gc->alloc);
    }
    if (gc->alloc >= gc->max * 64) {
        gc->alloc = 4 + gc->alloc / 16;
        gc->objs = vm_realloc(gc->objs, sizeof(vm_gc_entry_t *) * gc->alloc);
    }
}

void vm_gc_start(vm_gc_t *gc)
{
#if defined(VM_USE_MIMALLOC)
    gc->heap = mi_heap_new();
#endif
    gc->len = 0;
    gc->max = 4;
    gc->alloc = 4;
    gc->objs = vm_malloc(sizeof(vm_gc_entry_t *) * gc->alloc);
}

void vm_gc_stop(vm_gc_t *gc)
{
#if defined(VM_USE_MIMALLOC)
    mi_heap_destroy(gc->heap);
#else
    for (size_t index = 0; index < gc->len; index++)
    {
        vm_free(gc->objs[index]);
    }
    vm_free(gc->objs);
#endif
}

vm_gc_entry_t *vm_gc_new(vm_gc_t *gc, size_t size, vm_obj_t *values)
{
    vm_gc_entry_t *entry = vm_malloc(sizeof(vm_gc_entry_t) + sizeof(vm_obj_t) * size);
    *entry = (vm_gc_entry_t){
        .keep = false,
        .len = size,
    };
    for (size_t i = 0; i < size; i++)
    {
        entry->obj[i] = values[i];
    }
    gc->objs[gc->len++] = entry;
    return entry;
}

vm_obj_t vm_gc_get_index(vm_gc_entry_t* ptr, size_t index)
{
    return ptr->obj[index];
}

size_t vm_gc_sizeof(vm_gc_entry_t *ptr)
{
    return ptr->len;
}