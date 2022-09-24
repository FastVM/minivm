
#if !defined(VM_HEADER_CONFIG)
#define VM_HEADER_CONFIG

#if !defined(VM_CONFIG_NUM_FRAMES)
#define VM_CONFIG_NUM_FRAMES 1000
#endif

#if !defined(VM_CONFIG_NUM_REGS)
#define VM_CONFIG_NUM_REGS (VM_CONFIG_NUM_FRAMES * 16)
#endif

#if !defined(VM_CONFIG_GROW_STACK)
#define VM_CONFIG_GROW_STACK (1)
#endif
#endif

#if !defined(VM_INT_DEBUG_OPCODE)
#define VM_INT_DEBUG_OPCODE 0
#endif

#if !defined(VM_INT_DEBUG_LOAD)
#define VM_INT_DEBUG_LOAD 0
#endif

#if !defined(VM_TABLE_OPT)
#define VM_TABLE_OPT 1
#endif
