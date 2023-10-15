#include "cv.h"

static uint32_t hash_buffer(uint32_t hash, size_t n, const void* s) {
    const uint8_t* p = s;
    while (n--) {
        hash = (hash ^ *p++) * 16777619;
    }
    return hash;
}

static void align_up_type_record(TB_Emitter* e) {
    // type records need to be 4 byte aligned with a decreasing number of LF_PAD0+i ending in 0
    size_t align = e->count % 4;
    if (align == 0) return;

    size_t pad = 4 - align;
    while (pad > 0) {
        tb_out1b(e, LF_PAD0 + pad);
        pad -= 1;
    }
}

static void align_up_emitter(TB_Emitter* e, size_t u) {
    size_t pad = align_up(e->count, u) - e->count;
    while (pad--) tb_out1b(e, 0x00);
}

static CV_TypeIndex find_or_make_cv_type(CV_Builder* builder, size_t length, const void* k) {
    #if 0
    // This "dead" code is for educational purposes since i know that there aren't many Codeview resources.
    // the path that gets correctly run will merely deduplicate the results i simulate here as a size-optimization
    CV_TypeIndex type_index = builder->type_entry_count++;
    tb_outs(sect, length, k);
    return type_index;
    #else
    // Hash it
    const uint8_t* key = k;
    uint32_t hash = hash_buffer(0, length, key);

    // Search (if there's a collision replace the old one)
    assert(tb_is_power_of_two(builder->lookup_table_size));
    size_t index = hash & (builder->lookup_table_size - 1);
    CV_TypeEntry lookup = builder->lookup_table[index];

    // printf("Lookup %zu (%x hash, has match? %s)\n", index, hash, lookup.key ? "yea" : "naw");
    if (lookup.key) {
        // verify it even matches
        size_t lookup_size = tb_get2b(&builder->type_section, lookup.key) + 2;

        if (length == lookup_size && memcmp(key, &builder->type_section.data[lookup.key], length) == 0) {
            //printf("Saved %zu bytes (%d)\n", length, lookup.value);
            return lookup.value;
        }
    }

    CV_TypeIndex type_index = builder->type_entry_count++;
    // printf("Used %zu bytes (%d)\n", length, type_index);

    builder->lookup_table[index].key   = builder->type_section.count;
    builder->lookup_table[index].value = type_index;

    // NOTE: MSF/PDB requires the type entries to be properly aligned to 4bytes... we
    // might wanna add an option to the builder for that.
    tb_outs(&builder->type_section, length, key);
    return type_index;
    #endif
}

CV_Builder tb_codeview_builder_create(size_t cache_size, CV_TypeEntry* cache) {
    assert(tb_is_power_of_two(cache_size));

    // start the type table
    TB_Emitter e = { 0 };
    tb_out4b(&e, 0x00000004);

    if (cache == NULL) {
        cache = tb_platform_heap_alloc(cache_size * sizeof(CV_TypeEntry));
    }

    return (CV_Builder){
        .type_section = e,

        .needs_to_free_lookup = (cache == NULL),
        .lookup_table_size = cache_size,
        .lookup_table = cache,

        // user defined types start at 0x1000
        .type_entry_count = 0x1000,
    };
}

void tb_codeview_builder_done(CV_Builder* builder) {
    if (builder->needs_to_free_lookup) {
        tb_platform_heap_free(builder);
    }

    // align_up_type_record(&builder->type_section);
}

CV_TypeIndex tb_codeview_builder_add_array(CV_Builder* builder, CV_TypeIndex base, size_t byte_count) {
    #pragma pack(push,1)
    typedef struct {
        uint16_t len;
        uint16_t leaf;     // LF_ARRAY
        uint32_t elemtype; // type index of element type
        uint32_t idxtype;  // type index of indexing type

        // uint8_t  data[];   // variable length data specifying size in bytes and name
        //
        // This is technically what needs to be written but within this builder we hardcode
        // a count and a null name
        struct {
            uint16_t type;
            uint32_t value;
        } count;

        char name[1]; // keep as zero
    } Array;
    #pragma pack(pop)

    assert(byte_count == (uint32_t) byte_count);
    Array arr = {
        .len = sizeof(Array) - 2,
        .leaf = LF_ARRAY,
        .elemtype = base,
        .idxtype = T_INT8,
        .count = { LF_LONG, byte_count },
    };

    return find_or_make_cv_type(builder, sizeof(arr), &arr);
}

CV_TypeIndex tb_codeview_builder_add_pointer(CV_Builder* builder, CV_TypeIndex ptr_to) {
    CV_LFPointer ptr = {
        .len = sizeof(CV_LFPointer) - 2,
        .leaf = LF_POINTER,
        .utype = ptr_to,
        .attr = {
            .ptrtype = 0x0c, // CV_PTR_64
            .ptrmode = 0,    // CV_PTR_MODE_PTR
            .size    = 8,
        }
    };

    return find_or_make_cv_type(builder, sizeof(ptr), &ptr);
}

CV_TypeIndex tb_codeview_builder_add_arg_list(CV_Builder* builder, size_t count, const CV_TypeIndex* args, bool has_varargs) {
    enum { SMALL_ARR_CAP = 8 + (16 * 8) };
    uint8_t tmp[SMALL_ARR_CAP];

    // this is what the arglist is like
    #pragma pack(push,1)
    typedef struct {
        uint16_t len;
        uint16_t type; // LF_ARGLIST
        uint32_t arg_count;
        CV_TypeIndex args[];
    } Arglist;
    #pragma pack(pop)

    // use allocated space if we couldn't fit
    Arglist* data = (Arglist*) tmp;
    size_t length = sizeof(Arglist) + ((count + has_varargs) * sizeof(CV_TypeIndex));
    if (length >= SMALL_ARR_CAP) data = tb_platform_heap_alloc(length);

    data->len = length - 2;
    data->type = LF_ARGLIST;
    data->arg_count = count;
    memcpy(data->args, args, count * sizeof(CV_TypeIndex));

    // varargs add a dummy type at the end of the list
    if (has_varargs) {
        data->args[count] = 0;
    }

    CV_TypeIndex type = find_or_make_cv_type(builder, length, data);
    if (length >= SMALL_ARR_CAP) tb_platform_heap_free(data);

    return type;
}

CV_TypeIndex tb_codeview_builder_add_procedure(CV_Builder* builder, CV_TypeIndex return_type, CV_TypeIndex arg_list, size_t param_count) {
    #pragma pack(push,1)
    struct Procedure {
        uint16_t len;
        uint16_t leaf;      // LF_PROCEDURE
        uint32_t rvtype;    // type index of return value
        uint8_t  calltype;  // calling convention (CV_call_t)
        uint8_t  funcattr;  // attributes
        uint16_t parmcount; // number of parameters
        uint32_t arglist;   // type index of argument list
    } proc = {
        .len = sizeof(struct Procedure) - 2,
        .leaf = LF_PROCEDURE,
        .rvtype = return_type,
        .parmcount = param_count,
        .arglist = arg_list,
    };
    #pragma pack(pop)

    return find_or_make_cv_type(builder, sizeof(proc), &proc);
}

CV_TypeIndex tb_codeview_builder_add_function_id(CV_Builder* builder, CV_TypeIndex proc_type, const char* name) {
    size_t name_size = strlen(name) + 1;

    #pragma pack(push,1)
    // TODO(NeGate): support scopes
    struct Function {
        uint16_t len;     // doesn't include itself, so sizeof(T)-2
        uint16_t leaf;    // LF_FUNC_ID
        uint32_t scopeId; // parent scope of the ID, 0 if global
        uint32_t type;    // function type
        uint8_t  name[];  // null-terminated string
    } func = {
        .len = sizeof(struct Function) + name_size - 2,
        .leaf = LF_FUNC_ID,
        .scopeId = 0,
        .type = proc_type,
    };
    #pragma pack(pop)

    CV_TypeIndex id = builder->type_entry_count++;
    tb_outs(&builder->type_section, sizeof(func), &func);
    tb_outs(&builder->type_section, name_size, name);
    return id;
}

CV_TypeIndex tb_codeview_builder_add_alias(CV_Builder* builder, CV_RecordType base, const char* name) {
    CV_TypeIndex id = builder->type_entry_count++;
    size_t name_size = strlen(name) + 1;

    CV_LFAlias a = {
        .len = sizeof(CV_LFAlias) + name_size - 2,
        .leaf = LF_ALIAS,
        .utype = base,
    };
    tb_outs(&builder->type_section, sizeof(a), &a);
    tb_outs(&builder->type_section, name_size, name);
    return id;
}

CV_TypeIndex tb_codeview_builder_add_incomplete_record(CV_Builder* builder, CV_RecordType rec_type, const char* name) {
    CV_TypeIndex id = builder->type_entry_count++;
    size_t name_size = strlen(name) + 1;

    CV_LFStruct s = {
        .len = sizeof(CV_LFStruct) + 3 + name_size - 2,
        .leaf = rec_type,

        // it's a forward declaration
        .property.fwdref = 1,
    };
    tb_outs(&builder->type_section, sizeof(s), &s);

    // write 0 as the size
    tb_out2b(&builder->type_section, LF_CHAR);
    tb_out1b(&builder->type_section, 0);

    tb_outs(&builder->type_section, name_size, name);
    return id;
}

CV_TypeIndex tb_codeview_builder_add_field_list(CV_Builder* builder, size_t count, const CV_Field* fields) {
    // write field list
    CV_TypeIndex id = builder->type_entry_count++;

    size_t patch_pos = builder->type_section.count;
    tb_out2b(&builder->type_section, 0); // length (we'll patch it later)
    tb_out2b(&builder->type_section, LF_FIELDLIST); // type

    FOREACH_N(i, 0, count) {
        size_t name_size = strlen(fields[i].name) + 1;

        // write member heading
        CV_LFMember m = {
            .leaf = LF_MEMBER,
            .index = fields[i].type,
        };
        tb_outs(&builder->type_section, sizeof(CV_LFMember), &m);

        // write offset
        if (fields[i].offset == (int8_t) fields[i].offset) {
            // write it as 3 bytes instead of 6 since it's a common case
            tb_out2b(&builder->type_section, LF_CHAR);
            tb_out1b(&builder->type_section, fields[i].offset);
        } else {
            tb_out2b(&builder->type_section, LF_LONG);
            tb_out4b(&builder->type_section, fields[i].offset);
        }

        // write out C string
        tb_outs(&builder->type_section, name_size, fields[i].name);
        align_up_type_record(&builder->type_section);
    }
    tb_patch2b(&builder->type_section, patch_pos, (builder->type_section.count - patch_pos) - 2);
    return id;
}

CV_TypeIndex tb_codeview_builder_add_record(CV_Builder* builder, CV_RecordType rec_type, size_t field_count, CV_TypeIndex field_list, TB_CharUnits size, const char* name) {
    // write struct type
    CV_TypeIndex id = builder->type_entry_count++;
    size_t name_size = strlen(name) + 1;

    size_t patch_pos = builder->type_section.count;
    CV_LFStruct s = {
        .leaf = rec_type,
        .count = field_count,
        .field = field_list,
    };
    tb_outs(&builder->type_section, sizeof(s), &s);

    if (size == (int8_t) size) {
        // write it as 3 bytes instead of 6 since it's a common case
        tb_out2b(&builder->type_section, LF_CHAR);
        tb_out1b(&builder->type_section, size);
    } else {
        // write size (simple LF_LONG)
        tb_out2b(&builder->type_section, LF_LONG);
        tb_out4b(&builder->type_section, size);
    }

    // write name
    tb_outs(&builder->type_section, name_size, name);

    // backpatch length
    tb_patch2b(&builder->type_section, patch_pos, (builder->type_section.count - patch_pos) - 2);
    return id;
}
