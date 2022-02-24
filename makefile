OPT ?= -Os

default: all

all: minivm dis

minivm: .dummy
	$(CC) $(OPT) minivm.c -o minivm $(CFLAGS)

dis: .dummy
	$(CC) $(OPT) dis.c -o dis $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm dis
