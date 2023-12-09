
OPT ?= -Os -flto

EXE ?= 

BUILD_DIR ?= build
OBJ_DIR ?= $(BUILD_DIR)/obj
TMP_DIR ?= $(BUILD_DIR)/tmp
BIN_DIR ?= $(BUILD_DIR)/bin
RES_DIR ?= $(BUILD_DIR)/res

UNAME_S != uname -s
UNAME_O != uname -o

PROG_SRCS = main/minivm.c
PROG_OBJS = $(PROG_SRCS:%.c=$(OBJ_DIR)/%.o)

GC_SRCS = vendor/bdwgc/alloc.c vendor/bdwgc/allchblk.c vendor/bdwgc/blacklst.c vendor/bdwgc/dbg_mlc.c vendor/bdwgc/dyn_load.c vendor/bdwgc/finalize.c vendor/bdwgc/headers.c vendor/bdwgc/malloc.c vendor/bdwgc/mallocx.c vendor/bdwgc/mark.c vendor/bdwgc/mach_dep.c vendor/bdwgc/mark_rts.c vendor/bdwgc/misc.c vendor/bdwgc/new_hblk.c vendor/bdwgc/obj_map.c vendor/bdwgc/os_dep.c vendor/bdwgc/ptr_chck.c vendor/bdwgc/reclaim.c
GC_OBJS = $(GC_SRCS:%.c=$(OBJ_DIR)/%.o)

TREES_SRCS := vendor/trees/alloc.c vendor/trees/get_changed_ranges.c vendor/trees/language.c vendor/trees/lexer.c vendor/trees/node.c vendor/trees/parser.c vendor/trees/query.c vendor/trees/stack.c vendor/trees/subtree.c vendor/trees/tree_cursor.c vendor/trees/tree.c

STD_SRCS := vm/std/libs/io.c vm/std/std.c
VM_SRCS := vm/ir/ir.c vm/lib.c vm/ir/type.c vm/ast/build.c vm/ast/comp.c vm/ast/print.c vm/obj.c vm/backend/tb.c vm/ir/check.c vm/ir/rblock.c vm/lua/parser.c vm/lua/scan.c vm/lua/ast.c vm/lua/repl.c vendor/isocline/isocline.c

ALL_SRCS = $(VM_SRCS) $(STD_SRCS) $(EXTRA_SRCS) $(TREES_SRCS)
ALL_OBJS = $(ALL_SRCS:%.c=$(OBJ_DIR)/%.o)

TB_SRCS := vendor/cuik/common/common.c vendor/cuik/common/perf.c vendor/cuik/tb/src/libtb.c vendor/cuik/tb/src/x64/x64.c
TB_OBJS = $(TB_SRCS:%.c=$(OBJ_DIR)/%.o)

OBJS = $(ALL_OBJS) $(GC_OBJS) $(TB_OBJS)

CFLAGS += $(FLAGS)
LDFLAGS += $(FLAGS)

RUNNER ?= $(BIN_DIR)/minivm

OBJS_FreeBSD = 

OBJS := $(OBJS) $(OBJS_$(UNAME_S))

LDFLAGS_S_Darwin = -w -Wl,-pagezero_size,0x4000
LDFLAGS_S_Linux = -lm
LDFLAGS_O_Cygwin =
LDFLAGS_S_FreeBSD = -lm -lstdthreads

LDFLAGS := $(LDFLAGS_S_$(UNAME_S)) $(LDFLAGS_O_$(UNAME_O)) $(LDFLAGS)

CFLAGS_O_Cygwin = -D_WIN32

CFLAGS := $(CFLAGS_O_$(UNAME_O)) $(CFLAGS)

default: all

all: bins

# windows

clang-windows: .dummy
	rm -rf build
	$(MAKE) -Bj$(J) CC=clang EXE=.exe OPT="$(OPT)" CFLAGS="-Icuik/c11threads $(CFLAGS)" LDFLAGS="$(LDFLAGS)" EXTRA_SRCS="vendor/cuik/c11threads/threads_msvc.c"

gcc-windows: .dummy
	rm -rf build
	$(MAKE) -Bj$(J) CC=gcc EXE=.exe OPT="$(OPT)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS) -lSynchronization"

# binaries

bins: $(BIN_DIR)/minivm$(EXE)

minivm$(EXE) $(BIN_DIR)/minivm$(EXE): $(OBJ_DIR)/main/minivm.o $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(CC) $(OPT)  $(OBJ_DIR)/main/minivm.o $(OBJS) -o $(@) $(LDFLAGS)

# intermediate files

$(TB_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -w -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS) -I vendor/cuik/tb/include -I vendor/cuik/common -DCUIK_USE_TB -DLOG_SUPPRESS

$(PROG_OBJS) $(ALL_OBJS) $(GC_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS)

# format

format: .dummy
	clang-format -i $(ALL_OBJS:$(OBJ_DIR)/%.o=%.c)

# cleanup

clean: .dummy
	rm -rf $(BUILD_DIR)

# dummy

.dummy:
