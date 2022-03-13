
OPT ?= -Os

SRCS := vm/vm.c vm/gc.c vm/main.c

default: all

all: minivm dis

minivm: .dummy
	$(CC) $(OPT) $(SRCS) -o minivm $(CFLAGS)

dis: .dummy
	$(CC) $(OPT) vm/dis.c -o dis $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm dis minivm.profdata minivm.profraw
