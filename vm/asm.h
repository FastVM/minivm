
#if !defined(VM_HEADER_ASM)
#define VM_HEADER_ASM

#include "./ir.h"

struct vm_parser_t;
typedef struct vm_parser_t vm_parser_t;

struct vm_parser_t {
    size_t len;
    size_t alloc;
    const char **names;
    vm_block_t **blocks;
    const char **src;
    size_t line;
    size_t col;
};

bool vm_parse_state(vm_parser_t *state, vm_block_t *block);
vm_block_t *vm_parse_asm(const char *src);

#endif
