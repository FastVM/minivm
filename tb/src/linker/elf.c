#define NL_STRING_MAP_IMPL
#include "linker.h"
#include <tb_elf.h>

static void elf_append_object(TB_Linker* l, TB_Slice obj_name, TB_Slice content) {
    // implement this
}

static void elf_append_library(TB_Linker* l, TB_Slice ar_name, TB_Slice ar_file) {
    // implement this
}

static void elf_append_module(TB_Linker* l, TB_Module* m) {
    CUIK_TIMED_BLOCK("layout section") {
        tb_module_layout_sections(m);
    }

    TB_LinkerInputHandle mod_index = tb__track_module(l, 0, m);

    DynArray(TB_ModuleSection) sections = m->sections;
    dyn_array_for(i, sections) {
        uint32_t flags = TB_PF_R;
        if (sections[i].flags & TB_MODULE_SECTION_WRITE) flags |= TB_PF_W;
        if (sections[i].flags & TB_MODULE_SECTION_EXEC)  flags |= TB_PF_X;
        tb__append_module_section(l, mod_index, &sections[i], sections[i].name, flags);
    }

    tb__append_module_symbols(l, m);
}

static TB_LinkerSymbol* elf_resolve_sym(TB_Linker* l, TB_LinkerSymbol* sym, TB_Slice name, TB_Slice* alt, uint32_t reloc_i) {
    // resolve any by-name symbols
    if (sym == NULL) {
        sym = tb__find_symbol(&l->symtab, name);
        if (sym != NULL) goto done;

        if (alt) {
            sym = tb__find_symbol(&l->symtab, *alt);
            if (sym != NULL) goto done;
        }

        tb__unresolved_symbol(l, name)->reloc = reloc_i;
        return NULL;
    }

    done:
    return sym;
}

static void elf_init(TB_Linker* l) {
    l->entrypoint = "_start";
    l->resolve_sym = elf_resolve_sym;
}

#define WRITE(data, size) (memcpy(&output[write_pos], data, size), write_pos += (size))
static TB_ExportBuffer elf_export(TB_Linker* l) {
    CUIK_TIMED_BLOCK("GC sections") {
        gc_mark_root(l, l->entrypoint);
    }

    if (!tb__finalize_sections(l)) {
        return (TB_ExportBuffer){ 0 };
    }

    TB_Emitter strtbl = { 0 };
    tb_out_reserve(&strtbl, 1024);
    tb_out1b(&strtbl, 0); // null string in the table

    size_t final_section_count = 0;
    nl_map_for_str(i, l->sections) {
        if (l->sections[i].v->generic_flags & TB_LINKER_SECTION_DISCARD) continue;

        // reserve space for names
        NL_Slice name = l->sections[i].v->name;
        l->sections[i].v->name_pos = tb_outs(&strtbl, name.length + 1, name.data);
        tb_out1b_UNSAFE(&strtbl, 0);

        // we're keeping it for export
        final_section_count += 1;
    }

    TB_Elf64_Shdr strtab = {
        .name = tb_outstr_nul_UNSAFE(&strtbl, ".strtab"),
        .type = TB_SHT_STRTAB,
        .flags = 0,
        .addralign = 1,
        .size = strtbl.count,
    };

    size_t size_of_headers = sizeof(TB_Elf64_Ehdr)
        + (final_section_count * sizeof(TB_Elf64_Phdr))
        + ((2+final_section_count) * sizeof(TB_Elf64_Shdr));

    size_t section_content_size = 0;
    uint64_t virt_addr = size_of_headers;
    CUIK_TIMED_BLOCK("layout sections") {
        nl_map_for_str(i, l->sections) {
            TB_LinkerSection* s = l->sections[i].v;
            if (s->generic_flags & TB_LINKER_SECTION_DISCARD) continue;

            s->offset = size_of_headers + section_content_size;
            section_content_size += s->total_size;

            s->address = virt_addr;
            virt_addr += s->total_size;
            // virt_addr = align_up(virt_addr + s->total_size, 4096);
        }
    }

    strtab.offset = size_of_headers + section_content_size;
    section_content_size += strtbl.count;

    uint16_t machine = 0;
    switch (l->target_arch) {
        case TB_ARCH_X86_64: machine = TB_EM_X86_64; break;
        case TB_ARCH_AARCH64: machine = TB_EM_AARCH64; break;
        default: tb_todo();
    }

    size_t output_size = size_of_headers + section_content_size;
    size_t write_pos = 0;

    TB_ExportChunk* chunk = tb_export_make_chunk(output_size);
    uint8_t* restrict output = chunk->data;

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
        .type = TB_ET_DYN, // executable
        .version = 1,
        .machine = machine,
        .entry = 0,

        .flags = 0,

        .ehsize = sizeof(TB_Elf64_Ehdr),

        .phentsize = sizeof(TB_Elf64_Phdr),
        .phoff     = sizeof(TB_Elf64_Ehdr),
        .phnum     = final_section_count,

        .shoff = sizeof(TB_Elf64_Ehdr) + (sizeof(TB_Elf64_Phdr) * final_section_count),
        .shentsize = sizeof(TB_Elf64_Shdr),
        .shnum = final_section_count + 2,
        .shstrndx  = 1,
    };

    // text section crap
    TB_LinkerSection* text = tb__find_section(l, ".text");
    TB_LinkerSymbol* sym = tb__find_symbol_cstr(&l->symtab, l->entrypoint);
    if (text && sym) {
        if (sym->tag == TB_LINKER_SYMBOL_NORMAL) {
            header.entry = text->address + sym->normal.piece->offset + sym->normal.secrel;
        } else if (sym->tag == TB_LINKER_SYMBOL_TB) {
            header.entry = text->address + sym->tb.piece->offset + tb__get_symbol_pos(sym->tb.sym);
        } else {
            tb_todo();
        }
    } else {
        fprintf(stderr, "tblink: could not find entrypoint!\n");
    }
    WRITE(&header, sizeof(header));

    // write program headers
    nl_map_for_str(i, l->sections) {
        TB_LinkerSection* s = l->sections[i].v;
        TB_Elf64_Phdr sec = {
            .type   = TB_PT_LOAD,
            .flags  = s->flags,
            .offset = s->offset,
            .vaddr  = s->address,
            .filesz = s->total_size,
            .memsz  = s->total_size,
            .align  = 1,
        };
        WRITE(&sec, sizeof(sec));
    }

    // write section headers
    memset(&output[write_pos], 0, sizeof(TB_Elf64_Shdr)), write_pos += sizeof(TB_Elf64_Shdr);
    WRITE(&strtab, sizeof(strtab));
    nl_map_for_str(i, l->sections) {
        TB_LinkerSection* s = l->sections[i].v;
        TB_Elf64_Shdr sec = {
            .name = s->name_pos,
            .type = TB_SHT_PROGBITS,
            .flags = TB_SHF_ALLOC | ((s->flags & TB_PF_X) ? TB_SHF_EXECINSTR : 0) | ((s->flags & TB_PF_W) ? TB_SHF_WRITE : 0),
            .addralign = 1,
            .size = s->total_size,
            .addr = s->address,
            .offset = s->offset,
        };
        WRITE(&sec, sizeof(sec));
    }

    TB_LinkerSection* data  = tb__find_section(l, ".data");
    TB_LinkerSection* rdata = tb__find_section(l, ".rdata");

    // write section contents
    write_pos = tb__apply_section_contents(l, output, write_pos, text, data, rdata, 1, 0);
    WRITE(strtbl.data, strtbl.count);
    assert(write_pos == output_size);

    // TODO(NeGate): multithread this too
    CUIK_TIMED_BLOCK("apply final relocations") {
        dyn_array_for(i, l->ir_modules) {
            tb__apply_module_relocs(l, l->ir_modules[i], output);
        }

        // tb__apply_external_relocs(l, output, opt_header.image_base);
    }

    // write section contents
    return (TB_ExportBuffer){ .total = output_size, .head = chunk, .tail = chunk };
}

TB_LinkerVtbl tb__linker_elf = {
    .init           = elf_init,
    .append_object  = elf_append_object,
    .append_library = elf_append_library,
    .append_module  = elf_append_module,
    .export         = elf_export
};
