
GCC ?= gcc
LLVM_PROFDATA ?= llvm-profdata
CLANG ?= clang
OPT ?= -O2
HOST_CC ?= $(CC)

LUA ?= bin/minilua

PROG_SRCS := main/asm.c
PROG_OBJS := $(PROG_SRCS:%.c=%.o)

JITC_SRCS := vm/jit/x64.dasc
JITC_OBJS :=  $(JITC_SRCS:%.dasc=%.o)

VM_SRCS := vm/ir.c vm/type.c vm/lang/paka.c
VM_OBJS := $(VM_SRCS:%.c=%.o)

OBJS := $(VM_OBJS) $(JITC_OBJS)

default: all

all: bins libs

format: .dummy
	find . -name '*.c' | xargs -I FILENAME clang-format -style=file -i FILENAME
	find . -name '*.h' | xargs -I FILENAME clang-format -style=file -i FILENAME

# profile guided optimization

gcc-pgo-posix: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-generate -fomit-frame-pointer -fno-stack-protector' ./bin/minivm
	./bin/minivm bench/fib35.vasm || true
	./bin/minivm bench/fib40.vasm || true
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-use -fomit-frame-pointer -fno-stack-protector' ./bin/minivm

clang-pgo-posix: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' ./bin/minivm
	./bin/minivm bench/fib35.vasm || true
	./bin/minivm bench/fib40.vasm || true
	$(LLVM_PROFDATA) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' ./bin/minivm

clang-pgo-windows: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' CFLAGS+=-D_CRT_SECURE_NO_WARNINGS ./bin/minivm.exe
	./bin/minivm.exe bench/fib35.vasm || true
	./bin/minivm.exe bench/fib40.vasm || true
	$(LLVM_PROFDATA) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' CFLAGS+=-D_CRT_SECURE_NO_WARNINGS ./bin/minivm.exe

# windows

clang-windows: .dummy
	$(MAKE) -B CC=$(CLANG) OPT='$(OPT)' LDFLAGS='$(LDFLAGS)' CFLAGS='$(CFLAGS) -D_CRT_SECURE_NO_WARNINGS' bin/minivm.exe

gcc-windows: .dummy
	$(MAKE) -B CC=$(GCC) OPT='$(OPT)' LDFLAGS='$(LDFLAGS)' CFLAGS='$(CFLAGS) -D_CRT_SECURE_NO_WARNINGS' bin/minivm.exe

msvc-windows: main/msvc.c $(VM_SRCS) 
	cl main/msvc.c $(VM_SRCS) /Fe:bin/minivm.exe

# binaries

libs: bin/libminivm.a

bins: bin/minivm

lua luajit lua5.4 lua5.3 lua5.2 lua5.1: .dummy

bin/minilua: dynasm/onelua.c
	@mkdir -p bin
	$(CC) -o bin/minilua dynasm/onelua.c -lm 

bin/libminivm.lib: $(OBJS)
	@mkdir -p bin
	lib /out:$(@) $(OBJS)

bin/libminivm.a: $(OBJS)
	@mkdir -p bin
	$(AR) cr $(@) $(OBJS)

bin/minivm.exe: main/asm.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/asm.o $(OBJS) -o $(@) $(LDFLAGS)

bin/minivm: main/asm.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/asm.o $(OBJS) -o $(@) -lm $(LDFLAGS)

# clean

clean: gcc-pgo-clean clang-pgo-clean objs-clean

clang-pgo-clean: .dummy
	rm -f *.profdata *.profraw

gcc-pgo-clean: .dummy
	rm -f $(PROG_SRCS:%.c=%.gcda) $(SRCS:%.c=%.gcda)

objs-clean: .dummy
	rm -f $(OBJS) $(PROG_OBJS) bin/minivm libmimivm.a

# intermediate files

$(JITC_OBJS): $(@:%.o=%.tmp.c) $(LUA)
	$(LUA) dynasm/dynasm.lua -o $(@:%.o=%.tmp.c) -D X64 -M -L $(@:%.o=%.dasc)
	$(CC) -mabi=sysv -c $(OPT) $(@:%.o=%.tmp.c) -o $(@) $(CFLAGS)

$(PROG_OBJS) $(VM_OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

# dummy

.dummy:
