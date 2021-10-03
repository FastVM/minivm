#pragma once

#include <vm/obj.h>
#include <vm/gc.h>

#define gcvec_new(gc, size, values) (vm_gc_new(gc, size, values))
#define gcvec_size(gc, vec) (vm_gc_sizeof(gc, vm_obj_to_ptr(vec)))
#define gcvec_get(gc, vec, index) (vm_gc_index(gc, vm_obj_to_ptr(vec), index))