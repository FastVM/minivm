OPT ?= -Os

OUT = minivm

default: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(OPT) minivm.c -o $(OUT) -lc $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) $(OUT)
