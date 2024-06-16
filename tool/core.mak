


OPT ?= -Os -flto

EXE ?=

TEST_TIME = 15
TEST_LUA = lua
TEST_DIFF = diff

BUILD_DIR ?= build
OBJ_DIR ?= $(BUILD_DIR)/obj
TMP_DIR ?= $(BUILD_DIR)/tmp
BIN_DIR ?= $(BUILD_DIR)/bin
RES_DIR ?= $(BUILD_DIR)/res
TEST_DIR ?= $(BUILD_DIR)/test

VENDOR_DIR ?= vendor
ISOCLINE_DIR ?= $(VENDOR_DIR)/isocline
TREE_SITTER_DIR ?= $(VENDOR_DIR)/tree-sitter

UNAME_S != uname -s

OS_Windows_NT = WINDOWS
OS_Cygwin = WINDOWS
OS_Darwin = MAC
OS_Linux = LINUX
OS_FreeBSD = FREEBSD

OS ?= $(OS_$(UNAME_S))

PROG_SRCS = main/minivm.c
PROG_OBJS = $(PROG_SRCS:%.c=$(OBJ_DIR)/%.o)

CFLAGS_VM_RAYLIB_YES = -DVM_USE_RAYLIB
CFLAGS_VM += $(CFLAGS_VM_RAYLIB_$(RAYLIB))

THREAD_SRCS_OS_WINDOWS = vendor/c11threads/threads_msvc.c
THREAD_SRCS_OS_MAC = vendor/c11threads/threads_posix.c
THREAD_SRCS_OS_LINUX = vendor/c11threads/threads_posix.c
THREAD_SRCS_OS_POSIX = vendor/c11threads/threads_posix.c
THREAD_SRCS += $(THREAD_SRCS_OS_$(OS))

ISOCLINE_SRCS += $(ISOCLINE_DIR)/src/isocline.c
TREES_SRCS += $(TREE_SITTER_DIR)/lib/src/alloc.c $(TREE_SITTER_DIR)/lib/src/get_changed_ranges.c $(TREE_SITTER_DIR)/lib/src/language.c $(TREE_SITTER_DIR)/lib/src/lexer.c $(TREE_SITTER_DIR)/lib/src/node.c $(TREE_SITTER_DIR)/lib/src/parser.c $(TREE_SITTER_DIR)/lib/src/query.c $(TREE_SITTER_DIR)/lib/src/stack.c $(TREE_SITTER_DIR)/lib/src/subtree.c $(TREE_SITTER_DIR)/lib/src/tree_cursor.c $(TREE_SITTER_DIR)/lib/src/tree.c $(TREE_SITTER_DIR)/lib/src/wasm_store.c

VENDOR_SRCS += $(ISOCLINE_SRCS) $(TREES_SRCS) $(THREAD_SRCS)

VENDOR_OBJS = $(VENDOR_SRCS:%.c=$(OBJ_DIR)/%.o)

VM_AST_SRCS += vm/ast/build.c vm/ast/ast.c vm/ast/comp.c vm/ast/print.c
VM_BACKEND_SRCS += vm/backend/backend.c
VM_BASE_SRCS += vm/lib.c vm/obj.c
VM_DATA_SRCS += vm/save/io.c vm/save/write.c vm/save/read.c
VM_IR_SRCS += vm/ir.c
VM_LUA_SRCS += vm/lua/parser/parser.c vm/lua/parser/scan.c vm/lua/repl.c vm/lua/ast.c
VM_STD_SRCS += vm/io.c vm/std.c
VM_SRCS += $(VM_AST_SRCS) $(VM_BASE_SRCS) $(VM_BACKEND_SRCS) $(VM_DATA_SRCS) $(VM_IR_SRCS) $(VM_LUA_SRCS) $(VM_STD_SRCS)
VM_OBJS = $(VM_SRCS:%.c=$(OBJ_DIR)/%.o)

OBJS = $(VM_OBJS) $(VENDOR_OBJS)

LDFLAGS_GCCJIT_NO =
LDFLAGS_GCCJIT_YES = -lgccjit

LDFLAGS_MAC_GCCJIT_NO =
LDFLAGS_MAC_GCCJIT_YES = -L/opt/homebrew/lib/gcc/current

LDFLAGS_WINDOWS = -lSynchronization
LDFLAGS_MAC = $(LDFLAGS_MAC_GCCJIT_$(GCCJIT))
LDFLAGS_LINUX = -lm -ldl
LDFLAGS_FREEBSD = -lm -ldl -lpthread

LDFLAGS := $(LDFLAGS_$(OS)) $(LDFLAGS_GCCJIT_$(GCCJIT)) $(LDFLAGS)

CFLAGS_VENDOR := -I$(TREE_SITTER_DIR)/lib/include -I$(TREE_SITTER_DIR)/lib/src $(CFLAGS_VENDOR)

CFLAGS_MAC = -I/opt/homebrew/include
CFLAGS := $(CFLAGS_$(OS)) $(CFLAGS)

LDFLAGS := $(FLAGS) $(LDFLAGS)
CFLAGS := $(FLAGS) $(CFLAGS)

MKDIR = mkdir -p

# find test -name '*.lua' | xargs echo
TEST_LUAS = test/closure/funcons.lua test/closure/ccall.lua test/closure/recursive.lua test/closure/funcret.lua test/closure/delay.lua test/closure/yodacall.lua test/tables/len.lua test/tables/test.lua test/tables/tbench.lua test/tables/concat.lua test/tables/trees_no_loop.lua test/tables/trees2.lua test/tables/abench.lua test/tables/trees.lua test/types/scary.lua test/basic/boolean.lua test/basic/countdown.lua test/basic/method.lua test/basic/mod.lua test/basic/if.lua test/basic/hello.lua test/basic/types.lua test/basic/while.lua test/basic/idiv.lua test/basic/numtype.lua test/basic/arg.lua test/rec/tak.lua test/rec/tarai.lua test/fib/tab.lua test/fib/ptr.lua test/fib/fib.lua test/loop/squares.lua test/loop/primes_for.lua test/loop/loop2.lua test/loop/eval.lua test/loop/primes4.lua test/loop/primes5.lua test/loop/primes2.lua test/loop/primes3.lua test/loop/primes1.lua test/loop/sqrt.lua test/loop/range.lua test/loop/loop.lua
TEST_TEXTS = $(TEST_LUAS:%.lua=$(TEST_DIR)/%.diff)
TEST_TIMEOUT = timeout $(TEST_TIME)s

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

# tests

test: $(TEST_TEXTS)
	cat $(TEST_TEXTS) > $(TEST_DIR)/diff.txt

$(TEST_TEXTS): $(BIN_DIR)/minivm$(EXE) $(@:$(TEST_DIR)/%.diff=%.lua)
	$(MKDIR) $(dir $(@))
	$(TEST_TIMEOUT) $(BIN_DIR)/minivm$(EXE) $(TEST_FLAGS) $(@:$(TEST_DIR)/%.diff=%.lua) 2>&1 > $(@:$(TEST_DIR)/%.diff=$(TEST_DIR)/%.vm.log) || true
	$(TEST_TIMEOUT) $(TEST_LUA) $(@:$(TEST_DIR)/%.diff=%.lua) 2>&1 > $(@:$(TEST_DIR)/%.diff=$(TEST_DIR)/%.lua.log) || true
	echo "--- $(@:$(TEST_DIR)/%.diff=%.lua) ---" > $(@)
	$(TEST_DIFF) $(@:$(TEST_DIR)/%.diff=$(TEST_DIR)/%.lua.log) $(@:$(TEST_DIR)/%.diff=$(TEST_DIR)/%.vm.log) >> $(@) || true
	echo "--- end ---" >> $(@)
	echo >> $(@)

# compile c

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
