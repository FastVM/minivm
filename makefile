
 # Must be a GCC or Clang
CC ?= cc

OPT ?= -O2 -flto

# Reset flags for base
BASE_CFLAGS := ${OPT} -Ivendor/tree-sitter/lib/include ${CFLAGS}
BASE_LDFLAGS := ${OPT} ${LDFLAGS}

# object files and depends
MAIN_SRCS = main/minivm.c
VM_SRCS := $(shell find vm | grep \\.c)
TS_SRCS += vendor/tree-sitter/lib/src/alloc.c vendor/tree-sitter/lib/src/get_changed_ranges.c vendor/tree-sitter/lib/src/language.c vendor/tree-sitter/lib/src/lexer.c vendor/tree-sitter/lib/src/node.c vendor/tree-sitter/lib/src/parser.c vendor/tree-sitter/lib/src/query.c vendor/tree-sitter/lib/src/stack.c vendor/tree-sitter/lib/src/subtree.c vendor/tree-sitter/lib/src/tree_cursor.c vendor/tree-sitter/lib/src/tree.c vendor/tree-sitter/lib/src/wasm_store.c
IC_SRCS := vendor/isocline/src/isocline.c
MAIN_SRCS += ${VM_SRCS} ${TS_SRCS} ${IC_SRCS}
MAIN_OBJS = ${MAIN_SRCS:%.c=build/obj/%.o}

MAIN_DEPS = ${MAIN_SRCS:%.c=build/dep/%.dep}

# configure gc
BASE_CFLAGS += -DVM_NO_GC 

MAKE_INCLUDE ?= 

default: all

all: build/bin/minivm

build/bin/minivm: ${MAIN_OBJS}
	@mkdir -p ${dir ${@}}
	${CC} -o ${@} ${MAIN_OBJS} ${BASE_LDFLAGS}

${MAIN_OBJS}:
	@mkdir -p ${dir ${@}}
	@mkdir -p ${dir ${@:build/obj/%.o=build/dep/%.dep}}
	${CC} ${@:build/obj/%.o=%.c}  -c -o ${@} ${BASE_CFLAGS}
	${CC} ${@:build/obj/%.o=%.c} -MM -MF ${@:build/obj/%.o=build/dep/%.dep} -MT ${@} ${BASE_CFLAGS}

include ${wildcard ${MAIN_DEPS}}

.dummy:
