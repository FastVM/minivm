
CCLUA ?= $(CC)

OPT ?= -Os

SRCS := vm/jump.c vm/int.c vm/gc.c

default: all

all: bin/minivm-asm

bin/minivm-asm:
	@$(MAKE) --no-print-directory -f util/makefile bin/minivm-asm

bin: .dummy
	mkdir -p $(@)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw vm/*.tmp.c vm/*/*.tmp.c vm/*.o vm/*/*.o
