OUT=minivm

OPT_C=-Ofast

CFILES=vm/vm.c vm/state.c vm/gc.c main/main.c
OBJS=$(CFILES:%.c=%.o)

default: $(OUT)

$(OUT): $(OBJS)
	: mkdir -p bin
	$(CC) $(OBJS) -o $(OUT) -lm $(LFLAGS)

$(OBJS): $(@:%.o=%.c) $(basename $@)
	$(CC) -c $(OPT_C) -o $@ $(@:%.o=%.c) $(CFLAGS)

info:
	@echo $(XCUR)

.dummy:

clean: .dummy
	rm -f $(OBJS) $(OUT)
