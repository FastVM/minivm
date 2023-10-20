// This is the TB unity build
#include "tb.c"
#include "hash.c"
#include "abi.c"
#include "tb_builder.c"
#include "debug_builder.c"
#include "ir_printer.c"
#include "exporter.c"
#include "symbols.c"

// JIT
#include "jit.c"

// Optimizer
#include "opt/optimizer.c"

// Parsers
#define TB_COFF_IMPL
#include <tb_coff.h>

// Debug
#include "debug/cv.c"
#include "debug/fut.c"

// Objects
#include "objects/coff.c"
#include "objects/elf64.c"
#include "objects/macho.c"

// Linker
#include "linker/linker.c"
#include "linker/pe.c"
#include "linker/elf.c"

// Platform layer
#if defined(_WIN32)
#pragma comment(lib, "onecore.lib")

void* tb_platform_valloc(size_t size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void tb_platform_vfree(void* ptr, size_t size) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

bool tb_platform_vprotect(void* ptr, size_t size, TB_MemProtect prot) {
    DWORD protect;
    switch (prot) {
        case TB_PAGE_RO:  protect = PAGE_READONLY; break;
        case TB_PAGE_RW:  protect = PAGE_READWRITE; break;
        case TB_PAGE_RX:  protect = PAGE_EXECUTE_READ; break;
        case TB_PAGE_RXW: protect = PAGE_EXECUTE_READWRITE; break;
        default: return false;
    }

    DWORD old_protect;
    return VirtualProtect(ptr, size, protect, &old_protect);
}


size_t get_large_pages(void) {
    static bool init;
    static size_t large_page_size;

    if (!init) {
        init = true;

        unsigned long err = 0;
        HANDLE token = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
            goto err;
        }

        TOKEN_PRIVILEGES tp;
        if (!LookupPrivilegeValue(NULL, TEXT("SeLockMemoryPrivilege"), &tp.Privileges[0].Luid)) {
            goto err;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if (!AdjustTokenPrivileges(token, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0)) {
            goto err;
        }

        large_page_size = GetLargePageMinimum();

        err:
        CloseHandle(token);
    }

    return large_page_size;
}

#if NTDDI_VERSION >= NTDDI_WIN10_RS4
void* tb_jit_create_stack(size_t* out_size) {
    size_t size = get_large_pages();

    // TODO(NeGate): we don't support non-large page stacks
    if (size == 0) tb_todo();

    // natural alignment stack because it makes it easy to always find
    // the base.
    MEM_EXTENDED_PARAMETER param = {
        .Type = MemExtendedParameterAddressRequirements,
        .Pointer = &(MEM_ADDRESS_REQUIREMENTS){ .Alignment = size }
    };

    *out_size = size;
    return VirtualAlloc2(GetCurrentProcess(), NULL, size, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE, &param, 1);
}
#endif /* NTDDI_VERSION >= NTDDI_WIN10_RS4 */
#elif defined(_POSIX_C_SOURCE)
void* tb_platform_valloc(size_t size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void* tb_platform_valloc_guard(size_t size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void tb_platform_vfree(void* ptr, size_t size) {
    munmap(ptr, size);
}

bool tb_platform_vprotect(void* ptr, size_t size, TB_MemProtect prot) {
    uint32_t protect;
    switch (prot) {
        case TB_PAGE_RO:  protect = PROT_READ; break;
        case TB_PAGE_RW:  protect = PROT_READ | PROT_WRITE; break;
        case TB_PAGE_RX:  protect = PROT_READ | PROT_EXEC; break;
        case TB_PAGE_RXW: protect = PROT_READ | PROT_WRITE | PROT_EXEC; break;
        default: return false;
    }

    return mprotect(ptr, size, protect) == 0;
}
#endif
