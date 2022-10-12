
#include "toir.h"

#include "build.h"
#include "const.h"
#include "tag.h"

struct vm_parser_t;
typedef struct vm_parser_t vm_parser_t;

struct vm_parser_t {
    size_t len;
    size_t alloc;
    const char **names;
    vm_block_t **blocks;
};

static vm_block_t *vm_parser_find(vm_parser_t *state, const char *name) {
    for (size_t i = 0; i < state->len; i++) {
        if (!strcmp(state->names[i], name)) {
            return state->blocks[i];
        }
    }
    size_t where = state->len++;
    if (where >= state->alloc) {
        state->alloc = state->len * 4;
        state->names = vm_realloc(state->names, sizeof(const char *) * state->alloc);
        state->blocks = vm_realloc(state->blocks, sizeof(vm_block_t *) * state->alloc);
    }
    state->names[where] = name;
    vm_block_t *block = vm_malloc(sizeof(vm_block_t));
    *block = (vm_block_t) {
        .tag = VM_TAG_BLOCK,
    };
    state->blocks[where] = block;
    return state->blocks[where];
}

static void vm_ir_parse(vm_parser_t *state, const char **psrc) {
    
}

vm_block_t *vm_parse(const char *src) {
    vm_parser_t parse;
    parse.names = NULL;
    parse.blocks = NULL;
    parse.len = 0;
    parse.alloc = 0;
    vm_ir_parse(&parse, &src);
    return NULL;
}
