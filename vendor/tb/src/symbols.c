#include "tb_internal.h"
#include <stdatomic.h>

TB_SymbolIter tb_symbol_iter(TB_Module* mod) {
    return (TB_SymbolIter){ .info = mod->first_info_in_module, .i = 0 };
}

TB_Symbol* tb_symbol_iter_next(TB_SymbolIter* iter) {
    for (TB_ThreadInfo* info = iter->info; info != NULL; info = info->next_in_module) {
        size_t cap = 1ull << info->symbols.exp;
        for (size_t i = iter->i; i < cap; i++) {
            void* ptr = info->symbols.data[i];
            if (ptr == NULL) continue;
            if (ptr == NL_HASHSET_TOMB) continue;

            iter->i = i + 1;
            iter->info = info;
            return (TB_Symbol*) ptr;
        }
    }

    return NULL;
}

void tb_module_kill_symbol(TB_Module* m, TB_Symbol* sym) {
}

void tb_symbol_append(TB_Module* m, TB_Symbol* s) {
    atomic_fetch_add(&m->symbol_count[s->tag], 1);

    // append to thread's symbol table
    TB_ThreadInfo* info = tb_thread_info(m);
    {
        mtx_lock(&info->symbol_lock);
        if (info->symbols.data == NULL) {
            info->symbols = nl_hashset_alloc(256);
        }

        s->info = info;
        nl_hashset_put(&info->symbols, s);
        mtx_unlock(&info->symbol_lock);
    }
}

TB_Symbol* tb_symbol_alloc(TB_Module* m, TB_SymbolTag tag, ptrdiff_t len, const char* name, size_t size) {
    // TODO(NeGate): probably wanna have a custom heap for the symbol table
    assert(tag != TB_SYMBOL_NONE);
    TB_Symbol* s = tb_platform_heap_alloc(size);
    memset(s, 0, size);

    s->tag = tag;
    s->name = tb__arena_strdup(m, len, name);
    s->module = m;
    tb_symbol_append(m, s);
    return s;
}

// converts external into global
TB_Global* tb_extern_transmute(TB_External* e, TB_DebugType* dbg_type, TB_Linkage linkage) {
    TB_SymbolTag tag = TB_SYMBOL_EXTERNAL;
    if (!atomic_compare_exchange_strong(&e->super.tag, &tag, TB_SYMBOL_GLOBAL)) {
        // we're already an external
        return NULL;
    }

    TB_Module* m = e->super.module;
    TB_ThreadInfo* new_info = tb_thread_info(m);
    TB_ThreadInfo* old_info = e->super.info;

    // migrate counts, i bet CPUs don't like me btw but they
    // don't like LLVM either so im comparatively good.
    atomic_fetch_sub(&m->symbol_count[TB_SYMBOL_EXTERNAL], 1);
    atomic_fetch_add(&m->symbol_count[TB_SYMBOL_GLOBAL],   1);

    if (old_info == new_info) {
        // fast path, we're on the same thread so we don't move it
    } else {
        // i love double-locking
        mtx_lock(&new_info->symbol_lock);
        mtx_lock(&old_info->symbol_lock);

        if (new_info->symbols.data == NULL) {
            new_info->symbols = nl_hashset_alloc(512);
        }

        nl_hashset_remove(&old_info->symbols, e);
        nl_hashset_put(&new_info->symbols, e);

        mtx_unlock(&old_info->symbol_lock);
        mtx_unlock(&new_info->symbol_lock);
    }

    // convert into global
    TB_Global* g = (TB_Global*) e;
    g->super.info = new_info;
    g->dbg_type = dbg_type;
    g->linkage = linkage;
    return g;
}

TB_API TB_Function* tb_symbol_as_function(TB_Symbol* s) {
    return s && s->tag == TB_SYMBOL_FUNCTION ? (TB_Function*) s : NULL;
}

TB_API TB_External* tb_symbol_as_external(TB_Symbol* s) {
    return s && s->tag == TB_SYMBOL_EXTERNAL ? (TB_External*) s : NULL;
}

TB_API TB_Global* tb_symbol_as_global(TB_Symbol* s) {
    return s && s->tag == TB_SYMBOL_GLOBAL ? (TB_Global*) s : NULL;
}
