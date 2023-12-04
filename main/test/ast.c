#include "../../vm/ast/build.h"
#include "../../vm/ast/comp.h"
#include "../../vm/ast/print.h"
#include "../../vm/be/tb.h"
#include "../../vm/ir.h"
#include "../../vm/std/libs/io.h"
#include "../../vm/std/std.h"

typedef vm_ast_node_t (*vm_test_func_t)(void);

static vm_ast_node_t vm_main_test_math_add(void) {
    vm_ast_node_t ten = vm_ast_build_literal(i64, 10);
    vm_ast_node_t twenty = vm_ast_build_literal(i64, 20);
    vm_ast_node_t add = vm_ast_build_add(ten, twenty);
    return add;
}

static vm_ast_node_t vm_main_test_math_sub(void) {
    vm_ast_node_t ten = vm_ast_build_literal(i64, 10);
    vm_ast_node_t twenty = vm_ast_build_literal(i64, 5);
    vm_ast_node_t add = vm_ast_build_sub(ten, twenty);
    return add;
}

static vm_ast_node_t vm_main_test_math_mul(void) {
    vm_ast_node_t ten = vm_ast_build_literal(i64, 49);
    vm_ast_node_t twenty = vm_ast_build_literal(i64, 84);
    vm_ast_node_t add = vm_ast_build_mul(ten, twenty);
    return add;
}

static vm_ast_node_t vm_main_test_math_div(void) {
    vm_ast_node_t ten = vm_ast_build_literal(i64, 18);
    vm_ast_node_t twenty = vm_ast_build_literal(i64, 3);
    vm_ast_node_t add = vm_ast_build_div(ten, twenty);
    return add;
}

static vm_ast_node_t vm_main_test_math_mod(void) {
    vm_ast_node_t ten = vm_ast_build_literal(i64, 10);
    vm_ast_node_t twenty = vm_ast_build_literal(i64, 20);
    vm_ast_node_t add = vm_ast_build_mod(ten, twenty);
    return add;
}

static vm_ast_node_t vm_main_test_math_chain(void) {
    return vm_ast_build_add(
        vm_ast_build_mul(
            vm_ast_build_literal(i64, 1),
            vm_ast_build_literal(i64, 2)
        ),
        vm_ast_build_mul(
            vm_ast_build_literal(i64, 3),
            vm_ast_build_literal(i64, 4)
        )
    );
}

static vm_ast_node_t vm_main_test_huge_fib_rec(size_t n) {
    if (n < 2) {
        return vm_ast_build_literal(i64, n);
    } else {
        return vm_ast_build_add(
            vm_main_test_huge_fib_rec(n - 2),
            vm_main_test_huge_fib_rec(n - 1)
        );
    }
}

static vm_ast_node_t vm_main_test_huge_fib(void) {
    return vm_main_test_huge_fib_rec(15);
}

static vm_ast_node_t vm_main_test_math_fac(void) {
    vm_ast_node_t value = vm_ast_build_literal(i64, 1);
    for (size_t i = 2; i < 10; i++) {
        value = vm_ast_build_mul(vm_ast_build_literal(i64, i), value);
    }
    return value;
}

static vm_ast_node_t vm_main_test_table_env(void) {
    return vm_ast_build_env();
}

static vm_ast_node_t vm_main_test_table_load_basic(void) {
    return vm_ast_build_load(
        vm_ast_build_env(),
        vm_ast_build_literal(str, "print")
    );
}

static vm_ast_node_t vm_main_test_if_const_false(void) {
    vm_ast_node_t cond = vm_ast_build_literal(i64, 0);
    vm_ast_node_t iftrue = vm_ast_build_literal(str, "zero is true");
    vm_ast_node_t iffalse = vm_ast_build_literal(str, "zero is false");
    return vm_ast_build_if(cond, iftrue, iffalse);
}

static vm_ast_node_t vm_main_test_if_const_true(void) {
    vm_ast_node_t cond = vm_ast_build_literal(i64, 1);
    vm_ast_node_t iftrue = vm_ast_build_literal(str, "one is true");
    vm_ast_node_t iffalse = vm_ast_build_literal(str, "one is false");
    return vm_ast_build_if(cond, iftrue, iffalse);
}

static vm_ast_node_t vm_main_test_count_loop(void) {
    vm_ast_node_t n = vm_ast_build_ident("n");
    vm_ast_node_t i = vm_ast_build_ident("i");
    return vm_ast_build_block(
        4,
        vm_ast_build_set(
            n,
            vm_ast_build_literal(i64, 0)
        ),
        vm_ast_build_set(
            i,
            vm_ast_build_literal(i64, 0)
        ),
        vm_ast_build_while(
            vm_ast_build_lt(i, vm_ast_build_literal(i64, 1000000)),
            vm_ast_build_block(
                2,
                vm_ast_build_set(
                    i,
                    vm_ast_build_add(
                        i,
                        vm_ast_build_literal(i64, 1)
                    )
                ),
                vm_ast_build_set(
                    n,
                    vm_ast_build_add(
                        n,
                        i
                    )
                )
            )
        ),
        vm_ast_build_return(n)
    );
}

void vm_test(vm_config_t *config, const char *name, vm_test_func_t gen) {
    printf("\ntest: %s\n", name);
    clock_t p1 = clock();

    vm_ast_node_t node = vm_ast_build_return(gen());

    if (config->dump_ast) {
        printf("\n--- ast ---\n");
        vm_ast_print_node(stdout, 0, "", node);
    }

    clock_t p2 = clock();

    vm_ast_blocks_t blocks = vm_ast_comp(node);
    
    clock_t p3 = clock();

    if (config->dump_ir) {
        vm_print_blocks(stdout, blocks.len, blocks.blocks);
    }

    vm_std_value_t value = vm_tb_run_main(config, blocks.entry, blocks.len, blocks.blocks, vm_std_new());
    
    clock_t p4 = clock();

    vm_io_debug(stdout, 0, "result: ", value, NULL);
    if (config->dump_time) {
        double diff1 = (double)(p2 - p1) / CLOCKS_PER_SEC * 1000;
        double diff2 = (double)(p3 - p2) / CLOCKS_PER_SEC * 1000;
        double diff3 = (double)(p4 - p3) / CLOCKS_PER_SEC * 1000;
        printf("took: %.3fms / %.3fms / %.3fms\n", diff1, diff2, diff3);
    }
}

#define vm_test(...) vm_test(config, __VA_ARGS__)

int main(int argc, char **argv) {
    vm_init_mem();
    vm_config_t val_config = (vm_config_t) {
        .use_tb_opt = true,
        .dump_src = false,
        .dump_ast = false,
        .dump_ir = false,
        .dump_ver = false,
        .dump_tb = false,
        .dump_tb_opt = false,
        .dump_x86 = false,
        .dump_args = false,
        .dump_time = true,
    };
    vm_config_t *config = &val_config;
    for (size_t i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (!strcmp(arg, "--")) {
            break;
        }
        if (!strncmp(arg, "--dump-", 7)) {
            arg += 7;
            if (!strcmp(arg, "src")) {
                config->dump_src = true;
            }
            if (!strcmp(arg, "ast")) {
                config->dump_ast = true;
            }
            if (!strcmp(arg, "ir")) {
                config->dump_ir = true;
            }
            if (!strcmp(arg, "ver")) {
                config->dump_ver = true;
            }
            if (!strcmp(arg, "tb")) {
                config->dump_tb = true;
            }
            if (!strcmp(arg, "opt")) {
                config->dump_tb_opt = true;
            }
            if (!strcmp(arg, "x86")) {
                config->dump_x86 = true;
            }
            if (!strcmp(arg, "args")) {
                config->dump_args = true;
            }
        }
    }
    vm_test(
        "math.add",
        vm_main_test_math_add
    );
    vm_test(
        "math.sub",
        vm_main_test_math_sub
    );
    vm_test(
        "math.mul",
        vm_main_test_math_mul
    );
    vm_test(
        "math.div",
        vm_main_test_math_div
    );
    vm_test(
        "math.mod",
        vm_main_test_math_mod
    );
    vm_test(
        "math.chain",
        vm_main_test_math_chain
    );
    vm_test(
        "math.fac",
        vm_main_test_math_fac
    );
    vm_test(
        "huge.fib",
        vm_main_test_huge_fib
    );
    // vm_test(
    //     "table.env",
    //     vm_main_test_table_env
    // );
    vm_test(
        "table.load.basic",
        vm_main_test_table_load_basic
    );
    vm_test(
        "if.const.false",
        vm_main_test_if_const_false
    );
    vm_test(
        "if.const.true",
        vm_main_test_if_const_true
    );
    vm_test(
        "huge.while",
        vm_main_test_count_loop
    );
    return 0;
}
