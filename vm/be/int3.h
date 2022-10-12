#if !defined(VM_HEADER_IR_BE_INT3)
#define VM_HEADER_IR_BE_INT3

#include "../ir.h"

typedef enum vm_opcode_t {
    VM_OPCODE_NOP,
} vm_opcode_t;

struct vm_state_t;
typedef struct vm_state_t vm_state_t;

struct vm_state_t {
    vm_opcode_t *head;
    size_t framesize;
    size_t nlocals;
    void *locals;
};

vm_state_t *vm_state_init(size_t nregs);
void vm_state_deinit(vm_state_t *state);
void vm_run(vm_state_t *state, vm_block_t *block);

#endif
