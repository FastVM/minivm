
CCLUA ?= $(CC)

OPT ?= -O3

SRCS := vm/jump.c vm/int.c vm/gc.c

default: all

all: bin/minivm-asm

bin/minivm-asm:
	@env OPT='$(OPT)' CFLAGS='$(CFLAGS)' $(MAKE) --no-print-directory -f util/makefile bin/minivm-asm

bin: .dummy
	mkdir -p $(@)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw vm/*.tmp.c vm/*/*.tmp.c vm/*.o vm/*/*.o
