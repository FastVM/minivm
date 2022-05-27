#pragma once

#if !defined(VM_CONFIG_GC_ENTRIES)
#define VM_CONFIG_GC_ENTRIES 200
#endif

#if !defined(VM_CONFIG_GC_INIT)
#define VM_CONFIG_GC_INIT (1000)
#endif

#if !defined(VM_CONFIG_GC_SHRINK)
#define VM_CONFIG_GC_SHRINK 0
#endif

struct vm_config_t;
typedef struct vm_config_t vm_config_t;

#include "lib.h"

struct vm_config_t {
  size_t gc_ents;
  size_t gc_init;
  size_t gc_shrink;
};

static inline vm_config_t vm_config_init(void) {
  return (vm_config_t) {
    .gc_ents = VM_CONFIG_GC_ENTRIES,
    .gc_init = VM_CONFIG_GC_INIT,
    .gc_shrink = VM_CONFIG_GC_SHRINK,
  };
}
