CC=gcc
OPT=fast
BIN=bin

$(shell mkdir -p $(BIN))

all: minivm

minivm: vm/main.c vm/minivm.c vm/asm.c vm/dis.c vm/debug.c vm/gc.c
	$(CC) -o $(BIN)/minivm $^ -I./ -lm -O$(OPT) $(CFLAGS)

lib: vm/main.c vm/minivm.c vm/asm.c vm/dis.c vm/debug.c vm/gc.c
	$(CC) -shared -fPIC -o $(BIN)/libminivm.so $^ -I./ -lm -O$(OPT) $(CFLAGS)

.dummy: 
