#pragma once
#include "../objects/coff.h"

typedef uint32_t CV_TypeIndex;

typedef struct {
    // Type table
    CV_TypeIndex type_entry_count;
    TB_Emitter type_section;

    // Type table cache
    bool needs_to_free_lookup;
    size_t lookup_table_size; // must be a power of two
    CV_TypeEntry* lookup_table;

    // Symbol table
    // TODO
} CV_Builder;

typedef struct {
    CV_TypeIndex type;
    const char* name;
    TB_CharUnits offset;
} CV_Field;

// map to their Codeview record types because it's simpler
typedef enum {
    CV_CLASS = 0x1504, CV_STRUCT = 0x1505, CV_UNION = 0x1506,
} CV_RecordType;

// if cache is NULL and cache_size is not 0, then it'll allocate and zeroed it for you
// which will also be freed on tb_codeview_builder_done
//
// cache_size must be a power of two
// if cache is non-NULL it has cache_size elements and zeroed
CV_Builder tb_codeview_builder_create(size_t cache_size, CV_TypeEntry* cache);

// doesn't free the section memory
void tb_codeview_builder_done(CV_Builder* builder);

// Type builder API
CV_TypeIndex tb_codeview_builder_add_array(CV_Builder* builder, CV_TypeIndex base, size_t count);
CV_TypeIndex tb_codeview_builder_add_pointer(CV_Builder* builder, CV_TypeIndex ptr_to);
CV_TypeIndex tb_codeview_builder_add_arg_list(CV_Builder* builder, size_t count, const CV_TypeIndex* args, bool has_varargs);
CV_TypeIndex tb_codeview_builder_add_procedure(CV_Builder* builder, CV_TypeIndex return_type, CV_TypeIndex arg_list, size_t param_count);

// function ID is just a procedure with name, it's used by the symbol table
CV_TypeIndex tb_codeview_builder_add_function_id(CV_Builder* builder, CV_TypeIndex proc_type, const char* name);

// When refering to records from other types you must use this forward declaration
CV_TypeIndex tb_codeview_builder_add_incomplete_record(CV_Builder* builder, CV_RecordType rec_type, const char* name);
CV_TypeIndex tb_codeview_builder_add_field_list(CV_Builder* builder, size_t count, const CV_Field* fields);
CV_TypeIndex tb_codeview_builder_add_record(CV_Builder* builder, CV_RecordType rec_type, size_t field_count, CV_TypeIndex field_list, TB_CharUnits size, const char* name);
