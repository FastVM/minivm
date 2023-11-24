
#if !defined(VM_HEADER_CONFIG)
#define VM_HEADER_CONFIG

#define VM_USE_NUM i64

#include "lib.h"

#define VM_USE_LEAKS_NOGC 0
#define VM_USE_LEAKS_TGC 1
#define VM_USE_LEAKS_BDWGC 2

#define VM_USE_LEAKS VM_USE_LEAKS_NOGC
#define VM_USE_DUMP 1

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

struct vm_config_t {
    uint8_t use_num: 3;
    bool use_tb_opt: 1;
    bool use_tailcall: 1;
    
    bool dump_src: 1;
    bool dump_ast: 1;
    bool dump_ir: 1;
    bool dump_ver: 1;
    bool dump_tb: 1;
    bool dump_tb_opt: 1;
    bool dump_tb_dot: 1;
    bool dump_tb_opt_dot: 1;
    bool dump_x86: 1;
    bool dump_args: 1;
    bool dump_time: 1;
};

#endif
