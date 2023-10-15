#include "linker.h"

#if __STDC_VERSION__ < 201112L || defined(__STDC_NO_ATOMICS__)
#error "Missing C11 support for stdatomic.h"
#endif

#include <stdatomic.h>

extern TB_LinkerVtbl tb__linker_pe, tb__linker_elf;

TB_API TB_ExecutableType tb_system_executable_format(TB_System s) {
    switch (s) {
        case TB_SYSTEM_WINDOWS: return TB_EXECUTABLE_PE;
        case TB_SYSTEM_LINUX:   return TB_EXECUTABLE_ELF;
        default: tb_todo();     return TB_EXECUTABLE_UNKNOWN;
    }
}

TB_API TB_Linker* tb_linker_create(TB_ExecutableType exe, TB_Arch arch) {
    TB_Linker* l = tb_platform_heap_alloc(sizeof(TB_Linker));
    memset(l, 0, sizeof(TB_Linker));
    l->target_arch = arch;
    l->messages = tb_platform_heap_alloc((1u << QEXP) * sizeof(TB_LinkerMsg));

    l->symtab.exp = 24;
    CUIK_TIMED_BLOCK("tb_platform_valloc") {
        l->symtab.ht = tb_platform_valloc((1u << l->symtab.exp) * sizeof(TB_LinkerSymbol));
    }

    TB_LinkerInput null_entry = { 0 };
    dyn_array_put(l->inputs, null_entry);

    switch (exe) {
        case TB_EXECUTABLE_PE:  l->vtbl = tb__linker_pe;  break;
        case TB_EXECUTABLE_ELF: l->vtbl = tb__linker_elf; break;
        default: break;
    }

    l->vtbl.init(l);
    return l;
}

void tb_linker_send_msg(TB_Linker* l, TB_LinkerMsg* msg) {
    ptrdiff_t i = 0;
    for (;;) {
        // might wanna change the memory order on this atomic op
        uint32_t r = l->queue;

        uint32_t mask = (1u << QEXP) - 1;
        uint32_t head = r & mask;
        uint32_t tail = (r >> 16) & mask;
        uint32_t next = (head + 1u) & mask;
        if (r & 0x8000) { // avoid overflow on commit
            l->queue &= ~0x8000;
        }

        // it don't fit...
        if (next != tail) {
            i = head;
            break;
        }
    }

    l->messages[i] = *msg;
    l->queue_count += 1;
    l->queue += 1;
}

TB_API bool tb_linker_get_msg(TB_Linker* l, TB_LinkerMsg* out_msg) {
    // pop work off the queue
    uint32_t r;
    TB_LinkerMsg* msg = NULL;
    do {
        r = l->queue;
        uint32_t mask = (1u << QEXP) - 1;
        uint32_t head = r & mask;
        uint32_t tail = (r >> 16) & mask;

        if (head == tail) {
            return false;
        }

        // copy out before we commit
        *out_msg = l->messages[tail];

        // don't continue until we successfully commited the queue pop
    } while (!atomic_compare_exchange_strong(&l->queue, &r, r + 0x10000));

    l->queue_count -= 1;
    return true;
}

TB_API void tb_linker_set_subsystem(TB_Linker* l, TB_WindowsSubsystem subsystem) {
    l->subsystem = subsystem;
}

TB_API void tb_linker_set_entrypoint(TB_Linker* l, const char* name) {
    l->entrypoint = name;
}

TB_API void tb_linker_append_object(TB_Linker* l, TB_Slice obj_name, TB_Slice content) {
    l->vtbl.append_object(l, obj_name, content);
}

TB_API void tb_linker_append_module(TB_Linker* l, TB_Module* m) {
    l->vtbl.append_module(l, m);
}

TB_API void tb_linker_append_library(TB_Linker* l, TB_Slice ar_name, TB_Slice ar_file) {
    l->vtbl.append_library(l, ar_name, ar_file);
}

TB_API TB_ExportBuffer tb_linker_export(TB_Linker* l) {
    return l->vtbl.export(l);
}

TB_API void tb_linker_destroy(TB_Linker* l) {
    tb_platform_heap_free(l);
}

TB_LinkerSectionPiece* tb__get_piece(TB_Linker* l, TB_LinkerSymbol* restrict sym) {
    if (sym && (sym->tag == TB_LINKER_SYMBOL_NORMAL || sym->tag == TB_LINKER_SYMBOL_TB)) {
        return sym->normal.piece;
    }

    return NULL;
}

size_t tb__get_symbol_pos(TB_Symbol* s) {
    if (s->tag == TB_SYMBOL_FUNCTION) {
        return ((TB_Function*) s)->output->code_pos;
    } else if (s->tag == TB_SYMBOL_GLOBAL) {
        return ((TB_Global*) s)->pos;
    } else {
        tb_todo();
    }
}

uint64_t tb__get_symbol_rva(TB_Linker* l, TB_LinkerSymbol* sym) {
    if (sym->tag == TB_LINKER_SYMBOL_ABSOLUTE) {
        return 0;
    } else if (sym->tag == TB_LINKER_SYMBOL_IMAGEBASE) {
        return sym->imagebase;
    }

    // normal or TB
    assert(sym->tag == TB_LINKER_SYMBOL_NORMAL || sym->tag == TB_LINKER_SYMBOL_TB);
    TB_LinkerSectionPiece* piece = sym->normal.piece;

    uint32_t rva = piece->parent->address + piece->offset;
    if (sym->tag == TB_LINKER_SYMBOL_NORMAL) {
        return rva + sym->normal.secrel;
    }

    TB_Symbol* s = sym->tb.sym;
    if (s->tag == TB_SYMBOL_FUNCTION) {
        TB_Function* f = (TB_Function*) s;
        assert(f->output != NULL);

        return rva + f->output->code_pos;
    } else if (s->tag == TB_SYMBOL_GLOBAL) {
        return rva + ((TB_Global*) s)->pos;
    } else {
        tb_todo();
    }
}

uint64_t tb__compute_rva(TB_Linker* l, TB_Module* m, const TB_Symbol* s) {
    if (s->tag == TB_SYMBOL_FUNCTION) {
        TB_Function* f = (TB_Function*) s;
        assert(f->output != NULL);

        TB_LinkerSectionPiece* piece = m->sections[f->section].piece;
        return piece->parent->address + piece->offset + f->output->code_pos;
    } else if (s->tag == TB_SYMBOL_GLOBAL) {
        TB_Global* g = (TB_Global*) s;
        TB_LinkerSectionPiece* piece = m->sections[g->parent].piece;
        return piece->parent->address + piece->offset + g->pos;
    } else {
        tb_todo();
        return 0;
    }
}

size_t tb__pad_file(uint8_t* output, size_t write_pos, char pad, size_t align) {
    size_t align_mask = align - 1;
    size_t end = (write_pos + align_mask) & ~align_mask;
    if (write_pos != end) {
        memset(output + write_pos, 0, end - write_pos);
        write_pos = end;
    }
    return write_pos;
}

void tb__merge_sections(TB_Linker* linker, TB_LinkerSection* from, TB_LinkerSection* to) {
    if (from == NULL || to == NULL) return;
    if (from->generic_flags & TB_LINKER_SECTION_DISCARD) return;

    // remove 'from' from final output
    from->generic_flags |= TB_LINKER_SECTION_DISCARD;

    if (to->last) to->last->next = from->first;
    else to->first = from->first;
    to->last = from->last;

    #if 1
    // we don't care about fixing up the offsets because they'll be properly laid out
    // after this is all done
    to->total_size += from->total_size;
    to->piece_count += from->piece_count;

    for (TB_LinkerSectionPiece* p = from->first; p != NULL; p = p->next) {
        p->parent = to;
    }
    #else
    if (from->last) {
        size_t offset = to->total_size;

        for (TB_LinkerSectionPiece* p = from->first; p != NULL; p = p->next) {
            p->parent = to;
            p->offset = offset;
            offset += p->size;
        }

        to->total_size += from->total_size;
        to->piece_count += from->piece_count;
        assert(offset == to->total_size);
    }
    #endif
}

void tb__append_module_section(TB_Linker* l, TB_LinkerInputHandle mod, TB_ModuleSection* section, const char* name, uint32_t flags) {
    if (section->total_size > 0) {
        TB_LinkerSection* ls = tb__find_or_create_section(l, name, flags);
        section->piece = tb__append_piece(ls, PIECE_MODULE_SECTION, section->total_size, section, mod);
    }
}

size_t tb__apply_section_contents(TB_Linker* l, uint8_t* output, size_t write_pos, TB_LinkerSection* text, TB_LinkerSection* data, TB_LinkerSection* rdata, size_t section_alignment, size_t image_base) {
    // write section contents
    // TODO(NeGate): we can actually parallelize this part of linking
    CUIK_TIMED_BLOCK("write sections") nl_map_for_str(i, l->sections) {
        TB_LinkerSection* s = l->sections[i].v;
        if (s->generic_flags & TB_LINKER_SECTION_DISCARD) continue;

        assert(s->offset == write_pos);
        for (TB_LinkerSectionPiece* p = s->first; p != NULL; p = p->next) {
            uint8_t* p_out = &output[write_pos];
            TB_LinkerInput in = l->inputs[p->input];

            switch (p->kind) {
                case PIECE_NORMAL: {
                    if (p->data == NULL) goto skip;

                    memcpy(p_out, p->data, p->size);
                    break;
                }
                case PIECE_MODULE_SECTION: {
                    tb_helper_write_section(in.module, 0, (TB_ModuleSection*) p->data, p_out, 0);
                    break;
                }
                case PIECE_PDATA: {
                    uint32_t* p_out32 = (uint32_t*) p_out;
                    TB_Module* m = in.module;

                    uint32_t rdata_rva = m->xdata->parent->address + m->xdata->offset;

                    dyn_array_for(i, m->sections) {
                        DynArray(TB_FunctionOutput*) funcs = m->sections[i].funcs;
                        TB_LinkerSectionPiece* piece = m->sections[i].piece;
                        uint32_t rva = piece->parent->address + piece->offset;

                        dyn_array_for(j, funcs) {
                            TB_FunctionOutput* out_f = funcs[j];
                            if (out_f != NULL) {
                                // both into the text section
                                *p_out32++ = rva + out_f->code_pos;
                                *p_out32++ = rva + out_f->code_pos + out_f->code_size;

                                // refers to rdata section
                                *p_out32++ = rdata_rva + out_f->unwind_info;
                            }
                        }
                    }
                    break;
                }
                case PIECE_RELOC: {
                    TB_Module* m = in.module;

                    dyn_array_for(i, m->sections) {
                        DynArray(TB_Global*) globals = m->sections[i].globals;
                        TB_LinkerSectionPiece* piece = m->sections[i].piece;

                        uint32_t data_rva = piece->parent->address + piece->offset;
                        uint32_t data_file = piece->parent->offset + piece->offset;

                        uint32_t last_page = 0xFFFFFFFF;
                        uint32_t* last_block = NULL;

                        dyn_array_for(j, globals) {
                            TB_Global* g = globals[j];
                            FOREACH_N(k, 0, g->obj_count) {
                                size_t actual_pos  = g->pos + g->objects[k].offset;
                                size_t actual_page = actual_pos & ~4095;
                                size_t page_offset = actual_pos - actual_page;

                                if (g->objects[k].type != TB_INIT_OBJ_RELOC) {
                                    continue;
                                }

                                const TB_Symbol* s = g->objects[k].reloc;
                                if (last_page != actual_page) {
                                    last_page  = data_rva + actual_page;
                                    last_block = (uint32_t*) p_out;

                                    last_block[0] = data_rva + actual_page;
                                    last_block[1] = 8; // block size field (includes RVA field and itself)
                                    p_out += 8;
                                }

                                // compute RVA
                                uint32_t file_pos = data_file + actual_pos;
                                *((uint64_t*) &output[file_pos]) = tb__compute_rva(l, m, s) + image_base;

                                // emit relocation
                                uint16_t payload = (10 << 12) | page_offset; // (IMAGE_REL_BASED_DIR64 << 12) | offset
                                *((uint16_t*) p_out) = payload, p_out += sizeof(uint16_t);
                                last_block[1] += 2;
                            }
                        }
                    }
                    break;
                }
                default: tb_todo();
            }

            write_pos += p->size;
            skip:;
        }

        write_pos = tb__pad_file(output, write_pos, 0x00, section_alignment);
    }

    return write_pos;
}

TB_LinkerSection* tb__find_section(TB_Linker* linker, const char* name) {
    ptrdiff_t search = nl_map_get_cstr(linker->sections, name);
    return search >= 0 ? linker->sections[search].v : NULL;
}

TB_LinkerSection* tb__find_or_create_section(TB_Linker* linker, const char* name, uint32_t flags) {
    // allocate new section if one doesn't exist already
    ptrdiff_t search = nl_map_get_cstr(linker->sections, name);
    if (search >= 0) {
        // assert(linker->sections[search]->flags == flags);
        return linker->sections[search].v;
    }

    TB_LinkerSection* s = tb_platform_heap_alloc(sizeof(TB_LinkerSection));
    *s = (TB_LinkerSection){ .name = { strlen(name), (const uint8_t*) name }, .flags = flags };
    nl_map_put_cstr(linker->sections, name, s);
    return s;
}

TB_LinkerSection* tb__find_or_create_section2(TB_Linker* linker, size_t name_len, const uint8_t* name_str, uint32_t flags) {
    // allocate new section if one doesn't exist already
    NL_Slice name = { name_len, name_str };
    ptrdiff_t search = nl_map_get(linker->sections, name);

    if (search >= 0) {
        // assert(linker->sections[search]->flags == flags);
        return linker->sections[search].v;
    }

    TB_LinkerSection* s = tb_platform_heap_alloc(sizeof(TB_LinkerSection));
    *s = (TB_LinkerSection){ .name = name, .flags = flags };

    nl_map_put(linker->sections, name, s);
    return s;
}

TB_LinkerSectionPiece* tb__append_piece(TB_LinkerSection* section, int kind, size_t size, const void* data, TB_LinkerInputHandle input) {
    // allocate some space for it, we might wanna make the total_size increment atomic
    TB_LinkerSectionPiece* piece = tb_platform_heap_alloc(sizeof(TB_LinkerSectionPiece));
    *piece = (TB_LinkerSectionPiece){
        .kind   = kind,
        .parent = section,
        .offset = section->total_size,
        .size   = size,
        .vsize  = size,
        .data   = data,
        .input  = input
    };
    section->total_size += size;
    section->piece_count += 1;

    if (section->last == NULL) {
        section->first = section->last = piece;
    } else {
        section->last->next = piece;
        section->last = piece;
    }
    return piece;
}

TB_LinkerInputHandle tb__track_module(TB_Linker* l, TB_LinkerInputHandle parent, TB_Module* mod) {
    log_debug("%p: track module %p", l, mod);
    TB_LinkerInput entry = { TB_LINKER_INPUT_MODULE, parent, .module = mod };

    size_t i = dyn_array_length(l->inputs);
    assert(i < 0xFFFF);

    dyn_array_put(l->inputs, entry);
    return i;
}

TB_LinkerInputHandle tb__track_object(TB_Linker* l, TB_LinkerInputHandle parent, TB_Slice name) {
    log_debug("%p: track object %.*s", l, (int) name.length, name.data);
    TB_LinkerInput entry = { TB_LINKER_INPUT_OBJECT, parent, .name = name };

    size_t i = dyn_array_length(l->inputs);
    assert(i < 0xFFFF);

    dyn_array_put(l->inputs, entry);
    return i;
}

// murmur3 32-bit without UB unaligned accesses
// https://github.com/demetri/scribbles/blob/master/hashing/ub_aware_hash_functions.c
static uint32_t murmur(const void* key, size_t len) {
    uint32_t h = 0;

    // main body, work on 32-bit blocks at a time
    for (size_t i=0;i<len/4;i++) {
        uint32_t k;
        memcpy(&k, &((char*) key)[i * 4], sizeof(k));

        k *= 0xcc9e2d51;
        k = ((k << 15) | (k >> 17))*0x1b873593;
        h = (((h^k) << 13) | ((h^k) >> 19))*5 + 0xe6546b64;
    }

    // load/mix up to 3 remaining tail bytes into a tail block
    uint32_t t = 0;
    const uint8_t *tail = ((const uint8_t*) key) + 4*(len/4);
    switch(len & 3) {
        case 3: t ^= tail[2] << 16;
        case 2: t ^= tail[1] <<  8;
        case 1: {
            t ^= tail[0] <<  0;
            h ^= ((0xcc9e2d51*t << 15) | (0xcc9e2d51*t >> 17))*0x1b873593;
        }
    }

    // finalization mix, including key length
    h = ((h^len) ^ ((h^len) >> 16))*0x85ebca6b;
    h = (h ^ (h >> 13))*0xc2b2ae35;
    return (h ^ (h >> 16));
}

ImportThunk* tb__find_or_create_import(TB_Linker* l, TB_LinkerSymbol* restrict sym) {
    assert(sym->tag == TB_LINKER_SYMBOL_IMPORT);
    TB_Slice sym_name = { sym->name.length - (sizeof("__imp_") - 1), sym->name.data + sizeof("__imp_") - 1 };

    ImportTable* table = &l->imports[sym->import.id];
    dyn_array_for(i, table->thunks) {
        if (table->thunks[i].name.length == sym_name.length &&
            memcmp(table->thunks[i].name.data, sym_name.data, sym_name.length) == 0) {
            return &table->thunks[i];
        }
    }

    ImportThunk t = { .name = sym_name, .ordinal = sym->import.ordinal };
    dyn_array_put(table->thunks, t);
    return &table->thunks[dyn_array_length(table->thunks) - 1];
}

TB_LinkerSymbol* tb__find_symbol_cstr(TB_SymbolTable* restrict symtab, const char* name) {
    return tb__find_symbol(symtab, (TB_Slice){ strlen(name), (const uint8_t*) name });
}

TB_LinkerSymbol* tb__find_symbol(TB_SymbolTable* restrict symtab, TB_Slice name) {
    uint32_t mask = (1u << symtab->exp) - 1;
    uint32_t hash = murmur(name.data, name.length);
    for (size_t i = hash;;) {
        // hash table lookup
        uint32_t step = (hash >> (32 - symtab->exp)) | 1;
        i = (i + step) & mask;

        if (symtab->ht[i].name.length == 0) {
            return NULL;
        } else if (name.length == symtab->ht[i].name.length && memcmp(name.data, symtab->ht[i].name.data, name.length) == 0) {
            return &symtab->ht[i];
        }
    }
}

TB_LinkerSymbol* tb__append_symbol(TB_SymbolTable* restrict symtab, const TB_LinkerSymbol* sym) {
    TB_Slice name = sym->name;

    uint32_t mask = (1u << symtab->exp) - 1;
    uint32_t hash = murmur(name.data, name.length);
    for (size_t i = hash;;) {
        // hash table lookup
        uint32_t step = (hash >> (32 - symtab->exp)) | 1;
        i = (i + step) & mask;

        if (symtab->ht[i].name.length == 0) {
            // empty slot
            if (symtab->len > mask) {
                printf("Symbol table: out of memory!\n");
                abort();
            }

            symtab->len++;
            memcpy(&symtab->ht[i], sym, sizeof(TB_LinkerSymbol));
            return &symtab->ht[i];
        } else if (name.length == symtab->ht[i].name.length && memcmp(name.data, symtab->ht[i].name.data, name.length) == 0) {
            // proper collision... this is a linker should we throw warnings?
            // memcpy(&symtab->ht[i], sym, sizeof(TB_LinkerSymbol));
            return &symtab->ht[i];
        }
    }
}

TB_UnresolvedSymbol* tb__unresolved_symbol(TB_Linker* l, TB_Slice name) {
    TB_UnresolvedSymbol* d = tb_platform_heap_alloc(sizeof(TB_UnresolvedSymbol));
    *d = (TB_UnresolvedSymbol){ .name = name };

    // mtx_lock(&parser->diag_mutex);
    NL_Slice name2 = { name.length, name.data };
    ptrdiff_t search = nl_map_get(l->unresolved_symbols, name2);
    if (search < 0) {
        nl_map_puti(l->unresolved_symbols, name2, search);
        l->unresolved_symbols[search].v = d;
    } else {
        TB_UnresolvedSymbol* old = l->unresolved_symbols[search].v;
        while (old->next != NULL) old = old->next;

        old->next = d;
    }
    // mtx_unlock(&parser->diag_mutex);

    return d;
}

static void tb__append_module_symbols(TB_Linker* l, TB_Module* m) {
    DynArray(TB_ModuleSection) sections = m->sections;

    CUIK_TIMED_BLOCK("apply symbols") {
        static const TB_SymbolTag tags[] = { TB_SYMBOL_FUNCTION, TB_SYMBOL_GLOBAL };
        TB_Slice obj_name = { sizeof("<tb module>")-1, (const uint8_t*) "<tb module>" };

        dyn_array_for(i, sections) {
            DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
            DynArray(TB_Global*) globals = sections[i].globals;
            TB_LinkerSectionPiece* piece = sections[i].piece;

            dyn_array_for(i, funcs) {
                const char* name = funcs[i]->parent->super.name;
                TB_LinkerSymbol ls = {
                    .name = { strlen(name), (const uint8_t*) name },
                    .tag = TB_LINKER_SYMBOL_TB,
                    .object_name = obj_name,
                    .tb = { piece, &funcs[i]->parent->super }
                };

                tb__append_symbol(&l->symtab, &ls);
            }

            dyn_array_for(i, globals) {
                const char* name = globals[i]->super.name;
                TB_LinkerSymbol ls = {
                    .name = { strlen(name), (const uint8_t*) name },
                    .tag = TB_LINKER_SYMBOL_TB,
                    .object_name = obj_name,
                    .tb = { piece, &globals[i]->super }
                };

                tb__append_symbol(&l->symtab, &ls);
            }
        }
    }

    dyn_array_put(l->ir_modules, m);
}

void tb__apply_module_relocs(TB_Linker* l, TB_Module* m, uint8_t* output) {
    TB_LinkerSection* text = tb__find_section(l, ".text");
    if (text == NULL) {
        return;
    }

    uint64_t trampoline_rva = text->address + l->trampoline_pos;

    dyn_array_for(i, m->sections) {
        DynArray(TB_FunctionOutput*) funcs = m->sections[i].funcs;
        TB_LinkerSectionPiece* piece = m->sections[i].piece;

        uint64_t text_piece_rva = piece->parent->address + piece->offset;
        uint64_t text_piece_file = piece->parent->offset + piece->offset;

        dyn_array_for(j, funcs) {
            TB_FunctionOutput* out_f = funcs[j];
            for (TB_SymbolPatch* patch = out_f->last_patch; patch; patch = patch->prev) {
                int32_t* dst = (int32_t*) &output[text_piece_file + out_f->code_pos + patch->pos];
                size_t actual_pos = text_piece_rva + out_f->code_pos + patch->pos + 4;

                int32_t p = 0;
                if (patch->target->tag == TB_SYMBOL_EXTERNAL) {
                    uintptr_t thunk_p = (uintptr_t) patch->target->address;
                    if (thunk_p & 1) {
                        TB_LinkerSymbol* sym = (TB_LinkerSymbol*) (thunk_p & ~1);
                        p = tb__get_symbol_rva(l, sym);
                    } else {
                        ImportThunk* thunk = (ImportThunk*) thunk_p;
                        assert(thunk != NULL);

                        p = (trampoline_rva + (thunk->thunk_id * 6)) - actual_pos;
                    }
                } else if (patch->target->tag == TB_SYMBOL_FUNCTION) {
                    // internal patching has already handled this
                } else if (patch->target->tag == TB_SYMBOL_GLOBAL) {
                    TB_Global* global = (TB_Global*) patch->target;
                    assert(global->super.tag == TB_SYMBOL_GLOBAL);

                    uint32_t flags = m->sections[global->parent].flags;
                    TB_LinkerSectionPiece* piece = m->sections[global->parent].piece;
                    uint32_t piece_rva = piece->parent->address + piece->offset;

                    int32_t* dst = (int32_t*) &output[text_piece_file + out_f->code_pos + patch->pos];
                    if (flags & TB_MODULE_SECTION_TLS) {
                        // section relative for TLS
                        p = piece_rva + global->pos;
                    } else {
                        p = (piece_rva + global->pos) - actual_pos;
                    }
                } else {
                    tb_todo();
                }

                *dst += p;
            }
        }
    }
}

static TB_Slice as_filename(TB_Slice s) {
    size_t last = 0;
    FOREACH_N(i, 0, s.length) {
        if (s.data[i] == '/' || s.data[i] == '\\') {
            last = i+1;
        }
    }

    return (TB_Slice){ s.length - last, s.data + last };
}

static int compare_linker_sections(const void* a, const void* b) {
    const TB_LinkerSectionPiece* sec_a = *(const TB_LinkerSectionPiece**) a;
    const TB_LinkerSectionPiece* sec_b = *(const TB_LinkerSectionPiece**) b;

    return sec_a->order - sec_b->order;
}

bool tb__finalize_sections(TB_Linker* l) {
    if (nl_map_get_capacity(l->unresolved_symbols) > 0) {
        nl_map_for_str(i, l->unresolved_symbols) {
            TB_UnresolvedSymbol* u = l->unresolved_symbols[i].v;

            fprintf(stderr, "\x1b[31merror\x1b[0m: unresolved external: %.*s\n", (int) u->name.length, u->name.data);
            size_t i = 0;
            for (; u && i < 5; u = u->next, i++) {
                // walk input stack
                TB_LinkerInputHandle curr = u->reloc;
                fprintf(stderr, "  in ");

                int depth = 0;
                while (curr != 0) {
                    TB_LinkerInput* input = &l->inputs[curr];

                    depth++;
                    if (depth) {
                        fprintf(stderr, "(");
                    }

                    if (input->tag == TB_LINKER_INPUT_MODULE) {
                        fprintf(stderr, "<tb-module %p>\n", input->module);
                    } else {
                        TB_Slice obj_name = as_filename(input->name);
                        fprintf(stderr, "%.*s", (int) obj_name.length, obj_name.data);
                    }

                    curr = input->parent;
                }

                while (depth--) fprintf(stderr, ")");
                fprintf(stderr, "\n");
            }

            if (u) {
                // count the rest
                while (u) u = u->next, i++;

                fprintf(stderr, "  ...and %zu more...\n", i - 5);
            }
            fprintf(stderr, "\n");
        }

        nl_map_free(l->unresolved_symbols);
        return false;
    }

    CUIK_TIMED_BLOCK("sort sections") {
        TB_LinkerSectionPiece** array_form = NULL;
        size_t num = 0;

        nl_map_for_str(i, l->sections) {
            TB_LinkerSection* s = l->sections[i].v;
            if (s->generic_flags & TB_LINKER_SECTION_DISCARD) continue;

            size_t piece_count = s->piece_count;

            ////////////////////////////////
            // Sort sections
            ////////////////////////////////
            // convert into array
            size_t j = 0;
            CUIK_TIMED_BLOCK("convert to array") {
                assert(s->piece_count != 0);
                array_form = tb_platform_heap_realloc(array_form, piece_count * sizeof(TB_LinkerSectionPiece*));

                for (TB_LinkerSectionPiece* p = s->first; p != NULL; p = p->next) {
                    if (p->size != 0 && (p->flags & TB_LINKER_PIECE_LIVE)) {
                        array_form[j++] = p;
                    }
                }

                // fprintf(stderr, "%.*s: %zu -> %zu\n", (int) s->name.length, s->name.data, piece_count, j);
                piece_count = j;
            }

            if (piece_count == 0) {
                s->generic_flags |= TB_LINKER_SECTION_DISCARD;
                continue;
            }

            // sort
            CUIK_TIMED_BLOCK("sort section") {
                qsort(array_form, piece_count, sizeof(TB_LinkerSectionPiece*), compare_linker_sections);
            }

            // convert back into linked list
            CUIK_TIMED_BLOCK("convert into list") {
                array_form[0]->offset = 0;

                size_t offset = array_form[0]->size;
                for (j = 1; j < piece_count; j++) {
                    array_form[j]->offset = offset;
                    offset += array_form[j]->size;

                    array_form[j-1]->next = array_form[j];
                }
                s->total_size = offset;

                s->first = array_form[0];
                s->last = array_form[piece_count - 1];
                s->piece_count = piece_count;

                array_form[piece_count - 1]->next = NULL;
            }

            s->number = num++;
        }
        tb_platform_heap_free(array_form);
    }

    return true;
}

static void gc_mark(TB_Linker* l, TB_LinkerSectionPiece* p) {
    if (p == NULL || p->size == 0 || (p->flags & TB_LINKER_PIECE_LIVE) || (p->parent->generic_flags & TB_LINKER_SECTION_DISCARD)) {
        return;
    }

    p->flags |= TB_LINKER_PIECE_LIVE;

    // mark module content
    if (l->inputs[p->input].tag == TB_LINKER_INPUT_MODULE) {
        TB_Module* m = l->inputs[p->input].module;

        dyn_array_for(i, m->sections) {
            gc_mark(l, m->sections[i].piece);
        }
    }

    // mark any kid symbols
    for (TB_LinkerSymbol* sym = p->first_sym; sym != NULL; sym = sym->next) {
        gc_mark(l, tb__get_piece(l, sym));
    }

    // mark any relocations
    dyn_array_for(i, p->abs_refs) {
        TB_LinkerRelocAbs* r = &p->abs_refs[i].info->absolutes[p->abs_refs[i].index];

        // resolve symbol
        r->target = l->resolve_sym(l, r->target, r->name, r->alt, r->input);
        gc_mark(l, tb__get_piece(l, r->target));
    }

    dyn_array_for(i, p->rel_refs) {
        TB_LinkerRelocRel* r = &p->rel_refs[i].info->relatives[p->rel_refs[i].index];

        // resolve symbol
        r->target = l->resolve_sym(l, r->target, r->name, r->alt, r->input);
        gc_mark(l, tb__get_piece(l, r->target));
    }

    if (p->associate) {
        gc_mark(l, p->associate);
    }
}

static void gc_mark_root(TB_Linker* l, const char* name) {
    TB_LinkerSectionPiece* p = tb__get_piece(l, tb__find_symbol_cstr(&l->symtab, name));
    gc_mark(l, p);
}
