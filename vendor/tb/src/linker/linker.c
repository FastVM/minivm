#include "linker.h"

#if __STDC_VERSION__ < 201112L || defined(__STDC_NO_ATOMICS__)
#error "Missing C11 support for stdatomic.h"
#endif

#include <stdatomic.h>

TB_LinkerThreadInfo* linker_thread_info(TB_Linker* l) {
    static thread_local TB_LinkerThreadInfo* chain;

    // almost always refers to one TB_ThreadInfo, but
    // we can't assume the user has merely on TB_Module
    // per thread.
    TB_LinkerThreadInfo* info = chain;
    while (info != NULL) {
        if (info->owner == l) {
            goto done;
        }
        info = info->next;
    }

    info = tb_platform_heap_alloc(sizeof(TB_LinkerThreadInfo));
    *info = (TB_LinkerThreadInfo){ .owner = l };

    // allocate memory for it
    info->perm_arena = tb_arena_create(TB_ARENA_LARGE_CHUNK_SIZE);
    info->tmp_arena = tb_arena_create(TB_ARENA_LARGE_CHUNK_SIZE);

    // thread local so it doesn't need to synchronize
    info->next = chain;
    if (chain != NULL) {
        chain->prev = info;
    }
    chain = info;

    // link to the TB_Module* (we need to this to free later)
    TB_LinkerThreadInfo* old_top;
    do {
        old_top = atomic_load(&l->first_thread_info);
        info->next_in_link = old_top;
    } while (!atomic_compare_exchange_strong(&l->first_thread_info, &old_top, info));

    done:
    return info;
}

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

    l->symbols = nl_table_alloc(256);
    l->sections = nl_table_alloc(16);

    switch (exe) {
        case TB_EXECUTABLE_PE:  l->vtbl = tb__linker_pe;  break;
        // case TB_EXECUTABLE_ELF: l->vtbl = tb__linker_elf; break;
        default: break;
    }

    l->vtbl.init(l);
    return l;
}

TB_API void tb_linker_set_subsystem(TB_Linker* l, TB_WindowsSubsystem subsystem) {
    l->subsystem = subsystem;
}

TB_API void tb_linker_set_entrypoint(TB_Linker* l, const char* name) {
    l->entrypoint = name;
}

TB_API void tb_linker_append_object(TB_Linker* l, TB_Slice obj_name, TB_Slice content) {
    CUIK_TIMED_BLOCK("append_object") {
        TB_LinkerThreadInfo* info = linker_thread_info(l);
        l->vtbl.append_object(l, info, obj_name, content);
    }
}

TB_API void tb_linker_append_module(TB_Linker* l, TB_Module* m) {
    CUIK_TIMED_BLOCK("append_module") {
        TB_LinkerThreadInfo* info = linker_thread_info(l);
        l->vtbl.append_module(l, info, m);
    }
}

TB_API void tb_linker_append_library(TB_Linker* l, TB_Slice ar_name, TB_Slice ar_file) {
    CUIK_TIMED_BLOCK("append_library") {
        TB_LinkerThreadInfo* info = linker_thread_info(l);
        l->vtbl.append_library(l, info, ar_name, ar_file);
    }
}

TB_API TB_ExportBuffer tb_linker_export(TB_Linker* l, TB_Arena* arena) {
    return l->vtbl.export(l, arena);
}

TB_API void tb_linker_destroy(TB_Linker* l) {
    tb_platform_heap_free(l);
}

void tb_linker_unresolved_sym(TB_Linker* l, TB_LinkerInternStr name) {
    fprintf(stderr, "\x1b[31merror\x1b[0m: unresolved external: %s\n", name);
}

TB_LinkerSectionPiece* tb_linker_get_piece(TB_Linker* l, TB_LinkerSymbol* restrict sym) {
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

void tb_linker_merge_sections(TB_Linker* linker, TB_LinkerSection* from, TB_LinkerSection* to) {
    if (from == NULL || to == NULL) return;
    if (from->generic_flags & TB_LINKER_SECTION_DISCARD) return;

    // remove 'from' from final output
    from->generic_flags |= TB_LINKER_SECTION_DISCARD;

    if (to->last) to->last->next = from->first;
    else to->first = from->first;
    to->last = from->last;

    // we don't care about fixing up the offsets because they'll be properly laid out
    // after this is all done
    to->total_size += from->total_size;
    to->piece_count += from->piece_count;

    for (TB_LinkerSectionPiece* p = from->first; p != NULL; p = p->next) {
        p->parent = to;
    }
}

void tb_linker_append_module_section(TB_Linker* l, TB_LinkerThreadInfo* info, TB_LinkerObject* mod, TB_ModuleSection* section, uint32_t flags) {
    assert(mod->module != NULL && "not a TB_Module's section?");
    if (section->total_size > 0) {
        TB_LinkerInternStr name = tb_linker_intern_string(l, strlen(section->name), section->name);

        TB_LinkerSection* ls = tb_linker_find_or_create_section(l, name, flags);
        section->piece = tb_linker_append_piece(info, ls, PIECE_MODULE_SECTION, section->total_size, section, mod);
    }
}

void tb_linker_associate(TB_Linker* l, TB_LinkerSectionPiece* a, TB_LinkerSectionPiece* b) {
    assert(a->assoc == NULL);
    a->assoc = b;
}

size_t tb__apply_section_contents(TB_Linker* l, uint8_t* output, size_t write_pos, TB_LinkerSection* text, TB_LinkerSection* data, TB_LinkerSection* rdata, size_t section_alignment, size_t image_base) {
    // write section contents
    // TODO(NeGate): we can actually parallelize this part of linking
    CUIK_TIMED_BLOCK("write sections") {
        nl_table_for(e, &l->sections) {
            TB_LinkerSection* s = e->v;
            if (s->generic_flags & TB_LINKER_SECTION_DISCARD) continue;

            assert(s->offset == write_pos);
            for (TB_LinkerSectionPiece* p = s->first; p != NULL; p = p->next) {
                uint8_t* p_out = &output[write_pos];
                TB_LinkerObject* obj = p->obj;

                switch (p->kind) {
                    case PIECE_NORMAL: {
                        if (p->data == NULL) goto skip;

                        memcpy(p_out, p->data, p->size);
                        break;
                    }
                    case PIECE_MODULE_SECTION: {
                        tb_helper_write_section(obj->module, 0, (TB_ModuleSection*) p->data, p_out, 0);
                        break;
                    }
                    case PIECE_PDATA: {
                        uint32_t* p_out32 = (uint32_t*) p_out;
                        TB_Module* m = obj->module;

                        uint32_t rdata_rva = m->xdata->parent->address + m->xdata->offset;

                        dyn_array_for(i, m->sections) {
                            DynArray(TB_FunctionOutput*) funcs = m->sections[i].funcs;
                            TB_LinkerSectionPiece* piece = m->sections[i].piece;
                            if (piece == NULL) {
                                continue;
                            }

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
                        TB_Module* m = obj->module;
                        assert(m != NULL);

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
    }

    return write_pos;
}

TB_LinkerSection* tb_linker_find_section(TB_Linker* l, TB_LinkerInternStr name) {
    return nl_table_get(&l->sections, name);
}

TB_LinkerSection* tb_linker_find_section2(TB_Linker* l, const char* name) {
    return nl_table_get(&l->sections, tb_linker_intern_cstring(l, name));
}

TB_LinkerSection* tb_linker_find_or_create_section(TB_Linker* l, TB_LinkerInternStr name, uint32_t flags) {
    // allocate new section if one doesn't exist already
    TB_LinkerSection* s = nl_table_get(&l->sections, name);
    if (s) {
        return s;
    }

    TB_LinkerThreadInfo* info = linker_thread_info(l);
    s = tb_arena_alloc(info->perm_arena, sizeof(TB_LinkerSection));
    *s = (TB_LinkerSection){ .name = name, .flags = flags };

    nl_table_put(&l->sections, name, s);
    return s;
}

TB_LinkerSectionPiece* tb_linker_append_piece(TB_LinkerThreadInfo* info, TB_LinkerSection* section, int kind, size_t size, const void* data, TB_LinkerObject* obj) {
    // allocate some space for it, we might wanna make the total_size increment atomic
    TB_LinkerSectionPiece* piece = tb_arena_alloc(info->perm_arena, sizeof(TB_LinkerSectionPiece));
    *piece = (TB_LinkerSectionPiece){
        .kind   = kind,
        .parent = section,
        .obj    = obj,
        .offset = section->total_size,
        .size   = size,
        .vsize  = size,
        .data   = data,
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

TB_LinkerSymbol* tb_linker_find_symbol(TB_Linker* l, TB_LinkerInternStr name) {
    return nl_table_get(&l->symbols, name);
}

TB_LinkerSymbol* tb_linker_find_symbol2(TB_Linker* l, const char* name) {
    return nl_table_get(&l->symbols, tb_linker_intern_cstring(l, name));
}

TB_LinkerSymbol* tb_linker_new_symbol(TB_Linker* l, TB_LinkerInternStr name) {
    cuikperf_region_start("append sym", NULL);

    TB_LinkerThreadInfo* info = linker_thread_info(l);
    TB_LinkerSymbol* sym = tb_arena_alloc(info->perm_arena, sizeof(TB_LinkerSymbol));

    TB_LinkerSymbol* old;
    if (old = nl_table_put(&l->symbols, name, sym), old) {
        tb_arena_free(info->perm_arena, sym, sizeof(TB_LinkerSymbol));
        sym = old;

        if (old->tag != TB_LINKER_SYMBOL_UNKNOWN) {
            // symbol collision if we're overriding something that's
            // not a forward ref.
            // __debugbreak();
        }
    } else {
        *sym = (TB_LinkerSymbol){ .name = name };
    }

    cuikperf_region_end();
    return sym;
}

TB_LinkerInternStr tb_linker_intern_string(TB_Linker* l, size_t len, const char* str) {
    if (l->interner.arr == NULL) {
        CUIK_TIMED_BLOCK("alloc atoms") {
            l->interner.exp = 20;
            l->interner.cnt = 0;
            l->interner.arr = cuik__valloc((1u << 20) * sizeof(TB_LinkerInternStr));
        }
    }

    uint32_t mask = (1 << l->interner.exp) - 1;
    uint32_t hash = tb__murmur3_32(str, len);
    size_t first = hash & mask, i = first;

    do {
        // linear probe
        if (LIKELY(l->interner.arr[i] == NULL)) {
            TB_LinkerThreadInfo* info = linker_thread_info(l);
            TB_LinkerInternStr newstr = tb_arena_alloc(info->perm_arena, len + 5);
            uint32_t len32 = len;
            memcpy(newstr, &len32, 4);
            memcpy(newstr + 4, str, len);
            newstr[len + 4] = 0;

            l->interner.arr[i] = newstr + 4;
            l->interner.cnt++;
            return &newstr[4];
        } else if (len == tb_linker_intern_len(l, l->interner.arr[i]) && memcmp(str, l->interner.arr[i], len) == 0) {
            return l->interner.arr[i];
        }

        i = (i + 1) & mask;
    } while (i != first);

    log_error("atoms arena: out of memory!\n");
    abort();
}

TB_LinkerInternStr tb_linker_intern_cstring(TB_Linker* l, const char* str) {
    return tb_linker_intern_string(l, strlen(str), str);
}

size_t tb_linker_intern_len(TB_Linker* l, TB_LinkerInternStr str) {
    return ((uint32_t*) str)[-1];
}

void tb_linker_append_module_symbols(TB_Linker* l, TB_Module* m) {
    DynArray(TB_ModuleSection) sections = m->sections;
    TB_LinkerThreadInfo* info = linker_thread_info(l);

    CUIK_TIMED_BLOCK("apply symbols") {
        dyn_array_for(i, sections) {
            DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
            DynArray(TB_Global*) globals = sections[i].globals;
            TB_LinkerSectionPiece* piece = sections[i].piece;

            dyn_array_for(i, funcs) {
                const char* name = funcs[i]->parent->super.name;
                TB_LinkerInternStr interned = tb_linker_intern_string(l, strlen(name), name);

                TB_LinkerSymbol* ls = tb_linker_new_symbol(l, interned);
                ls->tag = TB_LINKER_SYMBOL_TB;
                ls->tb.piece = piece;
                ls->tb.sym = &funcs[i]->parent->super;
            }

            dyn_array_for(i, globals) {
                const char* name = globals[i]->super.name;
                TB_LinkerInternStr interned = tb_linker_intern_string(l, strlen(name), name);

                TB_LinkerSymbol* ls = NULL;
                if (globals[i]->linkage == TB_LINKAGE_PRIVATE) {
                    ls = tb_arena_alloc(info->perm_arena, sizeof(TB_LinkerSymbol));
                    *ls = (TB_LinkerSymbol){ .name = interned };
                } else {
                    ls = tb_linker_new_symbol(l, interned);
                }
                ls->tag = TB_LINKER_SYMBOL_TB;
                ls->tb.piece = piece;
                ls->tb.sym = &globals[i]->super;
            }
        }
    }
}

void tb_linker_apply_module_relocs(TB_Linker* l, TB_Module* m, TB_LinkerSection* text, uint8_t* output) {
    uint64_t trampoline_rva = text->address + l->trampoline_pos;

    dyn_array_for(i, m->sections) {
        DynArray(TB_FunctionOutput*) funcs = m->sections[i].funcs;
        TB_LinkerSectionPiece* piece = m->sections[i].piece;
        if (piece == NULL) {
            continue;
        }

        uint64_t text_piece_rva = piece->parent->address + piece->offset;
        uint64_t text_piece_file = piece->parent->offset + piece->offset;

        dyn_array_for(j, funcs) {
            TB_FunctionOutput* out_f = funcs[j];
            for (TB_SymbolPatch* patch = out_f->first_patch; patch; patch = patch->next) {
                int32_t* dst = (int32_t*) &output[text_piece_file + out_f->code_pos + patch->pos];
                size_t actual_pos = text_piece_rva + out_f->code_pos + patch->pos + 4;

                int32_t p = 0;
                if (patch->target->tag == TB_SYMBOL_EXTERNAL) {
                    TB_LinkerSymbol* sym = patch->target->address;
                    if (sym->tag == TB_LINKER_SYMBOL_UNKNOWN) {
                        // TODO(NeGate): error for unresolved symbol
                        tb_todo();
                    } else if (sym->tag == TB_LINKER_SYMBOL_THUNK) {
                        p = trampoline_rva + (sym->thunk->import.thunk_id * 6);
                    } else if (sym->tag == TB_LINKER_SYMBOL_IMPORT) {
                        p = trampoline_rva + (sym->import.thunk_id * 6);
                    } else {
                        p = tb__get_symbol_rva(l, sym);
                    }

                    p -= actual_pos;
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

    return (TB_Slice){ s.data + last, s.length - last };
}

static int compare_linker_sections(const void* a, const void* b) {
    const TB_LinkerSectionPiece* sec_a = *(const TB_LinkerSectionPiece**) a;
    const TB_LinkerSectionPiece* sec_b = *(const TB_LinkerSectionPiece**) b;

    return sec_a->order - sec_b->order;
}

bool tb__finalize_sections(TB_Linker* l) {
    /*if (nl_map_get_capacity(l->unresolved_symbols) > 0) {
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
    }*/

    CUIK_TIMED_BLOCK("sort sections") {
        TB_LinkerSectionPiece** array_form = NULL;
        size_t num = 0;

        nl_table_for(e, &l->sections) {
            TB_LinkerSection* s = e->v;
            if (s->generic_flags & TB_LINKER_SECTION_DISCARD) {
                continue;
            }

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

TB_LinkerSymbol* tb_linker_get_target(TB_LinkerReloc* r) {
    if (r->target->tag == TB_LINKER_SYMBOL_UNKNOWN) {
        return r->alt && r->alt->tag != TB_LINKER_SYMBOL_UNKNOWN ? r->alt : NULL;
    } else {
        return r->target;
    }
}

void tb_linker_push_piece(TB_Linker* l, TB_LinkerSectionPiece* p) {
    if (p->size == 0 || (p->flags & TB_LINKER_PIECE_LIVE) || (p->parent->generic_flags & TB_LINKER_SECTION_DISCARD)) {
        return;
    }

    dyn_array_put(l->worklist, p);
}

void tb_linker_push_named(TB_Linker* l, const char* name) {
    TB_LinkerInternStr str = tb_linker_intern_string(l, strlen(name), name);
    TB_LinkerSymbol* sym   = tb_linker_find_symbol(l, str);
    if (sym != NULL) {
        tb_linker_push_piece(l, tb_linker_get_piece(l, sym));
    }
}

static TB_LinkerSymbol* resolve_external(TB_Linker* l, TB_External* ext) {
    TB_LinkerInternStr str = tb_linker_intern_cstring(l, ext->super.name);
    TB_LinkerSymbol* sym = tb_linker_find_symbol2(l, str);
    if (sym == NULL) {
        tb_todo();
    } else if (sym->tag == TB_LINKER_SYMBOL_THUNK) {
        sym->thunk->flags |= TB_LINKER_SYMBOL_USED;
    }

    ext->super.address = sym;
    sym->flags |= TB_LINKER_SYMBOL_USED;
    return sym;
}

void tb_linker_mark_live(TB_Linker* l) {
    while (dyn_array_length(l->worklist)) {
        TB_LinkerSectionPiece* p = dyn_array_pop(l->worklist);
        p->flags |= TB_LINKER_PIECE_LIVE;

        // associated section
        if (p->assoc) {
            tb_linker_push_piece(l, p->assoc);
        }

        // mark module content
        if (p->obj->module && !p->obj->module->visited) {
            p->obj->module->visited = true;

            TB_Module* m = p->obj->module;
            dyn_array_for(i, m->sections) {
                if (m->sections[i].piece) {
                    tb_linker_push_piece(l, m->sections[i].piece);
                }
            }

            // associate TB externals with linker symbols
            FOREACH_N(i, 0, m->exports.count) {
                if (&m->exports.data[i]->super == m->chkstk_extern && m->uses_chkstk == 0) {
                    continue;
                }

                TB_LinkerSymbol* sym = resolve_external(l, m->exports.data[i]);
                TB_LinkerSectionPiece* piece = tb_linker_get_piece(l, sym);
                if (piece) {
                    tb_linker_push_piece(l, piece);
                }
            }
        }

        // mark any kid symbols
        for (TB_LinkerSymbol* sym = p->first_sym; sym != NULL; sym = sym->next) {
            if (sym) {
                sym->flags |= TB_LINKER_SYMBOL_USED;
                if (sym->tag == TB_LINKER_SYMBOL_NORMAL || sym->tag == TB_LINKER_SYMBOL_TB) {
                    tb_linker_push_piece(l, sym->normal.piece);
                }
            }
        }

        // mark any relocations
        TB_LinkerThreadInfo* info = p->info;
        dyn_array_for(i, p->inputs) {
            size_t j = p->inputs[i];
            TB_LinkerSymbol* sym = tb_linker_get_target(&info->relocs[j]);

            if (sym) {
                sym->flags |= TB_LINKER_SYMBOL_USED;
                if (sym->tag == TB_LINKER_SYMBOL_NORMAL || sym->tag == TB_LINKER_SYMBOL_TB) {
                    tb_linker_push_piece(l, sym->normal.piece);
                }
            }
        }
    }
}
