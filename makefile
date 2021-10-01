
CC=gcc

BIN=bin
LIB=lib

SRCS=vm/vm.c vm/gc.c main/main.c

OPT=-O3

$(shell mkdir -p $(BIN) $(LIB))

default: all

all: $(BIN)/minivm

$(BIN)/minivm: $(SRCS)
	$(CC) $^ -o $@ $(CFLAGS)  -I. -lm $(OPT)

.dummy:
