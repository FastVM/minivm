
GCC ?= gcc
LLVM_PROFDATA ?= llvm-profdata
CLANG ?= clang
OPT ?= -O2
HOST_CC ?= $(CC)

BUILD_DIR ?= build
OBJ_DIR ?= $(BUILD_DIR)/obj
TMP_DIR ?= $(BUILD_DIR)/tmp
BIN_DIR ?= $(BUILD_DIR)/bin

LUA ?= $(BIN_DIR)/minilua

PROG_SRCS := main/asm.c
PROG_OBJS := $(PROG_SRCS:%.c=$(OBJ_DIR)/%.o)

JITC_SRCS := vm/jit/x64.dasc
JITC_OBJS :=  $(JITC_SRCS:%.dasc=$(OBJ_DIR)/%.o)

VM_SRCS := vm/ir.c vm/type.c vm/lang/paka.c
VM_OBJS := $(VM_SRCS:%.c=$(OBJ_DIR)/%.o)

OBJS := $(VM_OBJS) $(JITC_OBJS)

default: all

all: bins libs

format: .dummy
	find . -name '*.c' | xargs -I FILENAME clang-format -style=file -i FILENAME
	find . -name '*.h' | xargs -I FILENAME clang-format -style=file -i FILENAME

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
	$(BIN_DIR)/minivm bench/fib40.lua
	$(BIN_DIR)/minivm bench/primecount.lua
	$(BIN_DIR)/minivm bench/tak.lua

# windows

clang-windows: .dummy
	$(MAKE) -B CC=$(CLANG) OPT='$(OPT)' LDFLAGS='$(LDFLAGS)' CFLAGS='$(CFLAGS) -D_CRT_SECURE_NO_WARNINGS' $(BIN_DIR)/minivm.exe

gcc-windows: .dummy
	$(MAKE) -B CC=$(GCC) OPT='$(OPT)' LDFLAGS='$(LDFLAGS)' CFLAGS='$(CFLAGS) -D_CRT_SECURE_NO_WARNINGS' $(BIN_DIR)/minivm.exe

msvc-windows: main/msvc.c $(VM_SRCS) 
	cl main/msvc.c $(VM_SRCS) /Fe:$(BIN_DIR)/minivm.exe

# binaries

libs: $(BIN_DIR)/libminivm.a

bins: $(BIN_DIR)/minivm

lua luajit lua5.4 lua5.3 lua5.2 lua5.1: .dummy

$(BIN_DIR)/minilua: dynasm/onelua.c 
	@mkdir -p $$(dirname $(@))
	$(CC) -o $(BIN_DIR)/minilua dynasm/onelua.c -lm 

$(BIN_DIR)/libminivm.lib: $(OBJS)
	@mkdir -p $$(dirname $(@))
	lib /out:$(@) $(OBJS)

$(BIN_DIR)/libminivm.a: $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(AR) cr $(@) $(OBJS)

$(BIN_DIR)/minivm.exe: $(OBJ_DIR)/main/asm.o $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(CC) $(OPT) $(OBJ_DIR)/main/asm.o $(OBJS) -o $(@) $(LDFLAGS)

$(BIN_DIR)/minivm: $(OBJ_DIR)/main/asm.o $(OBJS)
	@mkdir -p $$(dirname $(@))
	$(CC) $(OPT) $(OBJ_DIR)/main/asm.o $(OBJS) -o $(@) -lm $(LDFLAGS)

# intermediate files

$(JITC_OBJS): $(@:$(OBJ_DIR)/%.o=%.dasc) $(LUA)
	@mkdir -p $$(dirname $(@:$(OBJ_DIR)/%.o=%.tmp.c))
	$(LUA) dynasm/dynasm.lua -o $(@:$(OBJ_DIR)/%.o=%.tmp.c) -D X64 -M -L $(@:$(OBJ_DIR)/%.o=%.dasc)
	@mkdir -p $$(dirname $(@))
	$(CC) -mabi=sysv -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.tmp.c) -o $(@) $(CFLAGS)

$(PROG_OBJS) $(VM_OBJS): $(@:$(OBJ_DIR)/%.o=%.c)
	@mkdir -p $$(dirname $(@))
	$(CC) -c $(OPT) $(@:$(OBJ_DIR)/%.o=%.c) -o $(@) $(CFLAGS)

# cleanup

clean: .dummy
	rm -rf $(BIN_DIR) $(OBJ_DIR) $(TMP_DIR)



# dummy

.dummy:
