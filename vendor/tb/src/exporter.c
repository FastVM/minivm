#include "tb_internal.h"

TB_ExportBuffer tb_coff_write_output(TB_Module* restrict m, const IDebugFormat* dbg);
TB_ExportBuffer tb_macho_write_output(TB_Module* restrict m, const IDebugFormat* dbg);
TB_ExportBuffer tb_elf64obj_write_output(TB_Module* restrict m, const IDebugFormat* dbg);
TB_ExportBuffer tb_wasm_write_output(TB_Module* restrict m, const IDebugFormat* dbg);

static const IDebugFormat* find_debug_format(TB_DebugFormat debug_fmt) {
    // Place all debug formats here
    extern IDebugFormat tb__codeview_debug_format;
    extern IDebugFormat tb__sdg_debug_format;

    switch (debug_fmt) {
        case TB_DEBUGFMT_SDG: return &tb__sdg_debug_format;
        case TB_DEBUGFMT_CODEVIEW: return &tb__codeview_debug_format;
        default: return NULL;
    }
}

TB_API TB_ExportBuffer tb_module_object_export(TB_Module* m, TB_DebugFormat debug_fmt){
    typedef TB_ExportBuffer ExporterFn(TB_Module* restrict m, const IDebugFormat* dbg);

    // map target systems to exporters (maybe we wanna decouple this later)
    static ExporterFn* const fn[TB_SYSTEM_MAX] = {
        [TB_SYSTEM_WINDOWS] = tb_coff_write_output,
        [TB_SYSTEM_MACOS]   = tb_macho_write_output,
        [TB_SYSTEM_LINUX]   = tb_elf64obj_write_output,
    };

    assert(fn[m->target_system] != NULL && "TODO");
    TB_ExportBuffer e;
    CUIK_TIMED_BLOCK("export") {
        e = fn[m->target_system](m, find_debug_format(debug_fmt));
    }
    return e;
}

TB_API bool tb_export_buffer_to_file(TB_ExportBuffer buffer, const char* path) {
    if (buffer.total == 0) {
        fprintf(stderr, "\x1b[31merror\x1b[0m: could not export '%s' (no contents)\n", path);
        return false;
    }

    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        fprintf(stderr, "\x1b[31merror\x1b[0m: could not open file for writing! %s\n", path);
        return false;
    }

    for (TB_ExportChunk* c = buffer.head; c != NULL; c = c->next) {
        if (c->size > 0 && fwrite(c->data, c->size, 1, file) != 1) {
            fprintf(stderr, "\x1b[31merror\x1b[0m: could not write to file! %s (not enough storage?)\n", path);
            return false;
        }
    }

    fclose(file);
    return true;
}

TB_API void tb_export_buffer_free(TB_ExportBuffer buffer) {
    TB_ExportChunk* c = buffer.head;
    while (c != NULL) {
        TB_ExportChunk* next = c->next;
        tb_platform_heap_free(c);
        c = next;
    }
}

static int compare_symbols(const void* a, const void* b) {
    const TB_Symbol* sym_a = *(const TB_Symbol**) a;
    const TB_Symbol* sym_b = *(const TB_Symbol**) b;
    return (sym_a->ordinal > sym_b->ordinal) - (sym_a->ordinal < sym_b->ordinal);
}

static int compare_functions(const void* a, const void* b) {
    const TB_FunctionOutput* sym_a = *(const TB_FunctionOutput**) a;
    const TB_FunctionOutput* sym_b = *(const TB_FunctionOutput**) b;
    return (sym_a->ordinal > sym_b->ordinal) - (sym_a->ordinal < sym_b->ordinal);
}

static void layout_section(TB_ModuleSection* restrict section) {
    CUIK_TIMED_BLOCK_ARGS("layout section", section->name) {
        size_t offset = 0;

        dyn_array_for(i, section->globals) {
            TB_Global* g = section->globals[i];

            offset = align_up(offset, g->align);
            g->pos = offset;
            offset += g->size;
        }
        section->total_size = offset;
    }
}

ExportList tb_module_layout_sections(TB_Module* m) {
    TB_Arena* arena = &tb_thread_info(m)->tmp_arena;

    size_t external_count = 0;
    TB_External** externals = tb_arena_alloc(arena, m->symbol_count[TB_SYMBOL_EXTERNAL] * sizeof(TB_External*));

    // unpack function data into the streams we actually care for.
    // avoids needing to walk so many sparse data structures later on.
    TB_ThreadInfo* info = atomic_load_explicit(&m->first_info_in_module, memory_order_relaxed);
    while (info != NULL) {
        TB_ThreadInfo* next = info->next_in_module;

        // unpack symbols
        TB_Symbol** syms = (TB_Symbol**) info->symbols.data;
        if (syms) {
            size_t cap = 1ull << info->symbols.exp;
            for (size_t i = 0; i < cap; i++) {
                TB_Symbol* s = syms[i];
                if (s == NULL || s == NL_HASHSET_TOMB) continue;

                switch (atomic_load_explicit(&s->tag, memory_order_relaxed)) {
                    case TB_SYMBOL_FUNCTION: {
                        TB_Function* f = (TB_Function*) s;
                        TB_ModuleSection* sec = &m->sections[f->section];

                        // we only care for compiled functions
                        TB_FunctionOutput* out_f = f->output;
                        if (out_f != NULL) {
                            out_f->ordinal = f->super.ordinal;
                            dyn_array_put(sec->funcs, out_f);
                        }
                        break;
                    }
                    case TB_SYMBOL_GLOBAL: {
                        TB_Global* g = (TB_Global*) s;
                        TB_ModuleSection* sec = &m->sections[g->parent];
                        dyn_array_put(sec->globals, g);
                        break;
                    }
                    case TB_SYMBOL_EXTERNAL: {
                        // resolved externals are just globals or functions, we don't add them here
                        TB_External* e = (TB_External*) s;
                        if (atomic_load_explicit(&e->resolved, memory_order_relaxed) == NULL) {
                            externals[external_count++] = e;
                        }
                        break;
                    }
                    default: break;
                }
            }
        }

        info = next;
    }

    dyn_array_for(i, m->sections) {
        TB_ModuleSection* sec = &m->sections[i];

        CUIK_TIMED_BLOCK_ARGS("sort", sec->name) {
            qsort(sec->funcs, dyn_array_length(sec->funcs), sizeof(TB_FunctionOutput*), compare_functions);
            qsort(sec->globals, dyn_array_length(sec->globals), sizeof(TB_Symbol*), compare_symbols);
        }

        // layout
        CUIK_TIMED_BLOCK_ARGS("layout", sec->name) {
            // place functions first
            size_t offset = 0;
            dyn_array_for(i, sec->funcs) {
                sec->funcs[i]->code_pos = offset;
                offset += sec->funcs[i]->code_size;
            }

            // then globals
            dyn_array_for(i, sec->globals) {
                TB_Global* g = sec->globals[i];

                offset = align_up(offset, g->align);
                g->pos = offset;
                offset += g->size;
            }

            sec->total_size = offset;
        }
    }

    return (ExportList){ external_count, externals };
}

size_t tb_helper_write_section(TB_Module* m, size_t write_pos, TB_ModuleSection* section, uint8_t* output, uint32_t pos) {
    assert(write_pos == pos);
    uint8_t* data = &output[pos];

    // place functions
    dyn_array_for(i, section->funcs) {
        TB_FunctionOutput* out_f = section->funcs[i];

        if (out_f != NULL) {
            memcpy(data + out_f->code_pos, out_f->code, out_f->code_size);
        }
    }

    // place globals
    dyn_array_for(i, section->globals) {
        TB_Global* restrict g = section->globals[i];

        memset(&data[g->pos], 0, g->size);
        FOREACH_N(k, 0, g->obj_count) {
            if (g->objects[k].type == TB_INIT_OBJ_REGION) {
                assert(g->objects[k].offset + g->objects[k].region.size <= g->size);
                memcpy(&data[g->pos + g->objects[k].offset], g->objects[k].region.ptr, g->objects[k].region.size);
            }
        }
    }

    return write_pos + section->total_size;
}

size_t tb__layout_relocations(TB_Module* m, DynArray(TB_ModuleSection) sections, const ICodeGen* restrict code_gen, size_t output_size, size_t reloc_size) {
    // calculate relocation layout
    dyn_array_for(i, sections) {
        TB_ModuleSection* sec = &sections[i];
        size_t reloc_count = 0;

        dyn_array_for(i, sec->funcs) {
            reloc_count += code_gen->emit_call_patches(m, sec->funcs[i]);
        }

        dyn_array_for(j, sec->globals) {
            TB_Global* restrict g = sec->globals[j];
            FOREACH_N(k, 0, g->obj_count) {
                reloc_count += (g->objects[k].type == TB_INIT_OBJ_RELOC);
            }
        }

        sec->reloc_count = reloc_count;
        sec->reloc_pos = output_size;
        output_size += reloc_count * reloc_size;
    }

    return output_size;
}

TB_ExportChunk* tb_export_make_chunk(size_t size) {
    TB_ExportChunk* c = tb_platform_heap_alloc(sizeof(TB_ExportChunk) + size);
    c->next = NULL;
    c->pos  = 0;
    c->size = size;
    return c;
}

void tb_export_append_chunk(TB_ExportBuffer* buffer, TB_ExportChunk* c) {
    if (buffer->head == NULL) {
        buffer->head = buffer->tail = c;
    } else {
        buffer->tail->next = c;
        buffer->tail = c;
    }

    c->pos = buffer->total;
    buffer->total += c->size;
}
