
OPT ?= -Os -flto

EXE ?=

BUILD_DIR ?= build
OBJ_DIR ?= $(BUILD_DIR)/obj
TMP_DIR ?= $(BUILD_DIR)/tmp
BIN_DIR ?= $(BUILD_DIR)/bin
RES_DIR ?= $(BUILD_DIR)/res

VENDOR_DIR ?= vendor
CUIK_DIR ?= $(VENDOR_DIR)/cuik
ISOCLINE_DIR ?= $(VENDOR_DIR)/isocline
TCC_DIR ?= $(VENDOR_DIR)/tcc
TREE_SITTER_DIR ?= $(VENDOR_DIR)/tree-sitter
XXHASH_DIR ?= $(VENDOR_DIR)/xxhash

UNAME_S != uname -s

OS_NAME_Windows_NT = WINDOWS
OS_NAME_Cygwin = WINDOWS
OS_NAME_Darwin = MACOS
OS_NAME_Linux = LINUX
OS_NAME_FreeBSD = FREEBSD

OS_NAME = $(OS_NAME_$(UNAME_S))

PROG_SRCS = main/minivm.c
PROG_OBJS = $(PROG_SRCS:%.c=$(OBJ_DIR)/%.o)

# GC_SRCS = vendor/bdwgc/alloc.c vendor/bdwgc/allchblk.c vendor/bdwgc/blacklst.c vendor/bdwgc/dbg_mlc.c vendor/bdwgc/dyn_load.c vendor/bdwgc/finalize.c vendor/bdwgc/headers.c vendor/bdwgc/malloc.c vendor/bdwgc/mallocx.c vendor/bdwgc/mark.c vendor/bdwgc/mach_dep.c vendor/bdwgc/mark_rts.c vendor/bdwgc/misc.c vendor/bdwgc/new_hblk.c vendor/bdwgc/obj_map.c vendor/bdwgc/os_dep.c vendor/bdwgc/ptr_chck.c vendor/bdwgc/reclaim.c
GC_OBJS = $(GC_SRCS:%.c=$(OBJ_DIR)/%.o)

TREES_SRCS = $(TREE_SITTER_DIR)/lib/src/alloc.c $(TREE_SITTER_DIR)/lib/src/get_changed_ranges.c $(TREE_SITTER_DIR)/lib/src/language.c $(TREE_SITTER_DIR)/lib/src/lexer.c $(TREE_SITTER_DIR)/lib/src/node.c $(TREE_SITTER_DIR)/lib/src/parser.c $(TREE_SITTER_DIR)/lib/src/query.c $(TREE_SITTER_DIR)/lib/src/stack.c $(TREE_SITTER_DIR)/lib/src/subtree.c $(TREE_SITTER_DIR)/lib/src/tree_cursor.c $(TREE_SITTER_DIR)/lib/src/tree.c $(TREE_SITTER_DIR)/lib/src/wasm_store.c

STD_SRCS = vm/std/io.c vm/std/std.c
ISOCLINE_SRCS = $(ISOCLINE_DIR)/src/isocline.c
XXH_SRCS = $(XXHASH_DIR)/xxhash.c
VM_SRCS = vm/ir/ir.c vm/lib.c vm/ir/type.c vm/ast/build.c vm/ast/ast.c vm/ast/comp.c vm/ast/print.c vm/obj.c vm/backend/tb.c vm/backend/exec.c vm/ir/check.c vm/ir/rblock.c vm/lua/parser/parser.c vm/lua/parser/scan.c vm/lua/ast.c vm/lua/repl.c $(ISOCLINE_SRCS) $(XXH_SRCS) $(TREES_SRCS)

ALL_SRCS = $(VM_SRCS) $(STD_SRCS) $(EXTRA_SRCS)
ALL_OBJS = $(ALL_SRCS:%.c=$(OBJ_DIR)/%.o)

TCC_SRCS ?= $(TCC_DIR)/libtcc.c $(TCC_DIR)/lib/libtcc1.c
TCC_OBJS = $(TCC_SRCS:%.c=$(OBJ_DIR)/%.o)

TB_SRCS_BASE = $(CUIK_DIR)/common/common.c $(CUIK_DIR)/common/perf.c $(CUIK_DIR)/tb/src/libtb.c $(CUIK_DIR)/tb/src/x64/x64_target.c
TB_SRCS_FREEBSD = $(CUIK_DIR)/c11threads/threads_posix.c
TB_SRCS = $(TB_SRCS_BASE) $(TB_SRCS_$(OS_NAME))
TB_OBJS = $(TB_SRCS:%.c=$(OBJ_DIR)/%.o)

BASE_OBJS = $(ALL_OBJS) $(GC_OBJS) $(TB_OBJS) $(TCC_OBJS)

CFLAGS += -I $(TREE_SITTER_DIR)/lib/include -I $(TREE_SITTER_DIR)/lib/src $(FLAGS)
LDFLAGS += $(FLAGS)

OBJS = $(BASE_OBJS)

LDFLAGS_WINDOWS =
LDFLAGS_MACOS = -w -Wl,-pagezero_size,0x4000
LDFLAGS_LINUX = -lm -ldl
LDFLAGS_FREEBSD = -lm -ldl -lpthread

LDFLAGS := $(LDFLAGS_$(OS_NAME)) $(LDFLAGS)

# CFLAGS_WINDOWS = -D_WIN32

CFLAGS := $(CFLAGS_$(OS_NAME)) $(CFLAGS)

MKDIR_WINDOWS = mkdir -p
MKDIR_MACOS = mkdir -p
MKDIR_LINUX = mkdir -p
MKDIR_FREEBSD = mkdir -p

MKDIR = $(MKDIR_$(OS_NAME))

default: all

all: bins

# tree sitter

VM_LUA_GRAMMAR_DIR := $(TMP_DIR)/grammar

pre: $(VM_LUA_GRAMMAR_DIR)

$(VM_LUA_GRAMMAR_DIR): vm/lua/parser/grammar.js
	$(MKDIR) $(VM_LUA_GRAMMAR_DIR)
	cp vm/lua/parser/grammar.js $(VM_LUA_GRAMMAR_DIR)
	cd $(VM_LUA_GRAMMAR_DIR) && cargo run --manifest-path $(TREE_SITTER_DIR)/Cargo.toml -- generate

vm/lua/parser/parser.c: $(VM_LUA_GRAMMAR_DIR) vm/lua/parser/tree_sitter
	cp $(VM_LUA_GRAMMAR_DIR)/src/parser.c $(@)

vm/lua/parser/tree_sitter: $(VM_LUA_GRAMMAR_DIR)
	cp -r $(VM_LUA_GRAMMAR_DIR)/src/tree_sitter $(@)

# binaries

bins: $(BIN_DIR)/minivm$(EXE)

minivm$(EXE) $(BIN_DIR)/minivm$(EXE): $(OBJ_DIR)/main/minivm.o $(OBJS)
	$(MKDIR) $(dir $(@))
	$(CC) $(OPT) $(OBJ_DIR)/main/minivm.o $(OBJS) -o $(@) $(LDFLAGS)

# intermediate files

$(TB_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	$(MKDIR) $(dir $(@))
	$(CC) -Wno-unused-value -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS) -I $(CUIK_DIR)/tb/include -I $(CUIK_DIR)/common -DCUIK_USE_TB -DLOG_SUPPRESS -DTB_HAS_X64

$(TCC_DIR)/config.h: $(TCC_DIR)/configure
	cd $(TCC_DIR) && ./configure

$(TCC_DIR)/tccdefs_.h: $(TCC_DIR)/include/tccdefs.h $(TCC_DIR)/config.h
	echo '""' > $(TCC_DIR)/tccdefs_.h

$(TCC_OBJS): $(@:$(OBJ_DIR)/%.o=%.c) $(TCC_DIR) $(TCC_DIR)/tccdefs_.h $(TCC_DIR)/config.h
	$(MKDIR) $(dir $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS)

$(PROG_OBJS) $(ALL_OBJS) $(GC_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	$(MKDIR) $(dir $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS)

# format

format: .dummy
	clang-format -i $(ALL_OBJS:$(OBJ_DIR)/%.o=%.c)

# cleanup

clean: .dummy
	rm -rf $(BUILD_DIR)

# dummy

.dummy:
