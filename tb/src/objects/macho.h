// imma shorten Mach-O into MO
#include "../tb_internal.h"

typedef struct {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
} MO_Header64;

typedef struct {
    uint32_t cmd;
    uint32_t cmdsize;
} MO_LoadCmd;

typedef struct {
	MO_LoadCmd header;
	uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
} MO_SymtabCmd;

typedef struct {
	MO_LoadCmd header;
    char segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
} MO_SegmentCmd64;

typedef struct {
    char sectname[16];
    char segname[16];
    uint64_t addr;
    uint64_t size;
    uint32_t offset; // file offset
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
} MO_Section64;

// extracted from the <mach-o/loader.h> header
#define MH_MAGIC_64 0xFEEDFACF
#define MH_CIGAM_64 0xCFFAEDFE

#define CPU_TYPE_X86_64  0x1000007
#define CPU_TYPE_AARCH64 0x100000C

#define MH_OBJECT   1
#define MH_EXECUTE  2
#define MH_FVMLIB   3
#define MH_CORE     4
#define MH_PRELOAD  5
#define MH_DYLIB    6
#define MH_DYLINKER 7
#define MH_BUNDLE   8

/* Constants for the cmd field of all load commands, the type */
#define LC_SEGMENT        0x1
#define LC_SYMTAB         0x2
#define LC_SYMSEG         0x3
#define LC_THREAD         0x4
#define LC_UNIXTHREAD     0x5
#define LC_LOADFVMLIB     0x6
#define LC_IDFVMLIB       0x7
#define LC_IDENT          0x8
#define LC_FVMFILE        0x9
#define LC_PREPAGE        0xa
#define LC_DYSYMTAB       0xb
#define LC_LOAD_DYLIB     0xc
#define LC_ID_DYLIB       0xd
#define LC_LOAD_DYLINKER  0xe
#define LC_ID_DYLINKER    0xf
#define LC_PREBOUND_DYLIB 0x10
#define LC_SEGMENT_64     0x19

#define N_EXT  0x01
#define N_SECT 0x0E

#define NO_SECT 0

#define S_ATTR_PURE_INSTRUCTIONS 0x80000000
#define S_ATTR_SOME_INSTRUCTIONS 0x00000400
