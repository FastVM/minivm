OPT ?= -Os

default: all

all: minivm dis

minivm: .dummy
	$(CC) $(OPT) vm/minivm.c -o minivm $(CFLAGS)

dis: .dummy
	$(CC) $(OPT) vm/dis.c -o dis $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm dis
