
GCC ?= gcc
LLVM_PROFDATA ?= llvm-profdata
CLANG ?= clang
OPT ?= -O2
HOST_CC ?= $(CC)


PROG_SRCS := main/asm.c main/run.c main/js.c
PROG_OBJS := $(PROG_SRCS:%.c=%.o)

VM_SRCS := vm/asm.c vm/gc.c vm/ir/build.c vm/ir/toir.c vm/ir/info.c vm/ir/const.c vm/ir/be/int3.c vm/ir/be/js.c vm/ir/be/spall.c
VM_OBJS := $(VM_SRCS:%.c=%.o)

OBJS := $(VM_OBJS)

default: all

all: bins libs

format: .dummy
	find . -name '*.c' | xargs -I FILENAME clang-format -style=file -i FILENAME
	find . -name '*.h' | xargs -I FILENAME clang-format -style=file -i FILENAME

# profile guided optimization

gcc-pgo-build: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-generate -fomit-frame-pointer -fno-stack-protector' bins
	./bin/minivm-asm bench/fib40.vasm || true
	./bin/minivm-asm bench/memfib35.vasm || true
	./bin/minivm-asm bench/primecount.vasm || true
	$(MAKE) -B CC='$(GCC)' OPT='$(OPT) -fprofile-use -fomit-frame-pointer -fno-stack-protector' all

clang-pgo-build: .dummy
	$(MAKE) clean
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-instr-generate=profraw.profraw -fomit-frame-pointer -fno-stack-protector' bins
	./bin/minivm-asm bench/fib40.vasm || true
	./bin/minivm-asm bench/memfib35.vasm || true
	./bin/minivm-asm bench/primecount.vasm || true
	$(LLVM_PROFDATA) merge -o profdata.profdata profraw.profraw
	$(MAKE) -B CC='$(CLANG)' OPT='$(OPT) -fprofile-use=profdata.profdata -fomit-frame-pointer -fno-stack-protector' all

# binaries

libs: bin/libminivm.a

bins: bin/minivm-run bin/minivm-asm bin/vm2js

bin/libminivm.a: $(OBJS)
	@mkdir -p bin
	ar cr $(@) $(OBJS)

bin/vm2js: main/js.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/js.o $(OBJS) -o $(@) -lm $(LDFLAGS)

bin/minivm-run: main/run.o $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) main/run.o $(OBJS) -o $(@) -lm $(LDFLAGS)

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
	rm -f main/asm.o $(PROG_OBJS) $(OBJS) bin/minivm-asm libmimivm.a

# intermediate files

$(PROG_OBJS) $(VM_OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

# dummy

.dummy:
