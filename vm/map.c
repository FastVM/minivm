#include "map.h"
#include "obj.h"
#include "math.h"

size_t vm_map_hash_obj(vm_obj_t obj)
{
    if (vm_obj_is_ptr(obj))
    {
        vm_gc_entry_t *ent = vm_obj_to_ptr(obj);
        switch (vm_gc_type(ent))
        {
        case VM_TYPE_ARRAY:
        {
            vm_gc_entry_array_t *arr = (vm_gc_entry_array_t *)ent;
            size_t ret = 0;
            for (size_t i = 0; i < arr->len; i++)
            {
                ret ^= vm_map_hash_obj(vm_gc_get_index(ent, vm_obj_of_int(i)));
                ret <<= 5;
            }
            return ret;
        }
        case VM_TYPE_STRING:
        {
            vm_gc_entry_string_t *arr = (vm_gc_entry_string_t *)ent;
            size_t ret = 0;
            for (size_t i = 0; i < arr->len; i++)
            {
                ret ^= vm_map_hash_obj(vm_gc_get_index(ent, vm_obj_of_int(i)));
                ret <<= 5;
            }
            return ret;
        }
        }
        return 0;
    }
    if (vm_obj_is_none(obj))
    {
        return 0;
    }
    if (vm_obj_is_bool(obj))
    {
        return vm_obj_to_bool(obj);
    }
    if (vm_obj_is_num(obj))
    {
        vm_number_t n = vm_obj_to_num(obj);
        return ((size_t) n ^ (size_t) (n * (1 << 16)));
    }
    if (vm_obj_is_fun(obj))
    {
        return vm_obj_to_fun(obj);
    }
    return 0;
}

struct vm_map_t
{
    vm_obj_t *keys;
    vm_obj_t *values;
    size_t used;
    size_t alloc;
};

void vm_map_grow(vm_map_t *map)
{
    size_t new_alloc = map->alloc + 2;
    vm_obj_t *keys = vm_malloc(sizeof(vm_obj_t) * (1 << new_alloc));
    vm_obj_t *values = vm_malloc(sizeof(vm_obj_t) * (1 << new_alloc));
    for (size_t i = 0; i < (1 << new_alloc); i++)
    {
        keys[i] = vm_obj_of_dead();
    }
    vm_map_t new_map = (vm_map_t) {
        .keys = keys,
        .values = values,
        .used = 0,
        .alloc = new_alloc,
    };
    for (size_t i = 0; i < (1 << map->alloc); i++)
    {
        if (!vm_obj_is_dead(map->keys[i]))
        {
            vm_map_set_index(&new_map, map->keys[i], map->values[i]);
        }
    }
    vm_free(map->keys);
    vm_free(map->values);
    *map = new_map;
}

void vm_map_set_index(vm_map_t *map, vm_obj_t key, vm_obj_t value)
{
    size_t mask = (1 << map->alloc) - 1;
    if (map->used * 2 > mask)
    {
        vm_map_grow(map);
        mask = (1 << map->alloc) - 1;
    }
    size_t hash = vm_map_hash_obj(key);
    while(true)
    {
        if (vm_obj_is_dead(map->keys[hash & mask]))
        {
            map->keys[hash & mask] = key;
            map->values[hash & mask] = value;
            map->used += 1;
            break;
        }

        if (vm_obj_eq(key, map->keys[hash & mask]))
        {
            map->values[hash & mask] = value;
            break;
        }

        hash += 1;
    }
}

vm_map_t *vm_map_new(void)
{
    vm_map_t *ret = vm_malloc(sizeof(vm_map_t));
    ret->used = 0;
    ret->alloc = 1;
    ret->keys = vm_malloc(sizeof(vm_obj_t) * (1 << ret->alloc));
    for (size_t i = 0; i < (1 << ret->alloc); i++)
    {
        ret->keys[i] = vm_obj_of_dead();
    }
    ret->values = vm_malloc(sizeof(vm_obj_t) * (1 << ret->alloc));
    return ret;
}

void vm_map_del(vm_map_t *map)
{
    vm_free(map->keys);
    vm_free(map->values);
    vm_free(map);
}

vm_obj_t vm_map_get_index(vm_map_t *map, vm_obj_t key)
{
    const size_t mask = (1 << map->alloc) - 1;
    size_t hash = vm_map_hash_obj(key);
    while(true)
    {
        if (vm_obj_is_dead(map->keys[hash & mask]))
        {
            return vm_obj_of_dead();
        }

        if (vm_obj_eq(key, map->keys[hash & mask]))
        {
            return map->values[hash & mask];
        }

        hash += 1;
    }
}

void vm_map_for_pairs(vm_map_t *map, void *state, int (*fn)(void *state, vm_obj_t key, vm_obj_t value))
{
    for (size_t i = 0; i < (1 << map->alloc); i++)
    {
        if (!vm_obj_is_dead(map->keys[i]))
        {
            int res = fn(state, map->keys[i], map->values[i]);
            if (res) {
                break;
            }
        }
    }
}

size_t vm_map_sizeof(vm_map_t *map)
{
    return map->used;
}
