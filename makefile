
GCC ?= gcc
CLANG ?= 14
OPT ?= -O2
NUM ?= 3

PROG_SRCS := main/asm.c main/run.c
PROG_OBJS := $(PROG_SRCS:%.c=%.o)

SRCS := vm/asm.c vm/jump.c vm/int/run.c vm/int/comp.c vm/int/gc.c vm/reguse.c vm/ir/build.c vm/ir/toir.c vm/ir/opt/const.c vm/ir/opt/reg.c vm/ir/opt/arg.c vm/ir/opt/dead.c vm/ir/info.c vm/ir/be/js.c vm/ir/be/lua.c vm/ir/be/jit.c
OBJS := $(SRCS:%.c=%.o)

default: all

all: bins libs

bins: bin/minivm-run bin/minivm-asm

libs: bin/libminivm.a

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

$(PROG_OBJS) $(OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

bin/libminivm.a: $(OBJS)
	ar cr $(@) $(OBJS)

bin/minivm-run: main/run.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/run.o $(OBJS) -o $(@) $(LDFLAGS)

bin/minivm-asm: main/asm.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/asm.o $(OBJS) -o $(@) $(LDFLAGS)

.dummy:

clean: gcc-pgo-clean clang-pgo-clean objs-clean

clang-pgo-clean: .dummy
	rm -f *.profdata *.profraw

gcc-pgo-clean: .dummy
	rm -f $(PROG_SRCS:%.c=%.gcda) $(SRCS:%.c=%.gcda)

objs-clean: .dummy
	rm -f main/asm.o $(PROG_OBJS) $(OBJS) bin/minivm-asm libmimivm.a
