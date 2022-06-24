
GCC ?= gcc
CLANG ?= 14
OPT ?= -O2
NUM ?= 3

SRCS := vm/asm.c vm/jump.c vm/int/run.c vm/int/comp.c vm/int/gc.c vm/reguse.c vm/ir/build.c vm/ir/toir.c vm/ir/opt/const.c vm/ir/opt/arg.c vm/ir/opt/dead.c vm/ir/info.c vm/ir/be/js.c vm/ir/be/lua.c
OBJS := $(SRCS:%.c=%.o)

default: bin/minivm-asm libminivm.a

gcc-pgo-build: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-generate -fomit-frame-pointer -fno-stack-protector' bin/minivm-asm
	./bin/minivm-asm bench/fib35.vasm
	./bin/minivm-asm bench/memfib.vasm
	./bin/minivm-asm bench/primecount.vasm
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-use -fomit-frame-pointer -fno-stack-protector' libminivm.a bin/minivm-asm

clang-pgo-build: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='clang-$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' bin/minivm-asm
	./bin/minivm-asm bench/fib40.vasm
	./bin/minivm-asm bench/memfib.vasm
	./bin/minivm-asm bench/primecount.vasm
	llvm-profdata-$(CLANG) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='clang-$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' libminivm.a bin/minivm-asm

util/main.o $(OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

libminivm.a: $(OBJS)
	ar cr $(@) $(OBJS)

bin/minivm-asm: util/main.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) util/main.o $(OBJS) -o $(@) $(LDFLAGS)

.dummy:

clean: gcc-pgo-clean clang-pgo-clean objs-clean

clang-pgo-clean: .dummy
	rm -f *.profdata *.profraw

gcc-pgo-clean: .dummy
	rm -f $(SRCS:%.c=%.gcda)

objs-clean: .dummy
	rm -f util/main.o $(OBJS) bin/minivm-asm libmimivm.a vm/*.o vm/*/*.o
