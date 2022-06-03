
GCC=gcc
OPT ?= -O2

SRCS := util/main.c vm/asm.c vm/jump.c vm/int/run.c vm/int/comp.c vm/int/gc.c vm/reguse.c
OBJS := $(SRCS:%.c=%.o)

default: gcc-pgo-build

gcc-pgo-build: .dummy
	$(MAKE) clean
	$(MAKE) CC='$(GCC)' OPT='$(OPT) -flto -fprofile-generate -fomit-frame-pointer -fno-stack-protector -fno-ssa-phiopt' bin/minivm-asm
	./bin/minivm-asm bench/fib35.vasm
	./bin/minivm-asm bench/primecount.vasm
	./bin/minivm-asm bench/fastfib.vasm
	$(MAKE) objs-clean
	$(MAKE) CC='$(GCC)' OPT='$(OPT) -flto -fprofile-use -fomit-frame-pointer -fno-stack-protector -fno-ssa-phiopt' bin/minivm-asm

$(OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

bin/minivm-asm: $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) $(OBJS) -o $(@) -l:libgmp.a $(LDFLAGS)

.dummy:

clean: gcc-pgo-clean objs-clean

gcc-pgo-clean: .dummy
	rm -f $(SRCS:%.c=%.gcda)

objs-clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw vm/*.o vm/*/*.o
