#include "coff.h"

typedef struct {
    char name[16];
    char date[12];

    // Microsoft tools don't actually do anything with this
    char user_id[6];
    char group_id[6];

    char mode[8];
    char size[10];

    uint8_t newline[2];
    uint8_t contents[];
} COFF_ArchiveMemberHeader;

// section related crap likes to be sorted in lexical order :p
static int compare_sections(const void* a, const void* b) {
    const TB_ObjectSection* sec_a = (const TB_ObjectSection*)a;
    const TB_ObjectSection* sec_b = (const TB_ObjectSection*)b;

    size_t shortest_len = sec_a->name.length < sec_b->name.length ? sec_a->name.length : sec_b->name.length;
    return memcmp(sec_a->name.data, sec_b->name.data, shortest_len);
}

static uint16_t read16be(uint8_t* ptr) {
    return (ptr[0] << 8u) | (ptr[1]);
}

static uint32_t read32be(uint8_t* ptr) {
    return (ptr[0] << 24u) | (ptr[1] << 16u) | (ptr[2] << 8u) | (ptr[3]);
}

bool tb_archive_parse(TB_Slice file, TB_ArchiveFileParser* restrict out_parser) {
    *out_parser = (TB_ArchiveFileParser){ file };

    if (memcmp(&file.data[0], "!<arch>\n", 8) != 0) {
        // TODO(NeGate): maybe we should make a custom error stream...
        fprintf(stderr, "TB archive parser: invalid .lib header!\n");
        return false;
    }

    size_t file_offset = 8;
    COFF_ArchiveMemberHeader* first = (COFF_ArchiveMemberHeader*) &file.data[file_offset];
    if (memcmp(first->name, (char[16]) { "/               " }, 16) != 0) {
        fprintf(stderr, "TB archive parser: first archive member name is invalid\n");
        return false;
    }
    size_t first_content_length = tb__parse_decimal_int(sizeof(first->size), first->size);

    file_offset += sizeof(COFF_ArchiveMemberHeader) + first_content_length;
    file_offset = (file_offset + 1u) & ~1u;

    // Process second member
    COFF_ArchiveMemberHeader* second = (COFF_ArchiveMemberHeader*) &file.data[file_offset];
    if (memcmp(second->name, (char[16]) { "/               " }, 16) != 0) {
        fprintf(stderr, "TB archive parser: second archive member name is invalid\n");
        return false;
    }
    size_t second_content_length = tb__parse_decimal_int(sizeof(second->size), second->size);

    // Extract number of symbols
    if (second_content_length >= 8) {
        memcpy(&out_parser->member_count, &second->contents[0], sizeof(uint32_t));
        out_parser->members = (uint32_t*) &second->contents[4];

        memcpy(&out_parser->symbol_count, &second->contents[4 + out_parser->member_count*sizeof(uint32_t)], sizeof(uint32_t));
        out_parser->symbols = (uint16_t*) &second->contents[8 + out_parser->member_count*sizeof(uint32_t)];
    }

    // Advance
    file_offset += sizeof(COFF_ArchiveMemberHeader) + second_content_length;
    file_offset = (file_offset + 1u) & ~1u;

    // Process long name member
    COFF_ArchiveMemberHeader* longnames = (COFF_ArchiveMemberHeader*) &file.data[file_offset];
    if (memcmp(longnames->name, (char[16]) { "//              " }, 16) == 0) {
        size_t longname_content_length = tb__parse_decimal_int(sizeof(second->size), second->size);
        out_parser->strtbl = (TB_Slice){ longnames->contents, longname_content_length };

        // Advance
        file_offset += sizeof(COFF_ArchiveMemberHeader) + longname_content_length;
        file_offset = (file_offset + 1u) & ~1u;
    }

    out_parser->pos = file_offset;
    return true;
}

size_t tb_archive_parse_entries(TB_ArchiveFileParser* restrict parser, size_t start, size_t count, TB_ArchiveEntry* out_entry) {
    TB_Slice file = parser->file;
    TB_Slice strtbl = parser->strtbl;
    size_t entry_count = 0;

    FOREACH_N(i, start, count) {
        COFF_ArchiveMemberHeader* restrict sym = (COFF_ArchiveMemberHeader*) &file.data[parser->members[i]];
        size_t len = tb__parse_decimal_int(sizeof(sym->size), sym->size);

        TB_Slice sym_name = { (uint8_t*) sym->name, strchr(sym->name, ' ') - sym->name };
        if (sym_name.data[0] == '/') {
            // name is actually just an index into the long names table
            size_t num = tb__parse_decimal_int(sym_name.length - 1, (char*)sym_name.data + 1);
            sym_name = (TB_Slice){ &strtbl.data[num], strlen((const char*) &strtbl.data[num]) };
        }

        uint32_t short_form_header = *(uint32_t*)sym->contents;
        if (short_form_header == 0xFFFF0000) {
            COFF_ImportHeader* import = (COFF_ImportHeader*) sym->contents;

            const char* imported_symbol = (const char*) &sym->contents[sizeof(COFF_ImportHeader)];
            const char* dll_path = (const char*) &sym->contents[sizeof(COFF_ImportHeader) + strlen(imported_symbol) + 1];

            TB_Slice import_name = { 0 };
            if (import->name_type == 3) {
                // for odd reasons windows has some symbols with @ and underscores (C++ amirite)
                // and we have to strip them out.
                const char* leading = imported_symbol;
                const char* at = strchr(imported_symbol, '@');
                if (at == NULL) at = imported_symbol + strlen(imported_symbol);

                for (const char* s = imported_symbol; s != at; s++) {
                    if (*s == '_') leading = s+1;
                }

                import_name.length = at - leading;
                import_name.data   = (const uint8_t*) leading;
            } else {
                import_name.length = strlen(imported_symbol);
                import_name.data   = (const uint8_t*) imported_symbol;
            }

            // printf("%s : %s : %d\n", dll_path, imported_symbol, import->name_type);
            out_entry[entry_count++] = (TB_ArchiveEntry){ { (const uint8_t*) dll_path, strlen(dll_path) }, .import_name = import_name, .ordinal = import->ordinal_hint };
        } else {
            out_entry[entry_count++] = (TB_ArchiveEntry){ sym_name, .content = { sym->contents, len } };
        }

        skip:;
    }

    return entry_count;
}
