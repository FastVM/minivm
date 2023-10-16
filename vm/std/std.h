#if !defined(VM_HEADER_STD_STD)
#define VM_HEADER_STD_STD

#include "../obj.h"

struct vm_std_value_t;
typedef struct vm_std_value_t vm_std_value_t;

struct vm_std_value_t {
    vm_value_t value;
    uint32_t tag;
};

vm_table_t *vm_std_new(void);

#endif
