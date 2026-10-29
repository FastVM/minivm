
#pragma once

#if !defined(MX_HEADER_LUA_FRONT_CHUNK)
#define MX_HEADER_LUA_FRONT_CHUNK

#include "../../ir/ir.h"
#include "../../value/error.h"
#include "../../vm/vm.h"

typedef struct mx_chunk_compiled_t mx_chunk_compiled_t;

struct mx_chunk_compiled_t {
    mx_ir_block_t *entry;
    mx_error_t *error;
};

uint32_t mx_chunk_hash(const char *src, uint32_t len);
mx_chunk_compiled_t mx_chunk_compile(mx_t *vm, const char *src, uint32_t len, const char *file);

#endif
