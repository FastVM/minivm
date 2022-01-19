OPT ?= -Os

OUT = minivm

default: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(OPT) minivm.c -o $(OUT) $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) $(OUT)
