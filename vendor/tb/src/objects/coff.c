#include "coff.h"

static int compare_relocs(const void* a, const void* b) {
    const COFF_ImageReloc* aa = (const COFF_ImageReloc*) a;
    const COFF_ImageReloc* bb = (const COFF_ImageReloc*) b;

    return aa->VirtualAddress - bb->VirtualAddress;
}

static int compare_pdata(const void* a, const void* b) {
    const uint32_t* sym_a = (const uint32_t*) a;
    const uint32_t* sym_b = (const uint32_t*) b;

    return (sym_a[0] > sym_b[0]) - (sym_a[0] < sym_b[0]);
}

static COFF_Symbol section_sym(const char* name, int num, int sc) {
    COFF_Symbol s = { .section_number = num, .storage_class = sc, .aux_symbols_count = 1 };
    strncpy((char*) s.short_name, name, 8);
    return s;
}

static COFF_AuxSectionSymbol section_aux_sym(COFF_SectionHeader* s, int num) {
    return (COFF_AuxSectionSymbol){
        .length = s->raw_data_size,
        .reloc_count = s->num_reloc,
        .number = num,
    };
}

typedef struct {
    size_t pos, size;
} StringTable;

// COFF unwind info is two sections, .pdata and .xdata
struct COFF_UnwindInfo {
    size_t section_num, patch_count;

    TB_ExportChunk* xdata_chunk;
    TB_ExportChunk* pdata_chunk;
    TB_ExportChunk* pdata_relocs;

    COFF_SectionHeader xdata_header;
    COFF_SectionHeader pdata_header;
};

static void generate_unwind_info(COFF_UnwindInfo* restrict u, const ICodeGen* restrict code_gen, size_t xdata_section, TB_ModuleSection* section) {
    size_t count = dyn_array_length(section->funcs);
    *u = (COFF_UnwindInfo){ .patch_count = count * 3, .section_num = (xdata_section+1) * 2 };

    // generate pdata
    u->pdata_chunk = tb_export_make_chunk(count * 3 * sizeof(uint32_t));
    uint32_t* pdata = (uint32_t*) u->pdata_chunk->data;

    bool overflow = count * 3 >= 0xFFFF;
    u->pdata_relocs = tb_export_make_chunk((overflow + count*3) * sizeof(COFF_ImageReloc));
    COFF_ImageReloc* relocs = (COFF_ImageReloc*) u->pdata_relocs->data;

    if (overflow) {
        *relocs++ = (COFF_ImageReloc){ .VirtualAddress = count * 3 };
    }

    // generate xdata
    TB_Emitter xdata = { 0 };
    DynArray(TB_FunctionOutput*) funcs = section->funcs;
    dyn_array_for(i, funcs) {
        TB_FunctionOutput* out_f = funcs[i];

        out_f->unwind_info = xdata.count;
        code_gen->emit_win64eh_unwind_info(&xdata, out_f, out_f->stack_usage);
        out_f->unwind_size = xdata.count - out_f->unwind_info;

        // write pdata
        size_t j = i*3;
        pdata[j+0] = 0;
        pdata[j+1] = out_f->code_size - out_f->nop_pads;
        pdata[j+2] = out_f->unwind_info;

        // pdata has relocations
        uint32_t sym = out_f->parent->super.symbol_id;
        relocs[j + 0] = (COFF_ImageReloc){
            .Type = IMAGE_REL_AMD64_ADDR32NB,
            .SymbolTableIndex = sym,
            .VirtualAddress = j * 4
        };

        relocs[j + 1] = (COFF_ImageReloc){
            .Type = IMAGE_REL_AMD64_ADDR32NB,
            .SymbolTableIndex = sym,
            .VirtualAddress = (j * 4) + 4
        };

        relocs[j + 2] = (COFF_ImageReloc){
            .Type = IMAGE_REL_AMD64_ADDR32NB,
            .SymbolTableIndex = u->section_num, // xdata section
            .VirtualAddress = (j * 4) + 8
        };
    }

    u->xdata_chunk = tb_export_make_chunk(xdata.count);
    memcpy(u->xdata_chunk->data, xdata.data, xdata.count);
    tb_platform_heap_free(xdata.data);

    // generate COFF headers
    u->pdata_header = (COFF_SectionHeader){
        .name = ".pdata",
        .characteristics = COFF_CHARACTERISTICS_RODATA | (u->patch_count >= 0xFFFF ? IMAGE_SCN_LNK_NRELOC_OVFL : 0),
        .raw_data_size = u->pdata_chunk->size,
        .num_reloc = (u->patch_count >= 0xFFFF ? 0xFFFF : u->patch_count),
    };

    u->xdata_header = (COFF_SectionHeader){
        .name = ".xdata",
        .characteristics = COFF_CHARACTERISTICS_RODATA,
        .raw_data_size = u->xdata_chunk->size,
    };
}

#define WRITE(data, size) (memcpy(&output[write_pos], data, size), write_pos += (size))
TB_ExportBuffer tb_coff_write_output(TB_Module* m, const IDebugFormat* dbg) {
    TB_Arena* arena = get_temporary_arena(m);
    TB_TemporaryStorage* tls = tb_tls_allocate();

    ExportList exports;
    CUIK_TIMED_BLOCK("layout section") {
        exports = tb_module_layout_sections(m);
    }

    // accumulate all sections
    DynArray(TB_ModuleSection) sections = m->sections;

    // mark correct flags on sections
    #define ON(mask, a, b) flags |= (sections[i].flags & mask ? (a) : (b))
    dyn_array_for(i, sections) {
        uint32_t flags = TB_COFF_SECTION_READ;
        ON(TB_MODULE_SECTION_WRITE, TB_COFF_SECTION_WRITE, 0);
        ON(TB_MODULE_SECTION_EXEC,  TB_COFF_SECTION_EXECUTE | TB_COFF_SECTION_CODE, TB_COFF_SECTION_INIT);
        if (sections[i].comdat.type != 0) flags |= TB_COFF_SECTION_COMDAT;
        sections[i].export_flags = flags;
    }
    #undef ON

    int dbg_section_count = (dbg ? dbg->number_of_debug_sections(m) : 0);
    int section_count = dyn_array_length(sections) + dbg_section_count;
    const ICodeGen* restrict code_gen = tb__find_code_generator(m);

    // mark each with a unique id
    size_t unique_id_counter = section_count * 2;
    CUIK_TIMED_BLOCK("alloc symbol IDs") {
        dyn_array_for(i, sections) {
            // unwind info
            if (code_gen->emit_win64eh_unwind_info && sections[i].funcs) {
                unique_id_counter += 4;
            }
        }

        dyn_array_for(i, sections) {
            DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
            DynArray(TB_Global*) globals = sections[i].globals;

            dyn_array_for(i, funcs) {
                funcs[i]->parent->super.symbol_id = unique_id_counter++;
            }

            dyn_array_for(i, globals) {
                globals[i]->super.symbol_id = unique_id_counter++;
            }
        }

        FOREACH_N(i, 0, exports.count) {
            exports.data[i]->super.symbol_id = unique_id_counter++;
        }
    }

    dyn_array_for(i, sections) {
        sections[i].section_num = 1 + i;

        // make unwind info for each section with functions
        if (code_gen->emit_win64eh_unwind_info && sections[i].funcs) {
            COFF_UnwindInfo* u = tb_arena_alloc(arena, sizeof(COFF_UnwindInfo));
            generate_unwind_info(u, code_gen, section_count, &sections[i]);

            sections[i].unwind = u;
            section_count += 2; // .pdata + .xdata
        }
    }

    size_t string_table_cap = unique_id_counter;
    const char** string_table = tb_arena_alloc(arena, string_table_cap * sizeof(char*));

    // COFF file header & section headers
    COFF_FileHeader header = {
        .section_count = section_count,
        .timestamp = 1056582000u, // my birthday since that's consistent :P
        .symbol_count = unique_id_counter,
        .symbol_table = 0,
        .flags = IMAGE_FILE_LINE_NUMS_STRIPPED
    };

    switch (m->target_arch) {
        case TB_ARCH_X86_64:  header.machine = COFF_MACHINE_AMD64; break;
        case TB_ARCH_AARCH64: header.machine = COFF_MACHINE_ARM64; break;
        default: tb_todo();
    }

    // calculate output size & layout object
    size_t output_size = sizeof(COFF_FileHeader);
    output_size += section_count * sizeof(COFF_SectionHeader);
    dyn_array_for(i, sections) {
        sections[i].raw_data_pos = output_size;
        output_size += sections[i].total_size;

        COFF_UnwindInfo* unwind = sections[i].unwind;
        if (unwind != NULL) {
            unwind->pdata_header.raw_data_pos = output_size;
            output_size += unwind->pdata_header.raw_data_size;

            unwind->xdata_header.raw_data_pos = output_size;
            output_size += unwind->xdata_header.raw_data_size;

            unwind->pdata_header.pointer_to_reloc = output_size;
            output_size += unwind->pdata_relocs->size;
        }
    }

    TB_SectionGroup debug_sections = { 0 };
    if (dbg) CUIK_TIMED_BLOCK("generate debug") {
        debug_sections = dbg->generate_debug_info(m, tls);
    }

    FOREACH_N(i, 0, debug_sections.length) {
        debug_sections.data[i].virtual_address = output_size;
        output_size += debug_sections.data[i].raw_data.length;
    }

    // calculate relocation layout
    output_size = tb__layout_relocations(m, sections, code_gen, output_size, sizeof(COFF_ImageReloc));

    FOREACH_N(i, 0, debug_sections.length) {
        size_t reloc_count = debug_sections.data[i].relocation_count;

        // we're storing the relocation array's file pos here
        debug_sections.data[i].user_data = (void*) (uintptr_t) output_size;
        output_size += reloc_count * sizeof(COFF_ImageReloc);
    }

    header.symbol_table = output_size;
    output_size += header.symbol_count * sizeof(COFF_Symbol);

    // first 4 bytes of the table are the size of the table
    StringTable strtbl = { .pos = output_size, .size = 4 };

    // compute string table size
    dyn_array_for(i, sections) {
        DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
        DynArray(TB_Global*) globals = sections[i].globals;

        // is a man not entitled to the dogshit of his cache locality?
        dyn_array_for(i, funcs) {
            size_t name_len = strlen(funcs[i]->parent->super.name);
            if (name_len >= 8) strtbl.size += name_len + 1;
        }

        dyn_array_for(i, globals) {
            size_t name_len = strlen(globals[i]->super.name);
            if (name_len >= 8) strtbl.size += name_len + 1;
        }
    }

    FOREACH_N(i, 0, exports.count) {
        size_t name_len = strlen(exports.data[i]->super.name);
        if (name_len >= 8) strtbl.size += name_len + 1;
    }

    output_size += strtbl.size;

    ////////////////////////////////
    // write output
    ////////////////////////////////
    TB_ExportBuffer buffer = { 0 };

    CUIK_TIMED_BLOCK("write output") {
        CUIK_TIMED_BLOCK("write headers") {
            TB_ExportChunk* headers = tb_export_make_chunk(sizeof(COFF_FileHeader) + (sizeof(COFF_SectionHeader) * section_count));

            // write COFF header
            COFF_FileHeader* file = (COFF_FileHeader*) headers->data;
            *file = header;

            // write sections headers
            COFF_SectionHeader* sec_headers = (COFF_SectionHeader*) (file + 1);
            dyn_array_for(i, sections) {
                COFF_SectionHeader header = {
                    .characteristics = sections[i].export_flags,
                    .raw_data_size = sections[i].total_size,
                    .raw_data_pos = sections[i].raw_data_pos,
                    .pointer_to_reloc = sections[i].reloc_pos
                };

                size_t len = strlen(sections[i].name);
                memcpy(header.name, sections[i].name, len > 8 ? 8 : len);

                if (sections[i].reloc_count >= 0xFFFF) {
                    header.num_reloc = 0xFFFF;
                    header.characteristics |= IMAGE_SCN_LNK_NRELOC_OVFL;
                } else {
                    header.num_reloc = sections[i].reloc_count;
                }

                *sec_headers++ = header;
            }

            FOREACH_N(i, 0, debug_sections.length) {
                COFF_SectionHeader header = {
                    .characteristics = COFF_CHARACTERISTICS_DEBUG,
                    .raw_data_size = debug_sections.data[i].raw_data.length,
                    .raw_data_pos = debug_sections.data[i].virtual_address,
                    .pointer_to_reloc = (uintptr_t) debug_sections.data[i].user_data
                };

                /*if (dbg == &tb__codeview_debug_format) {
                    header.characteristics |= IMAGE_SCN_MEM_DISCARDABLE;
                }*/

                TB_Slice name = debug_sections.data[i].name;
                assert(name.length <= 8);
                memcpy(header.name, name.data, name.length);

                size_t reloc_count = debug_sections.data[i].relocation_count;
                if (reloc_count >= 0xFFFF) {
                    header.num_reloc = 0xFFFF;
                    header.characteristics |= IMAGE_SCN_LNK_NRELOC_OVFL;
                } else {
                    header.num_reloc = reloc_count;
                }

                *sec_headers++ = header;
            }

            dyn_array_for(i, sections) {
                // pdata then xdata
                if (sections[i].unwind) {
                    memcpy(sec_headers + 0, &sections[i].unwind->pdata_header, sizeof(COFF_SectionHeader));
                    memcpy(sec_headers + 1, &sections[i].unwind->xdata_header, sizeof(COFF_SectionHeader));
                    sec_headers += 2;
                }
            }

            tb_export_append_chunk(&buffer, headers);
        }

        // write raw data
        dyn_array_for(i, sections) {
            TB_ExportChunk* sec = tb_export_make_chunk(sections[i].total_size);
            tb_helper_write_section(m, 0, &sections[i], sec->data, 0);
            tb_export_append_chunk(&buffer, sec);

            COFF_UnwindInfo* unwind = sections[i].unwind;
            if (unwind != NULL) {
                tb_export_append_chunk(&buffer, unwind->pdata_chunk);
                tb_export_append_chunk(&buffer, unwind->xdata_chunk);

                assert(unwind->pdata_header.pointer_to_reloc == buffer.total);
                tb_export_append_chunk(&buffer, unwind->pdata_relocs);
            }
        }

        FOREACH_N(i, 0, debug_sections.length) {
            TB_ExportChunk* sec = tb_export_make_chunk(debug_sections.data[i].raw_data.length);
            memcpy(sec->data, debug_sections.data[i].raw_data.data, debug_sections.data[i].raw_data.length);
            tb_export_append_chunk(&buffer, sec);
        }

        // write relocations
        dyn_array_for(i, sections) {
            size_t reloc_count = sections[i].reloc_count;
            DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
            DynArray(TB_Global*) globals = sections[i].globals;

            TB_ExportChunk* relocations = tb_export_make_chunk(reloc_count * sizeof(COFF_ImageReloc));
            COFF_ImageReloc* relocs = (COFF_ImageReloc*) relocations->data;

            dyn_array_for(j, funcs) {
                TB_FunctionOutput* func_out = funcs[j];
                size_t source_offset = func_out->code_pos;

                for (TB_SymbolPatch* p = func_out->first_patch; p; p = p->next) {
                    if (p->internal) continue;

                    size_t actual_pos = source_offset + p->pos;

                    TB_Symbol* target = p->target;
                    if (target->tag == TB_SYMBOL_EXTERNAL) {
                        TB_Symbol* resolved = atomic_load_explicit(&((TB_External*) target)->resolved, memory_order_relaxed);
                        if (resolved) target = resolved;
                    }

                    size_t symbol_id = target->symbol_id;
                    assert(symbol_id != 0);

                    if (target->tag == TB_SYMBOL_FUNCTION || target->tag == TB_SYMBOL_EXTERNAL) {
                        *relocs++ = (COFF_ImageReloc){
                            .Type = IMAGE_REL_AMD64_REL32,
                            .SymbolTableIndex = symbol_id,
                            .VirtualAddress = actual_pos
                        };
                    } else if (target->tag == TB_SYMBOL_GLOBAL) {
                        TB_Global* target_global = (TB_Global*) target;
                        bool is_tls = sections[target_global->parent].flags & TB_MODULE_SECTION_TLS;
                        *relocs++ = (COFF_ImageReloc){
                            .Type = is_tls ? IMAGE_REL_AMD64_SECREL : IMAGE_REL_AMD64_REL32,
                            .SymbolTableIndex = symbol_id,
                            .VirtualAddress = actual_pos
                        };
                    } else {
                        tb_todo();
                    }
                }
            }

            dyn_array_for(j, globals) {
                TB_Global* g = globals[j];

                FOREACH_N(k, 0, g->obj_count) {
                    size_t actual_pos = g->pos + g->objects[k].offset;

                    if (g->objects[k].type == TB_INIT_OBJ_RELOC) {
                        const TB_Symbol* s = g->objects[k].reloc;

                        *relocs++ = (COFF_ImageReloc){
                            .Type = IMAGE_REL_AMD64_ADDR64,
                            .SymbolTableIndex = s->symbol_id,
                            .VirtualAddress = actual_pos
                        };
                    }
                }
            }

            assert((relocs - (COFF_ImageReloc*) relocations->data) == reloc_count);
            tb_export_append_chunk(&buffer, relocations);

            assert(relocations->pos == sections[i].reloc_pos);
        }

        FOREACH_N(i, 0, debug_sections.length) {
            TB_ExportChunk* relocations = tb_export_make_chunk(debug_sections.data[i].relocation_count * sizeof(COFF_ImageReloc));
            COFF_ImageReloc* relocs = (COFF_ImageReloc*) relocations->data;

            FOREACH_N(j, 0, debug_sections.data[i].relocation_count) {
                TB_ObjectReloc* in_reloc = &debug_sections.data[i].relocations[j];

                int type = 0;
                switch (in_reloc->type) {
                    case TB_OBJECT_RELOC_SECREL:  type = IMAGE_REL_AMD64_SECREL; break;
                    case TB_OBJECT_RELOC_SECTION: type = IMAGE_REL_AMD64_SECTION; break;
                    case TB_OBJECT_RELOC_ADDR32NB:type = IMAGE_REL_AMD64_ADDR32NB; break;
                    default: tb_todo();
                }

                *relocs++ = (COFF_ImageReloc){
                    .Type = type,
                    .SymbolTableIndex = in_reloc->symbol_index,
                    .VirtualAddress = in_reloc->virtual_address
                };
            }

            tb_export_append_chunk(&buffer, relocations);
            assert(relocations->pos == (uintptr_t) debug_sections.data[i].user_data);
        }

        // write symbols
        uint32_t string_table_mark = 4;
        size_t string_table_length = 0;
        CUIK_TIMED_BLOCK("write symbols") {
            TB_ExportChunk* symtab = tb_export_make_chunk(header.symbol_count * sizeof(COFF_Symbol));

            size_t write_pos = 0;
            uint8_t* output = symtab->data;

            size_t symbol_count = 1;
            dyn_array_for(i, sections) {
                COFF_Symbol s = {
                    .section_number = symbol_count,
                    .storage_class = IMAGE_SYM_CLASS_STATIC,
                    .aux_symbols_count = 1
                };
                strncpy((char*) s.short_name, sections[i].name, 8);
                WRITE(&s, sizeof(s));

                COFF_AuxSectionSymbol aux = {
                    .length = sections[i].total_size,
                    .reloc_count = sections[i].reloc_count,
                    .number = symbol_count,
                    .selection = sections[i].comdat.type ? 2 /* pick any */ : 0,
                };
                WRITE(&aux, sizeof(aux));
                symbol_count++;
            }

            FOREACH_N(i, 0, debug_sections.length) {
                COFF_Symbol s = {
                    .section_number = symbol_count,
                    .storage_class = IMAGE_SYM_CLASS_STATIC,
                    .aux_symbols_count = 1
                };

                int l = debug_sections.data[i].name.length;
                if (l > 8) l = 8;
                strncpy((char*) s.short_name, (const char*) debug_sections.data[i].name.data, l);

                WRITE(&s, sizeof(s));

                COFF_AuxSectionSymbol aux = {
                    .length = debug_sections.data[i].raw_data.length,
                    .reloc_count = debug_sections.data[i].relocation_count,
                    .number = symbol_count,
                };
                WRITE(&aux, sizeof(aux));
                symbol_count++;
            }

            dyn_array_for(i, sections) {
                int section_num = sections[i].section_num;
                COFF_UnwindInfo* u = sections[i].unwind;
                if (u != NULL) {
                    COFF_Symbol s[2] = {
                        { // pdata
                            .short_name = ".pdata", .section_number = symbol_count,
                            .storage_class = IMAGE_SYM_CLASS_STATIC, .aux_symbols_count = 1
                        },
                        { // xdata
                            .short_name = ".xdata", .section_number = symbol_count + 1,
                            .storage_class = IMAGE_SYM_CLASS_STATIC, .aux_symbols_count = 1
                        },
                    };

                    COFF_AuxSectionSymbol aux[2] = {
                        { .length = u->pdata_chunk->size, .reloc_count = u->patch_count, .number = symbol_count }, // pdata
                        { .length = u->xdata_chunk->size, .number = symbol_count + 1 }, // .xdata
                    };

                    if (i > 0) {
                        // COMDAT functions go here
                        //   associative with their matching text section
                        aux[0].selection = 5;
                        aux[0].number = section_num;
                    }

                    WRITE(&s[0],   sizeof(COFF_Symbol));
                    WRITE(&aux[0], sizeof(COFF_AuxSectionSymbol));
                    WRITE(&s[1],   sizeof(COFF_Symbol));
                    WRITE(&aux[1], sizeof(COFF_AuxSectionSymbol));

                    symbol_count += 2;
                }
            }

            dyn_array_for(i, sections) {
                int section_num = sections[i].section_num;
                DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
                DynArray(TB_Global*) globals = sections[i].globals;

                dyn_array_for(j, funcs) {
                    TB_FunctionOutput* out_f = funcs[j];

                    bool is_extern = out_f->linkage == TB_LINKAGE_PUBLIC;
                    COFF_Symbol sym = {
                        .value = out_f->code_pos,
                        .section_number = section_num,
                        .storage_class = is_extern ? IMAGE_SYM_CLASS_EXTERNAL : IMAGE_SYM_CLASS_STATIC
                    };

                    const char* name = out_f->parent->super.name;
                    size_t name_len = strlen(name);
                    assert(name_len < UINT16_MAX);
                    if (name_len >= 8) {
                        sym.long_name[0] = 0; // this value is 0 for long names
                        sym.long_name[1] = string_table_mark;

                        string_table[string_table_length++] = (char*)name;
                        string_table_mark += name_len + 1;
                    } else {
                        memcpy(sym.short_name, name, name_len + 1);
                    }

                    WRITE(&sym, sizeof(sym));
                }

                dyn_array_for(j, globals) {
                    TB_Global* g = globals[j];
                    bool is_extern = g->linkage == TB_LINKAGE_PUBLIC;

                    assert(section_num == g->parent + 1);
                    COFF_Symbol sym = {
                        .value = g->pos,
                        .section_number = section_num,
                        .storage_class = is_extern ? IMAGE_SYM_CLASS_EXTERNAL : IMAGE_SYM_CLASS_STATIC
                    };

                    if (g->super.name[0] != 0) {
                        size_t name_len = strlen(g->super.name);
                        assert(name_len < UINT16_MAX);

                        if (name_len >= 8) {
                            sym.long_name[0] = 0; // this value is 0 for long names
                            sym.long_name[1] = string_table_mark;

                            string_table[string_table_length++] = g->super.name;
                            string_table_mark += name_len + 1;
                        } else {
                            memcpy(sym.short_name, g->super.name, name_len + 1);
                        }
                    } else {
                        snprintf((char*) sym.short_name, 8, "$%06zx", g->super.symbol_id);
                    }

                    WRITE(&sym, sizeof(sym));
                }
            }

            FOREACH_N(i, 0, exports.count) {
                TB_External* ext = exports.data[i];
                COFF_Symbol sym = {
                    .value = 0,
                    .section_number = 0,
                    .storage_class = IMAGE_SYM_CLASS_EXTERNAL
                };

                size_t name_len = strlen(ext->super.name);
                assert(name_len < UINT16_MAX);

                if (name_len >= 8) {
                    sym.long_name[0] = 0; // this value is 0 for long names
                    sym.long_name[1] = string_table_mark;

                    string_table[string_table_length++] = ext->super.name;
                    string_table_mark += name_len + 1;
                } else {
                    memcpy(sym.short_name, ext->super.name, name_len + 1);
                }

                WRITE(&sym, sizeof(sym));
            }

            assert(write_pos == symtab->size);
            tb_export_append_chunk(&buffer, symtab);
        }

        // write string table
        {
            // assert(write_pos == strtbl.pos);
            TB_ExportChunk* chunk = tb_export_make_chunk(strtbl.size);

            memcpy(chunk->data, &string_table_mark, sizeof(uint32_t));

            size_t j = 4;
            FOREACH_N(i, 0, string_table_length) {
                const char* s = string_table[i];
                size_t l = strlen(s) + 1;

                memcpy(&chunk->data[j], s, l), j += l;
            }

            tb_export_append_chunk(&buffer, chunk);
        }
    }

    tb_arena_clear(arena);
    return buffer;
}
