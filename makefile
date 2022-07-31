
GCC ?= gcc
CLANG ?= 14
OPT ?= -O2
NUM ?= 3
HOST_CC ?= $(CC)

PROG_SRCS := main/asm.c main/run.c
PROG_OBJS := $(PROG_SRCS:%.c=%.o)

DASM_SRCS := vm/ir/be/jit.dasc
DASM_OBJS := $(DASM_SRCS:%.dasc=%.o)

VM_SRCS := vm/asm.c vm/jump.c vm/int/run.c vm/int/comp.c vm/int/gc.c vm/reguse.c vm/ir/build.c vm/ir/toir.c vm/ir/info.c vm/ir/opt/reg.c
VM_OBJS := $(VM_SRCS:%.c=%.o)

OBJS := $(VM_OBJS) $(DASM_OBJS)

default: all

all: bins libs

# profile guided optimization

gcc-pgo-build: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-generate -fomit-frame-pointer -fno-stack-protector' bins
	./bin/minivm-asm bench/fib35.vasm
	./bin/minivm-asm bench/memfib.vasm
	./bin/minivm-asm bench/primecount.vasm
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-use -fomit-frame-pointer -fno-stack-protector' all

clang-pgo-build: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='clang-$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' bins
	./bin/minivm-asm bench/fib40.vasm
	./bin/minivm-asm bench/memfib.vasm
	./bin/minivm-asm bench/primecount.vasm
	llvm-profdata-$(CLANG) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='clang-$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' all

# binaries

libs: bin/libminivm.a

bins: bin/minivm-run bin/minivm-asm

bin/minilua: luajit/src/host/minilua.c
	@mkdir -p bin
	$(HOST_CC) -o $(@) luajit/src/host/minilua.c -lm

bin/libminivm.a: $(OBJS)
	ar cr $(@) $(OBJS)

bin/minivm-run: main/run.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/run.o $(OBJS) -o $(@) $(LDFLAGS)

bin/minivm-asm: main/asm.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/asm.o $(OBJS) -o $(@) $(LDFLAGS)

# clean

clean: gcc-pgo-clean clang-pgo-clean objs-clean

clang-pgo-clean: .dummy
	rm -f *.profdata *.profraw

gcc-pgo-clean: .dummy
	rm -f $(PROG_SRCS:%.c=%.gcda) $(SRCS:%.c=%.gcda)

objs-clean: .dummy
	rm -f main/asm.o $(PROG_OBJS) $(OBJS) bin/minivm-asm libmimivm.a

# intermediate files

$(PROG_OBJS) $(VM_OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

$(DASM_OBJS): $(@:%.o=%.dasc) luajit/dynasm/dynasm.lua | bin/minilua
	bin/minilua luajit/dynasm/dynasm.lua -o $(@:%.o=%.tmp.c) $(@:%.o=%.dasc)
	$(CC) -c $(OPT) $(@:%.o=%.tmp.c) -o $(@) $(CFLAGS)

# dummy

.dummy:
