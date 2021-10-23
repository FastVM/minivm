#pragma once

#include "../gc.h"

struct vm_map_t;
struct vm_map_pair_t;
typedef struct vm_map_t vm_map_t;
typedef struct vm_map_pair_t vm_map_pair_t;

vm_map_t *vm_map_new(void);
void vm_map_del(vm_map_t *map);

void vm_map_set_index(vm_map_t *map, vm_obj_t key, vm_obj_t value);
vm_obj_t vm_map_get_index(vm_map_t *map, vm_obj_t key);
