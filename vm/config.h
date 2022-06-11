
#if !defined(VM_HEADER_CONFIG)
#define VM_HEADER_CONFIG

#if !defined(VM_GC_MIN)
#define VM_GC_MIN (1 << 12)
#endif

#if !defined(VM_CONFIG_NUM_FRAMES)
#define VM_CONFIG_NUM_FRAMES 1000
#endif

#if !defined(VM_CONFIG_NUM_REGS)
#define VM_CONFIG_NUM_REGS (VM_CONFIG_NUM_FRAMES * 16)
#endif

#if !defined(VM_CONFIG_GROW_STACK)
#define VM_CONFIG_GROW_STACK (0)
#endif
#endif
