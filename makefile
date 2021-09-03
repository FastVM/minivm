
CC=gcc

BIN=bin
LIB=lib

SRCS=vm/minivm.c vm/gc.c main/main.c

OPT=-Os

CFLAGS+=-I. -lm $(OPT)

$(shell mkdir -p $(BIN) $(LIB))

default: all

all: $(BIN)/minivm

$(BIN)/minivm: $(LIB)/libmimalloc.a $(SRCS)
	$(CC) $^ -o $@ $(CFLAGS)

$(LIB)/libmimalloc.a: mimalloc
	$(MAKE) --no-print-directory -f mimalloc.mak LIB=$(LIB)

.dummy:
