

#include "../obj.h"
#include "../std/std.h"
#include "../type.h"
#include "../../tb/include/tb.h"

typedef double callable_t(double, double);

vm_std_value_t vm_x64_run(vm_block_t *block, vm_table_t *std, vm_std_value_t *args) {
    TB_FeatureSet features = (TB_FeatureSet) {0};
    TB_Module* module = tb_module_create_for_host(&features, true);
    TB_Function* function = tb_function_create( module, -1, "test", TB_LINKAGE_PUBLIC);
    TB_PrototypeParam proto_params[2] = {
        { TB_TYPE_F64 },
        { TB_TYPE_F64 },
    };
    TB_PrototypeParam proto_return[1] = {
        { TB_TYPE_F64 },
    };
    TB_FunctionPrototype *proto = tb_prototype_create(module, TB_CDECL, 2, proto_params, 1, proto_return, false);
    tb_function_set_prototype(
        function,
        -1, proto,
        NULL
    );

    // TB_Node *a0 = tb_inst_local(function, sizeof(double), _Alignof(double));
    // TB_Node *a1 = tb_inst_local(function, sizeof(double), _Alignof(double));

    // tb_inst_store(function, TB_TYPE_F64, a0, tb_inst_param(function, 0), _Alignof(double), false);
    // tb_inst_store(function, TB_TYPE_F64, a1, tb_inst_param(function, 1), _Alignof(double), false);

    TB_Node *a0 = tb_inst_param(function, 0);
    TB_Node *a1 = tb_inst_param(function, 1);

    TB_Node *add_result = tb_inst_add(
        function,
        a0,
        a1,
        0
    );
    
    tb_inst_ret(function, 1, &add_result);

    TB_Passes *passes = tb_pass_enter(function, tb_function_get_arena(function));
    TB_FunctionOutput *out = tb_pass_codegen(passes, true);
    tb_output_print_asm(out, stdout);

    TB_JIT *jit = tb_jit_begin(module, 0);
    callable_t *the_func = (callable_t *) tb_jit_place_function(jit, function);

    printf("%f\n", the_func(10, 20));
    tb_jit_end(jit);

    return (vm_std_value_t) {
        .tag = VM_TAG_NIL,
    };
}
