
GCC ?= gcc
OPT ?= -O2
NUM ?= 3

SRCS := util/main.c vm/asm.c vm/jump.c vm/int/run.c vm/int/comp.c vm/int/gc.c vm/reguse.c
OBJS := $(SRCS:%.c=%.o)

default: gcc-pgo-build

gcc-pgo-build: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-generate -fomit-frame-pointer -fno-stack-protector' bin/minivm-asm
	./bin/minivm-asm bench/fib40.vasm
	./bin/minivm-asm bench/memfib.vasm
	./bin/minivm-asm bench/primecount.vasm
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-use -fomit-frame-pointer -fno-stack-protector' libminivm.a bin/minivm-asm

gcc-pgo-mid: .dummy
	$(MAKE) gcc-pgo-clean
	: ./bin/minivm-asm bench/fib40.vasm
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-use -fprofile-generate -fomit-frame-pointer -fno-stack-protector' bin/minivm-asm

$(OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

libminivm.a: $(OBJS)
	ar cr $(@) $(OBJS)

bin/minivm-asm: $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) $(OBJS) -o $(@) $(LDFLAGS)

.dummy:

clean: gcc-pgo-clean objs-clean

gcc-pgo-clean: .dummy
	rm -f $(SRCS:%.c=%.gcda)

objs-clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw vm/*.o vm/*/*.o
