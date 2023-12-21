// Just some code generation tests for TB
#include <tb.h>

int main() {
    TB_FeatureSet features = { 0 };
    TB_Module* mod = tb_module_create_for_host(&features, true);

    TB_PrototypeParam ret = { TB_TYPE_I32 };
    TB_FunctionPrototype* proto = tb_prototype_create(mod, TB_CDECL, 1, &ret, 1, &ret, false);

    TB_Function* f = tb_function_create(mod, -1, "numbers", TB_LINKAGE_PUBLIC);
    tb_function_set_prototype(f, tb_module_get_text(mod), proto, NULL);

    TB_Node* v = tb_inst_uint(f, TB_TYPE_I32, 42);
    v = tb_inst_mul(f, v, tb_inst_param(f, 0), 0);

    tb_inst_ret(f, 1, &v);

    TB_Passes* passes = tb_pass_enter(f, NULL);
    TB_FunctionOutput* asm_out = tb_pass_codegen(passes, true);
    if (asm_out) {
        printf("\n");
        tb_output_print_asm(asm_out, stdout);
    }
    tb_pass_exit(passes);
    tb_module_destroy(mod);
    return 0;
}
