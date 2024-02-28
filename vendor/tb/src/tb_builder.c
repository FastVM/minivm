// IR BUILDER
//
// Handles generating the TB_Function IR via C functions.
// Note that these functions can perform certain simple
// optimizations while the generation happens to improve
// the machine code output or later analysis stages.
#include "tb_internal.h"

static void add_input_late(TB_Function* f, TB_Node* n, TB_Node* in);

TB_API void tb_inst_set_trace(TB_Function* f, TB_Trace trace) { f->trace = trace; }
TB_API TB_Trace tb_inst_get_trace(TB_Function* f) { return f->trace; }
TB_Node* tb_inst_get_control(TB_Function* f) { return f->trace.bot_ctrl; }
TB_API TB_Node* tb_inst_root_node(TB_Function* f) { return f->root_node; }

static TB_Node* transfer_ctrl(TB_Function* f, TB_Node* n) {
    TB_Node* prev = f->trace.bot_ctrl;
    f->trace.bot_ctrl = n;
    if (f->line_loc) {
        nl_table_put(&f->locations, n, f->line_loc);
    }
    return prev;
}

void tb_inst_set_control(TB_Function* f, TB_Node* control) {
    if (control == NULL) {
        f->trace.top_ctrl = NULL;
        f->trace.bot_ctrl = NULL;
        f->trace.mem = NULL;
    } else {
        assert(control->type == TB_REGION);
        f->trace.top_ctrl = control;
        f->trace.bot_ctrl = control;
        f->trace.mem = TB_NODE_GET_EXTRA_T(control, TB_NodeRegion)->mem_in;
    }
}

TB_Node* tb_inst_region_mem_in(TB_Function* f, TB_Node* region) {
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(region);
    return r->mem_in;
}

TB_Trace tb_inst_trace_from_region(TB_Function* f, TB_Node* region) {
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(region);
    return (TB_Trace){ region, region, r->mem_in };
}

static TB_Node* get_callgraph(TB_Function* f) { return f->root_node->inputs[0]; }
static TB_Node* peek_mem(TB_Function* f) { return f->trace.mem; }

// adds memory effect to region
static TB_Node* append_mem(TB_Function* f, TB_Node* new_mem) {
    TB_Node* old = f->trace.mem;
    f->trace.mem = new_mem;
    if (f->line_loc) {
        nl_table_put(&f->locations, new_mem, f->line_loc);
    }
    return old;
}

TB_Node* tb__make_proj(TB_Function* f, TB_DataType dt, TB_Node* src, int index) {
    assert(src->dt.type == TB_TUPLE);
    TB_Node* proj = tb_alloc_node(f, TB_PROJ, dt, 1, sizeof(TB_NodeProj));
    set_input(f, proj, src, 0);
    TB_NODE_SET_EXTRA(proj, TB_NodeProj, .index = index);
    return proj;
}

bool tb_node_is_constant_int(TB_Function* f, TB_Node* n, uint64_t imm) {
    return n->type == TB_INTEGER_CONST ? (TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value == imm) : false;
}

bool tb_node_is_constant_non_zero(TB_Node* n) {
    return n->type == TB_INTEGER_CONST ? (TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value != 0) : false;
}

bool tb_node_is_constant_zero(TB_Node* n) {
    return n->type == TB_INTEGER_CONST ? (TB_NODE_GET_EXTRA_T(n, TB_NodeInt)->value == 0) : false;
}

void tb_function_attrib_variable(TB_Function* f, TB_Node* n, TB_Node* parent, ptrdiff_t len, const char* name, TB_DebugType* type) {
    TB_NodeLocal* l = TB_NODE_GET_EXTRA(n);
    l->name = tb__arena_strdup(f->super.module, len, name);
    l->type = type;
}

void tb_function_attrib_scope(TB_Function* f, TB_Node* n, TB_Node* parent) {
}

void tb_inst_location(TB_Function* f, TB_SourceFile* file, int line, int column) {
    TB_NodeLocation* loc = tb_arena_alloc(f->arena, sizeof(TB_NodeLocation));
    loc->file = file;
    loc->line = line;
    loc->column = column;
    f->line_loc = loc;
}

void tb_inst_set_exit_location(TB_Function* f, TB_SourceFile* file, int line, int column) {
    TB_NodeLocation* loc = tb_arena_alloc(f->arena, sizeof(TB_NodeLocation));
    loc->file = file;
    loc->line = line;
    loc->column = column;
    nl_table_put(&f->locations, f->root_node->inputs[0], loc);
}

static void* alloc_from_node_arena(TB_Function* f, size_t necessary_size) {
    // return malloc(necessary_size);
    return tb_arena_alloc(f->arena, necessary_size);
}

TB_Node* tb_alloc_node_dyn(TB_Function* f, int type, TB_DataType dt, int input_count, int input_cap, size_t extra) {
    assert(input_count < UINT16_MAX && "too many inputs!");

    TB_Node* n = alloc_from_node_arena(f, sizeof(TB_Node) + extra);
    n->type = type;
    n->input_cap = input_cap;
    n->input_count = input_count;
    n->dt = dt;
    n->gvn = f->node_count++;
    n->users = NULL;

    if (input_cap > 0) {
        n->inputs = alloc_from_node_arena(f, input_cap * sizeof(TB_Node*));
        memset(n->inputs, 0, input_count * sizeof(TB_Node*));
    } else {
        n->inputs = NULL;
    }

    if (extra > 0) {
        memset(n->extra, 0, extra);
    }

    return n;
}

TB_Node* tb_alloc_node(TB_Function* f, int type, TB_DataType dt, int input_count, size_t extra) {
    return tb_alloc_node_dyn(f, type, dt, input_count, input_count, extra);
}

static TB_Node* tb_bin_arith(TB_Function* f, int type, TB_ArithmeticBehavior arith_behavior, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node(f, type, a->dt, 3, sizeof(TB_NodeBinopInt));
    set_input(f, n, a, 1);
    set_input(f, n, b, 2);
    TB_NODE_SET_EXTRA(n, TB_NodeBinopInt, .ab = arith_behavior);
    return tb_pass_gvn_node(f, n);
}

static TB_Node* tb_bin_farith(TB_Function* f, int type, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node(f, type, a->dt, 3, 0);
    set_input(f, n, a, 1);
    set_input(f, n, b, 2);
    return tb_pass_gvn_node(f, n);
}

static TB_Node* tb_unary(TB_Function* f, int type, TB_DataType dt, TB_Node* src) {
    TB_Node* n = tb_alloc_node(f, type, dt, 2, 0);
    set_input(f, n, src, 1);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_trunc(TB_Function* f, TB_Node* src, TB_DataType dt) {
    return tb_unary(f, TB_TRUNCATE, dt, src);
}

TB_Node* tb_inst_int2ptr(TB_Function* f, TB_Node* src) {
    return tb_unary(f, TB_BITCAST, TB_TYPE_PTR, src);
}

TB_Node* tb_inst_ptr2int(TB_Function* f, TB_Node* src, TB_DataType dt) {
    return tb_unary(f, TB_BITCAST, dt, src);
}

TB_Node* tb_inst_int2float(TB_Function* f, TB_Node* src, TB_DataType dt, bool is_signed) {
    assert(dt.type == TB_FLOAT);
    assert(src->dt.type == TB_INT);

    if (src->type == TB_INTEGER_CONST) {
        uint64_t y = TB_NODE_GET_EXTRA_T(src, TB_NodeInt)->value;
        if (is_signed) {
            y = tb__sxt(y, src->dt.data, 64);
        }

        if (dt.data == TB_FLT_32) {
            float x;
            if (is_signed) x = (int64_t) y;
            else x = (uint64_t) y;

            return tb_inst_float32(f, x);
        } else if (dt.data == TB_FLT_64) {
            double x;
            if (is_signed) x = (int64_t) y;
            else x = (uint64_t) y;

            return tb_inst_float64(f, x);
        }
    }

    return tb_unary(f, is_signed ? TB_INT2FLOAT : TB_UINT2FLOAT, dt, src);
}

TB_Node* tb_inst_float2int(TB_Function* f, TB_Node* src, TB_DataType dt, bool is_signed) {
    return tb_unary(f, is_signed ? TB_FLOAT2INT : TB_FLOAT2UINT, dt, src);
}

TB_Node* tb_inst_fpxt(TB_Function* f, TB_Node* src, TB_DataType dt) {
    return tb_unary(f, TB_FLOAT_EXT, dt, src);
}

TB_Node* tb_inst_sxt(TB_Function* f, TB_Node* src, TB_DataType dt) {
    if (src->type == TB_INTEGER_CONST) {
        uint64_t y = TB_NODE_GET_EXTRA_T(src, TB_NodeInt)->value;
        y = tb__sxt(y, src->dt.data, 64);
        return tb_inst_uint(f, dt, y);
    }

    return tb_unary(f, TB_SIGN_EXT, dt, src);
}

TB_Node* tb_inst_zxt(TB_Function* f, TB_Node* src, TB_DataType dt) {
    if (src->type == TB_INTEGER_CONST) {
        uint64_t y = TB_NODE_GET_EXTRA_T(src, TB_NodeInt)->value;
        return tb_inst_uint(f, dt, y);
    }

    return tb_unary(f, TB_ZERO_EXT, dt, src);
}

TB_Node* tb_inst_bitcast(TB_Function* f, TB_Node* src, TB_DataType dt) {
    return tb_unary(f, TB_BITCAST, dt, src);
}

TB_Node* tb_inst_param(TB_Function* f, int param_id) {
    assert(param_id < f->param_count);
    return f->params[3 + param_id];
}

void tb_get_data_type_size(TB_Module* mod, TB_DataType dt, size_t* size, size_t* align) {
    const ICodeGen* restrict code_gen = tb__find_code_generator(mod);
    code_gen->get_data_type_size(dt, size, align);
}

void tb_inst_unreachable(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_UNREACHABLE, TB_TYPE_CONTROL, 1, 0);
    set_input(f, n, transfer_ctrl(f, n), 0);
    add_input_late(f, f->root_node, n);
    f->trace.bot_ctrl = NULL;
}

void tb_inst_debugbreak(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_DEBUGBREAK, TB_TYPE_CONTROL, 1, 0);
    set_input(f, n, transfer_ctrl(f, n), 0);
}

void tb_inst_trap(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_TRAP, TB_TYPE_CONTROL, 1, 0);
    set_input(f, n, transfer_ctrl(f, n), 0);
    add_input_late(f, f->root_node, n);
    f->trace.bot_ctrl = NULL;
}

TB_Node* tb_inst_local(TB_Function* f, TB_CharUnits size, TB_CharUnits alignment) {
    assert(size > 0);
    assert(alignment > 0 && tb_is_power_of_two(alignment));

    // insert in the entry block
    TB_Node* n = tb_alloc_node(f, TB_LOCAL, TB_TYPE_PTR, 1, sizeof(TB_NodeLocal));
    set_input(f, n, f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeLocal, .size = size, .align = alignment);
    return tb_pass_gvn_node(f, n);
}

void tb_inst_safepoint_poll(TB_Function* f, void* tag, TB_Node* addr, int input_count, TB_Node** inputs) {
    TB_Node* n = tb_alloc_node(f, TB_SAFEPOINT_POLL, TB_TYPE_CONTROL, 3 + input_count, sizeof(TB_NodeSafepoint));
    set_input(f, n, transfer_ctrl(f, n), 0);
    set_input(f, n, peek_mem(f), 1);
    set_input(f, n, addr, 2);
    FOREACH_N(i, 0, input_count) {
        set_input(f, n, inputs[i], i + 3);
    }
    TB_NODE_SET_EXTRA(n, TB_NodeSafepoint, tag);
}

TB_Node* tb_inst_load(TB_Function* f, TB_DataType dt, TB_Node* addr, TB_CharUnits alignment, bool is_volatile) {
    assert(addr);

    if (is_volatile) {
        TB_Node* n = tb_alloc_node(f, TB_READ, TB_TYPE_TUPLE, 3, 0);
        set_input(f, n, f->trace.bot_ctrl, 0);
        set_input(f, n, peek_mem(f), 1);
        set_input(f, n, addr, 2);
        append_mem(f, tb__make_proj(f, TB_TYPE_MEMORY, n, 0));
        return tb__make_proj(f, dt, n, 1);
    } else {
        TB_Node* n = tb_alloc_node(f, TB_LOAD, dt, 3, sizeof(TB_NodeMemAccess));
        set_input(f, n, f->trace.bot_ctrl, 0);
        set_input(f, n, peek_mem(f), 1);
        set_input(f, n, addr, 2);
        TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = alignment);
        return tb_pass_gvn_node(f, n);
    }
}

void tb_inst_store(TB_Function* f, TB_DataType dt, TB_Node* addr, TB_Node* val, uint32_t alignment, bool is_volatile) {
    assert(TB_DATA_TYPE_EQUALS(dt, val->dt));

    TB_Node* n;
    if (is_volatile) {
        n = tb_alloc_node(f, TB_WRITE, TB_TYPE_MEMORY, 4, 0);
    } else {
        n = tb_alloc_node(f, TB_STORE, TB_TYPE_MEMORY, 4, sizeof(TB_NodeMemAccess));
        TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = alignment);
    }
    set_input(f, n, f->trace.bot_ctrl, 0);
    set_input(f, n, append_mem(f, n), 1);
    set_input(f, n, addr, 2);
    set_input(f, n, val, 3);
}

void tb_inst_memset(TB_Function* f, TB_Node* addr, TB_Node* val, TB_Node* size, TB_CharUnits align) {
    assert(TB_IS_POINTER_TYPE(addr->dt));
    assert(TB_IS_INTEGER_TYPE(val->dt) && val->dt.data == 8);

    TB_Node* n = tb_alloc_node(f, TB_MEMSET, TB_TYPE_MEMORY, 5, sizeof(TB_NodeMemAccess));
    set_input(f, n, f->trace.bot_ctrl, 0);
    set_input(f, n, append_mem(f, n), 1);
    set_input(f, n, addr, 2);
    set_input(f, n, val, 3);
    set_input(f, n, size, 4);
    TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = align);
}

void tb_inst_memcpy(TB_Function* f, TB_Node* addr, TB_Node* val, TB_Node* size, TB_CharUnits align) {
    assert(TB_IS_POINTER_TYPE(addr->dt));
    assert(TB_IS_POINTER_TYPE(val->dt));

    TB_Node* n = tb_alloc_node(f, TB_MEMCPY, TB_TYPE_MEMORY, 5, sizeof(TB_NodeMemAccess));
    set_input(f, n, f->trace.bot_ctrl, 0);
    set_input(f, n, append_mem(f, n), 1);
    set_input(f, n, addr, 2);
    set_input(f, n, val, 3);
    set_input(f, n, size, 4);
    TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = align);
}

void tb_inst_memzero(TB_Function* f, TB_Node* dst, TB_Node* count, TB_CharUnits align) {
    tb_inst_memset(f, dst, tb_inst_uint(f, TB_TYPE_I8, 0), count, align);
}

TB_Node* tb_inst_bool(TB_Function* f, bool imm) {
    TB_Node* n = tb_alloc_node(f, TB_INTEGER_CONST, TB_TYPE_BOOL, 1, sizeof(TB_NodeInt));
    set_input(f, n, f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeInt, .value = imm);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_uint(TB_Function* f, TB_DataType dt, uint64_t imm) {
    assert(TB_IS_POINTER_TYPE(dt) || TB_IS_INTEGER_TYPE(dt));

    if (dt.type == TB_INT && dt.data < 64) {
        uint64_t mask = ~UINT64_C(0) >> (64 - dt.data);
        imm &= mask;
    }

    TB_Node* n = tb_alloc_node(f, TB_INTEGER_CONST, dt, 1, sizeof(TB_NodeInt));
    set_input(f, n, f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeInt, .value = imm);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_sint(TB_Function* f, TB_DataType dt, int64_t imm) {
    assert(TB_IS_POINTER_TYPE(dt) || (TB_IS_INTEGER_TYPE(dt) && (dt.data <= 64)));

    TB_Node* n = tb_alloc_node(f, TB_INTEGER_CONST, dt, 1, sizeof(TB_NodeInt));
    set_input(f, n, f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeInt, .value = imm);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_float32(TB_Function* f, float imm) {
    TB_Node* n = tb_alloc_node(f, TB_FLOAT32_CONST, TB_TYPE_F32, 1, sizeof(TB_NodeFloat32));
    set_input(f, n, f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeFloat32, .value = imm);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_float64(TB_Function* f, double imm) {
    TB_Node* n = tb_alloc_node(f, TB_FLOAT64_CONST, TB_TYPE_F64, 1, sizeof(TB_NodeFloat64));
    set_input(f, n, f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeFloat64, .value = imm);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_string(TB_Function* f, size_t len, const char* str) {
    TB_Global* dummy = tb_global_create(f->super.module, 0, NULL, NULL, TB_LINKAGE_PRIVATE);
    tb_global_set_storage(f->super.module, tb_module_get_rdata(f->super.module), dummy, len, 1, 1);

    char* dst = tb_global_add_region(f->super.module, dummy, 0, len);
    memcpy(dst, str, len);

    return tb_inst_get_symbol_address(f, (TB_Symbol*) dummy);
}

TB_Node* tb_inst_cstring(TB_Function* f, const char* str) {
    return tb_inst_string(f, strlen(str) + 1, str);
}

TB_Node* tb_inst_array_access(TB_Function* f, TB_Node* base, TB_Node* index, int64_t stride) {
    TB_Node* n = tb_alloc_node(f, TB_ARRAY_ACCESS, TB_TYPE_PTR, 3, sizeof(TB_NodeArray));
    set_input(f, n, base, 1);
    set_input(f, n, index, 2);
    TB_NODE_SET_EXTRA(n, TB_NodeArray, .stride = stride);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_member_access(TB_Function* f, TB_Node* base, int64_t offset) {
    if (offset == 0) {
        return base;
    }

    TB_Node* n = tb_alloc_node(f, TB_MEMBER_ACCESS, TB_TYPE_PTR, 2, sizeof(TB_NodeMember));
    set_input(f, n, base, 1);
    TB_NODE_SET_EXTRA(n, TB_NodeMember, .offset = offset);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_get_symbol_address(TB_Function* f, TB_Symbol* target) {
    assert(target != NULL);

    TB_Node* n = tb_alloc_node(f, TB_SYMBOL, TB_TYPE_PTR, 1, sizeof(TB_NodeSymbol));
    set_input(f, n, f->root_node, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeSymbol, .sym = target);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_syscall(TB_Function* f, TB_DataType dt, TB_Node* syscall_num, size_t param_count, TB_Node** params) {
    TB_Node* n = tb_alloc_node(f, TB_SYSCALL, TB_TYPE_TUPLE, 3 + param_count, sizeof(TB_NodeCall) + sizeof(TB_Node*[3]));
    set_input(f, n, syscall_num, 2);
    FOREACH_N(i, 0, param_count) {
        set_input(f, n, params[i], i + 3);
    }

    // control proj
    TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, 0);
    set_input(f, n, transfer_ctrl(f, cproj), 0);

    // memory proj
    TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 1);
    set_input(f, n, append_mem(f, mproj), 1);

    // return value
    TB_Node* dproj = tb__make_proj(f, dt, n, 2);

    TB_NodeCall* c = TB_NODE_GET_EXTRA(n);
    c->proj_count = 3;
    c->proto = NULL;
    c->projs[0] = cproj;
    c->projs[1] = mproj;
    c->projs[2] = dproj;
    return dproj;
}

TB_MultiOutput tb_inst_call(TB_Function* f, TB_FunctionPrototype* proto, TB_Node* target, size_t param_count, TB_Node** params) {
    size_t proj_count = 2 + (proto->return_count > 1 ? proto->return_count : 1);

    TB_Node* n = tb_alloc_node(f, TB_CALL, TB_TYPE_TUPLE, 3 + param_count, sizeof(TB_NodeCall) + (sizeof(TB_Node*)*proj_count));
    set_input(f, n, target, 2);
    FOREACH_N(i, 0, param_count) {
        set_input(f, n, params[i], i + 3);
    }

    TB_NodeCall* c = TB_NODE_GET_EXTRA(n);
    c->proj_count = proj_count;
    c->proto = proto;

    // control proj
    TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, 0);
    set_input(f, n, transfer_ctrl(f, cproj), 0);

    // memory proj
    TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 1);
    set_input(f, n, append_mem(f, mproj), 1);

    // create data projections
    TB_PrototypeParam* rets = TB_PROTOTYPE_RETURNS(proto);
    FOREACH_N(i, 0, proto->return_count) {
        c->projs[i + 2] = tb__make_proj(f, rets[i].dt, n, i + 2);
    }

    // we'll slot a NULL so it's easy to tell when it's empty
    if (proto->return_count == 0) {
        c->projs[2] = NULL;
    }

    c->projs[0] = cproj;
    c->projs[1] = mproj;

    add_input_late(f, get_callgraph(f), n);

    if (proto->return_count == 1) {
        return (TB_MultiOutput){ .count = proto->return_count, .single = c->projs[2] };
    } else {
        return (TB_MultiOutput){ .count = proto->return_count, .multiple = c->projs + 2 };
    }
}

void tb_inst_tailcall(TB_Function* f, TB_FunctionPrototype* proto, TB_Node* target, size_t param_count, TB_Node** params) {
    TB_Node* n = tb_alloc_node(f, TB_TAILCALL, TB_TYPE_CONTROL, 3 + param_count, sizeof(TB_NodeTailcall));
    set_input(f, n, transfer_ctrl(f, n), 0);
    set_input(f, n, peek_mem(f), 1);
    set_input(f, n, target, 2);
    FOREACH_N(i, 0, param_count) {
        set_input(f, n, params[i], i + 3);
    }

    TB_NodeTailcall* c = TB_NODE_GET_EXTRA(n);
    c->proto = proto;

    add_input_late(f, get_callgraph(f), n);
    add_input_late(f, f->root_node, n);
}

TB_Node* tb_inst_poison(TB_Function* f, TB_DataType dt) {
    TB_Node* n = tb_alloc_node(f, TB_POISON, dt, 1, 0);
    set_input(f, n, f->root_node, 0);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_not(TB_Function* f, TB_Node* src) {
    TB_Node* n = tb_alloc_node(f, TB_NOT, src->dt, 2, 0);
    set_input(f, n, src, 1);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_bswap(TB_Function* f, TB_Node* src) {
    TB_Node* n = tb_alloc_node(f, TB_BSWAP, src->dt, 2, 0);
    set_input(f, n, src, 1);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_clz(TB_Function* f, TB_Node* src) {
    assert(TB_IS_INTEGER_TYPE(src->dt));
    TB_Node* n = tb_alloc_node(f, TB_CLZ, TB_TYPE_I32, 2, 0);
    set_input(f, n, src, 1);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_ctz(TB_Function* f, TB_Node* src) {
    assert(TB_IS_INTEGER_TYPE(src->dt));
    TB_Node* n = tb_alloc_node(f, TB_CTZ, TB_TYPE_I32, 2, 0);
    set_input(f, n, src, 1);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_popcount(TB_Function* f, TB_Node* src) {
    assert(TB_IS_INTEGER_TYPE(src->dt));
    TB_Node* n = tb_alloc_node(f, TB_POPCNT, TB_TYPE_I32, 2, 0);
    set_input(f, n, src, 1);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_neg(TB_Function* f, TB_Node* src) {
    TB_DataType dt = src->dt;
    if (src->type == TB_INTEGER_CONST) {
        uint64_t x = TB_NODE_GET_EXTRA_T(src, TB_NodeInt)->value;
        uint64_t mask = ~UINT64_C(0) >> (64 - dt.data);

        // two's complement negate is just invert and add 1
        uint64_t negated = (~x + 1) & mask;
        return tb_inst_sint(f, dt, negated);
    } else if (src->type == TB_FLOAT32_CONST) {
        float x = TB_NODE_GET_EXTRA_T(src, TB_NodeFloat32)->value;
        return tb_inst_float32(f, -x);
    } else if (src->type == TB_FLOAT64_CONST) {
        double x = TB_NODE_GET_EXTRA_T(src, TB_NodeFloat64)->value;
        return tb_inst_float64(f, -x);
    } else {
        return tb_unary(f, TB_NEG, src->dt, src);
    }
}

TB_Node* tb_inst_select(TB_Function* f, TB_Node* cond, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node(f, TB_SELECT, a->dt, 4, 0);
    set_input(f, n, cond, 1);
    set_input(f, n, a, 2);
    set_input(f, n, b, 3);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_and(TB_Function* f, TB_Node* a, TB_Node* b) {
    // bitwise operators can't wrap
    return tb_bin_arith(f, TB_AND, 0, a, b);
}

TB_Node* tb_inst_or(TB_Function* f, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));
    return tb_bin_arith(f, TB_OR, 0, a, b);
}

TB_Node* tb_inst_xor(TB_Function* f, TB_Node* a, TB_Node* b) {
    // bitwise operators can't wrap
    return tb_bin_arith(f, TB_XOR, 0, a, b);
}

TB_Node* tb_inst_add(TB_Function* f, TB_Node* a, TB_Node* b, TB_ArithmeticBehavior arith_behavior) {
    return tb_bin_arith(f, TB_ADD, arith_behavior, a, b);
}

TB_Node* tb_inst_sub(TB_Function* f, TB_Node* a, TB_Node* b, TB_ArithmeticBehavior arith_behavior) {
    return tb_bin_arith(f, TB_SUB, arith_behavior, a, b);
}

TB_Node* tb_inst_mul(TB_Function* f, TB_Node* a, TB_Node* b, TB_ArithmeticBehavior arith_behavior) {
    return tb_bin_arith(f, TB_MUL, arith_behavior, a, b);
}

TB_Node* tb_inst_div(TB_Function* f, TB_Node* a, TB_Node* b, bool signedness) {
    TB_Node* peek = b->type == TB_SIGN_EXT ? b->inputs[1] : b;
    if (peek->type == TB_INTEGER_CONST) {
        TB_NodeInt* i = TB_NODE_GET_EXTRA(peek);
        uint64_t log2 = tb_ffs(i->value) - 1;
        if (i->value == UINT64_C(1) << log2) {
            return tb_bin_arith(f, TB_SHR, 0, a, tb_inst_uint(f, a->dt, log2));
        }
    }

    // division can't wrap or overflow
    return tb_bin_arith(f, signedness ? TB_SDIV : TB_UDIV, 0, a, b);
}

TB_Node* tb_inst_mod(TB_Function* f, TB_Node* a, TB_Node* b, bool signedness) {
    // modulo can't wrap or overflow
    return tb_bin_arith(f, signedness ? TB_SMOD : TB_UMOD, 0, a, b);
}

TB_Node* tb_inst_shl(TB_Function* f, TB_Node* a, TB_Node* b, TB_ArithmeticBehavior arith_behavior) {
    return tb_bin_arith(f, TB_SHL, arith_behavior, a, b);
}

TB_Node* tb_inst_rol(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_bin_arith(f, TB_ROL, 0, a, b);
}

TB_Node* tb_inst_ror(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_bin_arith(f, TB_ROR, 0, a, b);
}

////////////////////////////////
// Atomics
////////////////////////////////
static TB_Node* atomic_op(TB_Function* f, int op, TB_DataType dt, TB_Node* addr, TB_Node* src, TB_MemoryOrder order) {
    TB_Node* n = tb_alloc_node(f, op, TB_TYPE_TUPLE, src ? 4 : 3, sizeof(TB_NodeAtomic));
    set_input(f, n, f->trace.bot_ctrl, 0);
    set_input(f, n, addr, 2);
    if (src) {
        set_input(f, n, src, 3);
    }

    TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 0);
    TB_Node* dproj = tb__make_proj(f, dt, n, 1);
    TB_NODE_SET_EXTRA(n, TB_NodeAtomic, .order = order, .order2 = TB_MEM_ORDER_SEQ_CST);

    // memory proj
    set_input(f, n, append_mem(f, mproj), 1);
    return dproj;
}

TB_Node* tb_inst_atomic_load(TB_Function* f, TB_Node* addr, TB_DataType dt, TB_MemoryOrder order) {
    return atomic_op(f, TB_ATOMIC_LOAD, dt, addr, NULL, order);
}

TB_Node* tb_inst_atomic_xchg(TB_Function* f, TB_Node* addr, TB_Node* src, TB_MemoryOrder order) {
    assert(src);
    return atomic_op(f, TB_ATOMIC_XCHG, src->dt, addr, src, order);
}

TB_Node* tb_inst_atomic_add(TB_Function* f, TB_Node* addr, TB_Node* src, TB_MemoryOrder order) {
    assert(src);
    return atomic_op(f, TB_ATOMIC_ADD, src->dt, addr, src, order);
}

TB_Node* tb_inst_atomic_sub(TB_Function* f, TB_Node* addr, TB_Node* src, TB_MemoryOrder order) {
    assert(src);
    return atomic_op(f, TB_ATOMIC_SUB, src->dt, addr, src, order);
}

TB_Node* tb_inst_atomic_and(TB_Function* f, TB_Node* addr, TB_Node* src, TB_MemoryOrder order) {
    assert(src);
    return atomic_op(f, TB_ATOMIC_AND, src->dt, addr, src, order);
}

TB_Node* tb_inst_atomic_xor(TB_Function* f, TB_Node* addr, TB_Node* src, TB_MemoryOrder order) {
    assert(src);
    return atomic_op(f, TB_ATOMIC_XOR, src->dt, addr, src, order);
}

TB_Node* tb_inst_atomic_or(TB_Function* f, TB_Node* addr, TB_Node* src, TB_MemoryOrder order) {
    assert(src);
    return atomic_op(f, TB_ATOMIC_OR, src->dt, addr, src, order);
}

TB_Node* tb_inst_atomic_cmpxchg(TB_Function* f, TB_Node* addr, TB_Node* expected, TB_Node* desired, TB_MemoryOrder succ, TB_MemoryOrder fail) {
    assert(TB_DATA_TYPE_EQUALS(desired->dt, expected->dt));
    TB_DataType dt = desired->dt;

    TB_Node* n = tb_alloc_node(f, TB_ATOMIC_CAS, TB_TYPE_TUPLE, 5, sizeof(TB_NodeAtomic));
    set_input(f, n, f->trace.bot_ctrl, 0);
    set_input(f, n, addr, 2);
    set_input(f, n, expected, 3);
    set_input(f, n, desired, 4);

    TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 0);
    TB_Node* dproj = tb__make_proj(f, dt, n, 1);
    TB_NODE_SET_EXTRA(n, TB_NodeAtomic, .order = succ, .order2 = fail);

    // memory proj
    set_input(f, n, append_mem(f, mproj), 1);
    return dproj;
}

// TODO(NeGate): Maybe i should split the bitshift operations into a separate kind of
// operator that has different arithmatic behaviors, maybe like trap on a large shift amount
TB_Node* tb_inst_sar(TB_Function* f, TB_Node* a, TB_Node* b) {
    // shift right can't wrap or overflow
    return tb_bin_arith(f, TB_SAR, 0, a, b);
}

TB_Node* tb_inst_shr(TB_Function* f, TB_Node* a, TB_Node* b) {
    // shift right can't wrap or overflow
    return tb_bin_arith(f, TB_SHR, 0, a, b);
}

TB_Node* tb_inst_fadd(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_bin_farith(f, TB_FADD, a, b);
}

TB_Node* tb_inst_fsub(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_bin_farith(f, TB_FSUB, a, b);
}

TB_Node* tb_inst_fmul(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_bin_farith(f, TB_FMUL, a, b);
}

TB_Node* tb_inst_fdiv(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_bin_farith(f, TB_FDIV, a, b);
}

TB_Node* tb_inst_va_start(TB_Function* f, TB_Node* a) {
    assert(a->type == TB_LOCAL);

    TB_Node* n = tb_alloc_node(f, TB_VA_START, TB_TYPE_PTR, 2, 0);
    set_input(f, n, a, 1);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_x86_ldmxcsr(TB_Function* f, TB_Node* a) {
    assert(a->dt.type == TB_INT && a->dt.data == 32);

    TB_Node* n = tb_alloc_node(f, TB_X86INTRIN_LDMXCSR, TB_TYPE_I32, 2, 0);
    set_input(f, n, a, 1);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_cycle_counter(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_CYCLE_COUNTER, TB_TYPE_I64, 1, 0);
    set_input(f, n, f->trace.bot_ctrl, 0);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_prefetch(TB_Function* f, TB_Node* addr, int level) {
    TB_Node* n = tb_alloc_node(f, TB_PREFETCH, TB_TYPE_MEMORY, 2, sizeof(TB_NodePrefetch));
    set_input(f, n, addr, 1);
    TB_NODE_SET_EXTRA(n, TB_NodePrefetch, .level = level);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_x86_stmxcsr(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_X86INTRIN_STMXCSR, TB_TYPE_I32, 1, 0);
    set_input(f, n, f->trace.bot_ctrl, 0);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_x86_sqrt(TB_Function* f, TB_Node* a) {
    return tb_unary(f, TB_X86INTRIN_SQRT, a->dt, a);
}

TB_Node* tb_inst_x86_rsqrt(TB_Function* f, TB_Node* a) {
    return tb_unary(f, TB_X86INTRIN_RSQRT, a->dt, a);
}

TB_Node* tb_inst_cmp(TB_Function* f, TB_NodeType type, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node(f, type, TB_TYPE_BOOL, 3, sizeof(TB_NodeCompare));
    set_input(f, n, a, 1);
    set_input(f, n, b, 2);
    TB_NODE_SET_EXTRA(n, TB_NodeCompare, .cmp_dt = a->dt);
    return tb_pass_gvn_node(f, n);
}

TB_Node* tb_inst_cmp_eq(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_inst_cmp(f, TB_CMP_EQ, a, b);
}

TB_Node* tb_inst_cmp_ne(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_inst_cmp(f, TB_CMP_NE, a, b);
}

TB_Node* tb_inst_cmp_ilt(TB_Function* f, TB_Node* a, TB_Node* b, bool signedness) {
    return tb_inst_cmp(f, signedness ? TB_CMP_SLT : TB_CMP_ULT, a, b);
}

TB_Node* tb_inst_cmp_ile(TB_Function* f, TB_Node* a, TB_Node* b, bool signedness) {
    return tb_inst_cmp(f, signedness ? TB_CMP_SLE : TB_CMP_ULE, a, b);
}

TB_Node* tb_inst_cmp_igt(TB_Function* f, TB_Node* a, TB_Node* b, bool signedness) {
    return tb_inst_cmp(f, signedness ? TB_CMP_SLT : TB_CMP_ULT, b, a);
}

TB_Node* tb_inst_cmp_ige(TB_Function* f, TB_Node* a, TB_Node* b, bool signedness) {
    return tb_inst_cmp(f, signedness ? TB_CMP_SLE : TB_CMP_ULE, b, a);
}

TB_Node* tb_inst_cmp_flt(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_inst_cmp(f, TB_CMP_FLT, a, b);
}

TB_Node* tb_inst_cmp_fle(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_inst_cmp(f, TB_CMP_FLE, a, b);
}

TB_Node* tb_inst_cmp_fgt(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_inst_cmp(f, TB_CMP_FLT, b, a);
}

TB_Node* tb_inst_cmp_fge(TB_Function* f, TB_Node* a, TB_Node* b) {
    return tb_inst_cmp(f, TB_CMP_FLE, b, a);
}

TB_Node* tb_inst_incomplete_phi(TB_Function* f, TB_DataType dt, TB_Node* region, size_t preds) {
    TB_Node* n = tb_alloc_node(f, TB_PHI, dt, 1 + preds, 0);
    set_input(f, n, region, 0);
    return n;
}

bool tb_inst_add_phi_operand(TB_Function* f, TB_Node* phi, TB_Node* region, TB_Node* val) {
    assert(region->type != TB_REGION && "umm... im expecting a region not whatever that was");
    TB_Node* phi_region = phi->inputs[0];

    // the slot to fill is based on the predecessor list of the region
    FOREACH_N(i, 0, phi_region->input_count) {
        TB_Node* pred = phi_region->inputs[i];
        while (pred->type != TB_REGION) pred = pred->inputs[0];

        if (pred == region) {
            set_input(f, phi, val, i + 1);
            return true;
        }
    }

    return false;
}

TB_Node* tb_inst_phi2(TB_Function* f, TB_Node* region, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node_dyn(f, TB_PHI, a->dt, 3, 3, 0);
    set_input(f, n, region, 0);
    set_input(f, n, a, 1);
    set_input(f, n, b, 2);
    return n;
}

TB_API TB_Node* tb_inst_region(TB_Function* f) {
    return tb_inst_new_trace(f).top_ctrl;
}

TB_API TB_Trace tb_inst_new_trace(TB_Function* f) {
    TB_Node* n = tb_alloc_node_dyn(f, TB_REGION, TB_TYPE_CONTROL, 0, 4, sizeof(TB_NodeRegion));

    TB_Node* phi = tb_alloc_node_dyn(f, TB_PHI, TB_TYPE_MEMORY, 1, 5, 0);
    set_input(f, phi, n, 0);
    TB_NODE_SET_EXTRA(n, TB_NodeRegion, .mem_in = phi);
    return (TB_Trace){ n, n, phi };
}

void tb_inst_set_region_name(TB_Function* f, TB_Node* n, ptrdiff_t len, const char* name) {
    if (len < 0) len = strlen(name);

    TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);

    char* newstr = alloc_from_node_arena(f, len + 1);
    memcpy(newstr, name, len + 1);
    r->tag = newstr;
}

// this has to move things which is not nice...
static void add_input_late(TB_Function* f, TB_Node* n, TB_Node* in) {
    assert(n->type == TB_REGION || n->type == TB_PHI || n->type == TB_ROOT || n->type == TB_CALLGRAPH || n->type == TB_MERGEMEM);

    if (n->input_count >= n->input_cap) {
        size_t new_cap = n->input_count * 2;
        TB_Node** new_inputs = alloc_from_node_arena(f, new_cap * sizeof(TB_Node*));
        if (n->inputs != NULL) {
            memcpy(new_inputs, n->inputs, n->input_count * sizeof(TB_Node*));
        }

        n->inputs = new_inputs;
        n->input_cap = new_cap;
    }

    n->inputs[n->input_count] = in;
    add_user(f, n, in, n->input_count, NULL);
    n->input_count += 1;
}

static void add_memory_edge(TB_Function* f, TB_Node* n, TB_Node* mem_state, TB_Node* target) {
    assert(target->type == TB_REGION);
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(target);
    assert(r->mem_in && r->mem_in->type == TB_PHI);
    add_input_late(f, r->mem_in, mem_state);
}

void tb_inst_goto(TB_Function* f, TB_Node* target) {
    TB_Node* mem_state = peek_mem(f);

    // there's no need for a branch if the path isn't diverging.
    TB_Node* n = f->trace.bot_ctrl;
    f->trace.bot_ctrl = NULL;

    // just add the edge directly.
    assert(n->dt.type == TB_CONTROL);
    add_input_late(f, target, n);
    add_memory_edge(f, n, mem_state, target);
}

TB_Node* tb_inst_if(TB_Function* f, TB_Node* cond, TB_Node* if_true, TB_Node* if_false) {
    TB_Node* mem_state = peek_mem(f);

    // generate control projections
    TB_Node* n = tb_alloc_node(f, TB_BRANCH, TB_TYPE_TUPLE, 2, sizeof(TB_NodeBranch) + sizeof(TB_BranchKey));
    set_input(f, n, transfer_ctrl(f, NULL), 0);
    set_input(f, n, cond, 1);

    FOREACH_N(i, 0, 2) {
        TB_Node* target = i ? if_false : if_true;

        TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, i);
        add_input_late(f, target, cproj);
        add_memory_edge(f, n, mem_state, target);
    }

    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
    br->total_hits = 100;
    br->succ_count  = 2;
    br->keys[0].key = 0;
    br->keys[0].taken = 50;
    return n;
}

TB_Node* tb_inst_if2(TB_Function* f, TB_Node* cond, TB_Node* projs[2]) {
    TB_Node* mem_state = peek_mem(f);

    // generate control projections
    TB_Node* n = tb_alloc_node(f, TB_BRANCH, TB_TYPE_TUPLE, 2, sizeof(TB_NodeBranch) + sizeof(TB_BranchKey));
    set_input(f, n, transfer_ctrl(f, NULL), 0);
    set_input(f, n, cond, 1);

    FOREACH_N(i, 0, 2) {
        projs[i] = tb__make_proj(f, TB_TYPE_CONTROL, n, i);
    }

    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
    br->total_hits = 100;
    br->succ_count = 2;
    br->keys[0].key = 0;
    br->keys[0].taken = 50;
    return n;
}

// n is a TB_BRANCH with two successors, taken is the number of times it's true
void tb_inst_set_branch_freq(TB_Function* f, TB_Node* n, int total_hits, int taken) {
    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
    assert(br->succ_count == 2 && "only works on if-branches");
    br->total_hits = total_hits;
    br->keys[0].taken = total_hits - taken;
}

TB_Node* tb_inst_branch(TB_Function* f, TB_DataType dt, TB_Node* key, TB_Node* default_label, size_t entry_count, const TB_SwitchEntry* entries) {
    TB_Node* mem_state = peek_mem(f);

    // generate control projections
    TB_Node* n = tb_alloc_node(f, TB_BRANCH, TB_TYPE_TUPLE, 2, sizeof(TB_NodeBranch) + (sizeof(TB_BranchKey) * entry_count));
    set_input(f, n, transfer_ctrl(f, NULL), 0);
    set_input(f, n, key, 1);

    FOREACH_N(i, 0, 1 + entry_count) {
        TB_Node* target = i ? entries[i - 1].value : default_label;

        TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, i);
        add_input_late(f, target, cproj);
        add_memory_edge(f, n, mem_state, target);
    }

    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
    br->total_hits = (1 + entry_count) * 10;
    br->succ_count = 1 + entry_count;
    FOREACH_N(i, 0, entry_count) {
        br->keys[i].key = entries[i].key;
        br->keys[i].taken = 10;
    }
    return n;
}

void tb_inst_ret(TB_Function* f, size_t count, TB_Node** values) {
    TB_Node* mem_state = peek_mem(f);

    // allocate return node
    TB_Node* ret = f->ret_node;
    TB_Node* ctrl = ret->inputs[0];
    assert(ctrl->type == TB_REGION);

    // add to PHIs
    assert(ret->input_count >= 3 + count);
    add_input_late(f, ret->inputs[1], mem_state);

    size_t i = 3;
    for (; i < count + 3; i++) {
        assert(ret->inputs[i]->dt.raw == values[i - 3]->dt.raw && "datatype mismatch");
        add_input_late(f, ret->inputs[i], values[i - 3]);
    }

    size_t phi_count = ret->input_count;
    for (; i < phi_count; i++) {
        // put poison in the leftovers?
        log_warn("%s: ir: generated poison due to inconsistent number of returned values", f->super.name);

        TB_Node* poison = tb_alloc_node(f, TB_POISON, ret->inputs[i]->dt, 1, 0);
        set_input(f, poison, f->ret_node, 0);

        poison = tb__gvn(f, poison, 0);
        add_input_late(f, ret->inputs[i], poison);
    }

    // basically just tb_inst_goto without the memory PHI (we did it earlier)
    TB_Node* n = f->trace.bot_ctrl;
    f->trace.bot_ctrl = NULL;

    add_input_late(f, ctrl, n);
}
