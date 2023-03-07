
GCC ?= gcc
LLVM_PROFDATA ?= llvm-profdata
CLANG ?= clang
OPT ?= -O2
HOST_CC ?= $(CC)

LUA ?= luajit

PROG_SRCS := main/asm.c
PROG_OBJS := $(PROG_SRCS:%.c=%.o)

JITC_SRCS := vm/jit/jit.dasc
JITC_OBJS :=  $(JITC_SRCS:%.dasc=%.o)

VM_SRCS := vm/asm.c vm/ir.c vm/info.c vm/interp/int3.c vm/interp/comp.c vm/type.c
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
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-generate -fomit-frame-pointer -fno-stack-protector' ./bin/minivm-asm
	./bin/minivm-asm bench/fib35.vasm || true
	./bin/minivm-asm bench/fib40.vasm || true
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-use -fomit-frame-pointer -fno-stack-protector' ./bin/minivm-asm

clang-pgo-posix: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' ./bin/minivm-asm
	./bin/minivm-asm bench/fib35.vasm || true
	./bin/minivm-asm bench/fib40.vasm || true
	$(LLVM_PROFDATA) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' ./bin/minivm-asm

clang-pgo-windows: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' CFLAGS+=-D_CRT_SECURE_NO_WARNINGS ./bin/minivm-asm.exe
	./bin/minivm-asm.exe bench/fib35.vasm || true
	./bin/minivm-asm.exe bench/fib40.vasm || true
	$(LLVM_PROFDATA) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' CFLAGS+=-D_CRT_SECURE_NO_WARNINGS ./bin/minivm-asm.exe

# windows

clang-windows: .dummy
	$(MAKE) -B CC=$(CLANG) OPT='$(OPT)' LDFLAGS='$(LDFLAGS)' CFLAGS='$(CFLAGS) -D_CRT_SECURE_NO_WARNINGS' bin/minivm-asm.exe

gcc-windows: .dummy
	$(MAKE) -B CC=$(GCC) OPT='$(OPT)' LDFLAGS='$(LDFLAGS)' CFLAGS='$(CFLAGS) -D_CRT_SECURE_NO_WARNINGS' bin/minivm-asm.exe

msvc-windows: main/msvc.c $(VM_SRCS) 
	cl main/msvc.c $(VM_SRCS) /Fe:bin/minivm-asm.exe

# binaries

libs: bin/libminivm.a

bins: bin/minivm-asm

bin/libminivm.lib: $(OBJS)
	@mkdir -p bin
	lib /out:$(@) $(OBJS)

bin/libminivm.a: $(OBJS)
	@mkdir -p bin
	$(AR) cr $(@) $(OBJS)

bin/minivm-asm.exe: main/asm.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/asm.o $(OBJS) -o $(@) $(LDFLAGS)

bin/minivm-asm: main/asm.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/asm.o $(OBJS) -o $(@) -lm $(LDFLAGS)

# clean

clean: gcc-pgo-clean clang-pgo-clean objs-clean

clang-pgo-clean: .dummy
	rm -f *.profdata *.profraw

gcc-pgo-clean: .dummy
	rm -f $(PROG_SRCS:%.c=%.gcda) $(SRCS:%.c=%.gcda)

objs-clean: .dummy
	rm -f $(OBJS) $(PROG_OBJS) bin/minivm-asm libmimivm.a

# intermediate files

$(JITC_OBJS): $(@:%.o=%.tmp.c)
	$(LUA) dynasm/dynasm.lua -o $(@:%.o=%.tmp.c) -D X64 $(@:%.o=%.dasc)
	$(CC) -mabi=sysv -c $(OPT) $(@:%.o=%.tmp.c) -o $(@) $(CFLAGS)

$(PROG_OBJS) $(VM_OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

# dummy

.dummy:
