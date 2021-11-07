OPT_C=-Ofast
OPT_D=

MICC=$(CC)

P=-p
FPIC=-fPIC

CFILES=vm/vm.c vm/state.c vm/gc.c main/main.c
OBJS=$(CFILES:%.c=%.o)

BINS=bin/minivm

default: bin/minivm

minivm bin/minivm: $(OBJS)
	: mkdir bin
	$(CC) $(OBJS) -obin/minivm -lm $(LFLAGS)

$(OBJS): $(@:%.o=%.c) $(basename $@)
	$(CC) $(FPIC) -c $(OPT_C) -o $@ $(@:%.o=%.c) $(CFLAGS)

info:
	@echo $(XCUR)

.dummy:

clean: .dummy
	rm -f $(OBJS) bin/minivm