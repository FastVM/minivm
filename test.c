
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <libkern/OSCacheControl.h>

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

uint32_t* c_get_memory(uint32_t size) {
    int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT | MAP_JIT;
    int fd = -1;
    int offset = 0;
    uint32_t* addr = 0;

    addr = (uint32_t*)mmap(0, size, prot, flags, -1, 0);
    if (addr == MAP_FAILED){
        printf("failure detected\n");
        exit(-1);
    }

    return addr;
}

int main() {
    printf("%p\n", c_get_memory(10));
}
