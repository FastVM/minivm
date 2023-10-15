#include "../tb_internal.h"

typedef enum {
    UNWIND_OP_PUSH_NONVOL = 0, /* info == register number */
    UNWIND_OP_ALLOC_LARGE,     /* no info, alloc size in next 2 slots */
    UNWIND_OP_ALLOC_SMALL,     /* info == size of allocation / 8 - 1 */
    UNWIND_OP_SET_FPREG,       /* no info, FP = RSP + UNWIND_INFO.FPRegOffset*16 */
    UNWIND_OP_SAVE_NONVOL,     /* info == register number, offset in next slot */
    UNWIND_OP_SAVE_NONVOL_FAR, /* info == register number, offset in next 2 slots */
    UNWIND_OP_SAVE_XMM128 = 8, /* info == XMM reg number, offset in next slot */
    UNWIND_OP_SAVE_XMM128_FAR, /* info == XMM reg number, offset in next 2 slots */
    UNWIND_OP_PUSH_MACHFRAME   /* info == 0: no error-code, 1: error-code */
} UnwindCodeOps;

typedef union {
    struct {
        uint8_t code_offset;
        uint8_t unwind_op : 4;
        uint8_t op_info   : 4;
    };
    uint16_t frame_offset;
} UnwindCode;

enum {
    UNWIND_FLAG_EHANDLER  = 0x01,
    UNWIND_FLAG_UHANDLER  = 0x02,
    UNWIND_FLAG_CHAININFO = 0x04,
};

typedef struct {
    uint8_t version : 3;
    uint8_t flags   : 5;
    uint8_t prolog_length;
    uint8_t code_count;
    uint8_t frame_register : 4;
    uint8_t frame_offset   : 4;
    UnwindCode code[]; // ((code_count + 1) & ~1) - 1
} UnwindInfo;

