
CC=gcc

BIN=bin
LIB=lib

SRCS=vm/minivm.c vm/gc.c vm/ffiop.c main/main.c

OPT=-O3

$(shell mkdir -p $(BIN) $(LIB))

default: all

all: $(BIN)/minivm

$(BIN)/minivm: $(SRCS)
	$(CC) $^ -o $@ $(CFLAGS) -lffi -ldl -I. -lm $(OPT)

# $(LIB)/libmimalloc.a: mimalloc
# 	$(MAKE) --no-print-directory -f mimalloc.mak LIB=$(LIB)

.dummy:
