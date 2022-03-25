
OPT ?= -Os

SRCS := vm/vm.c vm/main.c

default: all

all: minivm

minivm: .dummy
	$(CC) $(OPT) $(SRCS) -o minivm $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw
