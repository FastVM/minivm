
#if !defined(VM_HEADER_BE_EXEC)
#define VM_HEADER_BE_EXEC

void *vm_cache_dlsym(void *handle, const char *name);
void *vm_cache_comp(const char *comp, const char *flags, const char *src);

#endif
