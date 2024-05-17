
#include "exec.h"
#include "../lib.h"
#include "../std/io.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../../vendor/xxhash/xxhash.h"

void *vm_cache_comp(const char *comp, const char *flags, const char *src, const char *entry) {
    if (flags == NULL) {
        flags = "";
    }
    if (GetFileAttributes(".minivm-cache") == INVALID_FILE_ATTRIBUTES) {
        CreateDirectory(".minivm-cache", NULL);
    }
    size_t len = strlen(src);
    uint64_t hash = XXH3_64bits((void *)src, len);
    char so_file[128];
    snprintf(so_file, 64, ".minivm-cache/out-%s-%" PRIx64 ".so", comp, hash);
    if (GetFileAttributes(so_file) == INVALID_FILE_ATTRIBUTES) {
        char c_file[128];
        snprintf(c_file, 64, ".minivm-cache/src-%s-%" PRIx64 ".c", comp, hash);
        FILE *out = fopen(c_file, "w");
        fwrite(src, len, 1, out);
        fclose(out);
        vm_io_buffer_t *cmd_buf = vm_io_buffer_new();
        // vm_io_buffer_format(cmd_buf, "%s -shared -g3 -foptimize-sibling-calls -fPIC %s %s -o %s -w -pipe", comp, flags, c_file, so_file);
        vm_io_buffer_format(cmd_buf, "%s -shared -O2 -foptimize-sibling-calls -fPIC %s %s -o %s -w -pipe", comp, flags, c_file, so_file);
        int res = system(cmd_buf->buf);
        if (res) {
            return NULL;
        }
        remove(c_file);
    }
    void *handle = LoadLibrary(so_file);
    void *sym = GetProcAddress(handle, entry);
    remove(so_file);
    return sym;
}
#else
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(EMSCRIPTEN)
#include <emscripten.h>

EM_JS(void, vm_compile_c_to_wasm, (int n), {
    Module._vm_compile_c_to_wasm(n);
});

void *vm_cache_comp(const char *comp, const char *flags, const char *src, const char *entry) {
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
    fwrite(src, strlen(src), 1, out);
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

void *vm_cache_comp(const char *comp, const char *flags, const char *src, const char *entry) {
    if (flags == NULL) {
        flags = "";
    }
    struct stat st = {0};
    if (stat(".minivm-cache", &st) == -1) {
        mkdir(".minivm-cache", 0700);
    }
    size_t len = strlen(src);
    uint64_t hash = XXH3_64bits((void *)src, len);
    char so_file[128];
    snprintf(so_file, 64, ".minivm-cache/out-%s-%" PRIx64 ".so", comp, hash);
    if (stat(src, &st) == -1) {
        char c_file[128];
        snprintf(c_file, 64, ".minivm-cache/src-%s-%" PRIx64 ".c", comp, hash);
        FILE *out = fopen(c_file, "w");
        fwrite(src, len, 1, out);
        fclose(out);
        vm_io_buffer_t *cmd_buf = vm_io_buffer_new();
        vm_io_buffer_format(cmd_buf, "%s -shared -O2 -foptimize-sibling-calls -fPIC %s %s -o %s -w -pipe", comp, flags, c_file, so_file);
        // vm_io_buffer_format(cmd_buf, "%s -shared -g3 -fsanitize=memory -foptimize-sibling-calls -fPIC %s %s -o %s -w -pipe", comp, flags, c_file, so_file);
        int res = system(cmd_buf->buf);
        if (res) {
            return NULL;
        }
        remove(c_file);
    }
    void *handle = dlopen(so_file, RTLD_LAZY);
    void *sym = dlsym(handle, entry);
    remove(so_file);
    return sym;
}
#endif

#endif
