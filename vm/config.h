
#if !defined(VM_HEADER_CONFIG)
#define VM_HEADER_CONFIG

#define VM_USE_NUM i64

#include "lib.h"

#define VM_USE_LEAKS 1

struct vm_config_t;
typedef struct vm_config_t vm_config_t;

struct vm_config_t {
    bool use_tb_opt: 1;
    
    bool dump_src: 1;
    bool dump_ast: 1;
    bool dump_ir: 1;
    bool dump_ver: 1;
    bool dump_tb: 1;
    bool dump_tb_opt: 1;
    bool dump_x86: 1;
    bool dump_args: 1;
    bool dump_time: 1;
};

#endif
