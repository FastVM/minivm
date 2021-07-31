CC=gcc
OPT=fast

$(shell mkdir -p bin)

all: minivm

minivm: vm/main.c vm/minivm.c vm/asm.c vm/dis.c vm/debug.c vm/gc.c
	$(CC) -o bin/minivm $^ -I./ -lm -O$(OPT) $(CFLAGS)

.dummy: 
