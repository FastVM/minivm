OPT ?= -Os

OUT = minivm

default: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(OPT) single.c -o $(OUT) -lc -lgc $(LFLAGS) -L/usr/local/lib/ -L/usr/local/

clean: .dummy
	rm -f $(OBJS) $(OUT)
