#pragma once

#include "gc.h"

struct vm_map_t;
typedef struct vm_map_t vm_map_t;

vm_map_t *vm_map_new(void);
void vm_map_del(vm_map_t *map);

void vm_map_set_index(vm_map_t *map, vm_obj_t key, vm_obj_t value);
vm_obj_t vm_map_get_index(vm_map_t *map, vm_obj_t key);
size_t vm_map_sizeof(vm_map_t *map);

void vm_map_for_pairs(vm_map_t *map, void *state, int(*fn)(void *state, vm_obj_t key, vm_obj_t value));