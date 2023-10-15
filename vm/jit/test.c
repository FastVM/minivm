
#include "../lib.h"
#include "../../tb/include/tb.h"

typedef double __attribute__((cdecl)) callable_t(double, double);

int main() {
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

    TB_Node *a0 = tb_inst_param(function, 0);
    TB_Node *a1 = tb_inst_param(function, 1);

    TB_Node *add_result = tb_inst_fadd(
        function,
        a0,
        a1
    );
    
    tb_inst_ret(function, 1, &add_result);

    TB_Passes *passes = tb_pass_enter(function, tb_function_get_arena(function));
    tb_pass_print(passes);

    TB_FunctionOutput *out = tb_pass_codegen(passes, true);
    tb_output_print_asm(out, stdout);

    tb_pass_exit(passes);

    TB_JIT *jit = tb_jit_begin(module, 1 << 16);
    callable_t *the_func1 = (callable_t *)  tb_jit_place_function(jit, function);

    callable_t *the_func2 = (callable_t *) tb_jit_get_code_ptr(function);

    printf("%p %p\n", the_func1, the_func2);

    double x = 12.3;
    double y = 23.4;
    
    double res = the_func2(x, y);

    printf("%f = %f + %f\n", res, x, y);
    tb_jit_end(jit);

    return 0;
}