
OPT ?= -Os

ARCH := x86

SRCS := vm/main.c vm/arch/$(ARCH).tmp.c

default: all

all: minivm

./minilua: luajit/src/host/minilua.c
	$(CC) -o minilua luajit/src/host/minilua.c -lm

vm/arch/x86.tmp.c: vm/arch/x86.dasc ./minilua
	./minilua luajit/dynasm/dynasm.lua -o vm/arch/x86.tmp.c vm/arch/x86.dasc

vm/arch/opt.tmp.c: vm/arch/opt.c
	cp vm/arch/opt.c vm/arch/opt.tmp.c


minivm: $(SRCS)
	$(CC) $(OPT) $(SRCS) -o minivm $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw
