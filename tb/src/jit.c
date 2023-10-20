#include "tb_internal.h"
#include "host.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum {
    ALLOC_GRANULARITY = 16,

    ALLOC_COOKIE = 0xBAADF00D,
    ALLOC_IN_USE = 0x8000,
};

typedef struct AllocRegion AllocRegion;
struct AllocRegion {
    AllocRegion *next;

    // in bytes
    uint32_t size;
    uint8_t* data;

    // each AllocRegion has offsets for each
    // entry, this may grow as the region has
    // more allocations.
    uint32_t count, cap;
    uint16_t offsets[];
};

typedef struct {
    // linear allocator that backs our stuff
    size_t capacity, used;
    uint8_t* block;

    TB_MemProtect prot;
    AllocRegion* region;
} TB_JITHeap;

struct TB_JIT {
    NL_Strmap(void*) loaded_funcs;

    TB_JITHeap rx_heap;
    TB_JITHeap rw_heap;
};

static const char* prot_names[] = {
    "RO",
    "RW",
    "RX",
    "RXW",
};

static TB_JITHeap tb_jitheap_create(TB_MemProtect prot, void* ptr, size_t size) {
    return (TB_JITHeap){
        .prot = prot,
        .capacity = size,
        .block = ptr,
    };
}

static void* push_region(TB_JITHeap* c, size_t size) {
    void* ptr = &c->block[c->used];
    c->used += size;
    return ptr;
}

static void* tb_jitheap_alloc_region(TB_JITHeap* c, size_t size, size_t align) {
    size = (size + ALLOC_GRANULARITY - 1) & ~(ALLOC_GRANULARITY - 1);

    AllocRegion* r = c->region;
    size_t offset = 0;

    while (r != NULL && r->size < size) {
        // find free region
        size_t count = r->count, total_size = r->size;

        FOREACH_N(i, 0, count) {
            size_t curr = r->offsets[i];
            if (curr & ALLOC_IN_USE) continue;

            curr &= ~ALLOC_IN_USE;
            size_t next = i + 1 < count ? r->offsets[i + 1] : total_size;
            size_t obj_size = (next & ~ALLOC_IN_USE) - curr;
            if (obj_size >= size) {
                // split free region, we need to move things which is slow...
                assert(count + 1 < r->cap);

                // moved the free space over
                r->offsets[i] = (curr + size);
                if (curr + size == total_size) {
                    // remove free space, there's no more here
                    r->count--;
    	        }

                offset = curr & ~ALLOC_IN_USE;
                goto done;
            }
        }

        r = r->next;
    }

    // allocate enough free pages
    size_t rounded_size = tb_next_pow2(size);
    if (rounded_size < 4096) {
        rounded_size = 4096;
    }

    size_t entry_cap = (rounded_size + ALLOC_GRANULARITY - 1) / ALLOC_GRANULARITY;
    assert(entry_cap >= 2);

    r = tb_platform_heap_alloc(sizeof(AllocRegion) + sizeof(uint16_t)*entry_cap);
    *r = (AllocRegion){
        .size  = rounded_size,
        .cap   = entry_cap,
        .count = 1,
        .data = push_region(c, rounded_size)
    };

    r->offsets[0] = ALLOC_IN_USE | 0;
    if (size != rounded_size) {
        // put leftovers into region
        r->offsets[1] = size;
        r->count++;
    }

    r->next = c->region;
    c->region = r;

    done:
    log_debug("jit heap %s: alloc %-4zu => [ %-4zu - %-4zu ]", prot_names[c->prot], size, offset, offset + size);
    assert((offset & ~(align - 1)) == 0);
    return &r->data[offset];
}

void tb_jitheap_free_region(TB_JITHeap* c, void* ptr, size_t s) {
    tb_todo();
}

static void* get_proc(TB_JIT* jit, const char* name) {
    #ifdef _WIN32
    static HMODULE kernel32, user32, gdi32, opengl32, msvcrt;
    if (user32 == NULL) {
        kernel32 = LoadLibrary("kernel32.dll");
        user32   = LoadLibrary("user32.dll");
        gdi32    = LoadLibrary("gdi32.dll");
        opengl32 = LoadLibrary("opengl32.dll");
        msvcrt   = LoadLibrary("msvcrt.dll");
    }

    // check cache first
    ptrdiff_t search = nl_map_get_cstr(jit->loaded_funcs, name);
    if (search >= 0) return jit->loaded_funcs[search].v;

    void* addr = GetProcAddress(NULL, name);
    if (addr == NULL) addr = GetProcAddress(kernel32, name);
    if (addr == NULL) addr = GetProcAddress(user32, name);
    if (addr == NULL) addr = GetProcAddress(gdi32, name);
    if (addr == NULL) addr = GetProcAddress(opengl32, name);
    if (addr == NULL) addr = GetProcAddress(msvcrt, name);

    // printf("JIT: loaded %s (%p)\n", name, addr);
    nl_map_put_cstr(jit->loaded_funcs, name, addr);
    return addr;
    #else
    return NULL;
    #endif
}

static void* get_symbol_address(const TB_Symbol* s) {
    if (s->tag == TB_SYMBOL_GLOBAL) {
        return ((TB_Global*) s)->address;
    } else if (s->tag == TB_SYMBOL_FUNCTION) {
        return ((TB_Function*) s)->compiled_pos;
    } else {
        tb_todo();
    }
}

void* tb_jit_place_function(TB_JIT* jit, TB_Function* f) {
    TB_FunctionOutput* func_out = f->output;
    if (f->compiled_pos != NULL) {
        return f->compiled_pos;
    }

    // copy machine code
    char* dst = tb_jitheap_alloc_region(&jit->rx_heap, func_out->code_size, 16);
    memcpy(dst, func_out->code, func_out->code_size);
    f->compiled_pos = dst;

    log_debug("jit: apply function %s (%p)", f->super.name, dst);

    // apply relocations, any leftovers are mapped to thunks
    for (TB_SymbolPatch* p = func_out->last_patch; p; p = p->prev) {
        size_t actual_pos = p->pos;
        TB_SymbolTag tag = p->target->tag;

        int32_t* patch = (int32_t*) &dst[actual_pos];
        if (tag == TB_SYMBOL_FUNCTION) {
            TB_Function* f = (TB_Function*) p->target;
            void* addr = tb_jit_place_function(jit, f);

            int32_t rel32 = (intptr_t)addr - ((intptr_t)patch + 4);
            *patch += rel32;
        } else if (tag == TB_SYMBOL_EXTERNAL) {
            TB_External* e = (TB_External*) p->target;

            void* addr = e->thunk ? e->thunk : p->target->address;
            if (addr == NULL) {
                addr = get_proc(jit, p->target->name);
                if (addr == NULL) {
                    tb_panic("Could not find procedure: %s", p->target->name);
                }
            }

            ptrdiff_t rel = (intptr_t)addr - ((intptr_t)patch + 4);
            int32_t rel32 = rel;
            if (rel == rel32) {
                memcpy(dst + actual_pos, &rel32, sizeof(int32_t));
            } else {
                // generate thunk to make far call
                char* thunk = tb_jitheap_alloc_region(&jit->rx_heap, 6 + sizeof(void*), 1);
                thunk[0] = 0xFF; // jmp qword [rip]
                thunk[1] = 0x25;
                thunk[2] = 0x00;
                thunk[3] = 0x00;
                thunk[4] = 0x00;
                thunk[5] = 0x00;

                // write final address into the thunk
                memcpy(thunk + 6, &addr, sizeof(void*));
                e->thunk = thunk;

                int32_t rel32 = (intptr_t)thunk - ((intptr_t)patch + 4);
                *patch += rel32;
            }
        } else if (tag == TB_SYMBOL_GLOBAL) {
            TB_Global* g = (TB_Global*) p->target;
            void* addr = tb_jit_place_global(jit, g);

            int32_t* patch = (int32_t*) &dst[actual_pos];
            int32_t rel32 = (intptr_t)addr - ((intptr_t)patch + 4);
            *patch += rel32;
        } else {
            tb_todo();
        }
    }

    return dst;
}

void* tb_jit_place_global(TB_JIT* jit, TB_Global* g) {
    if (g->address != NULL) {
        return g->address;
    }

    char* data = tb_jitheap_alloc_region(&jit->rw_heap, g->size, g->align);
    g->address = data;

    log_debug("jit: apply global %s (%p)", g->super.name ? g->super.name : "<unnamed>", data);

    memset(data, 0, g->size);
    FOREACH_N(k, 0, g->obj_count) {
        if (g->objects[k].type == TB_INIT_OBJ_REGION) {
            memcpy(&data[g->objects[k].offset], g->objects[k].region.ptr, g->objects[k].region.size);
        }
    }

    FOREACH_N(k, 0, g->obj_count) {
        if (g->objects[k].type == TB_INIT_OBJ_RELOC) {
            uintptr_t addr = (uintptr_t) get_symbol_address(g->objects[k].reloc);

            uintptr_t* dst = (uintptr_t*) &data[g->objects[k].offset];
            *dst += addr;
        }
    }

    return data;
}

TB_JIT* tb_jit_begin(TB_Module* m, size_t jit_heap_capacity) {
    if (jit_heap_capacity == 0) {
        jit_heap_capacity = 2*1024*1024;
    }

    size_t semi_space = jit_heap_capacity / 2;
    char* ptr = tb_platform_valloc(jit_heap_capacity);
    tb_platform_vprotect(ptr, semi_space, TB_PAGE_RXW);

    TB_JIT* jit = tb_platform_heap_alloc(sizeof(TB_JIT));
    *jit = (TB_JIT){
        .rx_heap = tb_jitheap_create(TB_PAGE_RX, ptr, semi_space),
        .rw_heap = tb_jitheap_create(TB_PAGE_RW, &ptr[semi_space], semi_space)
    };

    return jit;
}

void tb_jit_end(TB_JIT* jit) {
    tb_platform_vfree(jit->rx_heap.block, jit->rx_heap.capacity);
    tb_platform_vfree(jit->rw_heap.block, jit->rw_heap.capacity);
    tb_platform_heap_free(jit);
}

void* tb_jit_get_code_ptr(TB_Function* f) {
    return f->compiled_pos;
}
