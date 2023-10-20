
OPT ?= -O2

BUILD_DIR ?= build
OBJ_DIR ?= $(BUILD_DIR)/obj
TMP_DIR ?= $(BUILD_DIR)/tmp
BIN_DIR ?= $(BUILD_DIR)/bin
RES_DIR ?= $(BUILD_DIR)/res

PROG_SRCS = main/asm.c
PROG_OBJS = $(PROG_SRCS:%.c=$(OBJ_DIR)/%.o)

GC_SRCS = bdwgc/alloc.c bdwgc/allchblk.c bdwgc/blacklst.c bdwgc/dbg_mlc.c bdwgc/dyn_load.c bdwgc/finalize.c bdwgc/headers.c bdwgc/malloc.c bdwgc/mallocx.c bdwgc/mark.c bdwgc/mach_dep.c bdwgc/mark_rts.c bdwgc/misc.c bdwgc/new_hblk.c bdwgc/obj_map.c bdwgc/os_dep.c bdwgc/ptr_chck.c bdwgc/reclaim.c
GC_OBJS = $(GC_SRCS:%.c=$(OBJ_DIR)/%.o)

STD_SRCS := vm/std/libs/io.c vm/std/std.c
OPT_SRCS := vm/opt/info.c vm/opt/inline.c vm/opt/jump.c vm/opt/opt.c vm/opt/unused.c
ALL_SRCS = vm/ir.c vm/std/std.c vm/lib.c vm/type.c vm/lang/paka.c vm/obj.c vm/jit/tb.c $(STD_SRCS)  $(OPT_SRCS) $(EXTRA_SRCS)
ALL_OBJS = $(ALL_SRCS:%.c=$(OBJ_DIR)/%.o)

# TB_SRCS := common/common.c common/perf.c tb/src/libtb.c tb/src/x64/x64.c c11threads/threads_msvc.c
TB_SRCS := common/common.c common/perf.c tb/src/libtb.c tb/src/x64/x64.c
TB_OBJS = $(TB_SRCS:%.c=$(OBJ_DIR)/%.o)

OBJS = $(ALL_OBJS) $(GC_OBJS) $(TB_OBJS)

CFLAGS += $(FLAGS)
LDFLAGS += $(FLAGS)

RUNNER ?= $(BIN_DIR)/minivm

UNAME_S := $(shell uname -s)
UNAME_O := $(shell uname -o)

LDFLAGS_S_Darwin = -w -Wl,-pagezero_size,0x4000 -lm
LDFLAGS_S_Linux = -Wl,--export-dynamic -lm
LDFLAGS_O_Cygwin = -lSynchronization

LDFLAGS := $(LDFLAGS_S_$(UNAME_S)) $(LDFLAGS_O_$(UNAME_O)) $(LDFLAGS)

CFLAGS_O_Cygwin = -D_WIN32

CFLAGS := $(CFLAGS_O_$(UNAME_O)) $(CFLAGS)

default: all

all: bins libs

# binaries

libs: $(BIN_DIR)/libminivm.a

bins: $(BIN_DIR)/minivm

libminivm.a $(BIN_DIR)/libminivm.a: $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(AR) cr $(@) $(OBJS)

minivm $(BIN_DIR)/minivm: $(OBJ_DIR)/main/asm.o $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(CC) $(OPT) $(OBJ_DIR)/main/asm.o $(OBJS) -o $(@) $(LDFLAGS)

# intermediate files

$(TB_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS) -I tb/include -I common -DCUIK_USE_TB -DLOG_SUPPRESS

$(PROG_OBJS) $(ALL_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS)

$(GC_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -w -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS)

# cleanup

clean: .dummy
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(TMP_DIR)

# dummy

.dummy:
