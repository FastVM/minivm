#pragma once

#if !defined(VM_FRAMES_UNITS)
#define VM_FRAMES_UNITS (10000)
#endif

#if !defined(VM_LOCALS_UNITS)
#define VM_LOCALS_UNITS (100000)
#endif

#if !defined(VM_OPS_UNITS)
#define VM_OPS_UNITS (1024 * 1024)
#endif

#if !defined(VM_MEM_MAX)
#define VM_MEM_MAX (1024 * 1024 * 256)
#endif

#if !defined(VM_MEM_MIN)
#define VM_MEM_MIN (1024 * 1024)
#endif

#if !defined(VM_MEM_GROWTH)
#define VM_MEM_GROWTH 2
#endif

#if !defined(VM_MEM_SHRINK)
#define VM_MEM_SHRINK 0
#endif

#if !defined(VM_SHRINK_GC_CAP)
#if VM_MEM_SHRINK
#define VM_SHRINK_GC_CAP 1
#else
#define VM_SHRINK_GC_CAP 0
#endif
#endif