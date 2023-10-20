// cgemit is short for Codegen Emitter
//
// for better runtime performance this is included into the specific
// files it's used in
#pragma once
#include "../tb_internal.h"

// We really only need the position where to patch
// it since it's all internal and the target is implicit.
typedef uint32_t ReturnPatch;

typedef struct LabelPatch {
    int pos;
    int target_lbl;
} LabelPatch;

typedef struct {
    // technically NULLable, just can't use patches if NULL
    TB_Function* f;
    TB_FunctionOutput* output;

    bool emit_asm;
    TB_Assembly *head_asm, *tail_asm;

    // this is mapped to a giant buffer and is technically
    // allow to use the entire rest of said buffer
    size_t count, capacity;
    uint8_t* data;

    NL_Map(TB_Node*, uint32_t) labels;
} TB_CGEmitter;

// Helper macros
#define EMITA(e, fmt, ...) tb_asm_print(e, fmt, ## __VA_ARGS__)
#define EMIT1(e, b) (*((uint8_t*)  tb_cgemit_reserve(e, 1)) = (b), (e)->count += 1)
#define EMIT2(e, b) do { uint16_t _b = (b); memcpy(tb_cgemit_reserve(e, 2), &_b, 2); (e)->count += 2; } while (0)
#define EMIT4(e, b) do { uint32_t _b = (b); memcpy(tb_cgemit_reserve(e, 4), &_b, 4); (e)->count += 4; } while (0)
#define EMIT8(e, b) do { uint64_t _b = (b); memcpy(tb_cgemit_reserve(e, 8), &_b, 8); (e)->count += 8; } while (0)
#define PATCH4(e, p, b) do { uint32_t _b = (b); memcpy(&(e)->data[p], &_b, 4); } while (0)
#define GET_CODE_POS(e) ((e)->count)
#define RELOC4(e, p, b) tb_reloc4(e, p, b)

static void tb_reloc4(TB_CGEmitter* restrict e, uint32_t p, uint32_t b) {
    void* ptr = &e->data[p];

    // i love UBsan...
    uint32_t tmp;
    memcpy(&tmp, ptr, 4);
    tmp += b;
    memcpy(ptr, &tmp, 4);
}

static void tb_asm_print(TB_CGEmitter* restrict e, const char* fmt, ...) {
    // let's hope the optimizer can hoist this early-out outside of the call
    if (!e->emit_asm) { return; }

    // make sure we have enough bytes for the operation
    TB_Assembly* new_head = e->tail_asm;
    if (new_head == NULL || new_head->length + 100 >= TB_ASSEMBLY_CHUNK_CAP) {
        new_head = tb_platform_valloc(TB_ASSEMBLY_CHUNK_CAP);
        // new_head->next = NULL;
        // new_head->length = 0;

        if (e->tail_asm == NULL) {
            e->tail_asm = e->head_asm = new_head;
        } else {
            e->tail_asm->next = new_head;
            e->tail_asm = new_head;
        }
    }

    va_list ap;
    va_start(ap, fmt);
    new_head->length += vsnprintf(&new_head->data[new_head->length], 100, fmt, ap);
    va_end(ap);
}

static void tb_emit_rel32(TB_CGEmitter* restrict e, uint32_t* head, uint32_t pos) {
    uint32_t curr = *head;
    if (curr & 0x80000000) {
        // the label target is resolved, we need to do the relocation now
        uint32_t target = curr & 0x7FFFFFFF;
        PATCH4(e, pos, target - (pos + 4));
    } else {
        PATCH4(e, pos, curr);
        *head = pos;
    }
}

static void tb_resolve_rel32(TB_CGEmitter* restrict e, uint32_t* head, uint32_t target) {
    // walk previous relocations
    uint32_t curr = *head;
    while (curr != 0 && (curr & 0x80000000) == 0) {
        uint32_t next;
        memcpy(&next, &e->data[curr], 4);
        PATCH4(e, curr, target - (curr + 4));
        curr = next;
    }

    // store the target and mark it as resolved
    *head = 0x80000000 | target;
}

static void* tb_cgemit_reserve(TB_CGEmitter* restrict e, size_t count) {
    if (e->count + count >= e->capacity) {
        // make new region
        TB_CodeRegion* new_region = tb_platform_valloc(CODE_REGION_BUFFER_SIZE);
        if (new_region == NULL) tb_panic("could not allocate code region!");

        new_region->capacity = CODE_REGION_BUFFER_SIZE - sizeof(TB_CodeRegion);
        e->output->code_region = new_region;

        // copy code into new region
        memcpy(new_region->data, e->data, e->count);
        e->data = new_region->data;
    }

    return &e->data[e->count];
}

static void tb_cgemit_commit(TB_CGEmitter* restrict e, size_t bytes) {
    e->count += bytes;
}
