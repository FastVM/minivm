
CC=gcc
MICC=$(CC)

BIN=bin
LIB=lib

SRCS=vm/minivm.c vm/gc.c main/main.c

OPT=-O3

$(shell mkdir -p $(BIN) $(LIB))

default: all

all: $(BIN)/minivm

$(BIN)/minivm: $(SRCS) $(LIB)/libmimalloc.a
	$(CC) $^ -o $@ $(CFLAGS)  -I. -lm $(OPT)

$(LIB)/libmimalloc.a: mimalloc
	@$(MAKE) --no-print-directory -f mimalloc.mak CC=$(MICC)
	cp minivm/lib/libmimalloc.a $@

.dummy:
