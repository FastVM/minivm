#include "tb_internal.h"
#include "host.h"
#include <setjmp.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

enum {
    ALLOC_COOKIE = 0xBAADF00D,
    ALLOC_GRANULARITY = 16,
    STACK_SIZE = 2*1024*1024
};

typedef struct {
    uint8_t* pos;

    // when we insert a breakpoint, we're
    // replacing some byte with an INT3 (at
    // least on x86), we need to restore it
    // before continuing execution.
    uint8_t prev_byte;
} TB_Breakpoint;

typedef struct {
    uint32_t cookie;
    uint32_t size; // if low bit is set, we're in use.
    char data[];
} FreeList;

// addr -> symbol
typedef struct {
    uint32_t k;
    void* v;
} Tag;

struct TB_JIT {
    size_t capacity;
    mtx_t lock;
    NL_Strmap(void*) loaded_funcs;

    DynArray(TB_Breakpoint) breakpoints;
    DynArray(Tag) tags;

    FreeList heap;
};

static const char* prot_names[] = {
    "RO", "RW", "RX", "RXW",
};

void tb_jit_tag_object(TB_JIT* jit, void* ptr, void* tag) {
    assert(tag);
    uint32_t offset = (char*) ptr - (char*) jit;

    size_t i = 0, count = dyn_array_length(jit->tags);
    for (; i < count; i++) {
        if (offset < jit->tags[i].k) break;
    }

    // we know where to insert
    dyn_array_put_uninit(jit->tags, 1);
    memmove(&jit->tags[i + 1], &jit->tags[i], (count - i) * sizeof(Tag));
    jit->tags[i] = (Tag){ offset, tag };
}

void* tb_jit_resolve_addr(TB_JIT* jit, void* ptr, uint32_t* out_offset) {
    mtx_lock(&jit->lock);
    ptrdiff_t offset = (char*) ptr - (char*) jit;
    if (offset < 0 || offset >= jit->capacity) goto bad;

    size_t left = 0;
    size_t right = dyn_array_length(jit->tags);
    if (right == 0) goto bad;

    Tag* tags = jit->tags;
    while (left < right) {
        size_t middle = (left + right) / 2;
        if (tags[middle].k > offset) {
            right = middle;
        } else {
            left = middle + 1;
        }
    }

    size_t i = right - 1;
    *out_offset = offset - tags[i].k;
    return tags[i].v;

    bad:
    mtx_unlock(&jit->lock);
    return NULL;
}

void* tb_jit_alloc_obj(TB_JIT* jit, size_t size, size_t align) {
    mtx_lock(&jit->lock);
    size = (size + ALLOC_GRANULARITY - 1) & ~(ALLOC_GRANULARITY - 1);

    FreeList* l = &jit->heap;
    FreeList* end = (FreeList*) ((char*) jit + jit->capacity);
    for (;;) {
        assert(l->cookie == ALLOC_COOKIE);

        // free slot, let's see if it's big enough
        if ((l->size & 1) == 0 && (l->size >> 1) >= size) {
            // if the node is bigger than one alloc, we need to split it
            size_t whole_size = l->size >> 1;
            size_t split_size = sizeof(FreeList) + size;
            if (whole_size > split_size) {
                // split
                l->size = (size << 1) | 1;

                // free leftovers
                FreeList* rest = (FreeList*) &l->data[l->size >> 1];
                rest->cookie = ALLOC_COOKIE;
                rest->size = (whole_size - split_size) << 1;
            } else {
                // take entire piece
                l->size |= 1;
            }

            mtx_unlock(&jit->lock);
            return l->data;
        }

        FreeList* next = (FreeList*) &l->data[l->size >> 1];
        if (next == end) {
            break;
        }

        // coalesce free regions when we see them here, it'll
        // make later walks faster
        while ((l->size & 1) == 0 && (next->size & 1) == 0) {
            size_t new_size = (l->size >> 1) + sizeof(FreeList) + (next->size >> 1);
            l->size = (new_size << 1);

            next = (FreeList*) &l->data[new_size];
        }

        l = next;
    }

    mtx_unlock(&jit->lock);
    return NULL;
}

void tb_jit_free_obj(TB_JIT* jit, void* ptr) {
    FreeList* obj = ((FreeList*) ptr) - 1;
    assert(obj->cookie == ALLOC_COOKIE);
    obj->size &= ~1;
}

void tb_jit_dump_heap(TB_JIT* jit) {
    mtx_lock(&jit->lock);

    FreeList* l = &jit->heap;
    FreeList* next;
    FreeList* end = (FreeList*) ((char*) jit + jit->capacity);

    printf("HEAP:\n");
    size_t tag = 0, tag_count = dyn_array_length(jit->tags);
    char* base = (char*) jit;
    for (;;) {
        if (l->size & 1) {
            printf("* ALLOC [%p %u", l->data, l->size >> 1);

            // found the next tag here
            if (tag < tag_count && l->data == &base[jit->tags[tag].k]) {
                printf(" TAG=%p", jit->tags[tag].v);
                tag += 1;
            }

            printf("]\n");
        }

        next = (FreeList*) &l->data[l->size >> 1];
        if (next == end) {
            break;
        }
        l = next;
    }
    mtx_unlock(&jit->lock);
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
    char* dst = tb_jit_alloc_obj(jit, func_out->code_size, 16);
    memcpy(dst, func_out->code, func_out->code_size);
    f->compiled_pos = dst;

    log_debug("jit: apply function %s (%p)", f->super.name, dst);

    // apply relocations, any leftovers are mapped to thunks
    for (TB_SymbolPatch* p = func_out->first_patch; p; p = p->next) {
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
                char* thunk = tb_jit_alloc_obj(jit, 6 + sizeof(void*), 1);
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

    char* data = tb_jit_alloc_obj(jit, g->size, g->align);
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

    TB_JIT* jit = tb_platform_valloc(jit_heap_capacity);
    mtx_init(&jit->lock, mtx_plain);
    jit->capacity = jit_heap_capacity;
    jit->heap.cookie = ALLOC_COOKIE;
    jit->heap.size = (jit_heap_capacity - sizeof(TB_JIT)) << 1;

    // a lil unsafe... im sorry momma
    tb_platform_vprotect(jit, jit_heap_capacity, TB_PAGE_RXW);
    return jit;
}

void tb_jit_end(TB_JIT* jit) {
    mtx_destroy(&jit->lock);
    tb_platform_vfree(jit, jit->capacity);
}

void* tb_jit_get_code_ptr(TB_Function* f) {
    return f->compiled_pos;
}

////////////////////////////////
// Debugger
////////////////////////////////
#if defined(CUIK__IS_X64) && defined(_WIN32)
struct TB_CPUContext {
    // used for stack crawling
    void* pc;
    void* sp;

    void* old_sp;
    void* except;

    TB_JIT* jit;
    size_t ud_size;

    volatile bool interrupted;
    volatile bool running;

    CONTEXT cont;
    CONTEXT state;

    // safepoint page
    _Alignas(4096) char poll_site[4096];

    char user_data[];
};

typedef struct {
    // win64: rcx rdx r8  r9
    // sysv:  rdi rsi rdx rcx r8 r9
    uint64_t gprs[6];
} X64Params;

// win64 trampoline ( sp pc params ctx -- rax )
__declspec(allocate(".text")) __declspec(align(16))
static const uint8_t tb_jit__trampoline[] = {
    // save old SP, we'll come back for it later
    0x49, 0x89, 0x61, 0x10,                   // mov [r9 + 0x10], rsp
    // use new SP
    0x48, 0x89, 0xCC,                         // mov rsp, rcx
    // shuffle some params into win64 volatile regs
    0x48, 0x89, 0xD0,                         // mov rax, rdx
    0x4D, 0x89, 0xC2,                         // mov r10, r8
    // fill GPR params
    0x49, 0x8B, 0x4A, 0x00,                   // mov rcx, [r10 + 0x00]
    0x49, 0x8B, 0x52, 0x08,                   // mov rdx, [r10 + 0x08]
    0x4D, 0x8B, 0x42, 0x10,                   // mov r8,  [r10 + 0x10]
    0x4D, 0x8B, 0x4A, 0x18,                   // mov r9,  [r10 + 0x18]
    0xFF, 0xD0,                               // call rax
    // restore stack & return normally
    0x48, 0x81, 0xE4, 0x00, 0x00, 0xE0, 0xFF, // and rsp, -0x200000
    0x48, 0x8B, 0x64, 0x24, 0x10,             // mov rsp, [rsp + 0x10]
    0xC3,                                     // ret
};

static LONG except_handler(EXCEPTION_POINTERS* e) {
    TB_CPUContext* cpu = (TB_CPUContext*) (e->ContextRecord->Rsp & -STACK_SIZE);

    // necessary for stack crawling later
    cpu->pc = (void*) e->ContextRecord->Rip;
    cpu->sp = (void*) e->ContextRecord->Rsp;
    cpu->interrupted = true;
    cpu->running = false;

    switch (e->ExceptionRecord->ExceptionCode) {
        case EXCEPTION_GUARD_PAGE: {
            if (e->ExceptionRecord->ExceptionInformation[1] == (uintptr_t) &cpu->poll_site[0]) {
                return EXCEPTION_CONTINUE_SEARCH;
            }

            // we hit a safepoint poll
            void* pc = (void*) e->ContextRecord->Rip;
            // TB_* sp = nl_table_get(cpu->safepoints, pc);

            // TODO(NeGate): copy values out of the regs/stk
            break;
        }

        case EXCEPTION_ACCESS_VIOLATION: {
            // TODO(NeGate): NULLchk segv
            // void* accessed = (void*) e->ExceptionRecord->ExceptionInformation[1];
            printf("PAUSE RIP=%p\n", cpu->pc);
            break;
        }

        case EXCEPTION_BREAKPOINT:
        case EXCEPTION_SINGLE_STEP:
        cpu->state = *e->ContextRecord;
        break;

        default:
        return EXCEPTION_CONTINUE_SEARCH;
    }

    // jump out of the JIT
    *e->ContextRecord = cpu->cont;
    return EXCEPTION_CONTINUE_EXECUTION;
}

TB_CPUContext* tb_jit_thread_create(TB_JIT* jit, size_t ud_size) {
    TB_CPUContext* cpu = tb_jit_stack_create();
    cpu->ud_size = ud_size;
    cpu->jit = jit;
    cpu->except = AddVectoredExceptionHandler(1, except_handler);
    memset(cpu->user_data, 0, ud_size);
    return cpu;
}

void* tb_jit_thread_pc(TB_CPUContext* cpu) { return cpu->pc; }
void* tb_jit_thread_sp(TB_CPUContext* cpu) { return cpu->sp; }

size_t tb_jit_thread_pollsite(void) { return offsetof(TB_CPUContext, poll_site); }

void* tb_jit_thread_get_userdata(TB_CPUContext* cpu) {
    return cpu->user_data;
}

void tb_jit_breakpoint(TB_JIT* jit, void* addr) {
    TB_Breakpoint bp = { addr };
    dyn_array_for(i, jit->breakpoints) {
        if (jit->breakpoints[i].pos == addr) {
            return;
        }
    }

    dyn_array_put(jit->breakpoints, bp);
}

bool tb_jit_thread_step(TB_CPUContext* cpu, uint64_t* ret, uintptr_t pc_start, uintptr_t pc_end) {
    // step until we're out of the current PC region
    DynArray(TB_Breakpoint) bp = cpu->jit->breakpoints;
    uintptr_t rip = cpu->state.Rip;
    while (rip >= pc_start && rip < pc_end) {
        // printf("step %#"PRIxPTR" [%#"PRIxPTR" %#"PRIxPTR"]\n", rip, pc_start, pc_end);

        // install breakpoints
        dyn_array_for(i, bp) {
            uint8_t* code = bp[i].pos;
            if (code != (uint8_t*) rip) {
                bp[i].prev_byte = *code;
                *code = 0xCC;
            }
        }

        // enable trap flag for single-stepping
        cpu->state.EFlags |= 0x100;

        // save our precious restore point
        cpu->running = true;
        cpu->state.ContextFlags = 0x10000f;
        RtlCaptureContext(&cpu->cont);
        if (cpu->running) {
            RtlRestoreContext(&cpu->state, NULL);
        }

        // uninstall breakpoints
        dyn_array_for(i, bp) {
            uint8_t* code = bp[i].pos;
            *code = bp[i].prev_byte;
        }

        rip = cpu->state.Rip;
    }

    uintptr_t t = (uintptr_t) &tb_jit__trampoline;
    if ((rip - t) < sizeof(tb_jit__trampoline)) {
        // we in the trampoline, read RAX for the return
        *ret = cpu->state.Rax;
        return true;
    } else {
        return false;
    }
}

bool tb_jit_thread_call(TB_CPUContext* cpu, void* pc, uint64_t* ret, size_t arg_count, void** args) {
    // save our precious restore point
    cpu->interrupted = false;
    cpu->running = true;
    RtlCaptureContext(&cpu->cont);

    if (cpu->running) {
        // continue in JITted code
        X64Params params;

        #ifdef _WIN32
        enum { ABI_GPR_COUNT = 4 };
        #else
        enum { ABI_GPR_COUNT = 6 };
        #endif

        size_t i = 0;
        for (; i < arg_count && i < ABI_GPR_COUNT; i++) {
            memcpy(&params.gprs[i], &args[i], sizeof(uint64_t));
        }

        size_t stack_usage = 8 + align_up((arg_count > 0 && arg_count < 4 ? 4 : arg_count) * 8, 16);

        // go to stack top (minus whatever alloc'd param space)
        uint8_t* sp = (uint8_t*) cpu;
        sp += 2*1024*1024;
        sp -= stack_usage;

        // fill param slots
        for (; i < arg_count; i++) {
            memcpy(&sp[i*8], &args[i], sizeof(uint64_t));
        }

        // install breakpoints
        DynArray(TB_Breakpoint) bp = cpu->jit->breakpoints;
        dyn_array_for(i, bp) {
            uint8_t* code = bp[i].pos;
            if (code != pc) {
                bp[i].prev_byte = *code;
                *code = 0xCC;
            }
        }

        typedef uint64_t (*Trampoline)(void* sp, void* pc, X64Params* params, TB_CPUContext* cpu);
        uint64_t r = ((Trampoline)tb_jit__trampoline)(sp, pc, &params, cpu);
        if (ret) { *ret = r; }

        // uninstall breakpoints
        dyn_array_for(i, bp) {
            uint8_t* code = bp[i].pos;
            *code = bp[i].prev_byte;
        }

        cpu->running = false;
    }

    return cpu->interrupted;
}
#endif
