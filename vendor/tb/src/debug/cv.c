#include "../tb_internal.h"
#include "cv.h"

#include "cv_type_builder.c"

#include <sys/stat.h>

#if defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define fileno _fileno
#define fstat  _fstat
#define stat   _stat
#define strdup _strdup
#endif

// constant sized "hash map" which is used to
// deduplicate types in the codeview
#define MAX_TYPE_ENTRY_LOOKUP_SIZE 1024

static void md5sum_file(uint8_t out_bytes[16], const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("Could not read file: %s\n", filepath);
        abort();
    }

    int descriptor = fileno(file);

    struct stat file_stats;
    if (fstat(descriptor, &file_stats) == -1) {
        fclose(file);
        abort();
    }

    size_t len  = file_stats.st_size;
    unsigned char* data = tb_platform_heap_alloc(len + 1);

    fseek(file, 0, SEEK_SET);
    fread(data, 1, len, file);

    tb__md5sum(out_bytes, data, len);

    fclose(file);
    tb_platform_heap_free(data);
}

static uint16_t get_codeview_type(TB_DataType dt) {
    switch (dt.type) {
        case TB_INT: {
            if (dt.data <= 0)  return 0x0003; // T_VOID
            if (dt.data <= 1)  return 0x0030; // T_BOOL08
            if (dt.data <= 8)  return 0x0020; // T_UCHAR
            if (dt.data <= 16) return 0x0073; // T_UINT2
            if (dt.data <= 32) return 0x0075; // T_UINT4
            if (dt.data <= 64) return 0x0023; // T_UQUAD
            return 0x0023; // T_64PUCHAR
        }
        case TB_FLOAT: {
            if (dt.data == TB_FLT_32) return 0x0040; // T_REAL32
            if (dt.data == TB_FLT_64) return 0x0041; // T_REAL64

            assert(0 && "Unknown float type");
        }
        case TB_PTR: {
            return 0x0023; // T_64PUCHAR
        }
        default: assert(0 && "TODO: missing type in CodeView output");
    }

    return 0x0003; // T_VOID
}

static uint16_t convert_to_codeview_type(CV_Builder* builder, TB_DebugType* type) {
    if (type->type_id != 0) {
        return type->type_id;
    }

    switch (type->tag) {
        case TB_DEBUG_TYPE_VOID: return (type->type_id = T_VOID);
        case TB_DEBUG_TYPE_BOOL: return (type->type_id = T_BOOL08); // T_BOOL08

        case TB_DEBUG_TYPE_INT:
        case TB_DEBUG_TYPE_UINT: {
            bool is_signed = (type->tag == TB_DEBUG_TYPE_INT);

            if (type->int_bits <= 8)  return is_signed ? T_CHAR : T_UCHAR;
            if (type->int_bits <= 16) return is_signed ? T_INT2 : T_UINT2;
            if (type->int_bits <= 32) return is_signed ? T_INT4 : T_UINT4;
            if (type->int_bits <= 64) return is_signed ? T_INT8 : T_UINT8;
            assert(0 && "Unsupported int type");
        }

        case TB_DEBUG_TYPE_FLOAT: {
            switch (type->float_fmt) {
                case TB_FLT_32: return (type->type_id = T_REAL32);
                case TB_FLT_64: return (type->type_id = T_REAL64);
                default: assert(0 && "Unknown float type");
            }
        }

        case TB_DEBUG_TYPE_ARRAY:
        return (type->type_id = tb_codeview_builder_add_array(builder, convert_to_codeview_type(builder, type->array.base), debug_type_size(TB_ABI_WIN64, type->array.base) * type->array.count));

        case TB_DEBUG_TYPE_POINTER:
        return (type->type_id = tb_codeview_builder_add_pointer(builder, convert_to_codeview_type(builder, type->ptr_to)));

        case TB_DEBUG_TYPE_FUNCTION:
        return (type->type_id = tb_codeview_builder_add_pointer(builder, 0x0003));

        case TB_DEBUG_TYPE_ALIAS:
        return (type->type_id = tb_codeview_builder_add_alias(builder, convert_to_codeview_type(builder, type->alias.type), type->alias.name));

        case TB_DEBUG_TYPE_STRUCT:
        case TB_DEBUG_TYPE_UNION: {
            if (type->type_id_fwd) {
                return type->type_id_fwd;
            }

            // generate forward declaration
            // TODO(NeGate): we might wanna avoid generating a forward declaration here if we never use it
            CV_RecordType rec_type = type->tag == TB_DEBUG_TYPE_STRUCT ? LF_STRUCTURE : LF_UNION;
            type->type_id_fwd = tb_codeview_builder_add_incomplete_record(builder, rec_type, type->record.tag);

            if (type->record.count == 0) {
                // it's incomplete so it doesn't matter
                return type->type_id_fwd;
            }

            TB_ArenaSavepoint sp = tb_arena_save(tmp_arena);
            CV_Field* list = tb_arena_alloc(tmp_arena, type->record.count * sizeof(CV_Field));
            FOREACH_N(i, 0, type->record.count) {
                const TB_DebugType* f = type->record.members[i];
                assert(f->tag == TB_DEBUG_TYPE_FIELD);

                list[i].type = convert_to_codeview_type(builder, f->field.type);
                list[i].name = f->field.name;
                list[i].offset = f->field.offset;
            }

            CV_TypeIndex field_list = tb_codeview_builder_add_field_list(builder, type->record.count, list);
            tb_arena_restore(tmp_arena, sp);

            return (type->type_id = tb_codeview_builder_add_record(builder, rec_type, type->record.count, field_list, type->record.size, type->record.tag));
        }

        default:
        assert(0 && "TODO: missing type in CodeView output");
        return 0x0003;
    }
}

static TB_Slice gimme_cstr_as_slice(TB_Arena* arena, const char* str) {
    TB_Slice s = { .length = strlen(str) };
    s.data = memcpy(tb_arena_alloc(arena, s.length), str, s.length);
    return s;
}

static void add_reloc(TB_ObjectSection* section, const TB_ObjectReloc* reloc, size_t cap) {
    assert(section->relocation_count < cap);
    section->relocations[section->relocation_count++] = *reloc;
}

static bool codeview_supported_target(TB_Module* m) {
    return true;
}

static int codeview_number_of_debug_sections(TB_Module* m) {
    return 2;
}

// Based on this, it's the only nice CodeView source out there:
// https://github.com/netwide-assembler/nasm/blob/master/output/codeview.c
static TB_SectionGroup codeview_generate_debug_info(TB_Module* m, TB_Arena* arena) {
    TB_ObjectSection* sections = tb_arena_alloc(arena, 2 * sizeof(TB_ObjectSection));
    sections[0] = (TB_ObjectSection){ gimme_cstr_as_slice(arena, ".debug$S") };
    sections[1] = (TB_ObjectSection){ gimme_cstr_as_slice(arena, ".debug$T") };

    size_t global_count = m->symbol_count[TB_SYMBOL_GLOBAL];

    // debug$S does quite a few relocations :P, namely saying that
    // certain things point to specific areas of .text section
    size_t reloc_cap = (2 * global_count) + (4 * m->compiled_function_count);
    sections[0].relocations = tb_arena_alloc(arena, reloc_cap * sizeof(TB_ObjectReloc));

    TB_ArenaSavepoint sp = tb_arena_save(arena);

    // Write type table
    size_t file_count = nl_map__get_header(m->files)->count;
    uint32_t* file_table_offset = tb_arena_alloc(arena, file_count * sizeof(uint32_t));

    TB_Emitter debugs_out = { 0 };

    CV_TypeEntry* lookup_table = tb_arena_alloc(arena, 1024 * sizeof(CV_TypeEntry));
    memset(lookup_table, 0, 1024 * sizeof(CV_TypeEntry));
    CV_Builder builder = tb_codeview_builder_create(1024, lookup_table);

    // Write symbol info table
    {
        static const char creator_str[] = "Bitch";
        uint32_t creator_length = 2 + 4 + 2 + (3 * 2) + (3 * 2) + sizeof(creator_str) + 2;

        static const char dummy_path[] = "fallback.o";
        tb_out4b(&debugs_out, 0x00000004);

        // File nametable
        CUIK_TIMED_BLOCK("write file nametable") {
            tb_out4b(&debugs_out, 0x000000F3);

            size_t field_length_patch = debugs_out.count;
            tb_out4b(&debugs_out, 0);
            tb_out1b(&debugs_out, 0);

            // skip the NULL file entry
            size_t pos = 1, counter = 0;
            file_table_offset[0] = 0;
            nl_map_for_str(i, m->files) {
                TB_SourceFile* f = m->files[i].v;
                f->id = counter++;

                tb_out_reserve(&debugs_out, f->len + 1);
                tb_outs_UNSAFE(&debugs_out, f->len + 1, (const uint8_t*) f->path);

                file_table_offset[f->id] = pos;
                pos += f->len + 1;
            }

            tb_patch4b(&debugs_out, field_length_patch, (debugs_out.count - field_length_patch) - 4);
            align_up_emitter(&debugs_out, 4);
        }

        // Source file table
        // we practically transmute the file_table_offset from meaning file string
        // table entries into source file entries.
        CUIK_TIMED_BLOCK("write file hashes") {
            tb_out4b(&debugs_out, 0x000000F4);

            size_t field_length_patch = debugs_out.count;
            tb_out4b(&debugs_out, 0);

            size_t pos = 0;
            nl_map_for_str(i, m->files) {
                TB_SourceFile* f = m->files[i].v;

                uint8_t sum[16];
                md5sum_file(sum, (const char*) f->path);

                tb_out4b(&debugs_out, file_table_offset[f->id]);
                tb_out2b(&debugs_out, 0x0110);
                tb_outs(&debugs_out, MD5_HASHBYTES, sum);
                tb_out2b(&debugs_out, 0);

                file_table_offset[f->id] = pos;
                pos += 4 + 2 + MD5_HASHBYTES + 2;
            }

            tb_patch4b(&debugs_out, field_length_patch, (debugs_out.count - field_length_patch) - 4);
            align_up_emitter(&debugs_out, 4);
        }

        // Line info table
        CUIK_TIMED_BLOCK("write line info") {
            dyn_array_for(i, m->sections) {
                DynArray(TB_FunctionOutput*) funcs = m->sections[i].funcs;
                dyn_array_for(j, funcs) {
                    TB_FunctionOutput* out_f = funcs[j];

                    // Layout crap
                    DynArray(TB_Location) lines = out_f->locations;

                    tb_out4b(&debugs_out, 0x000000F2);
                    size_t field_length_patch = debugs_out.count;
                    tb_out4b(&debugs_out, 0);

                    // Source mapping header
                    size_t func_id = out_f->parent->super.symbol_id;
                    {
                        size_t patch_pos = debugs_out.count;
                        add_reloc(&sections[0], &(TB_ObjectReloc){ TB_OBJECT_RELOC_SECREL, func_id, patch_pos }, reloc_cap);
                        add_reloc(&sections[0], &(TB_ObjectReloc){ TB_OBJECT_RELOC_SECTION, func_id, patch_pos + 4 }, reloc_cap);
                    }

                    tb_out4b(&debugs_out, 0); // SECREL  | .text
                    tb_out4b(&debugs_out, 0); // SECTION | .text
                    tb_out4b(&debugs_out, out_f->code_size - out_f->nop_pads);

                    // when we make new file line regions
                    // we backpatch the line count for the
                    // region we just finished
                    uint32_t backpatch = 0;
                    int last_line = 0;
                    TB_SourceFile* last_file = 0;
                    uint32_t current_line_count = 0;

                    dyn_array_for(line_id, lines) {
                        TB_Location loc = lines[line_id];

                        if (last_file != loc.file) {
                            if (backpatch) {
                                tb_patch4b(&debugs_out, backpatch, current_line_count);
                                tb_patch4b(&debugs_out, backpatch + 4, 12 + (current_line_count * 8));
                            }
                            last_file = loc.file;

                            // File entry
                            tb_out4b(&debugs_out, file_table_offset[loc.file->id]);
                            backpatch = debugs_out.count;
                            tb_out4b(&debugs_out, 0);
                            tb_out4b(&debugs_out, 0);

                            // printf("  FILE %s\n", loc.file->path);
                            current_line_count = 0;
                            last_line = 0;
                        }

                        if (last_line != loc.line) {
                            last_line = loc.line;
                            // printf("  * LINE %d : %x\n", loc.line, loc.pos);

                            tb_out4b(&debugs_out, loc.pos);
                            tb_out4b(&debugs_out, loc.line);
                            current_line_count++;
                        }
                    }

                    // finalize the patch work
                    if (backpatch) {
                        tb_patch4b(&debugs_out, backpatch, current_line_count);
                        tb_patch4b(&debugs_out, backpatch + 4, 12 + (current_line_count * 8));
                    }

                    tb_patch4b(&debugs_out, field_length_patch, (debugs_out.count - field_length_patch) - 4);
                    // printf("\n");
                }
            }

            align_up_emitter(&debugs_out, 4);
        }

        // Symbol table
        tb_out4b(&debugs_out, 0x000000F1);

        size_t field_length_patch = debugs_out.count;
        tb_out4b(&debugs_out, 0);

        // Symbol info object
        {
            uint32_t obj_length = 2 + 4 + sizeof(dummy_path);
            tb_out2b(&debugs_out, obj_length);
            tb_out2b(&debugs_out, 0x1101);
            tb_out4b(&debugs_out, 0);

            tb_out_reserve(&debugs_out, sizeof(dummy_path));
            tb_outs_UNSAFE(&debugs_out, sizeof(dummy_path), (const uint8_t*) dummy_path);
        }

        // Symbol info properties
        {
            tb_out2b(&debugs_out, creator_length);
            tb_out2b(&debugs_out, 0x1116);
            tb_out4b(&debugs_out, 0);

            tb_out2b(&debugs_out, 0x00D0); // machine
            tb_out2b(&debugs_out, 0);      // verFEMajor
            tb_out2b(&debugs_out, 0);      // verFEMinor
            tb_out2b(&debugs_out, 0);      // verFEBuild

            tb_out2b(&debugs_out, TB_VERSION_MAJOR); // verMajor
            tb_out2b(&debugs_out, TB_VERSION_MINOR); // verMinor
            tb_out2b(&debugs_out, TB_VERSION_PATCH); // verBuild

            tb_out_reserve(&debugs_out, sizeof(creator_str));
            tb_outs_UNSAFE(&debugs_out, sizeof(creator_str), (const uint8_t*)creator_str);

            tb_out2b(&debugs_out, 0);
        }

        CUIK_TIMED_BLOCK("cv: globals") {
            dyn_array_for(i, m->sections) {
                DynArray(TB_Global*) globals = m->sections[i].globals;
                dyn_array_for(j, globals) {
                    TB_Global* g = globals[j];
                    if (g->super.name == NULL) continue;

                    size_t name_len = strlen(g->super.name) + 1;
                    CV_TypeIndex type = g->dbg_type ? convert_to_codeview_type(&builder, g->dbg_type) : T_VOID;

                    // printf("%-20s : %d\n", name, type);
                    size_t baseline = debugs_out.count;
                    tb_out2b(&debugs_out, 0);
                    tb_out2b(&debugs_out, S_GDATA32);
                    tb_out4b(&debugs_out, type); // type index
                    {
                        size_t id = g->super.symbol_id;
                        size_t patch_pos = debugs_out.count;
                        add_reloc(&sections[0], &(TB_ObjectReloc){ TB_OBJECT_RELOC_SECREL, id, patch_pos }, reloc_cap);
                        add_reloc(&sections[0], &(TB_ObjectReloc){ TB_OBJECT_RELOC_SECTION, id, patch_pos + 4 }, reloc_cap);
                    }
                    tb_out4b(&debugs_out, 0); // offset
                    tb_out2b(&debugs_out, 0); // section

                    tb_out_reserve(&debugs_out, name_len);
                    tb_outs_UNSAFE(&debugs_out, name_len, (const uint8_t*) g->super.name);

                    // patch field length
                    tb_patch2b(&debugs_out, baseline, (debugs_out.count - baseline) - 2);
                }
            }
        }

        // Symbols
        CUIK_TIMED_BLOCK("cv: funcs") {
            dyn_array_for(i, m->sections) {
                DynArray(TB_FunctionOutput*) funcs = m->sections[i].funcs;
                dyn_array_for(j, funcs) {
                    TB_FunctionOutput* out_f = funcs[j];
                    TB_Function* f = out_f->parent;

                    const char* name = f->super.name;
                    size_t name_len = strlen(f->super.name) + 1;

                    size_t baseline = debugs_out.count;
                    tb_out2b(&debugs_out, 0);
                    tb_out2b(&debugs_out, S_GPROC32_ID);

                    tb_out4b(&debugs_out, 0); // pointer to the parent
                    tb_out4b(&debugs_out, 0); // pointer to this blocks end (left as zero?)
                    tb_out4b(&debugs_out, 0); // pointer to the next symbol (left as zero?)

                    CV_TypeIndex function_type;
                    {
                        const TB_FunctionPrototype* proto = f->prototype;
                        TB_ArenaSavepoint sp = tb_arena_save(arena);

                        // Create argument list
                        CV_TypeIndex* params = tb_arena_alloc(arena, proto->param_count * sizeof(CV_TypeIndex));
                        FOREACH_N(i, 0, proto->param_count) {
                            TB_DebugType* t = proto->params[i].debug_type;
                            params[i] = t ? convert_to_codeview_type(&builder, t) : T_VOID;
                        }

                        CV_TypeIndex arg_list = tb_codeview_builder_add_arg_list(&builder, proto->param_count, params, proto->has_varargs);
                        tb_arena_restore(arena, sp);

                        // Create return type... if it's multiple returns use a struct
                        CV_TypeIndex return_type = T_VOID;
                        if (proto->return_count == 1) {
                            const TB_PrototypeParam* ret = &TB_PROTOTYPE_RETURNS(proto)[0];
                            return_type = ret->debug_type ? convert_to_codeview_type(&builder, ret->debug_type) : get_codeview_type(ret->dt);
                        }

                        // Create the procedure type
                        CV_TypeIndex proc = tb_codeview_builder_add_procedure(&builder, return_type, arg_list, proto->param_count);

                        // Create the function ID type... which is somehow different from the procedure...
                        // it's basically referring to the procedure but it has a name
                        function_type = tb_codeview_builder_add_function_id(&builder, proc, name);
                    }

                    tb_out4b(&debugs_out, out_f->code_size - out_f->nop_pads); // procedure length
                    tb_out4b(&debugs_out, 0);                // debug start offset
                    tb_out4b(&debugs_out, 0);                // debug end offset
                    tb_out4b(&debugs_out, function_type);    // type index

                    // we save this location because there's two relocations
                    // we'll put there:
                    //   type      target     size
                    //   SECREL    .text     4 bytes
                    //   SECTION   .text     2 bytes
                    {
                        size_t func_id = f->super.symbol_id;
                        size_t patch_pos = debugs_out.count;
                        add_reloc(&sections[0], &(TB_ObjectReloc){ TB_OBJECT_RELOC_SECREL, func_id, patch_pos }, reloc_cap);
                        add_reloc(&sections[0], &(TB_ObjectReloc){ TB_OBJECT_RELOC_SECTION, func_id, patch_pos + 4 }, reloc_cap);
                    }
                    tb_out4b(&debugs_out, 0); // offset
                    tb_out2b(&debugs_out, 0); // segment

                    // the 1 means we have a frame pointer present
                    tb_out1b(&debugs_out, 0); // flags

                    tb_out_reserve(&debugs_out, name_len);
                    tb_outs_UNSAFE(&debugs_out, name_len, (const uint8_t*)name);

                    // patch field length
                    tb_patch2b(&debugs_out, baseline, (debugs_out.count - baseline) - 2);

                    {
                        // frameproc
                        size_t frameproc_baseline = debugs_out.count;

                        tb_out2b(&debugs_out, 0);
                        tb_out2b(&debugs_out, S_FRAMEPROC);

                        size_t stack_usage = out_f->stack_usage == 8 ? 0 : out_f->stack_usage;

                        tb_out4b(&debugs_out, stack_usage); // count of bytes of total frame of procedure
                        tb_out4b(&debugs_out, 0); // count of bytes of padding in the frame
                        tb_out4b(&debugs_out, 0); // offset (relative to frame poniter) to where padding starts
                        tb_out4b(&debugs_out, 0); // count of bytes of callee save registers
                        tb_out4b(&debugs_out, 0); // offset of exception handler
                        tb_out2b(&debugs_out, 0); // section id of exception handler
                        tb_out4b(&debugs_out, 0x00014000); // flags

                        tb_patch2b(&debugs_out, frameproc_baseline, (debugs_out.count - frameproc_baseline) - 2);

                        dyn_array_for(j, out_f->stack_slots) {
                            int stack_pos = out_f->stack_slots[j].storage.offset;
                            TB_DebugType* type = out_f->stack_slots[j].type;

                            const char* var_name = out_f->stack_slots[j].name;
                            assert(var_name);

                            size_t var_name_len = strlen(var_name);
                            uint32_t type_index = convert_to_codeview_type(&builder, type);

                            // define S_REGREL32
                            CV_RegRel32 l = {
                                .reclen = sizeof(CV_RegRel32) + (var_name_len + 1) - 2,
                                .rectyp = S_REGREL32,
                                .off = stack_pos,
                                .typind = type_index,
                                // AMD64_RBP is 334, AMD64_RSP is 335
                                .reg = 335,
                            };
                            tb_outs(&debugs_out, sizeof(CV_RegRel32), &l);
                            tb_outs(&debugs_out, var_name_len + 1, (const uint8_t*) var_name);
                        }
                    }

                    // end the block
                    tb_out2b(&debugs_out, 2);
                    tb_out2b(&debugs_out, S_PROC_ID_END);
                }
            }

            tb_patch4b(&debugs_out, field_length_patch, (debugs_out.count - field_length_patch) - 4);
            align_up_emitter(&debugs_out, 4);
        }
    }
    tb_codeview_builder_done(&builder);
    tb_arena_restore(arena, sp);

    sections[0].raw_data = (TB_Slice){ debugs_out.data, debugs_out.count };
    sections[1].raw_data = (TB_Slice){ builder.type_section.data, builder.type_section.count };

    return (TB_SectionGroup) { 2, sections };
}

IDebugFormat tb__codeview_debug_format = {
    "CodeView",
    codeview_supported_target,
    codeview_number_of_debug_sections,
    codeview_generate_debug_info
};
