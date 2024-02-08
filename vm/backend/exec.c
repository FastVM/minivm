
#include "../lib.h"
#include "../std/io.h"

#ifdef _WIN32
// unsupported
void *vm_cache_comp(const char *comp, const char **srcs, const char *entry) {
    return NULL;
}
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

void *vm_cache_comp(const char *comp, const char **srcs, const char *entry) {
    struct stat st = {0};
    if (stat(".minivm-cache", &st) == -1) {
        mkdir(".minivm-cache", 0700);
    }
    vm_io_buffer_t *c_buf = vm_io_buffer_new();
    vm_io_buffer_format(c_buf, ".minivm-cache/src-%zu.c", getpid());
    vm_io_buffer_t *so_buf = vm_io_buffer_new();
    vm_io_buffer_format(so_buf, ".minivm-cache/out-%zu.so", getpid());
    const char *c_file = c_buf->buf;
    const char *so_file = so_buf->buf;
    FILE *out = fopen(c_file, "w");
    for (size_t i = 0; srcs[i] != NULL; i++) {
        fwrite(srcs[i], strlen(srcs[i]), 1, out);
    }
    fclose(out);
    vm_io_buffer_t *cmd_buf = vm_io_buffer_new();
    vm_io_buffer_format(cmd_buf, "%s -shared -Ofast %s -o %s -fpic", comp, c_file, so_file);
    system(cmd_buf->buf);
    void *handle = dlopen(so_file, RTLD_LAZY);
    void *sym = dlsym(handle, entry);
    // remove(c_file);
    // remove(so_file);
    // dlclose(handle);
    // printf("%p\n", sym);
    return sym;
}
#endif
