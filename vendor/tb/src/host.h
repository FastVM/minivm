// architecture
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
#define TB_HOST_X86_64 1
#elif defined(i386) || defined(__i386) || defined(__i386__)
#define TB_HOST_X86 2
#elif defined(__aarch64__)
#define TB_HOST_ARM64 3
#elif defined(__arm__)
#define TB_HOST_ARM32 4
#endif

// system
#if defined(__APPLE__) && defined(__MACH__)
#define TB_HOST_OSX 1
#elif defined(__gnu_linux__) || defined(__linux__) || defined(EMSCRIPTEN)
#define TB_HOST_LINUX 2
#elif defined(_WIN32)
#define TB_HOST_WINDOWS 3
#endif
