#define NL_STRING_MAP_IMPL
#include "linker.h"
#include "../objects/coff.h"
#include "../objects/lib_parse.h"

#include <ctype.h>

typedef struct {
    uint16_t page_rva;
    uint16_t block_size; // includes the header
    uint16_t payload[];
} BaseRelocSegment;

enum { IMP_PREFIX_LEN = sizeof("__imp_") - 1 };

const static uint8_t dos_stub[] = {
    // header
    0x4d,0x5a,0x78,0x00,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x00,0x00,0x00,

    // machine code
    0x0e,0x1f,0xba,0x0e,0x00,0xb4,0x09,0xcd,0x21,0xb8,0x01,0x4c,0xcd,0x21,0x54,0x68,
    0x69,0x73,0x20,0x70,0x72,0x6f,0x67,0x72,0x61,0x6d,0x20,0x63,0x61,0x6e,0x6e,0x6f,
    0x74,0x20,0x62,0x65,0x20,0x72,0x75,0x6e,0x20,0x69,0x6e,0x20,0x44,0x4f,0x53,0x20,
    0x6d,0x6f,0x64,0x65,0x2e,0x24,0x00,0x00
};

static int symbol_cmp(const void* a, const void* b) {
    const TB_ObjectSymbol* sym_a = (const TB_ObjectSymbol*)a;
    const TB_ObjectSymbol* sym_b = (const TB_ObjectSymbol*)b;

    return sym_a->ordinal - sym_b->ordinal;
}

// true if we replace the old one
static bool process_comdat(int select, TB_LinkerSectionPiece* old_p, TB_LinkerSectionPiece* new_p) {
    switch (select) {
        case 2: return false;                     // any
        case 6: return new_p->size > old_p->size; // largest
        default: tb_todo();
    }
}

// Musl's impl for this
static int string_case_cmp(const char *_l, const char *_r, size_t n) {
    const unsigned char *l=(void *)_l, *r=(void *)_r;
    if (!n--) return 0;
    for (; *l && *r && n && (*l == *r || tolower(*l) == tolower(*r)); l++, r++, n--);
    return tolower(*l) - tolower(*r);
}

static bool strequals(TB_Slice a, const char* b) {
    return a.length == strlen(b) && memcmp(a.data, b, a.length) == 0;
}

static bool strprefix(const char* str, const char* pre, size_t len) {
    size_t prelen = strlen(pre);
    return string_case_cmp(pre, str, len < prelen ? len : prelen) == 0;
}

void pe_append_object(TB_Linker* l, TB_LinkerThreadInfo* info, TB_Slice obj_name, TB_Slice content) {
    TB_COFF_Parser parser = { obj_name, content };
    tb_coff_parse_init(&parser);

    // We don't *really* care about this info beyond nicer errors
    TB_LinkerObject* obj_file = tb_arena_alloc(info->perm_arena, sizeof(TB_LinkerObject));
    *obj_file = (TB_LinkerObject){ .len = obj_name.length, .name = (char*) obj_name.data };

    TB_ArenaSavepoint sp = tb_arena_save(info->tmp_arena);

    // Apply all sections (generate lookup for sections based on ordinals)
    TB_LinkerSectionPiece *text_piece = NULL, *pdata_piece = NULL;
    TB_ObjectSection* sections = tb_arena_alloc(info->tmp_arena, parser.section_count * sizeof(TB_ObjectSection));
    FOREACH_N(i, 0, parser.section_count) {
        TB_ObjectSection* restrict s = &sections[i];
        tb_coff_parse_section(&parser, i, s);

        // trim the dollar sign (if applies)
        uint32_t order = 0;
        FOREACH_N(j, 0, s->name.length) {
            if (s->name.data[j] == '$') {
                // convert letters into score
                FOREACH_N(k, j + 1, s->name.length) {
                    order <<= 8;
                    order += s->name.data[k];
                }

                s->name.length = j;
                break;
            }
        }

        size_t drectve_len = sizeof(".drectve")-1;

        if (s->name.length == drectve_len && memcmp(s->name.data, ".drectve", drectve_len) == 0 && s->raw_data.length > 3) {
            const uint8_t* curr = s->raw_data.data + 3;
            const uint8_t* end_directive = s->raw_data.data + s->raw_data.length;

            while (curr != end_directive) {
                const uint8_t* end = curr;
                while (end != end_directive && *end != ' ') end++;

                log_info("directive: %.*s", (int) (end - curr), curr);

                if (curr == end_directive) {
                    break;
                } else if (strprefix((const char*) curr, "/merge:", end - curr)) {
                    curr += sizeof("/merge:")-1;

                    const uint8_t* equals = curr;
                    while (*equals && *equals != '=') equals++;

                    if (*equals == '=') {
                        TB_LinkerCmd cmd = {
                            .from = { curr, equals - curr },
                            .to   = { equals + 1, (end - equals) - 1 },
                        };
                        dyn_array_put(info->merges, cmd);
                    }
                } else if (strprefix((const char*) curr, "/defaultlib:", end - curr)) {
                    curr += sizeof("/defaultlib:")-1;

                    // TB_LinkerMsg m = { .tag = TB_LINKER_MSG_IMPORT, .import_path = { end - curr, (const char*) curr } };
                    // tb_linker_send_msg(l, &m);
                } else if (strprefix((const char*) curr, "/alternatename:", end - curr)) {
                    curr += sizeof("/alternatename:")-1;

                    const uint8_t* equals = curr;
                    while (*equals && *equals != '=') equals++;

                    if (*equals == '=') {
                        TB_LinkerCmd cmd = {
                            .from = { curr, equals - curr },
                            .to   = { equals + 1, (end - equals) - 1 },
                        };
                        dyn_array_put(info->alternates, cmd);
                    }
                } else {
                    log_warn("unknown linker directive: %.*s", (int) (end - curr), curr);
                    break;
                }

                curr = end+1;
            }

            continue;
        }

        // remove all the alignment flags, they don't appear in linker sections
        TB_LinkerInternStr str = tb_linker_intern_string(l, s->name.length, (char*) s->name.data);
        TB_LinkerSection* ls = tb_linker_find_or_create_section(l, str, s->flags & ~0x00F00000);

        if (s->flags & IMAGE_SCN_LNK_REMOVE) {
            ls->generic_flags |= TB_LINKER_SECTION_DISCARD;
        }

        if (s->flags & IMAGE_SCN_LNK_COMDAT) {
            ls->generic_flags |= TB_LINKER_SECTION_COMDAT;
        }

        const void* raw_data = s->raw_data.data;
        if (s->flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
            raw_data = NULL;
        }

        TB_LinkerSectionPiece* p = tb_linker_append_piece(info, ls, PIECE_NORMAL, s->raw_data.length, raw_data, obj_file);
        s->user_data = p;

        p->order = order;
        p->flags = 1;
        // p->vsize = s->virtual_size;

        if (s->name.length == 5 && memcmp(s->name.data, ".text", 5) == 0) {
            text_piece = p;
        } else if (s->name.length == 6 && memcmp(s->name.data, ".pdata", 6) == 0) {
            pdata_piece = p;
        }
    }

    // associate the pdata with the text
    if (text_piece) {
        tb_linker_associate(l, text_piece, pdata_piece);
    }

    // append all symbols
    size_t sym_count = 0;
    TB_ObjectSymbol* syms = tb_arena_alloc(info->tmp_arena, sizeof(TB_ObjectSymbol) * parser.symbol_count);

    COFF_AuxSectionSymbol* comdat_aux = NULL;
    CUIK_TIMED_BLOCK("apply symbols") {
        size_t i = 0;
        while (i < parser.symbol_count) {
            TB_ObjectSymbol* restrict sym = &syms[sym_count++];
            i += tb_coff_parse_symbol(&parser, i, sym);

            TB_LinkerInternStr name = tb_linker_intern_string(l, sym->name.length, (const char*) sym->name.data);
            TB_LinkerSymbol* s = NULL;

            if (sym->section_num > 0) {
                TB_ObjectSection* sec = &sections[sym->section_num - 1];
                if (sec->name.length == sym->name.length && memcmp(sym->name.data, sec->name.data, sym->name.length) == 0) {
                    // we're a section symbol
                    // COMDAT is how linkers handle merging of inline functions in C++
                    if ((sec->flags & IMAGE_SCN_LNK_COMDAT)) {
                        // the next symbol is the actual COMDAT symbol
                        comdat_aux = sym->extra;
                    }

                    // sections without a piece are ok
                    if (sec->user_data == NULL) {
                        continue;
                    }
                }

                TB_LinkerSectionPiece* p = sec->user_data;
                assert(p != NULL);
                if (sym->type == TB_OBJECT_SYMBOL_STATIC) {
                    s = tb_arena_alloc(info->perm_arena, sizeof(TB_LinkerSymbol));
                    *s = (TB_LinkerSymbol){ .name = name };
                    s->tag = TB_LINKER_SYMBOL_NORMAL;
                    s->normal.piece = p;
                    s->normal.secrel = sym->value;
                } else if (sym->type == TB_OBJECT_SYMBOL_WEAK_EXTERN) {
                    if (comdat_aux) {
                        assert(0 && "COMDAT and weak external?");
                        comdat_aux = NULL;
                    }

                    s = tb_linker_new_symbol(l, name);
                    s->tag = TB_LINKER_SYMBOL_NORMAL;
                    s->normal.piece = p;
                    s->normal.secrel = sym->value;
                } else if (sym->type == TB_OBJECT_SYMBOL_EXTERN) {
                    if (comdat_aux) {
                        // check if it already exists as a COMDAT
                        s = tb_linker_find_symbol(l, name);
                        if (s && (s->flags & TB_LINKER_SYMBOL_COMDAT)) {
                            assert(s->tag == TB_LINKER_SYMBOL_NORMAL);

                            bool replace = process_comdat(comdat_aux->selection, s->normal.piece, p);
                            if (replace) {
                                s->normal.piece->size = 0;
                            } else {
                                p->size = 0;
                            }
                        }

                        if (s == NULL) {
                            s = tb_linker_new_symbol(l, name);
                            s->tag = TB_LINKER_SYMBOL_NORMAL;
                            s->flags |= TB_LINKER_SYMBOL_COMDAT;
                            s->normal.piece = p;
                            s->normal.secrel = sym->value;
                        }

                        comdat_aux = NULL;
                    } else {
                        s = tb_linker_find_symbol(l, name);
                        if (s == NULL) {
                            s = tb_linker_new_symbol(l, name);
                            s->tag = TB_LINKER_SYMBOL_NORMAL;
                        } else if (s->tag == TB_LINKER_SYMBOL_UNKNOWN) {
                            s->tag = TB_LINKER_SYMBOL_NORMAL;
                        }

                        assert(s->tag == TB_LINKER_SYMBOL_NORMAL);
                        s->normal.piece = p;
                        s->normal.secrel = sym->value;
                    }
                }

                // add to the section piece's symbol list
                if (s) {
                    s->next = p->first_sym;
                    p->first_sym = s;
                }
            } else if (sym->type == TB_OBJECT_SYMBOL_EXTERN || sym->type == TB_OBJECT_SYMBOL_WEAK_EXTERN) {
                // symbols without a section number are proper externals (ones defined somewhere
                // else that we might want)
                s = tb_linker_find_symbol(l, name);
                if (s == NULL) {
                    s = tb_linker_new_symbol(l, name);
                    s->tag = TB_LINKER_SYMBOL_UNKNOWN;
                }
            } else {
                log_debug("skipped %s", name);
            }

            sym->user_data = s;
        }
    }

    CUIK_TIMED_BLOCK("parse relocations") FOREACH_N(i, 1, parser.section_count) {
        TB_ObjectSection* restrict s = &sections[i];
        TB_LinkerSectionPiece* restrict p = s->user_data;

        assert(p->info == NULL);
        p->info = info;

        // some relocations point to sections within the same object file, we resolve
        // their symbols early.
        FOREACH_N(j, 0, s->relocation_count) {
            TB_ObjectReloc* restrict reloc = &s->relocations[j];

            // resolve address used in relocation, symbols are sorted so we can binary search
            TB_ObjectSymbol key = { .ordinal = reloc->symbol_index };
            TB_ObjectSymbol* restrict src_symbol = bsearch(&key, syms, sym_count, sizeof(TB_ObjectSymbol), symbol_cmp);
            if (src_symbol->user_data == NULL) {
                continue;
            }

            TB_LinkerReloc r = {
                .type = reloc->type,
                .target = src_symbol->user_data,
                .src_piece = p,
                .src_offset = reloc->virtual_address,
                .addend = reloc->addend,
            };

            if (reloc->type == TB_OBJECT_RELOC_ADDR64) {
                if (src_symbol->type == TB_OBJECT_SYMBOL_WEAK_EXTERN) {
                    // weak aux
                    uint32_t* weak_sym = src_symbol->extra;

                    TB_ObjectSymbol* restrict alt_sym = bsearch(
                        &(TB_ObjectSymbol){ .ordinal = *weak_sym },
                        syms, sym_count, sizeof(TB_ObjectSymbol),
                        symbol_cmp
                    );

                    r.alt = alt_sym->user_data;
                }
            } else {
                if (src_symbol->type == TB_OBJECT_SYMBOL_WEAK_EXTERN) {
                    tb_todo();
                }
            }

            dyn_array_put(p->inputs, dyn_array_length(info->relocs));
            dyn_array_put(info->relocs, r);
        }
    }

    tb_arena_restore(info->tmp_arena, sp);
}

static void pe_append_library(TB_Linker* l, TB_LinkerThreadInfo* info, TB_Slice ar_name, TB_Slice ar_file) {
    log_debug("linking against %.*s", (int) ar_name.length, ar_name.data);

    TB_ArchiveFileParser ar_parser = { 0 };
    if (!tb_archive_parse(ar_file, &ar_parser)) {
        return;
    }

    TB_Arena* perm_arena = info->perm_arena;
    TB_Arena* arena = info->tmp_arena;
    TB_ArenaSavepoint sp = tb_arena_save(arena);

    TB_ArchiveEntry* entries = tb_arena_alloc(arena, ar_parser.member_count * sizeof(TB_ArchiveEntry));
    size_t new_count;
    CUIK_TIMED_BLOCK("parse_entries") {
        new_count = tb_archive_parse_entries(&ar_parser, 0, ar_parser.member_count, entries);
    }

    FOREACH_N(i, 0, new_count) {
        TB_ArchiveEntry* restrict e = &entries[i];

        if (e->import_name.length) {
            // import from DLL
            TB_Slice libname = e->name;
            ptrdiff_t import_index = -1;
            dyn_array_for(j, l->imports) {
                ImportTable* table = &l->imports[j];

                if (table->libpath.length == libname.length &&
                    memcmp(table->libpath.data, libname.data, libname.length) == 0) {
                    import_index = j;
                    break;
                }
            }

            if (import_index < 0) {
                // we haven't used this DLL yet, make an import table for it
                import_index = dyn_array_length(l->imports);

                ImportTable t = {
                    .libpath = libname,
                    .thunks = dyn_array_create(TB_LinkerSymbol*, 4096)
                };
                dyn_array_put(l->imports, t);
            }

            // make __imp_ form which refers to raw address
            size_t newlen = e->import_name.length + sizeof("__imp_") - 1;
            uint8_t* newstr = tb_arena_alloc(perm_arena, newlen);
            memcpy(newstr, "__imp_", sizeof("__imp_"));
            memcpy(newstr + sizeof("__imp_") - 1, e->import_name.data, e->import_name.length);
            newstr[newlen] = 0;

            TB_LinkerInternStr name = tb_linker_intern_string(l, newlen, (char*) newstr);
            CUIK_TIMED_BLOCK_ARGS("archive", name) {
                TB_LinkerSymbol* import_sym = tb_linker_new_symbol(l, name);
                import_sym->tag = TB_LINKER_SYMBOL_IMPORT;
                import_sym->import.id = import_index;
                import_sym->import.ordinal = e->ordinal;

                // make the thunk-like symbol
                TB_LinkerSymbol* sym = tb_linker_new_symbol(l, tb_linker_intern_string(l, e->import_name.length, (char*) e->import_name.data));
                sym->tag = TB_LINKER_SYMBOL_THUNK;
                sym->thunk = import_sym;

                TB_ArchiveEntry* entries = tb_arena_alloc(arena, ar_parser.member_count * sizeof(TB_ArchiveEntry));
                dyn_array_put(l->imports[import_index].thunks, import_sym);
            }
        } else {
            CUIK_TIMED_BLOCK("append object file") {
                pe_append_object(l, info, e->name, e->content);
            }
        }
    }

    tb_arena_restore(arena, sp);
}

static void pe_append_module(TB_Linker* l, TB_LinkerThreadInfo* info, TB_Module* m) {
    m->visited = false;

    // Also resolves internal call patches which is cool
    ExportList exports;
    CUIK_TIMED_BLOCK("layout section") {
        m->exports = tb_module_layout_sections(m);
    }

    dyn_array_put(info->ir_modules, m);

    // We don't *really* care about this info beyond nicer errors
    TB_LinkerObject* obj_file = tb_arena_alloc(info->perm_arena, sizeof(TB_LinkerObject));
    *obj_file = (TB_LinkerObject){ .module = m };

    // Convert module into sections which we can then append to the output
    DynArray(TB_ModuleSection) sections = m->sections;
    dyn_array_for(i, sections) {
        uint32_t flags = TB_COFF_SECTION_READ;
        if (sections[i].flags & TB_MODULE_SECTION_WRITE) flags |= TB_COFF_SECTION_WRITE;
        if (sections[i].flags & TB_MODULE_SECTION_EXEC)  flags |= TB_COFF_SECTION_EXECUTE | IMAGE_SCN_CNT_CODE;
        if (sections[i].comdat.type != 0) flags |= TB_COFF_SECTION_COMDAT;

        tb_linker_append_module_section(l, info, obj_file, &sections[i], flags);
    }

    const ICodeGen* restrict code_gen = tb__find_code_generator(m);
    if (m->compiled_function_count > 0 && code_gen->emit_win64eh_unwind_info) {
        TB_LinkerInternStr rdata_name = tb_linker_intern_cstring(l, ".rdata");
        TB_LinkerSection* rdata = tb_linker_find_or_create_section(l, rdata_name, IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);

        CUIK_TIMED_BLOCK("generate xdata") {
            TB_Emitter xdata = { 0 };

            dyn_array_for(i, sections) {
                DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
                dyn_array_for(j, funcs) {
                    TB_FunctionOutput* out_f = funcs[j];
                    out_f->unwind_info = xdata.count;
                    code_gen->emit_win64eh_unwind_info(&xdata, out_f, out_f->stack_usage);
                }
            }

            TB_LinkerSectionPiece* x = tb_linker_append_piece(info, rdata, PIECE_NORMAL, xdata.count, xdata.data, obj_file);

            TB_LinkerInternStr pdata_name = tb_linker_intern_cstring(l, ".pdata");
            TB_LinkerSection* pdata = tb_linker_find_or_create_section(l, pdata_name, IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);
            TB_LinkerSectionPiece* pdata_piece = tb_linker_append_piece(info, pdata, PIECE_PDATA, m->compiled_function_count * 12, NULL, obj_file);

            tb_linker_associate(l, pdata_piece, x);

            dyn_array_for(i, sections) {
                DynArray(TB_FunctionOutput*) funcs = sections[i].funcs;
                if (dyn_array_length(funcs) && sections[i].piece) {
                    tb_linker_associate(l, sections[i].piece, pdata_piece);
                }
            }
            m->xdata = x;
        }
    }

    CUIK_TIMED_BLOCK(".reloc") {
        uint32_t last_page = UINT32_MAX, reloc_size = 0;
        dyn_array_for(i, m->sections) {
            DynArray(TB_Global*) globals = m->sections[i].globals;
            dyn_array_for(j, globals) {
                TB_Global* g = globals[j];
                FOREACH_N(k, 0, g->obj_count) {
                    size_t actual_page = g->pos + g->objects[k].offset;

                    if (g->objects[k].type == TB_INIT_OBJ_RELOC) {
                        if (last_page != actual_page) {
                            last_page = actual_page;
                            reloc_size += 8;
                        }

                        reloc_size += 2;
                    }
                }
            }

            if (reloc_size > 0) {
                TB_LinkerInternStr reloc_name = tb_linker_intern_cstring(l, ".reloc");
                TB_LinkerSection* reloc = tb_linker_find_or_create_section(l, reloc_name, IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);
                TB_LinkerSectionPiece* reloc_piece = tb_linker_append_piece(info, reloc, PIECE_RELOC, reloc_size, NULL, obj_file);

                tb_linker_associate(l, sections[i].piece, reloc_piece);
            }
        }
    }

    tb_linker_append_module_symbols(l, m);
}

// just run whatever reloc function from the spec
static int32_t resolve_reloc(TB_LinkerSymbol* sym, TB_ObjectRelocType type, uint32_t source_pos, uint32_t target_rva, int addend) {
    switch (type) {
        case TB_OBJECT_RELOC_ADDR32NB:
        return target_rva;

        case TB_OBJECT_RELOC_SECTION:
        return sym->normal.piece->parent->number;

        case TB_OBJECT_RELOC_SECREL:
        return sym->normal.piece->parent->address;

        case TB_OBJECT_RELOC_REL32:
        return target_rva - (source_pos + addend);

        default:
        tb_todo();
    }
}

static void apply_external_relocs(TB_Linker* l, uint8_t* output, uint64_t image_base) {
    TB_LinkerSection* text  = tb_linker_find_section2(l, ".text");
    uint32_t trampoline_rva = text->address + l->trampoline_pos;
    uint32_t iat_pos = l->iat_pos;

    // TODO(NeGate): we can multithread this code with a job stealing queue
    for (TB_LinkerThreadInfo* restrict info = l->first_thread_info; info; info = info->next) {
        dyn_array_for(i, info->ir_modules) {
            tb_linker_apply_module_relocs(l, info->ir_modules[i], text, output);
        }

        // relative relocations
        dyn_array_for(i, info->relocs) {
            TB_LinkerReloc* restrict rel = &info->relocs[i];
            if (rel->type == TB_OBJECT_RELOC_ADDR64) continue;
            if ((rel->src_piece->flags & TB_LINKER_PIECE_LIVE) == 0) continue;

            // resolve source location
            uint32_t target_rva = 0;
            TB_LinkerSymbol* sym = tb_linker_get_target(rel);
            if (sym == NULL) {
                tb_linker_unresolved_sym(l, rel->target->name);
                continue;
            }

            assert(sym->tag != TB_LINKER_SYMBOL_UNKNOWN);
            if (sym->tag == TB_LINKER_SYMBOL_IMPORT) {
                target_rva = iat_pos + (sym->import.thunk_id * 8);
            } else if (sym->tag == TB_LINKER_SYMBOL_THUNK) {
                TB_LinkerSymbol* import_sym = sym->thunk;
                target_rva = trampoline_rva + (import_sym->import.thunk_id * 6);
            } else {
                target_rva = tb__get_symbol_rva(l, sym);
            }

            // find patch position
            TB_LinkerSectionPiece* restrict p = rel->src_piece;
            TB_LinkerSection* restrict s = p->parent;

            int32_t* dst = (int32_t*) &output[s->offset + p->offset + rel->src_offset];
            uint32_t actual_pos = s->address + p->offset + rel->src_offset;

            *dst += resolve_reloc(sym, rel->type, actual_pos, target_rva, rel->addend);
        }
    }

    // this part will probably stay single threaded for simplicity
    if (l->main_reloc) {
        uint8_t* p_start = (uint8_t*) &output[l->main_reloc->parent->offset + l->main_reloc->offset];
        uint8_t* p_out = p_start;

        uint32_t last_page = 0xFFFFFFFF;
        uint32_t* last_block = NULL;
        TB_LinkerSectionPiece* last_piece = NULL;
        for (TB_LinkerThreadInfo* restrict info = l->first_thread_info; info; info = info->next) {
            dyn_array_for(i, info->relocs) {
                TB_LinkerReloc* restrict r = &info->relocs[i];
                if (r->type != TB_OBJECT_RELOC_ADDR64) continue;

                TB_LinkerSectionPiece* restrict p = r->src_piece;
                if ((p->flags & TB_LINKER_PIECE_LIVE) == 0) continue;

                TB_LinkerSection* restrict s = p->parent;
                if (s->generic_flags & TB_LINKER_SECTION_DISCARD) continue;

                // patch RVA
                size_t actual_pos = p->offset + r->src_offset;
                uint32_t file_pos = s->offset + actual_pos;
                uint64_t* reloc = (uint64_t*) &output[file_pos];

                TB_LinkerSymbol* sym = tb_linker_get_target(r);
                if (sym == NULL) {
                    tb_linker_unresolved_sym(l, r->target->name);
                    continue;
                }

                assert(sym->tag != TB_LINKER_SYMBOL_UNKNOWN);
                if (sym->tag == TB_LINKER_SYMBOL_ABSOLUTE) {
                    *reloc += sym->absolute;
                    continue;
                } else if (sym->tag == TB_LINKER_SYMBOL_IMPORT) {
                    *reloc += iat_pos + (sym->import.thunk_id * 8);
                } else if (sym->tag == TB_LINKER_SYMBOL_THUNK) {
                    TB_LinkerSymbol* import_sym = sym->thunk;
                    *reloc += trampoline_rva + (import_sym->import.thunk_id * 6);
                } else {
                    *reloc += image_base + tb__get_symbol_rva(l, sym);
                }

                // generate relocation
                size_t actual_page = actual_pos & ~4095;
                size_t page_offset = actual_pos - actual_page;

                if (last_piece != p && last_page != actual_page) {
                    last_piece = p;
                    last_page  = actual_page;
                    last_block = (uint32_t*) p_out;

                    last_block[0] = s->address + actual_page;
                    last_block[1] = 8; // block size field (includes RVA field and itself)
                    p_out += 8;
                }

                // emit relocation
                uint16_t payload = (10 << 12) | page_offset; // (IMAGE_REL_BASED_DIR64 << 12) | offset
                *((uint16_t*) p_out) = payload, p_out += 2;
                last_block[1] += 2;
            }
        }

        size_t actual_reloc_size = p_out - p_start;
        (void) actual_reloc_size;
        assert(actual_reloc_size == l->main_reloc->size);
    }
}

// returns the two new section pieces for the IAT and ILT
static COFF_ImportDirectory* gen_imports(TB_Linker* l, PE_ImageDataDirectory* imp_dir, PE_ImageDataDirectory* iat_dir) {
    // cull unused imports
    size_t j = 0;
    size_t import_entry_count = 0;
    dyn_array_for(i, l->imports) {
        DynArray(TB_LinkerSymbol*) tbl = l->imports[j].thunks;

        size_t cnt = 0;
        dyn_array_for(k, tbl) if (tbl[k]->flags & TB_LINKER_SYMBOL_USED) {
            tbl[cnt++] = tbl[k];
        }
        dyn_array_set_length(tbl, cnt);
        l->imports[i].thunks = tbl;

        if (dyn_array_length(l->imports[i].thunks) != 0) {
            l->imports[j++] = l->imports[i];

            // there's an extra NULL terminator for the import entry lists
            import_entry_count += dyn_array_length(l->imports[i].thunks) + 1;
        }
    }

    if (l->imports) {
        dyn_array_set_length(l->imports, j); // trimmed
    }

    if (import_entry_count == 0) {
        *imp_dir = (PE_ImageDataDirectory){ 0 };
        *iat_dir = (PE_ImageDataDirectory){ 0 };
        return NULL;
    }

    // Generate import thunks
    uint32_t thunk_id_counter = 0;
    l->trampolines = (TB_Emitter){ 0 };
    dyn_array_for(i, l->imports) {
        ImportTable* imp = &l->imports[i];

        dyn_array_for(j, imp->thunks) {
            imp->thunks[j]->import.ds_address = l->trampolines.count;
            imp->thunks[j]->import.thunk_id = thunk_id_counter++;

            // TODO(NeGate): This trampoline is x64 specific, we should
            // implement a system to separate this from the core PE export
            // code.
            tb_out1b(&l->trampolines, 0xFF);
            tb_out1b(&l->trampolines, 0x25);
            // we're gonna relocate this to map onto an import thunk later
            tb_out4b(&l->trampolines, 0);
        }
    }

    ////////////////////////////////
    // Generate import table
    ////////////////////////////////
    size_t import_dir_size = (1 + dyn_array_length(l->imports)) * sizeof(COFF_ImportDirectory);
    size_t iat_size = import_entry_count * sizeof(uint64_t);
    size_t total_size = import_dir_size + 2*iat_size;
    dyn_array_for(i, l->imports) {
        ImportTable* imp = &l->imports[i];
        total_size += imp->libpath.length + 1;

        dyn_array_for(j, imp->thunks) {
            TB_LinkerSymbol* t = imp->thunks[j];
            total_size += tb_linker_intern_len(l, t->name) + 3;
        }
    }

    TB_LinkerThreadInfo* info = linker_thread_info(l);
    uint8_t* output = tb_arena_alloc(info->perm_arena, total_size);

    COFF_ImportDirectory* import_dirs = (COFF_ImportDirectory*) &output[0];
    uint64_t* iat = (uint64_t*) &output[import_dir_size];
    uint64_t* ilt = (uint64_t*) &output[import_dir_size + iat_size];
    size_t strtbl_pos = import_dir_size + iat_size*2;

    // We put both the IAT and ILT into the rdata, the PE loader doesn't care but it
    // means the user can't edit these... at least not easily
    TB_LinkerInternStr rdata_name = tb_linker_intern_cstring(l, ".rdata");
    TB_LinkerSection* rdata = tb_linker_find_or_create_section(l, rdata_name, IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);
    TB_LinkerSectionPiece* import_piece = tb_linker_append_piece(info, rdata, PIECE_NORMAL, total_size, output, 0);

    *imp_dir = (PE_ImageDataDirectory){ import_piece->offset, import_dir_size };
    *iat_dir = (PE_ImageDataDirectory){ import_piece->offset + import_dir_size, iat_size };

    size_t p = 0;
    dyn_array_for(i, l->imports) {
        ImportTable* imp = &l->imports[i];
        COFF_ImportDirectory* header = &import_dirs[i];
        TB_Slice lib = imp->libpath;

        // after we resolve RVAs we need to backpatch stuff
        imp->iat = &iat[p], imp->ilt = &ilt[p];

        *header = (COFF_ImportDirectory){
            .import_lookup_table  = import_piece->offset + import_dir_size + iat_size + p*sizeof(uint64_t),
            .import_address_table = import_piece->offset + import_dir_size + p*sizeof(uint64_t),
            .name = import_piece->offset + strtbl_pos,
        };

        memcpy(&output[strtbl_pos], lib.data, lib.length), strtbl_pos += lib.length;
        output[strtbl_pos++] = 0;

        dyn_array_for(j, imp->thunks) {
            TB_LinkerSymbol* t = imp->thunks[j];

            const char* name = t->name + IMP_PREFIX_LEN;
            size_t len = tb_linker_intern_len(l, t->name) - IMP_PREFIX_LEN;

            // import-by-name
            uint64_t value = import_piece->offset + strtbl_pos;
            memcpy(&output[strtbl_pos], &t->import.ordinal, sizeof(uint16_t)), strtbl_pos += 2;
            memcpy(&output[strtbl_pos], name, len), strtbl_pos += len;
            output[strtbl_pos++] = 0;

            // both the ILT and IAT are practically identical at this point
            iat[p] = ilt[p] = value, p++;
        }

        // NULL terminated
        iat[p] = ilt[p] = 0, p++;
    }
    assert(p == import_entry_count);

    // add NULL import directory at the end
    import_dirs[dyn_array_length(l->imports)] = (COFF_ImportDirectory){ 0 };

    // trampolines require .text section
    if (l->trampolines.count) {
        TB_LinkerSection* text = tb_linker_find_section(l, tb_linker_intern_cstring(l, ".text"));
        assert(text != NULL);

        TB_LinkerSectionPiece* piece = tb_linker_append_piece(info, text, PIECE_NORMAL, l->trampolines.count, l->trampolines.data, 0);
        l->trampoline_pos = piece->offset;
    }

    return import_dirs;
}

static void* gen_reloc_section(TB_Linker* l) {
    // generates .reloc for all object files
    uint32_t reloc_sec_size = 0;
    uint32_t last_page = UINT32_MAX;
    TB_LinkerSectionPiece* last_piece = NULL;

    for (TB_LinkerThreadInfo* restrict info = l->first_thread_info; info; info = info->next) {
        dyn_array_for(i, info->relocs) {
            TB_LinkerReloc* restrict r = &info->relocs[i];
            if (r->type == TB_OBJECT_RELOC_ADDR64) continue;
            if ((r->src_piece->flags & TB_LINKER_PIECE_LIVE) == 0) continue;

            TB_LinkerSection* s = r->src_piece->parent;
            if (s->generic_flags & TB_LINKER_SECTION_DISCARD) continue;
            if (r->target == NULL || r->target->tag == TB_LINKER_SYMBOL_ABSOLUTE) continue;

            size_t actual_pos = r->src_piece->offset + r->src_offset;
            size_t actual_page = actual_pos & ~4095;

            if (last_piece != r->src_piece && last_page != actual_page) {
                last_piece = r->src_piece;
                last_page = actual_page;

                reloc_sec_size += 8;
            }

            reloc_sec_size += 2;
        }
    }

    if (reloc_sec_size) {
        TB_LinkerThreadInfo* info = linker_thread_info(l);
        void* reloc = tb_arena_alloc(info->perm_arena, reloc_sec_size);

        TB_LinkerSection* reloc_sec = tb_linker_find_or_create_section(l, ".reloc", IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA);
        l->main_reloc = tb_linker_append_piece(info, reloc_sec, PIECE_NORMAL, reloc_sec_size, reloc, 0);

        return reloc;
    } else {
        return NULL;
    }
}

#define CSTRING(str) { sizeof(str)-1, (const uint8_t*) str }
static void pe_init(TB_Linker* l) {
    l->entrypoint = "mainCRTStartup";
    l->subsystem = TB_WIN_SUBSYSTEM_CONSOLE;

    #define symbol(name_) tb_linker_new_symbol(l, tb_linker_intern_cstring(l, name_))
    symbol("__ImageBase")->tag = TB_LINKER_SYMBOL_IMAGEBASE;

    // This is practically just ripped from LLD
    //   https://github.com/llvm/llvm-project/blob/3d0a5bf7dea509f130c51868361a38daeee7816a/lld/COFF/Driver.cpp#L2192
    symbol("__AbsoluteZero");
    // add_abs("__volatile_metadata");
    // add_abs("__guard_memcpy_fptr");
    symbol("__guard_fids_count");
    symbol("__guard_fids_table");
    symbol("__guard_flags");
    symbol("__guard_iat_count");
    symbol("__guard_iat_table");
    symbol("__guard_longjmp_count");
    symbol("__guard_longjmp_table");
    // Needed for MSVC 2017 15.5 CRT.
    symbol("__enclave_config");
    // Needed for MSVC 2019 16.8 CRT.
    symbol("__guard_eh_cont_count");
    symbol("__guard_eh_cont_table");
    #undef symbol
}

#define WRITE(data, size) (memcpy(&output[write_pos], data, size), write_pos += (size))
static TB_ExportBuffer pe_export(TB_Linker* l) {
    PE_ImageDataDirectory imp_dir, iat_dir;
    COFF_ImportDirectory* import_dirs;

    for (TB_LinkerThreadInfo* restrict info = l->first_thread_info; info; info = info->next) {
        dyn_array_for(i, info->alternates) {
            TB_Slice to = info->alternates[i].to;
            TB_Slice from = info->alternates[i].from;

            TB_LinkerInternStr to_str = tb_linker_intern_string(l, to.length, (const char*) to.data);
            TB_LinkerInternStr from_str = tb_linker_intern_string(l, from.length, (const char*) from.data);

            TB_LinkerSymbol* old = tb_linker_find_symbol(l, to_str);
            if (old) {
                TB_LinkerSymbol* sym = tb_linker_find_symbol(l, from_str);
                assert(sym->tag == TB_LINKER_SYMBOL_UNKNOWN);

                *sym = *old;
                sym->name = from_str;
            }
        }

        /* dyn_array_for(i, info->merges) {
            NL_Slice to_name = { info->merges[i].to.length, info->merges[i].to.data };
            ptrdiff_t to = nl_map_get(l->sections, to_name);
            if (to < 0) continue;

            NL_Slice from_name = { info->merges[i].from.length, info->merges[i].from.data };
            ptrdiff_t from = nl_map_get(l->sections, from_name);
            if (from < 0) continue;

            tb__merge_sections(l, l->sections[from].v, l->sections[to].v);
        } */
    }

    // personally like merging these
    TB_LinkerSection* rdata = tb_linker_find_section2(l, ".rdata");
    tb_linker_merge_sections(l, tb_linker_find_section2(l, ".00cfg"), rdata);
    tb_linker_merge_sections(l, tb_linker_find_section2(l, ".idata"), rdata);
    tb_linker_merge_sections(l, tb_linker_find_section2(l, ".xdata"), rdata);

    // this will resolve the sections, GC any pieces which aren't used and
    // resolve symbols.
    CUIK_TIMED_BLOCK("Resolve & GC") {
        tb_linker_push_named(l, l->entrypoint);
        tb_linker_push_named(l, "_tls_used");
        tb_linker_push_named(l, "_load_config_used");
        tb_linker_mark_live(l);
    }

    if (!tb__finalize_sections(l)) {
        return (TB_ExportBuffer){ 0 };
    }

    CUIK_TIMED_BLOCK("generate imports") {
        import_dirs = gen_imports(l, &imp_dir, &iat_dir);
    }

    CUIK_TIMED_BLOCK("generate .reloc") {
        gen_reloc_section(l);
    }

    size_t final_section_count = 0;
    nl_table_for(e, &l->sections) {
        TB_LinkerSection* sec = e->v;
        final_section_count += (sec->generic_flags & TB_LINKER_SECTION_DISCARD) == 0;
    }

    size_t size_of_headers = sizeof(dos_stub)
        + sizeof(uint32_t) // PE magic number
        + sizeof(COFF_FileHeader)
        + sizeof(PE_OptionalHeader64)
        + (final_section_count * sizeof(PE_SectionHeader));

    size_of_headers = align_up(size_of_headers, 512);

    size_t pe_code_size   = 0; // bytes in total marked as IMAGE_SCN_CNT_CODE
    size_t pe_init_size   = 0; // bytes in total marked as IMAGE_SCN_CNT_INITIALIZED_DATA
    size_t pe_uninit_size = 0; // bytes in total marked as IMAGE_SCN_CNT_UNINITIALIZED_DATA

    size_t section_content_size = 0;
    uint64_t virt_addr = align_up(size_of_headers, 4096); // this area is reserved for the PE header stuff
    CUIK_TIMED_BLOCK("layout sections") {
        nl_table_for(e, &l->sections) {
            TB_LinkerSection* s = e->v;
            if (s->generic_flags & TB_LINKER_SECTION_DISCARD) continue;

            if (s->flags & IMAGE_SCN_CNT_CODE) pe_code_size += s->total_size;
            if (s->flags & IMAGE_SCN_CNT_INITIALIZED_DATA) pe_init_size += s->total_size;
            if (s->flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) pe_uninit_size += s->total_size;

            s->offset = size_of_headers + section_content_size;
            if ((s->flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0) {
                section_content_size += align_up(s->total_size, 512);
            }

            s->address = virt_addr;
            virt_addr += align_up(s->total_size, 4096);
        }
    }

    TB_LinkerSection* text = tb_linker_find_section2(l, ".text");
    TB_LinkerSection* data = tb_linker_find_section2(l, ".data");

    if (import_dirs != NULL) {
        iat_dir.virtual_address += rdata->address;
        imp_dir.virtual_address += rdata->address;
        CUIK_TIMED_BLOCK("relocate imports and trampolines") {
            dyn_array_for(i, l->imports) {
                ImportTable* imp = &l->imports[i];
                COFF_ImportDirectory* header = &import_dirs[i];
                header->import_lookup_table  += rdata->address;
                header->import_address_table += rdata->address;
                header->name += rdata->address;

                uint64_t *ilt = imp->ilt, *iat = imp->iat;
                uint64_t iat_rva = header->import_address_table;
                uint64_t trampoline_rva = text->address + l->trampoline_pos;

                dyn_array_for(j, imp->thunks) {
                    if (iat[j] != 0) {
                        iat[j] += rdata->address;
                        ilt[j] += rdata->address;
                    }

                    // Relocate trampoline entries to point into the IAT, the PE loader will
                    // fill these slots in with absolute addresses of the symbols.
                    int32_t* trampoline_dst = (int32_t*) &l->trampolines.data[imp->thunks[j]->import.ds_address + 2];
                    assert(*trampoline_dst == 0 && "We set this earlier... why isn't it here?");

                    uint32_t target_rva = iat_rva + j*8;
                    uint32_t source_pos = trampoline_rva + imp->thunks[j]->import.ds_address + 6;
                    (*trampoline_dst) += target_rva - source_pos;
                }
            }
        }
        l->iat_pos = iat_dir.virtual_address;
    }

    size_t output_size = size_of_headers + section_content_size;
    COFF_FileHeader header = {
        .machine = 0x8664,
        .section_count = final_section_count,
        .timestamp = 1056582000u, // my birthday since that's consistent :P
        .symbol_table = 0,
        .symbol_count = 0,
        .optional_header_size = sizeof(PE_OptionalHeader64),
        .flags = 0x2 | 0x0020 /* executable, >2GB */
    };

    if (l->subsystem == TB_WIN_SUBSYSTEM_EFI_APP) {
        header.flags |= 0x2000; // DLL
    }

    static const uint32_t subsys[] = {
        [TB_WIN_SUBSYSTEM_WINDOWS] = IMAGE_SUBSYSTEM_WINDOWS_GUI,
        [TB_WIN_SUBSYSTEM_CONSOLE] = IMAGE_SUBSYSTEM_WINDOWS_CUI,
        [TB_WIN_SUBSYSTEM_EFI_APP] = IMAGE_SUBSYSTEM_EFI_APPLICATION,
    };

    PE_OptionalHeader64 opt_header = {
        .magic = 0x20b,
        .section_alignment = 0x1000,
        .file_alignment = 0x200,

        .image_base = 0x140000000,

        .size_of_code = pe_code_size,
        .size_of_initialized_data = pe_init_size,
        .size_of_uninitialized_data = pe_uninit_size,

        .size_of_image = virt_addr,
        .size_of_headers = (size_of_headers + 0x1FF) & ~0x1FF,
        .subsystem = subsys[l->subsystem],
        .dll_characteristics = 0x40 | 0x20 | 0x0100 | 0x8000, /* dynamic base, high entropy, nx compat, terminal server aware */

        .size_of_stack_reserve = 2 << 20,
        .size_of_stack_commit = 4096,

        .rva_size_count = IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
        .data_directories = {
            [IMAGE_DIRECTORY_ENTRY_IMPORT] = imp_dir,
            [IMAGE_DIRECTORY_ENTRY_IAT] = iat_dir,
        }
    };

    if (l->subsystem != TB_WIN_SUBSYSTEM_EFI_APP) {
        opt_header.major_os_ver = 6;
        opt_header.minor_os_ver = 0;
        opt_header.major_subsystem_ver = 6;
        opt_header.minor_subsystem_ver = 0;
    }

    TB_LinkerSymbol* tls_used_sym = tb_linker_find_symbol2(l, "_tls_used");
    if (tls_used_sym) {
        opt_header.data_directories[IMAGE_DIRECTORY_ENTRY_TLS] = (PE_ImageDataDirectory){ tb__get_symbol_rva(l, tls_used_sym), sizeof(PE_TLSDirectory) };
    }

    TB_LinkerSymbol* load_config_used_sym = tb_linker_find_symbol2(l, "_load_config_used");
    if (load_config_used_sym) {
        opt_header.data_directories[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] = (PE_ImageDataDirectory){ tb__get_symbol_rva(l, load_config_used_sym), 0x140 };
    }

    TB_LinkerSection* pdata = tb_linker_find_section2(l, ".pdata");
    if (pdata) {
        opt_header.data_directories[IMAGE_DIRECTORY_ENTRY_EXCEPTION] = (PE_ImageDataDirectory){ pdata->address, pdata->total_size };
    }

    TB_LinkerSection* reloc = tb_linker_find_section2(l, ".reloc");
    if (reloc) {
        opt_header.data_directories[IMAGE_DIRECTORY_ENTRY_BASERELOC] = (PE_ImageDataDirectory){ reloc->address, reloc->total_size };
    }

    // text section crap
    if (text) {
        opt_header.base_of_code = text->address;
        opt_header.size_of_code = align_up(text->total_size, 4096);

        TB_LinkerSymbol* sym = tb_linker_find_symbol2(l, l->entrypoint);
        if (sym) {
            if (sym->tag == TB_LINKER_SYMBOL_NORMAL) {
                opt_header.entrypoint = text->address + sym->normal.piece->offset + sym->normal.secrel;
            } else if (sym->tag == TB_LINKER_SYMBOL_TB) {
                opt_header.entrypoint = text->address + sym->tb.piece->offset + tb__get_symbol_pos(sym->tb.sym);
            } else {
                tb_todo();
            }
        } else {
            printf("tblink: could not find entrypoint! %s\n", l->entrypoint);
        }
    }

    size_t write_pos = 0;
    TB_ExportChunk* chunk = tb_export_make_chunk(output_size);
    uint8_t* restrict output = chunk->data;

    uint32_t pe_magic = 0x00004550;
    WRITE(dos_stub,    sizeof(dos_stub));
    WRITE(&pe_magic,   sizeof(pe_magic));
    WRITE(&header,     sizeof(header));
    WRITE(&opt_header, sizeof(opt_header));

    nl_table_for(e, &l->sections) {
        TB_LinkerSection* s = e->v;
        if (s->generic_flags & TB_LINKER_SECTION_DISCARD) continue;

        PE_SectionHeader sec_header = {
            .virtual_size = align_up(s->total_size, 4096),
            .virtual_address = s->address,
            .characteristics = s->flags,
        };

        if ((s->flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0) {
            sec_header.pointer_to_raw_data = s->offset;
            sec_header.size_of_raw_data = s->total_size;
        }

        size_t len = tb_linker_intern_len(l, s->name);
        memcpy(sec_header.name, s->name, len > 8 ? 8 : len);
        WRITE(&sec_header, sizeof(sec_header));
    }
    write_pos = tb__pad_file(output, write_pos, 0x00, 0x200);

    tb__apply_section_contents(l, output, write_pos, text, data, rdata, 512, opt_header.image_base);

    TB_ExportBuffer chk = { .total = output_size, .head = chunk, .tail = chunk };

    // TODO(NeGate): multithread this too
    CUIK_TIMED_BLOCK("apply final relocations") {
        apply_external_relocs(l, output, opt_header.image_base);
    }

    return chk;
}

TB_LinkerVtbl tb__linker_pe = {
    .init           = pe_init,
    .append_object  = pe_append_object,
    .append_library = pe_append_library,
    .append_module  = pe_append_module,
    .export         = pe_export
};
