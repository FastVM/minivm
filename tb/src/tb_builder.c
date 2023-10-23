// IR BUILDER
//
// Handles generating the TB_Function IR via C functions.
// Note that these functions can perform certain simple
// optimizations while the generation happens to improve
// the machine code output or later analysis stages.
#include "tb_internal.h"

static void append_attrib(TB_Function* f, TB_Node* n, TB_Attrib a) {
    ptrdiff_t search = nl_map_get(f->attribs, n);
    if (search < 0) {
        DynArray(TB_Attrib) attribs = dyn_array_create(TB_Attrib, 2);
        dyn_array_put(attribs, a);
        nl_map_put(f->attribs, n, attribs);
    } else {
        dyn_array_put(f->attribs[search].v, a);
    }
}

// adds memory effect to region
static TB_Node* append_mem(TB_Function* f, TB_Node* new_mem) {
    TB_Node* bb = tb_get_parent_region(f->active_control_node);
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(bb);

    TB_Node* old_mem = r->mem_out;
    assert(old_mem != NULL && "how?");

    r->mem_out = new_mem;
    return old_mem;
}

static TB_Node* peek_mem(TB_Function* f, TB_Node* ctrl) {
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(tb_get_parent_region(ctrl));
    return r->mem_out;
}

TB_Node* tb__make_proj(TB_Function* f, TB_DataType dt, TB_Node* src, int index) {
    assert(src->dt.type == TB_TUPLE);
    TB_Node* proj = tb_alloc_node(f, TB_PROJ, dt, 1, sizeof(TB_NodeProj));
    proj->inputs[0] = src;
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
    append_attrib(f, n, (TB_Attrib){ TB_ATTRIB_VARIABLE, .var = { parent, tb__arena_strdup(f->super.module, len, name), type } });
}

void tb_function_attrib_scope(TB_Function* f, TB_Node* n, TB_Node* parent) {
    append_attrib(f, n, (TB_Attrib){ TB_ATTRIB_SCOPE, .scope = { parent } });
}

void tb_function_attrib_location(TB_Function* f, TB_Node* n, TB_SourceFile* file, int line, int column) {
    append_attrib(f, n, (TB_Attrib){ TB_ATTRIB_LOCATION, .loc = { file, line, column } });
}

void tb_inst_set_location(TB_Function* f, TB_SourceFile* file, int line, int column) {
    f->line_attrib = (TB_Attrib){ TB_ATTRIB_LOCATION, .loc = { file, line, column } };
}

void tb_inst_set_exit_location(TB_Function* f, TB_SourceFile* file, int line, int column) {
    f->exit_attrib = (TB_Attrib){ TB_ATTRIB_LOCATION, .loc = { file, line, column } };
}

void tb_inst_reset_location(TB_Function* f) {
    f->line_attrib.loc.file = NULL;
}

static void* alloc_from_node_arena(TB_Function* f, size_t necessary_size) {
    // return malloc(necessary_size);
    return tb_arena_alloc(f->arena, necessary_size);
}

TB_Node* tb_alloc_node(TB_Function* f, int type, TB_DataType dt, int input_count, size_t extra) {
    assert(input_count < UINT16_MAX && "too many inputs!");

    TB_Node* n = alloc_from_node_arena(f, sizeof(TB_Node) + extra);
    n->type = type;
    n->dt = dt;
    n->gvn = f->node_count++;
    n->input_count = input_count;
    n->users = NULL;

    if (input_count > 0) {
        n->inputs = alloc_from_node_arena(f, input_count * sizeof(TB_Node*));
        memset(n->inputs, 0, input_count * sizeof(TB_Node*));
    } else {
        // basically only true for START, maybe it's best
        // we just give it a NULL slot to avoid certain awkward
        // checks?
        n->inputs = NULL;
    }

    if (extra > 0) {
        memset(n->extra, 0, extra);
    }

    // give most side effect the location attrib
    if (type != TB_REGION && type != TB_END && type != TB_PHI) {
        if (f->line_attrib.loc.file != NULL) {
            append_attrib(f, n, f->line_attrib);
        }
    }

    return n;
}

static TB_Node* tb_bin_arith(TB_Function* f, int type, TB_ArithmeticBehavior arith_behavior, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node(f, type, a->dt, 3, sizeof(TB_NodeBinopInt));
    n->inputs[1] = a;
    n->inputs[2] = b;
    TB_NODE_SET_EXTRA(n, TB_NodeBinopInt, .ab = arith_behavior);
    return n;
}

static TB_Node* tb_bin_farith(TB_Function* f, int type, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node(f, type, a->dt, 3, 0);
    n->inputs[1] = a;
    n->inputs[2] = b;
    return n;
}

static TB_Node* tb_unary(TB_Function* f, int type, TB_DataType dt, TB_Node* src) {
    TB_Node* n = tb_alloc_node(f, type, dt, 2, 0);
    n->inputs[1] = src;
    return n;
}

TB_Node* tb_inst_trunc(TB_Function* f, TB_Node* src, TB_DataType dt) {
    return tb_unary(f, TB_TRUNCATE, dt, src);
}

TB_Node* tb_inst_int2ptr(TB_Function* f, TB_Node* src) {
    return tb_unary(f, TB_INT2PTR, TB_TYPE_PTR, src);
}

TB_Node* tb_inst_ptr2int(TB_Function* f, TB_Node* src, TB_DataType dt) {
    return tb_unary(f, TB_PTR2INT, dt, src);
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
    return tb_unary(f, TB_SIGN_EXT, dt, src);
}

TB_Node* tb_inst_zxt(TB_Function* f, TB_Node* src, TB_DataType dt) {
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

void tb_inst_set_control(TB_Function* f, TB_Node* control) {
    f->active_control_node = control;
}

TB_Node* tb_inst_get_control(TB_Function* f) {
    return f->active_control_node;
}

void tb_inst_unreachable(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_UNREACHABLE, TB_TYPE_CONTROL, 1, 0);
    n->inputs[0] = f->active_control_node;
    f->active_control_node = NULL;
    dyn_array_put(f->terminators, n);
}

void tb_inst_debugbreak(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_DEBUGBREAK, TB_TYPE_CONTROL, 1, 0);
    n->inputs[0] = f->active_control_node;
    f->active_control_node = n;
}

void tb_inst_trap(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_TRAP, TB_TYPE_CONTROL, 1, 0);
    n->inputs[0] = f->active_control_node;
    f->active_control_node = NULL;
    dyn_array_put(f->terminators, n);
}

TB_Node* tb_inst_local(TB_Function* f, TB_CharUnits size, TB_CharUnits alignment) {
    assert(size > 0);
    assert(alignment > 0 && tb_is_power_of_two(alignment));

    // insert in the entry block
    TB_Node* n = tb_alloc_node(f, TB_LOCAL, TB_TYPE_PTR, 1, sizeof(TB_NodeLocal));
    n->inputs[0] = f->start_node; // pin to START
    TB_NODE_SET_EXTRA(n, TB_NodeLocal, .size = size, .align = alignment);
    return n;
}

void tb_inst_safepoint_poll(TB_Function* f, TB_Node* addr, int input_count, TB_Node** inputs) {
    TB_Node* n = tb_alloc_node(f, TB_SAFEPOINT_POLL, TB_TYPE_CONTROL, 3 + input_count, sizeof(TB_NodeSafepoint));
    n->inputs[0] = f->active_control_node;
    n->inputs[1] = peek_mem(f, f->active_control_node);
    n->inputs[2] = addr;
    if (input_count > 0) {
        memcpy(n->inputs + 3, inputs, input_count * sizeof(TB_Node*));
    }
    TB_NODE_SET_EXTRA(n, TB_NodeSafepoint, 0);
    f->active_control_node = n;
}

TB_Node* tb_inst_load(TB_Function* f, TB_DataType dt, TB_Node* addr, TB_CharUnits alignment, bool is_volatile) {
    assert(addr);

    TB_Node* n = tb_alloc_node(f, is_volatile ? TB_READ : TB_LOAD, is_volatile ? TB_TYPE_TUPLE : dt, 3, sizeof(TB_NodeMemAccess));
    n->inputs[0] = f->active_control_node;
    n->inputs[1] = peek_mem(f, f->active_control_node);
    n->inputs[2] = addr;
    TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = alignment);

    if (is_volatile) {
        append_mem(f, tb__make_proj(f, TB_TYPE_MEMORY, n, 0));
        return tb__make_proj(f, dt, n, 1);
    } else {
        return n;
    }
}

void tb_inst_store(TB_Function* f, TB_DataType dt, TB_Node* addr, TB_Node* val, uint32_t alignment, bool is_volatile) {
    assert(TB_DATA_TYPE_EQUALS(dt, val->dt));

    TB_Node* n = tb_alloc_node(f, is_volatile ? TB_WRITE : TB_STORE, TB_TYPE_MEMORY, 4, sizeof(TB_NodeMemAccess));
    n->inputs[0] = f->active_control_node;
    n->inputs[1] = append_mem(f, n);
    n->inputs[2] = addr;
    n->inputs[3] = val;
    TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = alignment);
}

void tb_inst_memset(TB_Function* f, TB_Node* dst, TB_Node* val, TB_Node* size, TB_CharUnits align) {
    assert(TB_IS_POINTER_TYPE(dst->dt));
    assert(TB_IS_INTEGER_TYPE(val->dt) && val->dt.data == 8);

    TB_Node* n = tb_alloc_node(f, TB_MEMSET, TB_TYPE_MEMORY, 5, sizeof(TB_NodeMemAccess));
    n->inputs[0] = f->active_control_node;
    n->inputs[1] = append_mem(f, n);
    n->inputs[2] = dst;
    n->inputs[3] = val;
    n->inputs[4] = size;
    TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = align);
}

void tb_inst_memcpy(TB_Function* f, TB_Node* dst, TB_Node* val, TB_Node* size, TB_CharUnits align) {
    assert(TB_IS_POINTER_TYPE(dst->dt));
    assert(TB_IS_POINTER_TYPE(val->dt));

    TB_Node* n = tb_alloc_node(f, TB_MEMCPY, TB_TYPE_MEMORY, 5, sizeof(TB_NodeMemAccess));
    n->inputs[0] = f->active_control_node;
    n->inputs[1] = append_mem(f, n);
    n->inputs[2] = dst;
    n->inputs[3] = val;
    n->inputs[4] = size;
    TB_NODE_SET_EXTRA(n, TB_NodeMemAccess, .align = align);
}

void tb_inst_memzero(TB_Function* f, TB_Node* dst, TB_Node* count, TB_CharUnits align) {
    tb_inst_memset(f, dst, tb_inst_uint(f, TB_TYPE_I8, 0), count, align);
}

TB_Node* tb_inst_bool(TB_Function* f, bool imm) {
    TB_Node* n = tb_alloc_node(f, TB_INTEGER_CONST, TB_TYPE_BOOL, 1, sizeof(TB_NodeInt));
    TB_NODE_SET_EXTRA(n, TB_NodeInt, .value = imm);
    return n;
}

TB_Node* tb_inst_uint(TB_Function* f, TB_DataType dt, uint64_t imm) {
    assert(TB_IS_POINTER_TYPE(dt) || TB_IS_INTEGER_TYPE(dt));

    if (dt.type == TB_INT && dt.data < 64) {
        uint64_t mask = ~UINT64_C(0) >> (64 - dt.data);
        imm &= mask;
    }

    TB_Node* n = tb_alloc_node(f, TB_INTEGER_CONST, dt, 1, sizeof(TB_NodeInt));
    TB_NODE_SET_EXTRA(n, TB_NodeInt, .value = imm);
    return n;
}

TB_Node* tb_inst_sint(TB_Function* f, TB_DataType dt, int64_t imm) {
    assert(TB_IS_POINTER_TYPE(dt) || (TB_IS_INTEGER_TYPE(dt) && (dt.data <= 64)));

    TB_Node* n = tb_alloc_node(f, TB_INTEGER_CONST, dt, 1, sizeof(TB_NodeInt));
    TB_NODE_SET_EXTRA(n, TB_NodeInt, .value = imm);
    return n;
}

TB_Node* tb_inst_float32(TB_Function* f, float imm) {
    TB_Node* n = tb_alloc_node(f, TB_FLOAT32_CONST, TB_TYPE_F32, 1, sizeof(TB_NodeFloat32));
    TB_NODE_SET_EXTRA(n, TB_NodeFloat32, .value = imm);
    return n;
}

TB_Node* tb_inst_float64(TB_Function* f, double imm) {
    TB_Node* n = tb_alloc_node(f, TB_FLOAT64_CONST, TB_TYPE_F64, 1, sizeof(TB_NodeFloat64));
    TB_NODE_SET_EXTRA(n, TB_NodeFloat64, .value = imm);
    return n;
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
    n->inputs[1] = base;
    n->inputs[2] = index;
    TB_NODE_SET_EXTRA(n, TB_NodeArray, .stride = stride);
    return n;
}

TB_Node* tb_inst_member_access(TB_Function* f, TB_Node* base, int64_t offset) {
    if (offset == 0) {
        return base;
    }

    TB_Node* n = tb_alloc_node(f, TB_MEMBER_ACCESS, TB_TYPE_PTR, 2, sizeof(TB_NodeMember));
    n->inputs[1] = base;
    TB_NODE_SET_EXTRA(n, TB_NodeMember, .offset = offset);
    return n;
}

TB_Node* tb_inst_get_symbol_address(TB_Function* f, TB_Symbol* target) {
    assert(target != NULL);

    TB_Node* n = tb_alloc_node(f, TB_SYMBOL, TB_TYPE_PTR, 1, sizeof(TB_NodeSymbol));
    TB_NODE_SET_EXTRA(n, TB_NodeSymbol, .sym = target);
    return n;
}

TB_Node* tb_inst_syscall(TB_Function* f, TB_DataType dt, TB_Node* syscall_num, size_t param_count, TB_Node** params) {
    TB_Node* n = tb_alloc_node(f, TB_SYSCALL, TB_TYPE_TUPLE, 3 + param_count, sizeof(TB_NodeCall) + sizeof(TB_Node*[3]));
    n->inputs[0] = f->active_control_node;
    n->inputs[2] = syscall_num;
    memcpy(n->inputs + 3, params, param_count * sizeof(TB_Node*));

    // control proj
    TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, 0);
    f->active_control_node = cproj;

    // memory proj
    TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 1);
    n->inputs[1] = append_mem(f, mproj);

    // return value
    TB_Node* dproj = tb__make_proj(f, dt, n, 2);

    TB_NodeCall* c = TB_NODE_GET_EXTRA(n);
    c->proto = NULL;
    c->projs[0] = cproj;
    c->projs[1] = mproj;
    c->projs[2] = dproj;
    return dproj;
}

TB_MultiOutput tb_inst_call(TB_Function* f, TB_FunctionPrototype* proto, TB_Node* target, size_t param_count, TB_Node** params) {
    size_t proj_count = 2 + (proto->return_count > 1 ? proto->return_count : 1);

    TB_Node* n = tb_alloc_node(f, TB_CALL, TB_TYPE_TUPLE, 3 + param_count, sizeof(TB_NodeCall) + (sizeof(TB_Node*)*proj_count));
    n->inputs[0] = f->active_control_node;
    n->inputs[2] = target;
    memcpy(n->inputs + 3, params, param_count * sizeof(TB_Node*));

    TB_NodeCall* c = TB_NODE_GET_EXTRA(n);
    c->proto = proto;

    // control proj
    TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, 0);

    // memory proj
    TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 1);
    n->inputs[1] = append_mem(f, mproj);

    // create data projections
    TB_PrototypeParam* rets = TB_PROTOTYPE_RETURNS(proto);
    FOREACH_N(i, 0, proto->return_count) {
        c->projs[i + 2] = tb__make_proj(f, rets[i].dt, n, i + 2);
    }

    // we'll slot a NULL so it's easy to tell when it's empty
    if (proto->return_count == 0) {
        c->projs[1] = NULL;
    }

    c->projs[0] = cproj;
    c->projs[1] = mproj;
    f->active_control_node = cproj;

    if (proto->return_count == 1) {
        return (TB_MultiOutput){ .count = proto->return_count, .single = c->projs[2] };
    } else {
        return (TB_MultiOutput){ .count = proto->return_count, .multiple = c->projs + 2 };
    }
}

void tb_inst_tailcall(TB_Function* f, TB_FunctionPrototype* proto, TB_Node* target, size_t param_count, TB_Node** params) {
    size_t proj_count = 2 + (proto->return_count > 1 ? proto->return_count : 1);

    TB_Node* n = tb_alloc_node(f, TB_CALL, TB_TYPE_TUPLE, 3 + param_count, sizeof(TB_NodeCall) + (sizeof(TB_Node*)*proj_count));
    n->inputs[0] = f->active_control_node;
    n->inputs[2] = target;
    memcpy(n->inputs + 3, params, param_count * sizeof(TB_Node*));

    TB_NodeCall* c = TB_NODE_GET_EXTRA(n);
    c->proto = proto;

    dyn_array_put(f->terminators, n);
    f->active_control_node = NULL;
}

TB_Node* tb_inst_not(TB_Function* f, TB_Node* src) {
    TB_Node* n = tb_alloc_node(f, TB_NOT, src->dt, 2, 0);
    n->inputs[1] = src;
    return n;
}

TB_Node* tb_inst_bswap(TB_Function* f, TB_Node* src) {
    TB_Node* n = tb_alloc_node(f, TB_BSWAP, src->dt, 2, 0);
    n->inputs[1] = src;
    return n;
}

TB_Node* tb_inst_clz(TB_Function* f, TB_Node* src) {
    assert(TB_IS_INTEGER_TYPE(src->dt));
    TB_Node* n = tb_alloc_node(f, TB_CLZ, TB_TYPE_I32, 2, 0);
    n->inputs[1] = src;
    return n;
}

TB_Node* tb_inst_ctz(TB_Function* f, TB_Node* src) {
    assert(TB_IS_INTEGER_TYPE(src->dt));
    TB_Node* n = tb_alloc_node(f, TB_CTZ, TB_TYPE_I32, 2, 0);
    n->inputs[1] = src;
    return n;
}

TB_Node* tb_inst_popcount(TB_Function* f, TB_Node* src) {
    assert(TB_IS_INTEGER_TYPE(src->dt));
    TB_Node* n = tb_alloc_node(f, TB_POPCNT, TB_TYPE_I32, 2, 0);
    n->inputs[1] = src;
    return n;
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
    }

    return tb_unary(f, TB_NEG, src->dt, src);
}

TB_Node* tb_inst_select(TB_Function* f, TB_Node* cond, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node(f, TB_SELECT, a->dt, 4, 0);
    n->inputs[1] = cond;
    n->inputs[2] = a;
    n->inputs[3] = b;
    return n;
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
    n->inputs[0] = f->active_control_node; // control edge
    n->inputs[2] = addr;
    if (src) n->inputs[3] = src;

    TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 0);
    TB_Node* dproj = tb__make_proj(f, dt, n, 1);
    TB_NODE_SET_EXTRA(n, TB_NodeAtomic, .order = order, .order2 = TB_MEM_ORDER_SEQ_CST, .proj0 = mproj, .proj1 = dproj);

    // memory proj
    n->inputs[1] = append_mem(f, mproj);
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
    n->inputs[0] = f->active_control_node; // control edge
    n->inputs[2] = addr;
    n->inputs[3] = expected;
    n->inputs[4] = desired;

    TB_Node* mproj = tb__make_proj(f, TB_TYPE_MEMORY, n, 0);
    TB_Node* dproj = tb__make_proj(f, dt, n, 1);
    TB_NODE_SET_EXTRA(n, TB_NodeAtomic, .order = succ, .order2 = fail, .proj0 = mproj, .proj1 = dproj);

    // memory proj
    n->inputs[1] = append_mem(f, mproj);
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
    n->inputs[1] = a;
    return n;
}

TB_Node* tb_inst_x86_ldmxcsr(TB_Function* f, TB_Node* a) {
    assert(a->dt.type == TB_INT && a->dt.data == 32);

    TB_Node* n = tb_alloc_node(f, TB_X86INTRIN_LDMXCSR, TB_TYPE_I32, 2, 0);
    n->inputs[1] = a;
    return n;
}

TB_Node* tb_inst_cycle_counter(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_CYCLE_COUNTER, TB_TYPE_I64, 1, 0);
    n->inputs[0] = f->active_control_node;
    return n;
}

TB_Node* tb_inst_x86_stmxcsr(TB_Function* f) {
    return tb_alloc_node(f, TB_X86INTRIN_STMXCSR, TB_TYPE_I32, 1, 0);
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
    n->inputs[1] = a;
    n->inputs[2] = b;
    TB_NODE_SET_EXTRA(n, TB_NodeCompare, .cmp_dt = a->dt);
    return n;
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
    n->inputs[0] = region;
    memset(n->inputs + 1, 0, preds * sizeof(TB_Node*));
    return n;
}

bool tb_inst_add_phi_operand(TB_Function* f, TB_Node* phi, TB_Node* region, TB_Node* val) {
    region = tb_get_parent_region(region);

    TB_Node* phi_region = phi->inputs[0];

    // the slot to fill is based on the predecessor list of the region
    FOREACH_N(i, 0, phi_region->input_count) {
        TB_Node* pred = phi_region->inputs[i];
        while (pred->type != TB_REGION && pred->type != TB_START) pred = pred->inputs[0];

        if (pred == region) {
            phi->inputs[i+1] = val;
            return true;
        }
    }

    return false;
}

TB_Node* tb_inst_phi2(TB_Function* f, TB_Node* region, TB_Node* a, TB_Node* b) {
    assert(TB_DATA_TYPE_EQUALS(a->dt, b->dt));

    TB_Node* n = tb_alloc_node(f, TB_PHI, a->dt, 3, 0);
    n->inputs[0] = region;
    n->inputs[1] = a;
    n->inputs[2] = b;
    return n;
}

TB_Node* tb_inst_region(TB_Function* f) {
    TB_Node* n = tb_alloc_node(f, TB_REGION, TB_TYPE_CONTROL, 0, sizeof(TB_NodeRegion));
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(n);

    TB_Node* phi = tb_alloc_node(f, TB_PHI, TB_TYPE_MEMORY, 1, 0);
    phi->inputs[0] = n;
    r->mem_in = r->mem_out = phi;
    return n;
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
    // detach old predecessor list, make bigger one
    assert(n->type == TB_REGION || n->type == TB_PHI);

    size_t old_count = n->input_count;
    TB_Node** new_inputs = alloc_from_node_arena(f, (old_count + 1) * sizeof(TB_Node*));
    if (n->inputs != NULL) {
        memcpy(new_inputs, n->inputs, old_count * sizeof(TB_Node*));
    }
    new_inputs[old_count] = in;

    n->inputs = new_inputs;
    n->input_count = old_count + 1;
}

static void add_memory_edge(TB_Function* f, TB_Node* n, TB_Node* mem_state, TB_Node* target) {
    assert(target->type == TB_REGION);
    TB_NodeRegion* r = TB_NODE_GET_EXTRA(target);
    assert(r->mem_in && r->mem_in->type == TB_PHI);
    add_input_late(f, r->mem_in, mem_state);
}

void tb_inst_goto(TB_Function* f, TB_Node* target) {
    TB_Node* mem_state = peek_mem(f, f->active_control_node);

    // there's no need for a branch if the path isn't diverging.
    TB_Node* n = f->active_control_node;
    dyn_array_put(f->terminators, n);
    f->active_control_node = NULL;

    // just add the edge directly.
    assert(n->dt.type == TB_CONTROL);
    add_input_late(f, target, n);
    add_memory_edge(f, n, mem_state, target);
}

void tb_inst_if(TB_Function* f, TB_Node* cond, TB_Node* if_true, TB_Node* if_false) {
    TB_Node* mem_state = peek_mem(f, f->active_control_node);

    // generate control projections
    TB_Node* n = tb_alloc_node(f, TB_BRANCH, TB_TYPE_TUPLE, 2, sizeof(TB_NodeBranch) + sizeof(int64_t));
    n->inputs[0] = f->active_control_node; // control edge
    n->inputs[1] = cond;

    FOREACH_N(i, 0, 2) {
        TB_Node* target = i ? if_false : if_true;

        TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, i);
        add_input_late(f, target, cproj);
        add_memory_edge(f, n, mem_state, target);
    }

    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
    br->succ_count = 2;
    br->keys[0] = 0;

    dyn_array_put(f->terminators, n);
    f->active_control_node = NULL;
}

void tb_inst_branch(TB_Function* f, TB_DataType dt, TB_Node* key, TB_Node* default_label, size_t entry_count, const TB_SwitchEntry* entries) {
    TB_Node* mem_state = peek_mem(f, f->active_control_node);

    // generate control projections
    TB_Node* n = tb_alloc_node(f, TB_BRANCH, TB_TYPE_TUPLE, 2, sizeof(TB_NodeBranch) + (sizeof(int64_t) * entry_count));
    n->inputs[0] = f->active_control_node; // control edge
    n->inputs[1] = key;

    FOREACH_N(i, 0, 1 + entry_count) {
        TB_Node* target = i ? entries[i - 1].value : default_label;

        TB_Node* cproj = tb__make_proj(f, TB_TYPE_CONTROL, n, i);
        add_input_late(f, target, cproj);
        add_memory_edge(f, n, mem_state, target);
    }

    TB_NodeBranch* br = TB_NODE_GET_EXTRA(n);
    br->succ_count = 1 + entry_count;
    FOREACH_N(i, 0, entry_count) {
        br->keys[i] = entries[i].key;
    }

    dyn_array_put(f->terminators, n);
    f->active_control_node = NULL;
}

void tb_inst_ret(TB_Function* f, size_t count, TB_Node** values) {
    TB_Node* mem_state = peek_mem(f, f->active_control_node);

    // allocate return node
    TB_Node* end = f->stop_node;
    if (end == NULL) {
        TB_Node* region = tb_alloc_node(f, TB_REGION, TB_TYPE_CONTROL, 0, sizeof(TB_NodeRegion));

        end = tb_alloc_node(f, TB_END, TB_TYPE_CONTROL, 3 + count, 0);
        end->inputs[0] = region;
        end->inputs[2] = f->params[2];

        if (f->exit_attrib.loc.file != NULL) {
            append_attrib(f, end, f->exit_attrib);
        }

        TB_Node* mem_phi = tb_alloc_node(f, TB_PHI, TB_TYPE_MEMORY, 2, 0);
        mem_phi->inputs[0] = region;
        mem_phi->inputs[1] = mem_state;
        end->inputs[1] = mem_phi;

        FOREACH_N(i, 0, count) {
            TB_Node* phi = tb_alloc_node(f, TB_PHI, values[i]->dt, 2, 0);
            phi->inputs[0] = region;
            phi->inputs[1] = values[i];

            // add phi to STOP
            end->inputs[3 + i] = phi;
        }

        f->stop_node = end;
        TB_NODE_SET_EXTRA(region, TB_NodeRegion, .mem_in = mem_phi, .mem_out = mem_phi, .tag = "ret");

        dyn_array_put(f->terminators, end);
    } else {
        // add to PHIs
        assert(end->input_count >= 3 + count);
        add_input_late(f, end->inputs[1], mem_state);

        size_t i = 3;
        for (; i < count + 3; i++) {
            assert(end->inputs[i]->dt.raw == values[i - 3]->dt.raw && "datatype mismatch");
            add_input_late(f, end->inputs[i], values[i - 3]);
        }

        size_t phi_count = end->input_count;
        for (; i < phi_count; i++) {
            // put poison in the leftovers?
            log_warn("%s: ir: generated poison due to inconsistent number of returned values", f->super.name);

            TB_Node* poison = tb_alloc_node(f, TB_POISON, end->inputs[i]->dt, 1, 0);
            add_input_late(f, end->inputs[i], poison);
        }
    }

    // basically just tb_inst_goto without the memory PHI (we did it earlier)
    TB_Node* n = f->active_control_node;
    f->active_control_node = NULL;
    add_input_late(f, end->inputs[0], n);
}
