
#if !defined(VM_HEADER_CONFIG)
#define VM_HEADER_CONFIG

#define VM_USE_DUMP 1
#define VM_NO_TAILCALL 1

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
    unsigned int use_num: 3;
    _Bool use_tb_opt: 1;
    
    _Bool dump_src: 1;
    _Bool dump_ast: 1;
    _Bool dump_ir: 1;
    _Bool dump_ver: 1;
    _Bool dump_tb: 1;
    _Bool dump_tb_opt: 1;
    _Bool dump_tb_dot: 1;
    _Bool dump_tb_opt_dot: 1;
    _Bool dump_x86: 1;
    _Bool dump_args: 1;
    _Bool dump_time: 1;

    _Bool is_repl: 1;
};

#endif
