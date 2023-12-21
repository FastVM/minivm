#include <dyn_array.h>

typedef struct {
    TB_Module* m;
    TB_JIT* jit;
} RunTest;

RunTest create_jit() {
    TB_FeatureSet features = { 0 };
    TB_Module* mod = tb_module_create_for_host(&features, true);
    TB_JIT* jit = tb_jit_begin(mod, 0);

    return (RunTest){ mod, jit };
}

#include "tb_test_regressions.inc"
#include "tb_test_exit_status.inc"
#include "tb_test_int_arith.inc"

typedef int (*TestFn)(void);

static int failed = 0, total = 0;

static void run_test(const char* name, TestFn fn) {
    size_t len = strlen(name);

    fflush(stdout);
    printf("%s\r", #proc_);
    fflush(stdout);
    int status_ = test_##proc_();
    fflush(stdout);

    printf("%s%.*s ", #proc_, (int) (41 - sizeof(#proc_)),
        " ........................................"
    );

    if (status_) {
        printf("OK\n");
    } else {
        printf("FAILED\n");
        failed++;
    }
    total++;
    fflush(stdout);

}

int main(int argc, char **argv) {
    TEST(regression_module_arena);
    TEST(regression_link_global);
    TEST(exit_status);

    TEST(i8_add);
    TEST(i8_sub);
    TEST(i8_mul);
    TEST(i8_div);
    TEST(i8_mod);

    TEST(i16_add);
    TEST(i16_sub);
    TEST(i16_mul);
    TEST(i16_div);
    TEST(i16_mod);

    TEST(i32_add);
    TEST(i32_sub);
    TEST(i32_mul);
    TEST(i32_div);
    TEST(i32_mod);

    TEST(i64_add);
    TEST(i64_sub);
    TEST(i64_mul);
    TEST(i64_div);
    TEST(i64_mod);

    TEST(u8_add);
    TEST(u8_sub);
    TEST(u8_mul);
    TEST(u8_div);
    TEST(u8_mod);

    TEST(u16_add);
    TEST(u16_sub);
    TEST(u16_mul);
    TEST(u16_div);
    TEST(u16_mod);

    TEST(u32_add);
    TEST(u32_sub);
    TEST(u32_mul);
    TEST(u32_div);
    TEST(u32_mod);

    TEST(u64_add);
    TEST(u64_sub);
    TEST(u64_mul);
    TEST(u64_div);
    TEST(u64_mod);

    fflush(stdout);
    if (failed > 0) {
        printf("\n%d of %d tests failed.\n", failed, total);
    } else {
        printf("\nAll %d tests succeeded.\n", total);
    }
    fflush(stdout);

    return failed;
}
