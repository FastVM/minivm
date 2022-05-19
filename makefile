
OPT ?= -O2

SRCS := util/asm.c vm/jump.c vm/gc.c vm/int/run.c vm/int/comp.c vm/reguse.c
OBJS := $(SRCS:%.c=%.o)

default: all

all: bin/minivm-asm

$(OBJS): $(@:%.o=%.c)
	$(CC) -c $(OPT) $(@:%.o=%.c) -o $(@) $(CFLAGS)

bin/minivm-asm: $(OBJS)
	@mkdir -p bin
	$(CC) $(OPT) $(OBJS) -o $(@) -lm $(LDFLAGS)

bin: .dummy
	mkdir -p $(@)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm minivm.profdata minivm.profraw vm/*.tmp.c vm/*/*.tmp.c vm/*.o vm/*/*.o
