#include "util.inc"

#define TEST_INT_ARITH_(prefix_, type_, inst_type_, inst_op_, arg0_, \
                        arg1_, res_)                                 \
  static int test_##prefix_##_##inst_op_(void) {                     \
    TB_TEST_MODULE_BEGIN_;                                           \
                                                                     \
    TB_Node *foo = tb_inst_##inst_type_(f_main, TB_TYPE_##type_,     \
                                        (arg0_));                    \
    TB_Node *bar = tb_inst_##inst_type_(f_main, TB_TYPE_##type_,     \
                                        (arg1_));                    \
    TB_Node *sum = tb_inst_##inst_op_(f_main, foo, bar,              \
                                      TB_ARITHMATIC_NONE);           \
                                                                     \
    EXIT_WITH_(sum);                                                 \
                                                                     \
    TB_TEST_MODULE_END_(test_##prefix_##_##inst_op_, (res_), 0);     \
    return status;                                                   \
  }

TEST_INT_ARITH_(i8, I8, sint, add, 50, -8, 42)
TEST_INT_ARITH_(i8, I8, sint, sub, 20, -10, 30)
TEST_INT_ARITH_(i8, I8, sint, mul, 7, 9, 63)
TEST_INT_ARITH_(i8, I8, sint, div, 100, 11, 9)
TEST_INT_ARITH_(i8, I8, sint, mod, 100, 11, 1)

TEST_INT_ARITH_(i16, I16, sint, add, 300, -240, 60)
TEST_INT_ARITH_(i16, I16, sint, sub, 1000, 934, 66)
TEST_INT_ARITH_(i16, I16, sint, mul, 9, 8, 72)
TEST_INT_ARITH_(i16, I16, sint, div, 999, 112, 8)
TEST_INT_ARITH_(i16, I16, sint, mod, 999, 112, 103)

TEST_INT_ARITH_(i32, I32, sint, add, 300, -240, 60)
TEST_INT_ARITH_(i32, I32, sint, sub, 1000, 934, 66)
TEST_INT_ARITH_(i32, I32, sint, mul, 9, 8, 72)
TEST_INT_ARITH_(i32, I32, sint, div, 999, 112, 8)
TEST_INT_ARITH_(i32, I32, sint, mod, 999, 112, 103)

TEST_INT_ARITH_(i64, I64, sint, add, 300, -240, 60)
TEST_INT_ARITH_(i64, I64, sint, sub, 1000, 934, 66)
TEST_INT_ARITH_(i64, I64, sint, mul, 9, 8, 72)
TEST_INT_ARITH_(i64, I64, sint, div, 999, 112, 8)
TEST_INT_ARITH_(i64, I64, sint, mod, 999, 112, 103)

TEST_INT_ARITH_(u8, I8, uint, add, 50, 8, 58)
TEST_INT_ARITH_(u8, I8, uint, sub, 30, 10, 20)
TEST_INT_ARITH_(u8, I8, uint, mul, 7, 9, 63)
TEST_INT_ARITH_(u8, I8, uint, div, 100, 11, 9)
TEST_INT_ARITH_(u8, I8, uint, mod, 100, 11, 1)

TEST_INT_ARITH_(u16, I16, uint, add, 30, 50, 80)
TEST_INT_ARITH_(u16, I16, uint, sub, 1000, 934, 66)
TEST_INT_ARITH_(u16, I16, uint, mul, 9, 8, 72)
TEST_INT_ARITH_(u16, I16, uint, div, 999, 112, 8)
TEST_INT_ARITH_(u16, I16, uint, mod, 999, 112, 103)

TEST_INT_ARITH_(u32, I32, uint, add, 50, 40, 90)
TEST_INT_ARITH_(u32, I32, uint, sub, 1000, 934, 66)
TEST_INT_ARITH_(u32, I32, uint, mul, 9, 8, 72)
TEST_INT_ARITH_(u32, I32, uint, div, 999, 112, 8)
TEST_INT_ARITH_(u32, I32, uint, mod, 999, 112, 103)

TEST_INT_ARITH_(u64, I64, uint, add, 20, 25, 45)
TEST_INT_ARITH_(u64, I64, uint, sub, 1000, 934, 66)
TEST_INT_ARITH_(u64, I64, uint, mul, 9, 8, 72)
TEST_INT_ARITH_(u64, I64, uint, div, 999, 112, 8)
TEST_INT_ARITH_(u64, I64, uint, mod, 999, 112, 103)
