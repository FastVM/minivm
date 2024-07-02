
#if !defined(VM_HEADER_ERRORS)
#define VM_HEADER_ERRORS

#include "./lib.h"

struct vm_location_t;
struct vm_location_range_t;
struct vm_error_t;

typedef struct vm_location_t vm_location_t;
typedef struct vm_location_range_t vm_location_range_t;
typedef struct vm_error_t vm_error_t;

struct vm_location_t {
    size_t byte;
};

struct vm_location_range_t {
    const char *src;
    vm_location_t start;
    vm_location_t stop;
};

struct vm_error_t {
    union {
        // if child == NULL
        const char *msg;
        // if child != NULL
        vm_location_range_t range;
    };

    vm_error_t *child;
};

vm_error_t *vm_error_from_msg(const char *msg);
vm_error_t *vm_error_from_error(vm_location_range_t range, vm_error_t *child);

#endif

