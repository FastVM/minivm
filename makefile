
OPT ?= -Os

SRCS := vm/main.c vm/jit.c

default: all

all: minivm

./minilua: luajit/src/host/minilua.c
	$(CC) -o minilua -lm luajit/src/host/minilua.c

vm/jit.c: vm/jit.dasc ./minilua
	./minilua luajit/dynasm/dynasm.lua -o vm/jit.c vm/jit.dasc -D X64

minivm: $(SRCS)
	$(CC) $(OPT) $(SRCS) -o minivm $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw
