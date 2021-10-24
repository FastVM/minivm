#include "map.h"
#include "../obj.h"
#include "../math.h"
#define kmalloc vm_malloc
#define kcalloc vm_calloc
#define krealloc vm_realloc
#define kfree vm_free
#include "../klib/khash.h"

size_t vm_map_hash_obj(vm_obj_t obj)
{
	if (vm_obj_is_ptr(obj))
	{
		vm_gc_entry_t *ent = vm_obj_to_ptr(obj);
		switch (vm_gc_type(ent))
		{
		case VM_TYPE_BOX:
		{
			return vm_map_hash_obj(vm_gc_get_box(ent)) * 2654435769;
		}
		case VM_TYPE_ARRAY:
		{
			size_t ret = 0;
			for (size_t i = 0; i < vm_gc_sizeof(ent); i++)
			{
				ret += vm_map_hash_obj(vm_gc_get_index(ent, vm_obj_of_int(i)));
				ret <<= 5;
			}
			return ret;
		}
		case VM_TYPE_STRING:
		{
			size_t ret = 0;
			for (size_t i = 0; i < vm_gc_sizeof(ent); i++)
			{
				ret += vm_map_hash_obj(vm_gc_get_index(ent, vm_obj_of_int(i)));
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
		return ((size_t) n) ^ (size_t) (n * 1000L * 1000L);
	}
	if (vm_obj_is_fun(obj))
	{
		return vm_obj_to_fun(obj);
	}
	return 0;
}

KHASH_INIT(omap, vm_obj_t, vm_obj_t, true, vm_map_hash_obj, vm_obj_eq);

struct vm_map_t
{
	kh_omap_t value;
};

vm_map_t *vm_map_new(void)
{
	return (vm_map_t *)kh_init_omap();
}
void vm_map_del(vm_map_t *map)
{
	kh_destroy_omap((kh_omap_t *)map);
}

void vm_map_set_index(vm_map_t *map, vm_obj_t key, vm_obj_t value)
{
	int ret = 0;
	size_t k = kh_put_omap((kh_omap_t *)map, key, &ret);
	kh_value((kh_omap_t *)map, k) = value;
}

vm_obj_t vm_map_get_index(vm_map_t *map, vm_obj_t key)
{
	size_t k = kh_get_omap((kh_omap_t *)map, key);
	return kh_value((kh_omap_t *)map, k);
}

void vm_map_for_pairs(vm_map_t *map, void *state, void(*fn)(void *state, vm_obj_t key, vm_obj_t value))
{
	vm_obj_t key;
	vm_obj_t value;
	kh_foreach((kh_omap_t *)map, key, value, {
		fn(state, key, value);
	});
}

size_t vm_map_sizeof(vm_map_t *map)
{
	vm_obj_t key, value;
	size_t ret;
	kh_foreach((kh_omap_t *)map, key, value, {
		ret += 1;
	})
	return ret;
}
