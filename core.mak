

OPT ?= -Os -flto

EXE ?=

GCCJIT ?= NO
TB_WASM ?= NO

POST_INSTALL ?= @

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

OS_Windows_NT = WINDOWS
OS_Cygwin = WINDOWS
OS_Darwin = MACOS
OS_Linux = LINUX
OS_FreeBSD = FREEBSD

OS ?= $(OS_$(UNAME_S))

PROG_SRCS = main/minivm.c
PROG_OBJS = $(PROG_SRCS:%.c=$(OBJ_DIR)/%.o)

# GC_SRCS = vendor/bdwgc/alloc.c vendor/bdwgc/allchblk.c vendor/bdwgc/blacklst.c vendor/bdwgc/dbg_mlc.c vendor/bdwgc/dyn_load.c vendor/bdwgc/finalize.c vendor/bdwgc/headers.c vendor/bdwgc/malloc.c vendor/bdwgc/mallocx.c vendor/bdwgc/mark.c vendor/bdwgc/mach_dep.c vendor/bdwgc/mark_rts.c vendor/bdwgc/misc.c vendor/bdwgc/new_hblk.c vendor/bdwgc/obj_map.c vendor/bdwgc/os_dep.c vendor/bdwgc/ptr_chck.c vendor/bdwgc/reclaim.c
ISOCLINE_SRCS += $(ISOCLINE_DIR)/src/isocline.c
XXH_SRCS += $(XXHASH_DIR)/xxhash.c
TREES_SRCS += $(TREE_SITTER_DIR)/lib/src/alloc.c $(TREE_SITTER_DIR)/lib/src/get_changed_ranges.c $(TREE_SITTER_DIR)/lib/src/language.c $(TREE_SITTER_DIR)/lib/src/lexer.c $(TREE_SITTER_DIR)/lib/src/node.c $(TREE_SITTER_DIR)/lib/src/parser.c $(TREE_SITTER_DIR)/lib/src/query.c $(TREE_SITTER_DIR)/lib/src/stack.c $(TREE_SITTER_DIR)/lib/src/subtree.c $(TREE_SITTER_DIR)/lib/src/tree_cursor.c $(TREE_SITTER_DIR)/lib/src/tree.c $(TREE_SITTER_DIR)/lib/src/wasm_store.c
GC_OBJS += $(GC_SRCS:%.c=$(OBJ_DIR)/%.o)
VENDOR_SRCS += $(ISOCLINE_SRCS) $(XXH_SRCS) $(TREES_SRCS) $(GC_OBJS)
VENDOR_OBJS = $(VENDOR_SRCS:%.c=$(OBJ_DIR)/%.o)

VM_AST_SRCS += vm/ast/build.c vm/ast/ast.c vm/ast/comp.c vm/ast/print.c
VM_BACKEND_SRCS += vm/backend/tb.c vm/backend/exec.c
VM_BASE_SRCS += vm/lib.c vm/obj.c
VM_DATA_SRCS += vm/save/io.c vm/save/write.c vm/save/read.c
VM_IR_SRCS += vm/ir/ir.c vm/ir/type.c vm/ir/rblock.c vm/ir/check.c
VM_LUA_SRCS += vm/lua/parser/parser.c vm/lua/parser/scan.c vm/lua/repl.c vm/lua/ast.c
VM_STD_SRCS += vm/std/io.c vm/std/std.c
VM_SRCS += $(VM_AST_SRCS) $(VM_BASE_SRCS) $(VM_BACKEND_SRCS) $(VM_DATA_SRCS) $(VM_IR_SRCS) $(VM_LUA_SRCS) $(VM_STD_SRCS)
VM_OBJS = $(VM_SRCS:%.c=$(OBJ_DIR)/%.o)

TCC_SRCS ?= $(TCC_DIR)/libtcc.c
TCC_OBJS = $(TCC_SRCS:%.c=$(OBJ_DIR)/%.o)

TB_SRCS_BASE = $(CUIK_DIR)/common/common.c $(CUIK_DIR)/common/perf.c $(CUIK_DIR)/tb/src/libtb.c $(CUIK_DIR)/tb/src/x64/x64_target.c $(CUIK_DIR)/tb/src/wasm/wasm_target.c
TB_SRCS_MACOS = $(CUIK_DIR)/c11threads/threads_posix.c
TB_SRCS_FREEBSD = $(CUIK_DIR)/c11threads/threads_posix.c
TB_SRCS_WINDOWS = $(CUIK_DIR)/c11threads/threads_msvc.c
TB_SRCS += $(TB_SRCS_BASE) $(TB_SRCS_$(OS))
TB_OBJS = $(TB_SRCS:%.c=$(OBJ_DIR)/%.o)

OBJS = $(VM_OBJS) $(TB_OBJS) $(TCC_OBJS) $(VENDOR_OBJS)

LDFLAGS_GCCJIT_NO =
LDFLAGS_GCCJIT_YES = -lgccjit

LDFLAGS_MACOS_GCCJIT_NO =
LDFLAGS_MACOS_GCCJIT_YES = -L/opt/homebrew/lib/gcc/current

LDFLAGS_WINDOWS =
LDFLAGS_MACOS = $(LDFLAGS_MACOS_GCCJIT_$(GCCJIT))
LDFLAGS_LINUX = -lm -ldl
LDFLAGS_FREEBSD = -lm -ldl -lpthread

LDFLAGS := $(LDFLAGS_$(OS)) $(LDFLAGS_GCCJIT_$(GCCJIT)) $(LDFLAGS)

CFLAGS_TB := -I$(CUIK_DIR)/tb/include -I$(CUIK_DIR)/c11threads -I$(CUIK_DIR)/common -DCUIK_USE_TB -DLOG_SUPPRESS -DTB_HAS_X64 $(CFLAGS_TB)
CFLAGS_VENDOR := -I$(TREE_SITTER_DIR)/lib/include -I$(TREE_SITTER_DIR)/lib/src $(CFLAGS_VENDOR)

CFLAGS_GCCJIT_NO =
CFLAGS_GCCJIT_YES = -DTB_USE_GCCJIT

CFLAGS_TB_WASM_NO =
CFLAGS_TB_WASM_YES = -DTB_HAS_WASM

CFLAGS_MACOS = -I/opt/homebrew/include
CFLAGS := $(CFLAGS_$(OS)) $(CFLAGS_GCCJIT_$(GCCJIT)) $(CFLAGS_TB_WASM_$(TB_WASM)) $(CFLAGS)

LDFLAGS := $(FLAGS) $(LDFLAGS)
CFLAGS := $(FLAGS) $(CFLAGS)

MKDIR = @mkdir -p

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
	$(POST_INSTALL)

# prepare minimal tcc

$(TCC_DIR)/config.h: $(TCC_DIR)/configure
	cd $(TCC_DIR) && ./configure

$(TCC_DIR)/tccdefs_.h: $(TCC_DIR)/include/tccdefs.h $(TCC_DIR)/config.h
	echo '""' > $(TCC_DIR)/tccdefs_.h

# compile c

$(TB_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	$(MKDIR) $(dir $(@))
	$(CC) -Wno-unused-value -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS) $(CFLAGS_TB)

$(TCC_OBJS): $(@:$(OBJ_DIR)/%.o=%.c) $(TCC_DIR) $(TCC_DIR)/tccdefs_.h $(TCC_DIR)/config.h
	$(MKDIR) $(dir $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS) $(CFLAGS_TCC)

$(VENDOR_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	$(MKDIR) $(dir $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS) $(CFLAGS_VENDOR)

$(PROG_OBJS) $(VM_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	$(MKDIR) $(dir $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS) $(CFLAGS_VM)

# cleanup

clean: .dummy
	rm -rf $(BUILD_DIR)

# dummy

.dummy:
