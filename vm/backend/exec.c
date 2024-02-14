
#include "../lib.h"
#include "../std/io.h"
#include "exec.h"

#if defined(_WIN32)
// unsupported
void *vm_cache_comp(const char *comp, const char **srcs, const char *entry) {
    return NULL;
}
#else

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

#if defined(EMSCRIPTEN)
#include <emscripten.h>

#if 0
void vm_compile_c_to_wasm(int n);
#endif

#if 1
EM_JS(void, vm_compile_c_to_wasm, (int n), {
    globalThis.vm_compile_c_to_wasm(n);
});
#endif

void *vm_cache_comp(const char *comp, const char **srcs, const char *entry) {
    static int n = 0;
    n += 1;
    struct stat st = {0};
    vm_io_buffer_t *c_buf = vm_io_buffer_new();
    vm_io_buffer_format(c_buf, "/in%i.c", n);
    const char *c_file = c_buf->buf;
    vm_io_buffer_t *so_buf = vm_io_buffer_new();
    vm_io_buffer_format(so_buf, "/out%i.so", n);
    const char *so_file = so_buf->buf;
    FILE *out = fopen(c_file, "w");
    for (size_t i = 0; srcs[i] != NULL; i++) {
        fwrite(srcs[i], strlen(srcs[i]), 1, out);
    }
    fclose(out);
    vm_compile_c_to_wasm(n);
    void *handle = dlopen(so_file, RTLD_LAZY);
    void *sym = dlsym(handle, entry);
    remove(c_file);
    remove(so_file);
    return sym;
}
#else

void *vm_cache_comp(const char *comp, const char **srcs, const char *entry) {
    struct stat st = {0};
    if (stat(".minivm-cache", &st) == -1) {
        mkdir(".minivm-cache", 0700);
    }
    vm_io_buffer_t *c_buf = vm_io_buffer_new();
    vm_io_buffer_format(c_buf, ".minivm-cache/src-%s-%zu.c", entry, getpid());
    const char *c_file = c_buf->buf;
    vm_io_buffer_t *so_buf = vm_io_buffer_new();
    vm_io_buffer_format(so_buf, ".minivm-cache/out-%s-%zu.so", entry, getpid());
    const char *so_file = so_buf->buf;
    FILE *out = fopen(c_file, "w");
    for (size_t i = 0; srcs[i] != NULL; i++) {
        fwrite(srcs[i], strlen(srcs[i]), 1, out);
    }
    fclose(out);
    vm_io_buffer_t *cmd_buf = vm_io_buffer_new();
    vm_io_buffer_format(cmd_buf, "%s -shared -O2 -foptimize-sibling-calls -fPIC %s -o %s -w -pipe", comp, c_file, so_file);
    int res = system(cmd_buf->buf);
    if (res) {
        return NULL;
    }
    void *handle = dlopen(so_file, RTLD_LAZY);
    void *sym = dlsym(handle, entry);
    remove(c_file);
    remove(so_file);
    // printf("<funcptr: %p>\n", sym);
    return sym;
}
#endif

#endif
