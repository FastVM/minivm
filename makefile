
OPT ?= -Os -flto

EXE ?= 

CURRENT_DIR != pwd

BUILD_DIR ?= $(CURRENT_DIR)/build
OBJ_DIR ?= $(BUILD_DIR)/obj
TMP_DIR ?= $(BUILD_DIR)/tmp
BIN_DIR ?= $(BUILD_DIR)/bin
RES_DIR ?= $(BUILD_DIR)/res

VENDOR_DIR ?= $(CURRENT_DIR)/vendor
VENDOR_CUIK_DIR ?= $(VENDOR_DIR)/cuik
VENDOR_ISOCLINE_DIR ?= $(VENDOR_DIR)/isocline
VENDOR_TCC_DIR ?= $(VENDOR_DIR)/tcc
VENDOR_TREE_SITTER_DIR ?= $(VENDOR_DIR)/tree-sitter
VENDOR_XXHASH_DIR ?= $(VENDOR_DIR)/xxhash

UNAME_S != uname -s
UNAME_O != uname -o

PROG_SRCS = main/minivm.c
PROG_OBJS = $(PROG_SRCS:%.c=$(OBJ_DIR)/%.o)

# GC_SRCS = vendor/bdwgc/alloc.c vendor/bdwgc/allchblk.c vendor/bdwgc/blacklst.c vendor/bdwgc/dbg_mlc.c vendor/bdwgc/dyn_load.c vendor/bdwgc/finalize.c vendor/bdwgc/headers.c vendor/bdwgc/malloc.c vendor/bdwgc/mallocx.c vendor/bdwgc/mark.c vendor/bdwgc/mach_dep.c vendor/bdwgc/mark_rts.c vendor/bdwgc/misc.c vendor/bdwgc/new_hblk.c vendor/bdwgc/obj_map.c vendor/bdwgc/os_dep.c vendor/bdwgc/ptr_chck.c vendor/bdwgc/reclaim.c
GC_OBJS = $(GC_SRCS:%.c=$(OBJ_DIR)/%.o)

TREES_SRCS = vendor/tree-sitter/lib/src/alloc.c vendor/tree-sitter/lib/src/get_changed_ranges.c vendor/tree-sitter/lib/src/language.c vendor/tree-sitter/lib/src/lexer.c vendor/tree-sitter/lib/src/node.c vendor/tree-sitter/lib/src/parser.c vendor/tree-sitter/lib/src/query.c vendor/tree-sitter/lib/src/stack.c vendor/tree-sitter/lib/src/subtree.c vendor/tree-sitter/lib/src/tree_cursor.c vendor/tree-sitter/lib/src/tree.c vendor/tree-sitter/lib/src/wasm_store.c 

STD_SRCS = vm/std/io.c vm/std/std.c
ISOCLINE_SRCS = vendor/isocline/src/isocline.c
XXH_SRCS = vendor/xxhash/xxhash.c
VM_SRCS = vm/ir/ir.c vm/lib.c vm/ir/type.c vm/ast/build.c vm/ast/ast.c vm/ast/comp.c vm/ast/print.c vm/obj.c vm/backend/tb.c vm/backend/exec.c vm/ir/check.c vm/ir/rblock.c vm/lua/parser/parser.c vm/lua/parser/scan.c vm/lua/ast.c vm/lua/repl.c $(ISOCLINE_SRCS) $(XXH_SRCS) $(TREES_SRCS)

ALL_SRCS = $(VM_SRCS) $(STD_SRCS) $(EXTRA_SRCS)
ALL_OBJS = $(ALL_SRCS:%.c=$(OBJ_DIR)/%.o)

TCC_SRCS ?= vendor/tcc/libtcc.c vendor/tcc/lib/libtcc1.c
TCC_OBJS = $(TCC_SRCS:%.c=$(OBJ_DIR)/%.o)

TB_SRCS_BASE = vendor/cuik/common/common.c vendor/cuik/common/perf.c vendor/cuik/tb/src/libtb.c vendor/cuik/tb/src/x64/x64_target.c
TB_SRCS_FreeBSD = vendor/cuik/c11threads/threads_posix.c
TB_SRCS = $(TB_SRCS_BASE) $(TB_SRCS_$(UNAME_S))
TB_OBJS = $(TB_SRCS:%.c=$(OBJ_DIR)/%.o)

BASE_OBJS = $(ALL_OBJS) $(GC_OBJS) $(TB_OBJS) $(TCC_OBJS)

CFLAGS += -I vendor/tree-sitter/lib/include -I vendor/tree-sitter/lib/src $(FLAGS)
LDFLAGS += $(FLAGS)

OBJS = $(BASE_OBJS)

LDFLAGS_S_Darwin = -w -Wl,-pagezero_size,0x4000
LDFLAGS_S_Linux = -lm -ldl
LDFLAGS_O_Cygwin =
LDFLAGS_S_FreeBSD = -lm -ldl -lpthread

LDFLAGS := $(LDFLAGS_S_$(UNAME_S)) $(LDFLAGS_O_$(UNAME_O)) $(LDFLAGS)

CFLAGS_O_Cygwin = -D_WIN32

CFLAGS := $(CFLAGS_O_$(UNAME_O)) $(CFLAGS)

default: all

all: bins

# tree sitter

VM_LUA_GRAMMAR_DIR := $(TMP_DIR)/grammar

pre: $(VM_LUA_GRAMMAR_DIR)

$(VM_LUA_GRAMMAR_DIR): vm/lua/parser/grammar.js
	mkdir -p $(VM_LUA_GRAMMAR_DIR)
	cp vm/lua/parser/grammar.js $(VM_LUA_GRAMMAR_DIR)
	cd $(VM_LUA_GRAMMAR_DIR) && cargo run --manifest-path $(VENDOR_TREE_SITTER_DIR)/Cargo.toml -- generate

vm/lua/parser/parser.c: $(VM_LUA_GRAMMAR_DIR) vm/lua/parser/tree_sitter
	cp $(VM_LUA_GRAMMAR_DIR)/src/parser.c $(@)

vm/lua/parser/tree_sitter: $(VM_LUA_GRAMMAR_DIR)
	cp -r $(VM_LUA_GRAMMAR_DIR)/src/tree_sitter $(@)

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
	$(CC) $(OPT) $(OBJ_DIR)/main/minivm.o $(OBJS) -o $(@) $(LDFLAGS)

# intermediate files

$(TB_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -Wno-unused-value -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS) -I vendor/cuik/tb/include -I vendor/cuik/common -DCUIK_USE_TB -DLOG_SUPPRESS -DTB_HAS_X64

$(PROG_OBJS) $(ALL_OBJS) $(GC_OBJS) $(TCC_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
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
