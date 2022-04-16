
CCLUA ?= $(CC)

OPT ?= -Os

SRCS := vm/main.c vm/jump.c vm/int.c vm/gc.c

default: all

all: bin/minivm bin/minivm-dis bin/minivm-color

bin/luajit-minilua: luajit/src/host/minilua.c | bin
	$(CCLUA) -o $(@) $(^) -lm

bin/minivm: $(SRCS) | bin
	$(CC) $(OPT) $(SRCS) -o bin/minivm $(CFLAGS)

bin/minivm-color:
	$(MAKE) -f util/makefile bin/minivm-color

bin/minivm-dis:
	$(MAKE) -f util/makefile bin/minivm-dis

bin: .dummy
	mkdir -p $(@)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw vm/*.tmp.c vm/*/*.tmp.c vm/*.o vm/*/*.o
