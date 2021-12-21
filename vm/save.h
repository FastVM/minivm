#pragma once

#include "libc.h"

struct vm_save_t;
typedef struct vm_save_t vm_save_t;

struct vm_save_t {
    uint8_t *str;
    size_t len;
    size_t alloc;
};

#include "type.h"
#include "state.h"
#include "gc.h"

// set and reset a state
void vm_save_init(vm_save_t *out);
void vm_save_deinit(vm_save_t *save);
void vm_save_rewind(vm_save_t *save);

// various serializers
void vm_save_byte(vm_save_t *save, uint8_t val);
void vm_save_uint(vm_save_t *save, uint32_t val);
void vm_save_obj(vm_save_t *save, vm_gc_t *gc, vm_obj_t obj);
void vm_save_gc(vm_save_t *save, vm_gc_t *gc);
void vm_save_state(vm_save_t *save, vm_state_t *state);

// various deserializers
uint8_t vm_save_get_byte(vm_save_t *save);
uint32_t vm_save_get_uint(vm_save_t *save);
vm_obj_t vm_save_get_obj(vm_save_t *save, vm_gc_t *gc);
void vm_save_get_gc(vm_save_t *save, vm_gc_t *state);
void vm_save_get_state(vm_save_t *save, vm_state_t *gc);
