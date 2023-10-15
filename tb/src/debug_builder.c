#include "tb_internal.h"

#define NEW(...) memcpy(make_type(m), &(TB_DebugType){ __VA_ARGS__ }, sizeof(TB_DebugType))

static TB_DebugType* make_type(TB_Module* m) {
    return tb_arena_alloc(get_permanent_arena(m), sizeof(TB_DebugType));
}

TB_API TB_DebugType* tb_debug_get_void(TB_Module* m) {
    static TB_DebugType type = { TB_DEBUG_TYPE_VOID };
    return &type;
}

TB_API TB_DebugType* tb_debug_get_bool(TB_Module* m) {
    static TB_DebugType type = { TB_DEBUG_TYPE_BOOL };
    return &type;
}

TB_API TB_DebugType* tb_debug_get_integer(TB_Module* m, bool is_signed, int bits) {
    static TB_DebugType types[] = {
        { .tag = TB_DEBUG_TYPE_UINT, .int_bits = 1 },
        { .tag = TB_DEBUG_TYPE_UINT, .int_bits = 8 },
        { .tag = TB_DEBUG_TYPE_UINT, .int_bits = 16 },
        { .tag = TB_DEBUG_TYPE_UINT, .int_bits = 32 },
        { .tag = TB_DEBUG_TYPE_UINT, .int_bits = 64 },
        { .tag = TB_DEBUG_TYPE_UINT, .int_bits = 128 },

        { .tag = TB_DEBUG_TYPE_INT, .int_bits = 1 },
        { .tag = TB_DEBUG_TYPE_INT, .int_bits = 8 },
        { .tag = TB_DEBUG_TYPE_INT, .int_bits = 16 },
        { .tag = TB_DEBUG_TYPE_INT, .int_bits = 32 },
        { .tag = TB_DEBUG_TYPE_INT, .int_bits = 64 },
        { .tag = TB_DEBUG_TYPE_INT, .int_bits = 128 },
    };

    int b = (is_signed ? 6 : 0);
    if (bits <= 1)   return &types[b + 0];
    if (bits <= 8)   return &types[b + 1];
    if (bits <= 16)  return &types[b + 2];
    if (bits <= 32)  return &types[b + 3];
    if (bits <= 64)  return &types[b + 4];
    if (bits <= 128) return &types[b + 5];
    tb_todo();
}

TB_API TB_DebugType* tb_debug_get_float(TB_Module* m, TB_FloatFormat fmt) {
    static TB_DebugType types[] = {
        [TB_FLT_32] = { TB_DEBUG_TYPE_FLOAT, .float_fmt = TB_FLT_32 },
        [TB_FLT_64] = { TB_DEBUG_TYPE_FLOAT, .float_fmt = TB_FLT_64 },
    };

    return &types[fmt];
}

TB_API TB_DebugType* tb_debug_create_ptr(TB_Module* m, TB_DebugType* base) {
    assert(base != NULL);
    return NEW(TB_DEBUG_TYPE_POINTER, .ptr_to = base);
}

TB_API TB_DebugType* tb_debug_create_array(TB_Module* m, TB_DebugType* base, size_t count) {
    return NEW(TB_DEBUG_TYPE_ARRAY, .array = { base, count });
}

TB_API TB_DebugType* tb_debug_create_struct(TB_Module* m, ptrdiff_t len, const char* tag) {
    TB_DebugType* t = NEW(TB_DEBUG_TYPE_STRUCT);
    t->record.tag = tb__arena_strdup(m, len, tag);
    return t;
}

TB_API TB_DebugType* tb_debug_create_union(TB_Module* m, ptrdiff_t len, const char* tag) {
    TB_DebugType* t = NEW(TB_DEBUG_TYPE_UNION);
    t->record.tag = tb__arena_strdup(m, len, tag);
    return t;
}

TB_API TB_DebugType* tb_debug_create_alias(TB_Module* m, TB_DebugType* base, ptrdiff_t len, const char* name) {
    return NEW(TB_DEBUG_TYPE_ALIAS, .alias = { tb__arena_strdup(m, len, name), base });
}

TB_API TB_DebugType* tb_debug_create_field(TB_Module* m, TB_DebugType* type, ptrdiff_t len, const char* name, TB_CharUnits offset) {
    assert(name);
    return NEW(TB_DEBUG_TYPE_FIELD, .field = { tb__arena_strdup(m, len, name), offset, type });
}

TB_API TB_DebugType** tb_debug_record_begin(TB_Module* m, TB_DebugType* type, size_t count) {
    type->record.count = count;
    return (type->record.members = tb_arena_alloc(get_permanent_arena(m), count * sizeof(TB_DebugType*)));
}

TB_API void tb_debug_record_end(TB_DebugType* type, TB_CharUnits size, TB_CharUnits align) {
    type->record.size = size;
    type->record.align = align;
}

TB_API TB_DebugType* tb_debug_create_func(TB_Module* m, TB_CallingConv cc, size_t param_count, size_t return_count, bool has_varargs) {
    TB_DebugType* t = NEW(TB_DEBUG_TYPE_FUNCTION);
    t->func.cc = cc;
    t->func.has_varargs = has_varargs;
    t->func.param_count = param_count;
    t->func.params = TB_ARENA_ARR_ALLOC(get_permanent_arena(m), param_count, TB_DebugType*);
    t->func.return_count = return_count;
    t->func.returns = TB_ARENA_ARR_ALLOC(get_permanent_arena(m), return_count, TB_DebugType*);
    return t;
}

TB_API size_t tb_debug_func_return_count(TB_DebugType* type) {
    return type->func.return_count;
}

TB_API size_t tb_debug_func_param_count(TB_DebugType* type) {
    return type->func.param_count;
}

TB_API TB_DebugType** tb_debug_func_params(TB_DebugType* type) {
    return type->func.params;
}

TB_API TB_DebugType** tb_debug_func_returns(TB_DebugType* type) {
    return type->func.returns;
}
