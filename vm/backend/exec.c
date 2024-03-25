
#include "exec.h"
#include "../lib.h"
#include "../std/io.h"

#if defined(_WIN32)
// unsupported
void *vm_cache_comp(const char *comp, const char **srcs, const char *entry) {
    return NULL;
}
#else

#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

#include "../../vendor/xxhash/xxhash.h"

void *vm_cache_comp(const char *comp, const char **srcs, const char *entry) {
    struct stat st = {0};
    if (stat(".minivm-cache", &st) == -1) {
        mkdir(".minivm-cache", 0700);
    }
    vm_io_buffer_t *src_buf = vm_io_buffer_new();
    for (size_t i = 0; srcs[i] != NULL; i++) {
        vm_io_buffer_format(src_buf, "%s", srcs[i]);
    }
    uint64_t hash = XXH3_64bits((void *)src_buf->buf, src_buf->len);
    char so_file[128];
    snprintf(so_file, 64, ".minivm-cache/out-%s-%" PRIx64 ".so", comp, hash);
    if (stat(src_buf->buf, &st) == -1) {
        char c_file[128];
        snprintf(c_file, 64, ".minivm-cache/src-%s-%" PRIx64 ".c", comp, hash);
        FILE *out = fopen(c_file, "w");
        fwrite(src_buf->buf, src_buf->len, 1, out);
        fclose(out);
        vm_io_buffer_t *cmd_buf = vm_io_buffer_new();
        vm_io_buffer_format(cmd_buf, "%s -shared -O2 -foptimize-sibling-calls -fPIC %s -o %s -w -pipe", comp, c_file, so_file);
        // vm_io_buffer_format(cmd_buf, "%s -shared -g2 -foptimize-sibling-calls -fPIC %s -o %s -w -pipe", comp, c_file, so_file);
        int res = system(cmd_buf->buf);
        if (res) {
            return NULL;
        }
        remove(c_file);
    }
    void *handle = dlopen(so_file, RTLD_LAZY);
    void *sym = dlsym(handle, entry);
    remove(so_file);
    // printf("<funcptr: %p>\n", sym);
    return sym;
}
#endif

#endif
