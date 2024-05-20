
#if !defined(VM_HEADER_CONFIG)
#define VM_HEADER_CONFIG

#include <stdbool.h>
#include <stddef.h>

#define VM_USE_DUMP 1
#define VM_FORMAT_FLOAT "%.14g"

struct vm_externs_t;
typedef struct vm_externs_t vm_externs_t;

struct vm_config_t;
typedef struct vm_config_t vm_config_t;

enum {
    VM_USE_NUM_I8,
    VM_USE_NUM_I16,
    VM_USE_NUM_I32,
    VM_USE_NUM_I64,
    VM_USE_NUM_F32,
    VM_USE_NUM_F64,
};

enum {
    VM_TARGET_TB,
#if defined(EMSCRIPTEN)
    VM_TARGET_TB_EMCC,
#else
#if defined(VM_USE_TCC)
    VM_TARGET_TB_TCC,
#endif
    VM_TARGET_TB_CC,
    VM_TARGET_TB_GCC,
    VM_TARGET_TB_CLANG,
#endif
};

enum {
    VM_USE_VERSION_COUNT_NONE,
    VM_USE_VERSION_COUNT_GLOBAL,
    VM_USE_VERSION_COUNT_FINE,
};

struct vm_externs_t {
    size_t id;
    void *value;
    vm_externs_t *last;
};

struct vm_config_t {
    vm_externs_t *externs;

    const char *cflags;

    unsigned int target : 4;
    unsigned int use_num : 3;
    unsigned int use_ver_count : 2;

    bool dump_src : 1;
    bool dump_ast : 1;
    bool dump_ir : 1;
    bool dump_ver : 1;
    bool dump_tb : 1;
    bool dump_tb_opt : 1;
    bool dump_asm : 1;
    bool dump_c : 1;
    bool dump_time : 1;

    bool is_repl : 1;

    bool tb_opt : 1;
    bool tb_recompile : 1;
    bool tb_regs_cast : 1;
    bool tb_regs_node : 1;
    bool tb_force_bitcast : 1;
    bool tb_tailcalls : 1;
    bool tb_lbbv : 1;
};

#endif
