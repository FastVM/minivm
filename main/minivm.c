#include "../vm/ast/build.h"
#include "../vm/ast/comp.h"
#include "../vm/ast/print.h"
#include "../vm/be/tb.h"
#include "../vm/ir.h"
#include "../vm/std/libs/io.h"
#include "../vm/std/std.h"

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

void vm_test(const char *name, vm_test_func_t gen) {
    printf("\ntest: %s\n", name);
    clock_t p1 = clock();

    vm_ast_node_t node0 = gen();

    vm_ast_node_t node = vm_ast_build_return(node0);

    clock_t p2 = clock();

    vm_ast_blocks_t blocks = vm_ast_comp(node);

    vm_print_blocks(stdout, blocks.len, blocks.blocks);

    clock_t p3 = clock();

    vm_std_value_t value = vm_tb_run(blocks.blocks[0], vm_std_new());
    clock_t p4 = clock();

    vm_io_debug(stdout, 0, "result: ", value, NULL);
    double diff1 = (double)(p2 - p1) / CLOCKS_PER_SEC * 1000;
    double diff2 = (double)(p3 - p2) / CLOCKS_PER_SEC * 1000;
    double diff3 = (double)(p4 - p3) / CLOCKS_PER_SEC * 1000;
    printf("took: %.3fms / %.3fms / %.3fms\n", diff1, diff2, diff3);
}

int main(int argc, char **argv) {
    vm_init_mem();
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
}
