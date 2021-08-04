#pragma once

#include <vm/nanbox.h>
#include <vm/gc.h>

#define gcvec_new(gc, size) (vm_gc_new(gc, size))
#define gcvec_size(gc, vec) (vm_gc_sizeof(gc, vec))
#define gcvec_get(gc, vec, index) (vm_gc_get(gc, vec, index))
#define gcvec_set(gc, vec, index, value) (vm_gc_set(gc, vec, index, value))
