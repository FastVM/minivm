
#include <vm/nanbox.h>
typedef nanbox_t vm_obj_t;

#define vm_obj_to_num(obj) (nanbox_to_int(obj))
#define vm_obj_of_num(obj) (nanbox_from_int(obj))
#define vm_obj_is_num(obj) (nanbox_is_int(obj))

#define vm_obj_to_ptr(obj) (nanbox_to_pointer(obj))
#define vm_obj_of_ptr(ptr) (nanbox_from_pointer(ptr))
#define vm_obj_is_ptr(obj) (nanbox_is_pointer(obj))

#define vm_obj_to_fun(obj) (nanbox_to_int(obj))
#define vm_obj_of_fun(fun) (nanbox_from_int(fun))

#define vm_obj_of_dead() (nanbox_empty())
#define vm_obj_is_dead(val) (nanbox_is_empty(val))
