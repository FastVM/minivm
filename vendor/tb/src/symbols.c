#include "tb_internal.h"
#include <stdatomic.h>

TB_SymbolIter tb_symbol_iter(TB_Module* mod) {
    return (TB_SymbolIter){ .info = mod->first_info_in_module, .i = 0 };
}

TB_Symbol* tb_symbol_iter_next(TB_SymbolIter* iter) {
    TB_ThreadInfo* info = iter->info;
    while (info) {
        size_t cap = 1ull << info->symbols.exp;
        for (size_t i = iter->i; i < cap; i++) {
            void* ptr = info->symbols.data[i];
            if (ptr == NULL) continue;
            if (ptr == NL_HASHSET_TOMB) continue;

            iter->i = i + 1;
            iter->info = info;
            return (TB_Symbol*) ptr;
        }

        info = atomic_load_explicit(&info->next_in_module, memory_order_relaxed);
    }

    return NULL;
}

void tb_module_kill_symbol(TB_Module* m, TB_Symbol* sym) {
}

void tb_symbol_append(TB_Module* m, TB_Symbol* s) {
    // append to thread's symbol table
    TB_ThreadInfo* info = tb_thread_info(m);
    if (info->symbols.data == NULL) {
        info->symbols = nl_hashset_alloc(256);
    }

    s->info = info;
    nl_hashset_put(&info->symbols, s);
    atomic_fetch_add(&m->symbol_count[s->tag], 1);
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

TB_API bool tb_extern_resolve(TB_External* e, TB_Symbol* sym) {
    TB_Symbol* expected = NULL;
    if (atomic_compare_exchange_strong(&e->resolved, &expected, sym)) {
        return true;
    } else {
        return false;
    }
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
