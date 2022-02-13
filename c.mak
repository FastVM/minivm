OPT ?= -Os

OUT = minivm

default: $(OUT)

$(OUT): .dummy
	$(CC) $(OPT) minivm.c -o $(OUT) $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) $(OUT)
