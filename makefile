
CC=clang
OPT=-O3

BIN=bin
P=-p

CFILES=vm/vm.c vm/gc.c vm/sys.c

MIMALLOC=$(DL)-lmimalloc -DVM_USE_MIMALLOC $(DL)-lpthread

default: all

all: $(BIN)/minivm

minivm $(BIN)/minivm: $(CFILES) main/main.c 
	@mkdir $(P) $(BIN)
	$(CC) $^ -o $@ $(CFLAGS) -I. -lm $(OPT) $(MIMALLOC)

.dummy:
