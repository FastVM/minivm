
OPT ?= -Os

ARCH := x86

SRCS_x86 := vm/main.c vm/arch/x86.tmp.c
SRCS_dis := vm/main.c vm/jump.c vm/arch/dis.c
SRCS_check := vm/main.c vm/jump.c vm/arch/check.c

SRCS := $(SRCS_$(ARCH))

default: all

all: minivm

./minilua: luajit/src/host/minilua.c
	$(CC) -o minilua luajit/src/host/minilua.c -lm

vm/arch/x86.tmp.c: vm/arch/x86.dasc ./minilua
	./minilua luajit/dynasm/dynasm.lua -o vm/arch/x86.tmp.c vm/arch/x86.dasc

minivm: $(SRCS)
	$(CC) $(OPT) $(SRCS) -o minivm $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw
