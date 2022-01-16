OPT ?= -Os

OUT = minivm

default: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(OPT) minivm.c -o $(OUT) -lc $(LFLAGS) -L/usr/local/lib/ -L/usr/local/

.dummy:

clean: .dummy
	rm -f $(OBJS) $(OUT)
