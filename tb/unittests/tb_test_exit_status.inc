#include "util.inc"

static int test_exit_status(void) {
  TB_TEST_MODULE_BEGIN_;

  TB_Node *exit_value = tb_inst_sint(f_main, TB_TYPE_I32, 42);
  EXIT_WITH_(exit_value);

  TB_TEST_MODULE_END_(test_exit_status, 42, 1);
  return status;
}
