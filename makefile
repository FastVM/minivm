
GCC ?= gcc
LLVM_PROFDATA ?= llvm-profdata
CLANG ?= clang
OPT ?= -O2
HOST_CC ?= $(CC)
DOT ?= dot

BUILD_DIR ?= build
OBJ_DIR ?= $(BUILD_DIR)/obj
TMP_DIR ?= $(BUILD_DIR)/tmp
BIN_DIR ?= $(BUILD_DIR)/bin
RES_DIR ?= $(BUILD_DIR)/res

LUA ?= $(BIN_DIR)/minilua

PROG_SRCS = main/asm.c
PROG_OBJS = $(PROG_SRCS:%.c=$(OBJ_DIR)/%.o)

JITC_SRCS = vm/jit/x64.dasc
JITC_OBJS =  $(JITC_SRCS:%.dasc=$(OBJ_DIR)/%.o)

GC_SRCS = bdwgc/alloc.c bdwgc/allchblk.c bdwgc/blacklst.c bdwgc/dbg_mlc.c bdwgc/dyn_load.c bdwgc/finalize.c bdwgc/headers.c bdwgc/malloc.c bdwgc/mallocx.c bdwgc/mark.c bdwgc/mach_dep.c bdwgc/mark_rts.c bdwgc/misc.c bdwgc/new_hblk.c bdwgc/obj_map.c bdwgc/os_dep.c bdwgc/ptr_chck.c bdwgc/reclaim.c
GC_OBJS = $(GC_SRCS:%.c=$(OBJ_DIR)/%.o)

STD_SRCS := $(shell find vm/std/libs -name '*.c')
OPT_SRCS := $(shell find vm/opt -name '*.c')
VM_SRCS = vm/ir.c vm/std/std.c vm/lib.c vm/type.c vm/lang/paka.c vm/obj.c $(STD_SRCS) $(OPT_SRCS)
VM_OBJS = $(VM_SRCS:%.c=$(OBJ_DIR)/%.o)

OBJS = $(VM_OBJS) $(GC_OBJS) $(JITC_OBJS)

CFLAGS += $(FLAGS)
LDFLAGS += $(FLAGS)

RUNNER ?= $(BIN_DIR)/minivm

UNAME_S := $(shell uname -s)

LDFLAGS_Darwin = -w -Wl,-pagezero_size,0x4000
LDFLAGS_Linux =  -Wl,--export-dynamic

LDFLAGS := $(LDFLAGS_$(UNAME_S)) -lreadline $(LDFLAGS)

LUA_FILES := $(shell find . -name '*.lua')
LUA_PNGS := $(LUA_FILES:./%.lua=$(RES_DIR)/%.png)

default: all

all: bins libs

test: $(RUNNER)
	@find bench -name '*.lua' | sort | xargs -I{} ./test.sh $(RUNNER) {}

flow: $(LUA_PNGS)
	@find $(RES_DIR) -size 0 -delete

# graph gen

$(LUA_PNGS): $(RUNNER)
	@mkdir -p $(dir $(@))
	sh -c "$(RUNNER) dot -O0 $(@:$(RES_DIR)/%.png=%.lua) | $(DOT) -Tpng > $(@)" || true
	sh -c "$(RUNNER) dot -O3 $(@:$(RES_DIR)/%.png=%.lua) | $(DOT) -Tpng > $(@:%.png=%.opt.png)" || true

# profile guided optimization

gcc-pgo-posix: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-generate -fomit-frame-pointer -fno-stack-protector' $(BIN_DIR)/minivm
	$(MAKE) pgo-runs
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-use -fomit-frame-pointer -fno-stack-protector' $(BIN_DIR)/minivm

clang-pgo-posix: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' $(BIN_DIR)/minivm
	$(MAKE) pgo-runs
	$(LLVM_PROFDATA) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' $(BIN_DIR)/minivm

clang-pgo-windows: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' CFLAGS+=-D_CRT_SECURE_NO_WARNINGS $(BIN_DIR)/minivm.exe
	$(MAKE) pgo-runs
	$(LLVM_PROFDATA) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' CFLAGS+=-D_CRT_SECURE_NO_WARNINGS $(BIN_DIR)/minivm.exe

pgo-runs:
	$(BIN_DIR)/minivm run bench/fib40.lua
	$(BIN_DIR)/minivm run bench/tree16raw.lua
	$(BIN_DIR)/minivm run bench/primecount.lua
	$(BIN_DIR)/minivm run bench/tak.lua

# windows

clang-windows: .dummy
	$(MAKE) -B CC=$(CLANG) OPT='$(OPT)' LDFLAGS='$(LDFLAGS)' CFLAGS='$(CFLAGS) -D_CRT_SECURE_NO_WARNINGS' $(BIN_DIR)/minivm.exe

gcc-windows: .dummy
	$(MAKE) -B CC=$(GCC) OPT='$(OPT)' LDFLAGS='$(LDFLAGS)' CFLAGS='$(CFLAGS) -D_CRT_SECURE_NO_WARNINGS' $(BIN_DIR)/minivm.exe

# binaries

libs: $(BIN_DIR)/libminivm.a

bins: $(BIN_DIR)/minivm

lua luajit lua5.4 lua5.3 lua5.2 lua5.1: .dummy

$(BIN_DIR)/minilua: dynasm/onelua.c 
	@mkdir -p $$(dirname $(@))
	$(CC) -o $(BIN_DIR)/minilua dynasm/onelua.c -lm 

libminivm.lib $(BIN_DIR)/libminivm.lib: $(OBJS)
	@mkdir -p $$(dirname $(@))
	lib /out:$(@) $(OBJS)

libminivm.a $(BIN_DIR)/libminivm.a: $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(AR) cr $(@) $(OBJS)

minivm.exe $(BIN_DIR)/minivm.exe: $(OBJ_DIR)/main/asm.o $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(CC) $(OPT) $(OBJ_DIR)/main/asm.o $(OBJS) -o $(@) $(LDFLAGS)

minivm $(BIN_DIR)/minivm: $(OBJ_DIR)/main/asm.o $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(CC) $(OPT) $(OBJ_DIR)/main/asm.o $(OBJS) -o $(@) -lm $(LDFLAGS)

# intermediate files

$(JITC_OBJS): $(@:$(OBJ_DIR)/%.o=%.dasc) $(LUA)
	@mkdir -p $$(dirname $(@:$(OBJ_DIR)/%.o=%.tmp.c))
	$(LUA) dynasm/dynasm.lua -o $(@:$(OBJ_DIR)/%.o=%.tmp.c) -D X64 -M -L $(@:$(OBJ_DIR)/%.o=%.dasc)
	@mkdir -p $$(dirname $(@))
	$(CC) -w -mabi=sysv -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.tmp.c) -o $(@) $(CFLAGS) -Wno-strict-aliasing

$(PROG_OBJS) $(VM_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -mabi=sysv -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS)

$(GC_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -w -mabi=sysv -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS)

# cleanup

clean: .dummy
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(TMP_DIR)
	find . -name '*.lua' | xargs -I{} rm -f {}.png

# dummy

.dummy:
