
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include "../lib.h"
#include "../std/io.h"

void *vm_cache_comp(const char *comp, const char *src) {
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
    fwrite(src, strlen(src), 1, out);
    fclose(out);
    vm_io_buffer_t *cmd_buf = vm_io_buffer_new();
    vm_io_buffer_format(cmd_buf, "%s -Ofast %s -o %s -fpic -shared", comp, c_file, so_file);
    system(cmd_buf->buf);
    void *handle = dlopen(so_file, RTLD_LAZY);
    void *sym = dlsym(handle, "entry");
    remove(c_file);
    remove(so_file);
    // dlclose(handle);
    // printf("%p\n", sym);
    return sym;
}
