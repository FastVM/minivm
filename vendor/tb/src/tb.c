#include "tb_internal.h"
#include "host.h"
#include "opt/passes.h"

static ICodeGen* tb__find_code_generator(TB_Module* m) {
    // Place all the codegen interfaces down here
    extern ICodeGen tb__x64_codegen;
    extern ICodeGen tb__aarch64_codegen;

    switch (m->target_arch) {
        #ifdef TB_HAS_X64
        case TB_ARCH_X86_64:  return &tb__x64_codegen;
        #endif

        #ifdef TB_HAS_AARCH64
        case TB_ARCH_AARCH64: return &tb__aarch64_codegen;
        #endif

        default: return NULL;
    }
}

TB_ThreadInfo* tb_thread_info(TB_Module* m) {
    static thread_local TB_ThreadInfo* chain;
    static thread_local mtx_t lock;
    static thread_local bool init;

    if (!init) {
        init = true;
        mtx_init(&lock, mtx_plain);
    }

    // there shouldn't really be contention here
    mtx_lock(&lock);

    // almost always refers to one TB_ThreadInfo, but
    // we can't assume the user has merely on TB_Module
    // per thread.
    TB_ThreadInfo* info = chain;
    while (info != NULL) {
        if (info->owner == m) {
            goto done;
        }
        info = info->next;
    }

    CUIK_TIMED_BLOCK("alloc thread info") {
        info = tb_platform_heap_alloc(sizeof(TB_ThreadInfo));
        *info = (TB_ThreadInfo){ .owner = m, .chain = &chain, .lock = &lock };

        // allocate memory for it
        tb_arena_create(&info->perm_arena, TB_ARENA_LARGE_CHUNK_SIZE);
        tb_arena_create(&info->tmp_arena, TB_ARENA_LARGE_CHUNK_SIZE);

        // thread local so it doesn't need to synchronize
        info->next = chain;
        if (chain != NULL) {
            chain->prev = info;
        }
        chain = info;

        // link to the TB_Module* (we need to this to free later)
        TB_ThreadInfo* old_top;
        do {
            old_top = atomic_load(&m->first_info_in_module);
            info->next_in_module = old_top;
        } while (!atomic_compare_exchange_strong(&m->first_info_in_module, &old_top, info));
    }

    done:
    mtx_unlock(&lock);
    return info;
}

static thread_local uint8_t* tb_thread_storage;

// we don't modify these strings
char* tb__arena_strdup(TB_Module* m, ptrdiff_t len, const char* src) {
    if (len < 0) len = src ? strlen(src) : 0;
    if (len == 0) return (char*) "";

    char* newstr = tb_arena_alloc(get_permanent_arena(m), len + 1);
    memcpy(newstr, src, len);
    newstr[len] = 0;
    return newstr;
}

TB_Module* tb_module_create_for_host(bool is_jit) {
    #if defined(TB_HOST_X86_64)
    TB_Arch arch = TB_ARCH_X86_64;
    #else
    TB_Arch arch = TB_ARCH_UNKNOWN;
    tb_panic("tb_module_create_for_host: cannot detect host platform");
    #endif

    #if defined(TB_HOST_WINDOWS)
    TB_System sys = TB_SYSTEM_WINDOWS;
    #elif defined(TB_HOST_OSX)
    TB_System sys = TB_SYSTEM_MACOS;
    #elif defined(TB_HOST_LINUX)
    TB_System sys = TB_SYSTEM_LINUX;
    #else
    tb_panic("tb_module_create_for_host: cannot detect host platform");
    #endif

    return tb_module_create(arch, sys, is_jit);
}

TB_ModuleSectionHandle tb_module_create_section(TB_Module* m, ptrdiff_t len, const char* name, TB_ModuleSectionFlags flags, TB_ComdatType comdat) {
    size_t i = dyn_array_length(m->sections);
    dyn_array_put_uninit(m->sections, 1);

    TB_ModuleSection* sec = &m->sections[i];
    *sec = (TB_ModuleSection){
        .name = tb__arena_strdup(m, len, name),
        .flags = flags,
        .comdat = { comdat },
    };
    return i;
}

TB_Module* tb_module_create(TB_Arch arch, TB_System sys, bool is_jit) {
    TB_Module* m = tb_platform_heap_alloc(sizeof(TB_Module));
    if (m == NULL) {
        fprintf(stderr, "tb_module_create: Out of memory!\n");
        return NULL;
    }
    memset(m, 0, sizeof(TB_Module));

    m->is_jit = is_jit;

    m->target_abi = (sys == TB_SYSTEM_WINDOWS) ? TB_ABI_WIN64 : TB_ABI_SYSTEMV;
    m->target_arch = arch;
    m->target_system = sys;
    m->codegen = tb__find_code_generator(m);

    mtx_init(&m->lock, mtx_plain);

    // AOT uses sections to know where to organize things in an executable file,
    // JIT does placement on the fly.
    if (!is_jit) {
        bool win = sys == TB_SYSTEM_WINDOWS;
        tb_module_create_section(m, -1, ".text",                    TB_MODULE_SECTION_EXEC,                          TB_COMDAT_NONE);
        tb_module_create_section(m, -1, ".data",                    TB_MODULE_SECTION_WRITE,                         TB_COMDAT_NONE);
        tb_module_create_section(m, -1, win ? ".rdata" : ".rodata", 0,                                               TB_COMDAT_NONE);
        tb_module_create_section(m, -1, win ? ".tls$"  : ".tls",    TB_MODULE_SECTION_WRITE | TB_MODULE_SECTION_TLS, TB_COMDAT_NONE);

        if (win) {
            m->chkstk_extern = (TB_Symbol*) tb_extern_create(m, -1, "__chkstk", TB_EXTERNAL_SO_LOCAL);
        }
    } else {
        if (sys == TB_SYSTEM_WINDOWS) {
            #ifdef _WIN32
            extern void __chkstk(void);

            // fill it with whatever MSVC/Clang gave us
            m->chkstk_extern = (TB_Symbol*) tb_extern_create(m, -1, "__chkstk", TB_EXTERNAL_SO_LOCAL);
            m->chkstk_extern->address = __chkstk;
            #endif
        }
    }

    return m;
}

TB_FunctionOutput* tb_pass_codegen(TB_Passes* p, TB_Arena* arena, const TB_FeatureSet* features, bool emit_asm) {
    TB_Function* f = p->f;
    TB_Module* m = f->super.module;

    TB_FunctionOutput* func_out = tb_arena_alloc(arena, sizeof(TB_FunctionOutput));
    *func_out = (TB_FunctionOutput){ .parent = f, .section = f->section, .linkage = f->linkage };
    m->codegen->compile_function(p, func_out, features, arena, emit_asm);
    atomic_fetch_add(&m->compiled_function_count, 1);

    f->output = func_out;
    return func_out;
}

void tb_output_print_asm(TB_FunctionOutput* out, FILE* fp) {
    if (fp == NULL) {
        fp = stdout;
    }

    TB_Assembly* a = tb_output_get_asm(out);
    for (; a; a = a->next) {
        fwrite(a->data, a->length, 1, fp);
    }
}

TB_Location* tb_output_get_locations(TB_FunctionOutput* out, size_t* out_count) {
    *out_count = dyn_array_length(out->locations);
    return &out->locations[0];
}

uint8_t* tb_output_get_code(TB_FunctionOutput* out, size_t* out_length) {
    *out_length = out->code_size;
    return out->code;
}

TB_Assembly* tb_output_get_asm(TB_FunctionOutput* out) {
    return out->asm_out;
}

TB_Arena* tb_function_get_arena(TB_Function* f) {
    return f->arena;
}

void tb_module_destroy(TB_Module* m) {
    // free thread info's arena
    TB_ThreadInfo* info = atomic_load(&m->first_info_in_module);
    while (info != NULL) {
        TB_ThreadInfo* next = info->next_in_module;

        // free symbols
        nl_hashset_free(info->symbols);

        tb_arena_destroy(&info->tmp_arena);
        tb_arena_destroy(&info->perm_arena);

        // unlink, this needs to be synchronized in case another thread is
        // accessing while we're freeing.
        mtx_lock(info->lock);
        if (info->prev == NULL) {
            *info->chain = info->next;
        } else {
            info->prev->next = info->next;
        }
        mtx_unlock(info->lock);

        tb_platform_heap_free(info);
        info = next;
    }

    dyn_array_destroy(m->files);
    tb_platform_heap_free(m);
}

TB_SourceFile* tb_get_source_file(TB_Module* m, ptrdiff_t len, const char* path) {
    mtx_lock(&m->lock);

    if (len <= 0) {
        len = path ? strlen(path) : 0;
    }

    NL_Slice key = {
        .length = len,
        .data = (const uint8_t*) path,
    };

    TB_SourceFile* file;
    ptrdiff_t search = nl_map_get(m->files, key);
    if (search < 0) {
        file = tb_arena_alloc(get_permanent_arena(m), sizeof(TB_SourceFile) + key.length + 1);
        file->id = -1;
        file->len = key.length;

        memcpy(file->path, key.data, key.length);
        key.data = file->path;

        nl_map_put(m->files, key, file);
    } else {
        file = m->files[search].v;
    }
    mtx_unlock(&m->lock);
    return file;
}

TB_FunctionPrototype* tb_prototype_create(TB_Module* m, TB_CallingConv cc, size_t param_count, const TB_PrototypeParam* params, size_t return_count, const TB_PrototypeParam* returns, bool has_varargs) {
    size_t size = sizeof(TB_FunctionPrototype) + ((param_count + return_count) * sizeof(TB_PrototypeParam));
    TB_FunctionPrototype* p = tb_arena_alloc(get_permanent_arena(m), size);

    p->call_conv = cc;
    p->param_count = param_count;
    p->has_varargs = has_varargs;
    if (param_count > 0) {
        memcpy(p->params, params, param_count * sizeof(TB_PrototypeParam));
    }
    if (return_count > 0 && !TB_IS_VOID_TYPE(returns[0].dt)) {
        memcpy(p->params + param_count, returns, return_count * sizeof(TB_PrototypeParam));
        p->return_count = return_count;
    } else {
        p->return_count = 0;
    }
    return p;
}

TB_Function* tb_function_create(TB_Module* m, ptrdiff_t len, const char* name, TB_Linkage linkage) {
    TB_Function* f = (TB_Function*) tb_symbol_alloc(m, TB_SYMBOL_FUNCTION, len, name, sizeof(TB_Function));
    f->linkage = linkage;
    return f;
}

void tb_symbol_set_name(TB_Symbol* s, ptrdiff_t len, const char* name) {
    s->name = tb__arena_strdup(s->module, len, name);
}

const char* tb_symbol_get_name(TB_Symbol* s) {
    return s->name;
}

void tb_function_set_prototype(TB_Function* f, TB_ModuleSectionHandle section, TB_FunctionPrototype* p, TB_Arena* arena) {
    assert(f->prototype == NULL);
    size_t param_count = p->param_count;

    if (arena == NULL) {
        f->arena = tb_platform_heap_alloc(sizeof(TB_Arena));
        tb_arena_create(f->arena, TB_ARENA_SMALL_CHUNK_SIZE);
    } else {
        f->arena = arena;
    }

    f->gvn_nodes = nl_hashset_alloc(16);
    f->locations = nl_table_alloc(16);

    f->section = section;
    f->node_count = 0;
    TB_Node* root = f->root_node = tb_alloc_node(f, TB_ROOT, TB_TYPE_TUPLE, 4 + p->return_count, 0);

    f->param_count = param_count;
    f->params = tb_arena_alloc(f->arena, (4 + param_count) * sizeof(TB_Node*));

    // fill in acceleration structure
    f->params[0] = tb__make_proj(f, TB_TYPE_CONTROL, f->root_node, 0);
    f->params[1] = tb__make_proj(f, TB_TYPE_MEMORY, f->root_node, 1);
    f->params[2] = tb__make_proj(f, TB_TYPE_PTR, f->root_node, 2);

    // initial trace
    f->trace.top_ctrl = f->trace.bot_ctrl = f->params[0];
    f->trace.mem = f->params[1];

    // create parameter projections
    TB_PrototypeParam* rets = TB_PROTOTYPE_RETURNS(p);
    FOREACH_N(i, 0, param_count) {
        TB_DataType dt = p->params[i].dt;
        f->params[3+i] = tb__make_proj(f, dt, f->root_node, 3+i);
    }

    // create callgraph node
    TB_Node* callgraph = tb_alloc_node_dyn(f, TB_CALLGRAPH, TB_TYPE_VOID, 1, 8, sizeof(TB_NodeRegion));
    set_input(f, callgraph, root, 0);

    // fill return crap
    {
        TB_Node* region = tb_alloc_node_dyn(f, TB_REGION, TB_TYPE_CONTROL, 0, 4, sizeof(TB_NodeRegion));
        TB_Node* mem_phi = tb_alloc_node_dyn(f, TB_PHI, TB_TYPE_MEMORY, 1, 5, 0);
        set_input(f, mem_phi, region, 0);

        set_input(f, root, region, 0);
        set_input(f, root, mem_phi, 1);
        set_input(f, root, f->params[2], 2);
        set_input(f, root, callgraph, 3);

        TB_PrototypeParam* returns = TB_PROTOTYPE_RETURNS(p);
        FOREACH_N(i, 0, p->return_count) {
            TB_Node* phi = tb_alloc_node_dyn(f, TB_PHI, returns[i].dt, 1, 5, 0);
            set_input(f, phi, region, 0);
            set_input(f, root, phi, i + 4);
        }

        TB_NODE_SET_EXTRA(region, TB_NodeRegion, .freq = 1.0f, .mem_in = mem_phi, .tag = "ret");
    }

    f->prototype = p;
}

TB_FunctionPrototype* tb_function_get_prototype(TB_Function* f) {
    return f->prototype;
}

void* tb_global_add_region(TB_Module* m, TB_Global* g, size_t offset, size_t size) {
    assert(offset == (uint32_t)offset);
    assert(size == (uint32_t)size);
    assert(g->obj_count + 1 <= g->obj_capacity);

    void* ptr = tb_platform_heap_alloc(size);
    g->objects[g->obj_count++] = (TB_InitObj) {
        .type = TB_INIT_OBJ_REGION, .offset = offset, .region = { .size = size, .ptr = ptr }
    };

    return ptr;
}

void tb_global_add_symbol_reloc(TB_Module* m, TB_Global* g, size_t offset, TB_Symbol* symbol) {
    assert(offset == (uint32_t) offset);
    assert(g->obj_count + 1 <= g->obj_capacity);
    assert(symbol != NULL);

    g->objects[g->obj_count++] = (TB_InitObj) { .type = TB_INIT_OBJ_RELOC, .offset = offset, .reloc = symbol };
}

TB_Global* tb_global_create(TB_Module* m, ptrdiff_t len, const char* name, TB_DebugType* dbg_type, TB_Linkage linkage) {
    TB_Global* g = tb_arena_alloc(get_permanent_arena(m), sizeof(TB_Global));
    *g = (TB_Global){
        .super = {
            .tag = TB_SYMBOL_GLOBAL,
            .name = tb__arena_strdup(m, len, name),
            .module = m,
        },
        .dbg_type = dbg_type,
        .linkage = linkage
    };
    tb_symbol_append(m, (TB_Symbol*) g);

    return g;
}

void tb_global_set_storage(TB_Module* m, TB_ModuleSectionHandle section, TB_Global* global, size_t size, size_t align, size_t max_objects) {
    assert(size > 0 && align > 0 && tb_is_power_of_two(align));
    global->parent = section;
    global->pos = 0;
    global->size = size;
    global->align = align;
    global->obj_count = 0;
    global->obj_capacity = max_objects;
    global->objects = TB_ARENA_ARR_ALLOC(get_permanent_arena(m), max_objects, TB_InitObj);
}

TB_Global* tb__small_data_intern(TB_Module* m, size_t len, const void* data) {
    assert(len <= 16);

    // copy into SmallConst
    SmallConst c = { .len = len };
    memcpy(c.data, data, c.len);

    mtx_lock(&m->lock);
    ptrdiff_t search = nl_map_get(m->global_interns, c);

    TB_Global* g;
    if (search >= 0) {
        g = m->global_interns[search].v;
    } else {
        g = tb_global_create(m, 0, NULL, NULL, TB_LINKAGE_PRIVATE);
        g->super.ordinal = *((uint64_t*) &c.data);
        tb_global_set_storage(m, tb_module_get_rdata(m), g, len, len, 1);

        char* buffer = tb_global_add_region(m, g, 0, len);
        memcpy(buffer, data, len);
        nl_map_put(m->global_interns, c, g);
    }
    mtx_unlock(&m->lock);
    return g;
}

TB_Safepoint* tb_safepoint_get(TB_Function* f, uint32_t relative_ip) {
    /*size_t left = 0;
    size_t right = f->safepoint_count;

    uint32_t ip = relative_ip;
    const TB_SafepointKey* keys = f->output->safepoints;
    while (left < right) {
        size_t middle = (left + right) / 2;

        if (keys[middle].ip == ip) return keys[left].sp;
        if (keys[middle].ip < ip) left = middle + 1;
        else right = middle;
    }*/

    return NULL;
}

TB_ModuleSectionHandle tb_module_get_text(TB_Module* m)  { return 0; }
TB_ModuleSectionHandle tb_module_get_data(TB_Module* m)  { return 1; }
TB_ModuleSectionHandle tb_module_get_rdata(TB_Module* m) { return 2; }
TB_ModuleSectionHandle tb_module_get_tls(TB_Module* m)   { return 3; }

void tb_module_set_tls_index(TB_Module* m, ptrdiff_t len, const char* name) {
    if (atomic_flag_test_and_set(&m->is_tls_defined)) {
        m->tls_index_extern = (TB_Symbol*) tb_extern_create(m, len, name, TB_EXTERNAL_SO_LOCAL);
    }
}

void tb_symbol_bind_ptr(TB_Symbol* s, void* ptr) {
    s->address = ptr;
}

TB_ExternalType tb_extern_get_type(TB_External* e) {
    return e->type;
}

TB_External* tb_extern_create(TB_Module* m, ptrdiff_t len, const char* name, TB_ExternalType type) {
    assert(name != NULL);

    TB_External* e = tb_arena_alloc(get_permanent_arena(m), sizeof(TB_External));
    *e = (TB_External){
        .super = {
            .tag = TB_SYMBOL_EXTERNAL,
            .name = tb__arena_strdup(m, len, name),
            .module = m,
        },
        .type = type,
    };
    tb_symbol_append(m, (TB_Symbol*) e);
    return e;
}

//
// TLS - Thread local storage
//
// Certain backend elements require memory but we would prefer to avoid
// making any heap allocations when possible to there's a preallocated
// block per thread that can run TB.
//
void tb_free_thread_resources(void) {
    if (tb_thread_storage != NULL) {
        tb_platform_vfree(tb_thread_storage, TB_TEMPORARY_STORAGE_SIZE);
        tb_thread_storage = NULL;
    }
}

void tb_emit_symbol_patch(TB_FunctionOutput* func_out, TB_Symbol* target, size_t pos) {
    TB_Module* m = func_out->parent->super.module;
    TB_SymbolPatch* p = TB_ARENA_ALLOC(get_permanent_arena(m), TB_SymbolPatch);

    // function local, no need to synchronize
    *p = (TB_SymbolPatch){ .target = target, .pos = pos };
    if (func_out->first_patch == NULL) {
        func_out->first_patch = func_out->last_patch = p;
    } else {
        func_out->last_patch->next = p;
        func_out->last_patch = p;
    }
    func_out->patch_count += 1;
}

// EMITTER CODE:
//   Simple linear allocation for the
//   backend's to output code with.
void* tb_out_reserve(TB_Emitter* o, size_t count) {
    if (o->count + count >= o->capacity) {
        if (o->capacity == 0) {
            o->capacity = 64;
        } else {
            o->capacity += count;
            o->capacity *= 2;
        }

        o->data = tb_platform_heap_realloc(o->data, o->capacity);
        if (o->data == NULL) tb_todo();
    }

    return &o->data[o->count];
}

void tb_out_commit(TB_Emitter* o, size_t count) {
    assert(o->count + count < o->capacity);
    o->count += count;
}

size_t tb_out_get_pos(TB_Emitter* o, void* p) {
    return (uint8_t*)p - o->data;
}

void* tb_out_grab(TB_Emitter* o, size_t count) {
    void* p = tb_out_reserve(o, count);
    o->count += count;

    return p;
}

void* tb_out_get(TB_Emitter* o, size_t pos) {
    assert(pos < o->count);
    return &o->data[pos];
}

size_t tb_out_grab_i(TB_Emitter* o, size_t count) {
    tb_out_reserve(o, count);

    size_t old = o->count;
    o->count += count;
    return old;
}

void tb_out1b_UNSAFE(TB_Emitter* o, uint8_t i) {
    assert(o->count + 1 < o->capacity);

    o->data[o->count] = i;
    o->count += 1;
}

void tb_out4b_UNSAFE(TB_Emitter* o, uint32_t i) {
    tb_out_reserve(o, 4);

    *((uint32_t*)&o->data[o->count]) = i;
    o->count += 4;
}

void tb_out1b(TB_Emitter* o, uint8_t i) {
    tb_out_reserve(o, 1);

    o->data[o->count] = i;
    o->count += 1;
}

void tb_out2b(TB_Emitter* o, uint16_t i) {
    tb_out_reserve(o, 2);

    *((uint16_t*)&o->data[o->count]) = i;
    o->count += 2;
}

void tb_out4b(TB_Emitter* o, uint32_t i) {
    tb_out_reserve(o, 4);

    *((uint32_t*)&o->data[o->count]) = i;
    o->count += 4;
}

void tb_patch1b(TB_Emitter* o, uint32_t pos, uint8_t i) {
    *((uint8_t*)&o->data[pos]) = i;
}

void tb_patch2b(TB_Emitter* o, uint32_t pos, uint16_t i) {
    *((uint16_t*)&o->data[pos]) = i;
}

void tb_patch4b(TB_Emitter* o, uint32_t pos, uint32_t i) {
    assert(pos + 4 <= o->count);
    *((uint32_t*)&o->data[pos]) = i;
}

void tb_patch8b(TB_Emitter* o, uint32_t pos, uint64_t i) {
    *((uint64_t*)&o->data[pos]) = i;
}

uint8_t tb_get1b(TB_Emitter* o, uint32_t pos) {
    return *((uint8_t*)&o->data[pos]);
}

uint16_t tb_get2b(TB_Emitter* o, uint32_t pos) {
    return *((uint16_t*)&o->data[pos]);
}

uint32_t tb_get4b(TB_Emitter* o, uint32_t pos) {
    return *((uint32_t*)&o->data[pos]);
}

void tb_out8b(TB_Emitter* o, uint64_t i) {
    tb_out_reserve(o, 8);

    *((uint64_t*)&o->data[o->count]) = i;
    o->count += 8;
}

void tb_out_zero(TB_Emitter* o, size_t len) {
    tb_out_reserve(o, len);
    memset(&o->data[o->count], 0, len);
    o->count += len;
}

size_t tb_outstr_nul_UNSAFE(TB_Emitter* o, const char* str) {
    size_t start = o->count;

    for (; *str; str++) {
        o->data[o->count++] = *str;
    }

    o->data[o->count++] = 0;
    return start;
}

void tb_outstr_UNSAFE(TB_Emitter* o, const char* str) {
    while (*str) o->data[o->count++] = *str++;
}

size_t tb_outs(TB_Emitter* o, size_t len, const void* str) {
    tb_out_reserve(o, len);
    size_t start = o->count;

    memcpy(&o->data[o->count], str, len);
    o->count += len;
    return start;
}

void tb_outs_UNSAFE(TB_Emitter* o, size_t len, const void* str) {
    memcpy(&o->data[o->count], str, len);
    o->count += len;
}
