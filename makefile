
 # Must be a GCC or Clang
CC ?= cc

OPT ?= -O3 -fno-math-errno -fno-trapping-math -flto -DNDEBUG

EXE ?= 

TIME_CMD ?= $(shell which gdate || which date) +%s%3N

LLVM_PROFDATA ?= llvm-profdata

LIBM ?= YES
LIBM_NO :=
LIBM_YES := -lm
LIBM_FLAGS := ${LIBM_${LIBM}}

# Reset flags for base
BASE_CFLAGS := ${OPT} -Ivendor/tree-sitter/lib/include -Ivendor/mimalloc/include -Ivendor/tree-sitter/lib/src ${CFLAGS}
BASE_LDFLAGS := ${OPT} ${LDFLAGS} ${LIBM_FLAGS}

# object files and depends
MAIN_SRCS = main/minivm.c
VM_SRCS := $(shell find vm | grep \\.c)
# MI_SRCS := vendor/mimalloc/src/prim/prim.c vendor/mimalloc/src/alloc-posix.c vendor/mimalloc/src/alloc-aligned.c vendor/mimalloc/src/alloc.c vendor/mimalloc/src/arena.c vendor/mimalloc/src/bitmap.c vendor/mimalloc/src/heap.c vendor/mimalloc/src/init.c vendor/mimalloc/src/libc.c vendor/mimalloc/src/options.c vendor/mimalloc/src/os.c vendor/mimalloc/src/page.c vendor/mimalloc/src/random.c vendor/mimalloc/src/segment-map.c vendor/mimalloc/src/segment.c vendor/mimalloc/src/stats.c
TS_SRCS += vendor/tree-sitter/lib/src/alloc.c vendor/tree-sitter/lib/src/get_changed_ranges.c vendor/tree-sitter/lib/src/language.c vendor/tree-sitter/lib/src/lexer.c vendor/tree-sitter/lib/src/node.c vendor/tree-sitter/lib/src/parser.c vendor/tree-sitter/lib/src/query.c vendor/tree-sitter/lib/src/stack.c vendor/tree-sitter/lib/src/subtree.c vendor/tree-sitter/lib/src/tree_cursor.c vendor/tree-sitter/lib/src/tree.c vendor/tree-sitter/lib/src/wasm_store.c
IC_SRCS := vendor/isocline/src/isocline.c
MAIN_SRCS += ${VM_SRCS} ${TS_SRCS} ${IC_SRCS} ${MI_SRCS}
MAIN_OBJS = ${MAIN_SRCS:%.c=build/obj/%.o}

MAIN_DEPS = ${MAIN_SRCS:%.c=build/dep/%.dep}

# tests

TEST_SRCS := $(wildcard test/lua/*/*.lua)
TEST_TXTS = ${TEST_SRCS:%.lua=build/test/%.log}

# setup targets
default: all

all: minivm

minivm: build/bin/minivm${EXE}

test tests: ${TEST_TXTS}
	find . -name '*.time' | xargs -I{} sh -c 'echo $$(cat {}) {}' | sort -n > build/bench/all.txt

# specific builds
clean: .dummy
	rm -rf build

gcc-pgo: .dummy
	$(MAKE) -Bj minivm OPT="$(OPT) -fgcse-sm -fgcse-las -fipa-pta -fdevirtualize-at-ltrans -fdevirtualize-speculatively -fno-exceptions -fomit-frame-pointer -fprofile-generate"
	build/bin/minivm test/lua/fib/fib.lua
	build/bin/minivm test/lua/tables/trees.lua
	build/bin/minivm test/lua/closure/funcret.lua
	$(MAKE) -Bj minivm OPT="$(OPT) -fgcse-sm -fgcse-las -fipa-pta -fdevirtualize-at-ltrans -fdevirtualize-speculatively -fno-exceptions -fomit-frame-pointer -fprofile-use"

clang-pgo: .dummy
	$(MAKE) -Bj minivm OPT="$(OPT) -mllvm -polly -fno-exceptions -fprofile-instr-generate"
	build/bin/minivm test/lua/fib/fib.lua
	build/bin/minivm test/lua/tables/trees.lua
	build/bin/minivm test/lua/closure/funcret.lua
	$(LLVM_PROFDATA) merge default.profraw -o default.profdata
	$(MAKE) -Bj minivm OPT="$(OPT) -mllvm -polly -fno-exceptions -fprofile-use"

wasm: .dummy
	$(MAKE) -Bj CC=emcc EXE=.wasm \
		OPT='-O3 -flto' \
		LDFLAGS='-s WASM=1 -s PURE_WASI=1 -fno-exceptions -s WASMFS=1'

# internal stuff
build/bin/minivm${EXE}: ${MAIN_OBJS}
	@mkdir -p ${dir ${@}}
	${CC} -o ${@} ${MAIN_OBJS} ${BASE_LDFLAGS}

${TEST_TXTS}: ${@:build/test/%.log=%.lua} minivm
	@mkdir -p ${dir ${@}}
	@mkdir -p ${dir ${@:build/test/%.log=build/bench/%.time}}
	START_TIME=$$(${TIME_CMD}); \
	build/bin/minivm${EXE} ${@:build/test/%.log=%.lua} > ${@}; \
	END_TIME=$$(${TIME_CMD}); \
	echo "$$END_TIME $$START_TIME - p" | dc > ${@:build/test/%.log=build/bench/%.time}

${MAIN_OBJS}:
	@mkdir -p ${dir ${@}}
	@mkdir -p ${dir ${@:build/obj/%.o=build/dep/%.dep}}
	${CC} ${@:build/obj/%.o=%.c} -c -o ${@} ${BASE_CFLAGS}
	${CC} ${@:build/obj/%.o=%.c} -MM -MF ${@:build/obj/%.o=build/dep/%.dep} -MT ${@} ${BASE_CFLAGS}

include ${wildcard ${MAIN_DEPS}}

# .PHONY feels backwards
.dummy:
