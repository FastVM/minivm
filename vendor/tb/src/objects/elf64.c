#include "../tb_internal.h"
#include <tb_elf.h>
#include <log.h>

static bool is_nonlocal(const TB_Symbol* s) {
    if (s->tag == TB_SYMBOL_GLOBAL) {
        return ((TB_Global*) s)->linkage == TB_LINKAGE_PUBLIC;
    } else if (s->tag == TB_SYMBOL_FUNCTION) {
        return ((TB_Function*) s)->linkage == TB_LINKAGE_PUBLIC;
    } else {
        return true;
    }
}

static int put_symbol(TB_Emitter* stab, uint32_t name, uint8_t sym_info, uint16_t section_index, uint64_t value, uint64_t size) {
    // Emit symbol
    TB_Elf64_Sym sym = {
        .name  = name,
        .info  = sym_info,
        .shndx = section_index,
        .value = value,
        .size  = size
    };
    tb_outs(stab, sizeof(sym), (uint8_t*)&sym);
    return (stab->count / sizeof(TB_Elf64_Sym)) - 1;
}

static void put_section_symbols(DynArray(TB_ModuleSection) sections, TB_Emitter* strtbl, TB_Emitter* stab, int t) {
    dyn_array_for(i, sections) {
        int sec_num = sections[i].section_num;
        DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
        DynArray(TB_Global*) globals = sections[i].globals;

        dyn_array_for(i, funcs) {
            TB_FunctionOutput* out_f = funcs[i];
            const char* name_str = out_f->parent->super.name;

            uint32_t name = name_str ? tb_outstr_nul_UNSAFE(strtbl, name_str) : 0;
            out_f->parent->super.symbol_id = put_symbol(stab, name, TB_ELF64_ST_INFO(t, TB_ELF64_STT_FUNC), sec_num, out_f->code_pos, out_f->code_size);
        }

        int acceptable = t == TB_ELF64_STB_GLOBAL ? TB_LINKAGE_PUBLIC : TB_LINKAGE_PRIVATE;
        dyn_array_for(i, globals) {
            TB_Global* g = globals[i];
            if (g->linkage != acceptable) {
                continue;
            }

            uint32_t name = 0;
            if (g->super.name) {
                name = tb_outstr_nul_UNSAFE(strtbl, g->super.name);
            } else {
                char buf[8];
                snprintf(buf, 8, "$%d_%td", sec_num, i);
                name = tb_outstr_nul_UNSAFE(strtbl, buf);
            }

            g->super.symbol_id = put_symbol(stab, name, TB_ELF64_ST_INFO(t, TB_ELF64_STT_OBJECT), sec_num, g->pos, 0);
        }
    }
}

#define WRITE(data, size) (memcpy(&output[write_pos], data, size), write_pos += (size))
TB_ExportBuffer tb_elf64obj_write_output(TB_Module* m, const IDebugFormat* dbg) {
    ExportList exports;
    CUIK_TIMED_BLOCK("layout section") {
        exports = tb_module_layout_sections(m);
    }

    const ICodeGen* restrict code_gen = tb__find_code_generator(m);

    uint16_t machine = 0;
    switch (m->target_arch) {
        case TB_ARCH_X86_64:  machine = TB_EM_X86_64;  break;
        case TB_ARCH_AARCH64: machine = TB_EM_AARCH64; break;
        case TB_ARCH_MIPS32:  machine = TB_EM_MIPS;    break;
        case TB_ARCH_MIPS64:  machine = TB_EM_MIPS;    break;
        default: tb_todo();
    }

    TB_Emitter strtbl = { 0 };
    tb_out_reserve(&strtbl, 1024);
    tb_out1b(&strtbl, 0); // null string in the table

    TB_Elf64_Ehdr header = {
        .ident = {
            [TB_EI_MAG0]       = 0x7F, // magic number
            [TB_EI_MAG1]       = 'E',
            [TB_EI_MAG2]       = 'L',
            [TB_EI_MAG3]       = 'F',
            [TB_EI_CLASS]      = 2, // 64bit ELF file
            [TB_EI_DATA]       = 1, // little-endian
            [TB_EI_VERSION]    = 1, // 1.0
            [TB_EI_OSABI]      = 0,
            [TB_EI_ABIVERSION] = 0
        },
        .type = TB_ET_REL, // relocatable
        .version = 1,
        .machine = machine,
        .entry = 0,

        // section headers go at the end of the file
        // and are filed in later.
        .shoff = 0,
        .flags = 0,

        .ehsize = sizeof(TB_Elf64_Ehdr),

        .shentsize = sizeof(TB_Elf64_Shdr),
        .shstrndx  = 1,
    };

    // accumulate all sections
    DynArray(TB_ModuleSection) sections = m->sections;

    int dbg_section_count = (dbg ? dbg->number_of_debug_sections(m) : 0);
    int section_count = 2 + dyn_array_length(sections) + dbg_section_count;

    size_t output_size = sizeof(TB_Elf64_Ehdr);
    dyn_array_for(i, sections) {
        sections[i].section_num = 3 + i;
        sections[i].raw_data_pos = output_size;
        output_size += sections[i].total_size;
    }

    // calculate relocation layout
    // each section with relocations needs a matching .rel section
    output_size = tb__layout_relocations(m, sections, code_gen, output_size, sizeof(TB_Elf64_Rela));
    dyn_array_for(i, sections) {
        if (sections[i].reloc_count > 0) {
            section_count += 1;
            tb_outs(&strtbl, 5, ".rela");
        }

        sections[i].name_pos = tb_outstr_nul_UNSAFE(&strtbl, sections[i].name);
    }

    // calculate symbol IDs
    TB_Emitter local_symtab = { 0 }, global_symtab = { 0 };
    tb_out_zero(&local_symtab, sizeof(TB_Elf64_Sym));
    dyn_array_for(i, sections) {
        put_symbol(&local_symtab, sections[i].name_pos, TB_ELF64_ST_INFO(TB_ELF64_STB_LOCAL, TB_ELF64_STT_SECTION), 1 + i, 0, 0);
    }

    // .rela sections
    dyn_array_for(i, sections) if (sections[i].reloc_count) {
        put_symbol(&local_symtab, sections[i].name_pos - 5, TB_ELF64_ST_INFO(TB_ELF64_STB_LOCAL, TB_ELF64_STT_SECTION), 1 + i, 0, 0);
    }

    assert(dbg_section_count == 0);

    put_section_symbols(sections, &strtbl, &local_symtab, TB_ELF64_STB_LOCAL);
    put_section_symbols(sections, &strtbl, &global_symtab, TB_ELF64_STB_GLOBAL);

    FOREACH_N(i, 0, exports.count) {
        TB_External* ext = exports.data[i];
        uint32_t name = tb_outstr_nul_UNSAFE(&strtbl, ext->super.name);
        ext->super.symbol_id = global_symtab.count / sizeof(TB_Elf64_Sym);

        put_symbol(&global_symtab, name, TB_ELF64_ST_INFO(TB_ELF64_STB_GLOBAL, 0), 0, 0, 0);
    }

    uint32_t symtab_name = tb_outstr_nul_UNSAFE(&strtbl, ".symtab");
    TB_Elf64_Shdr strtab = {
        .name = tb_outstr_nul_UNSAFE(&strtbl, ".strtab"),
        .type = TB_SHT_STRTAB,
        .flags = 0,
        .addralign = 1,
        .size = strtbl.count,
        .offset = output_size,
    };
    output_size += strtbl.count;

    TB_Elf64_Shdr symtab = {
        .name = symtab_name,
        .type = TB_SHT_SYMTAB,
        .flags = 0, .addralign = 1,
        .link = 1, .info = local_symtab.count / sizeof(TB_Elf64_Sym), /* first non-local */
        .size = local_symtab.count + global_symtab.count,
        .entsize = sizeof(TB_Elf64_Sym),
        .offset = output_size,
    };
    output_size += local_symtab.count;
    output_size += global_symtab.count;

    header.shoff = output_size;
    header.shnum = section_count + 1;
    // sections plus the NULL section at the start
    output_size += (1 + section_count) * sizeof(TB_Elf64_Shdr);

    ////////////////////////////////
    // write output
    ////////////////////////////////
    size_t write_pos = 0;
    TB_ExportChunk* chunk = tb_export_make_chunk(output_size);
    uint8_t* restrict output = chunk->data;

    WRITE(&header, sizeof(header));

    // write section content
    dyn_array_for(i, sections) {
        write_pos = tb_helper_write_section(m, write_pos, &sections[i], output, sections[i].raw_data_pos);
    }

    // write relocation arrays
    size_t local_sym_count = local_symtab.count / sizeof(TB_Elf64_Sym);
    dyn_array_for(i, sections) if (sections[i].reloc_count > 0) {
        assert(sections[i].reloc_pos == write_pos);
        TB_Elf64_Rela* rels = (TB_Elf64_Rela*) &output[write_pos];
        DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
        DynArray(TB_Global*) globals = sections[i].globals;

        dyn_array_for(j, funcs) {
            TB_FunctionOutput* func_out = funcs[j];

            size_t source_offset = func_out->code_pos;
            for (TB_SymbolPatch* p = func_out->first_patch; p; p = p->next) {
                if (p->internal) continue;

                size_t actual_pos = source_offset + p->pos;
                size_t symbol_id = p->target->symbol_id;
                if (is_nonlocal(p->target)) {
                    symbol_id += local_sym_count;
                }
                assert(symbol_id != 0);

                TB_ELF_RelocType type = p->target->tag == TB_SYMBOL_GLOBAL ? TB_ELF_X86_64_PC32 : TB_ELF_X86_64_PLT32;
                *rels++ = (TB_Elf64_Rela){
                    .offset = actual_pos,
                    // check when we should prefer R_X86_64_GOTPCREL
                    .info   = TB_ELF64_R_INFO(symbol_id, type),
                    .addend = -4
                };
            }
        }

        write_pos += sections[i].reloc_count * sizeof(TB_Elf64_Rela);
    }

    assert(write_pos == strtab.offset);
    WRITE(strtbl.data, strtbl.count);

    assert(write_pos == symtab.offset);
    WRITE(local_symtab.data, local_symtab.count);
    WRITE(global_symtab.data, global_symtab.count);

    // write section header
    memset(&output[write_pos], 0, sizeof(TB_Elf64_Shdr)), write_pos += sizeof(TB_Elf64_Shdr);
    WRITE(&strtab, sizeof(strtab));
    WRITE(&symtab, sizeof(symtab));
    dyn_array_for(i, sections) {
        TB_Elf64_Shdr sec = {
            .name = sections[i].name_pos,
            .type = TB_SHT_PROGBITS,
            .flags = TB_SHF_ALLOC,
            .addralign = 16,
            .size = sections[i].total_size,
            .offset = sections[i].raw_data_pos,
        };

        if (sections[i].flags & TB_MODULE_SECTION_WRITE) {
            sec.flags |= TB_SHF_WRITE;
        }

        if (sections[i].flags & TB_MODULE_SECTION_EXEC) {
            sec.flags |= TB_SHF_EXECINSTR;
        }

        WRITE(&sec, sizeof(sec));
    }

    dyn_array_for(i, sections) if (sections[i].reloc_count) {
        TB_Elf64_Shdr sec = {
            .name = sections[i].name_pos - 5,
            .type = TB_SHT_RELA,
            .flags = TB_SHF_INFO_LINK,
            .addralign = 16,
            .info = 3 + i,
            .link = 2,
            .size = sections[i].reloc_count * sizeof(TB_Elf64_Rela),
            .offset = sections[i].reloc_pos,
            .entsize = sizeof(TB_Elf64_Rela)
        };
        WRITE(&sec, sizeof(sec));
    }

    assert(write_pos == output_size);
    return (TB_ExportBuffer){ .total = output_size, .head = chunk, .tail = chunk };
}
