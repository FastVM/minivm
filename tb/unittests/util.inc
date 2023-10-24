#ifndef TB_TEST_UTIL_INC
#define TB_TEST_UTIL_INC

#if defined(__GCC__) || defined(__clang__)
#  pragma GCC diagnostic ignored "-Wunknown-pragmas"
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "../include/tb.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define TB_TEST_IS_WINDOWS_ 1
#  define WEXITSTATUS(x) x
#  define EXEC_EXTENSION ".exe"

static TB_System         tb_test_system   = TB_SYSTEM_WINDOWS;
static TB_ExecutableType tb_test_exe_type = TB_EXECUTABLE_PE;
#else
#  define TB_TEST_IS_WINDOWS_ 0
#  define EXEC_EXTENSION ""

#  include <sys/wait.h>

static TB_System         tb_test_system   = TB_SYSTEM_LINUX;
static TB_ExecutableType tb_test_exe_type = TB_EXECUTABLE_ELF;
#endif

static TB_Arch       tb_test_arch        = TB_ARCH_X86_64;
static TB_FeatureSet tb_test_feature_set = { .x64 = 0 };

static void tb_test_link_library(TB_Linker  *linker,
                                 char const *name) {
#if TB_TEST_IS_WINDOWS_
  //  FIXME
  //  Find and link library on Windows.
  //

  char full_path[200] = "W:\\MSVC\\Windows Kits\\10\\Lib\\10.0.22621.0"
                        "\\um\\x64\\";

  ptrdiff_t folder_len = strlen(full_path);
  ptrdiff_t name_len   = strlen(name);

  assert(folder_len + name_len + 1 < sizeof full_path);
  memcpy(full_path + folder_len, name, name_len);
  full_path[folder_len + name_len] = '\0';

  FILE *f = fopen(full_path, "rb");

  assert(f != NULL);
  if (f == NULL)
    return;

  ptrdiff_t chunk_size = 100000;
  ptrdiff_t data_size  = 0;

  uint8_t *data = NULL;
  uint8_t *p    = data;

  while (!feof(f)) {
    data = realloc(data, data_size + chunk_size);
    assert(data != NULL);
    if (data == NULL)
      return;

    ptrdiff_t n = fread(data + data_size, 1, chunk_size, f);
    if (n <= 0)
      break;
    data_size += n;
  }

  fclose(f);

  TB_Slice sl_name = { .length = name_len,
                       .data   = (uint8_t const *) name };
  TB_Slice sl_data = { .length = data_size, .data = data };
  tb_linker_append_library(linker, sl_name, sl_data);

  free(data);
#endif

  //  NOTE
  //  We don't need to link libraries for unit-testing on Linux yet.
  //
}

#define ERROR(x)                                                   \
  do {                                                             \
    printf("Error in %s (\"%s\" line %d): " #x "\n", __FUNCTION__, \
           __FILE__, (int) __LINE__);                              \
    status = 0;                                                    \
    goto _final;                                                   \
  } while (0)

#if TB_TEST_IS_WINDOWS_

#  define EXIT_WITH_(node_status_)                                    \
    do {                                                              \
      TB_PrototypeParam     param0         = { .dt   = TB_TYPE_I32,   \
                                               .name = "uExitCode" }; \
      TB_FunctionPrototype *fp_ExitProcess = tb_prototype_create(     \
          module, TB_STDCALL, 1, &param0, 0, NULL, 0);                \
                                                                      \
      TB_Node *addr_ExitProcess = tb_inst_get_symbol_address(         \
          f_main,                                                     \
          (TB_Symbol *) tb_extern_create(module, -1, "ExitProcess",   \
                                         TB_EXTERNAL_SO_LOCAL));      \
                                                                      \
      tb_inst_call(f_main, fp_ExitProcess, addr_ExitProcess, 1,       \
                   &(node_status_));                                  \
      tb_inst_ret(f_main, 0, NULL);                                   \
    } while (0)

#else

#  define EXIT_WITH_(node_status_)                           \
    do {                                                     \
      TB_Node *num_ = tb_inst_sint(f_main, TB_TYPE_I32, 60); \
      tb_inst_syscall(f_main, TB_TYPE_I64, num_, 1,          \
                      &(node_status_));                      \
      tb_inst_ret(f_main, 0, NULL);                          \
    } while (0)

#endif

#define TB_TEST_MODULE_BEGIN_                                 \
  int status = 1;                                             \
  int ret    = 0;                                             \
                                                              \
  TB_Module *module = NULL;                                   \
  TB_Linker *linker = NULL;                                   \
                                                              \
  module = tb_module_create(tb_test_arch, tb_test_system,     \
                            &tb_test_feature_set, 0);         \
                                                              \
  if (module == NULL)                                         \
    ERROR("tb_module_create failed.");                        \
                                                              \
  TB_FunctionPrototype *fp_main = tb_prototype_create(        \
      module, TB_CDECL, 0, NULL, 0, NULL, false);             \
                                                              \
  TB_Function *f_main = tb_function_create(                   \
      module, -1, "main", TB_LINKAGE_PUBLIC, TB_COMDAT_NONE); \
                                                              \
  tb_function_set_prototype(f_main, fp_main, NULL);           \
                                                              \
  if (f_main == NULL)                                         \
    ERROR("tb_function_create failed.");

#define TB_TEST_MODULE_END_(name_, result_, print_asm_)          \
  {                                                              \
    TB_Passes *passes = tb_pass_enter(f_main, NULL);             \
                                                                 \
    if (passes == NULL)                                          \
      ERROR("tb_pass_enter failed.");                            \
                                                                 \
    TB_FunctionOutput *asm_out = tb_pass_codegen(passes, 1);     \
                                                                 \
    if ((print_asm_) && asm_out != NULL) {                       \
      printf("\n");                                              \
      tb_output_print_asm(asm_out, stdout);                      \
    }                                                            \
                                                                 \
    tb_pass_exit(passes);                                        \
  }                                                              \
                                                                 \
  linker = tb_linker_create(tb_test_exe_type, tb_test_arch);     \
                                                                 \
  if (linker == NULL)                                            \
    ERROR("tb_linker_create failed.");                           \
                                                                 \
  tb_linker_append_module(linker, module);                       \
                                                                 \
  tb_linker_set_entrypoint(linker, "main");                      \
                                                                 \
  if (TB_TEST_IS_WINDOWS_)                                       \
    tb_test_link_library(linker, "kernel32.lib");                \
                                                                 \
  TB_ExportBuffer buf = tb_linker_export(linker);                \
  tb_export_buffer_to_file(buf, "bin/" #name_ EXEC_EXTENSION);   \
  tb_export_buffer_free(buf);                                    \
                                                                 \
  if (!TB_TEST_IS_WINDOWS_) {                                    \
    (void) system("chmod a+x bin/" #name_);                      \
    ret = WEXITSTATUS(system("./bin/" #name_));                  \
  } else                                                         \
    ret = WEXITSTATUS(system("start bin\\" #name_));             \
                                                                 \
  if (ret != (result_)) {                                        \
    printf("Got %d, expected %d\n", (int) ret, (int) (result_)); \
    status = 0;                                                  \
  }                                                              \
                                                                 \
_final:                                                          \
  if (module != NULL)                                            \
    tb_module_destroy(module);                                   \
  if (linker != NULL)                                            \
    tb_linker_destroy(linker);

#endif
