CC=gcc
OPT=fast

$(shell mkdir -p bin)

all: minivm asm

asm: vm/asm.c vm/debug.c vm/minivm.c
	$(CC) -o bin/asm $^ -I./ -lm -O$(OPT) $(CFLAGS)

minivm: vm/main.c vm/minivm.c
	$(CC) -o bin/minivm $^ -I./ -lm -O$(OPT) $(CFLAGS)

bench: .dummy
	$(MAKE) --no-print-directory -B asm
	$(CC) -O$(OPT) asm/fprimes.c -lm $(CFLAGS)
	\time -f"for: c fmod: %e" ./a.out
	$(CC) -O$(OPT) asm/iprimes.c $(CFLAGS)
	\time -f"for: c imod: %e" ./a.out
	\time -f"for: asm fmod: %e" ./bin/asm asm/fprimes.mini
	\time -f"for: asm imod: %e" ./bin/asm asm/iprimes.mini

bench-all: .dummy
	echo "info: CC=gcc OPT=fast"
	$(MAKE) --no-print-directory bench CC=gcc OPT=fast
	echo "info: CC=clang OPT=fast"
	$(MAKE) --no-print-directory bench CC=clang OPT=fast
	echo "info: CC=gcc OPT=3"
	$(MAKE) --no-print-directory bench CC=gcc OPT=3
	echo "info: CC=clang OPT=3"
	$(MAKE) --no-print-directory bench CC=clang OPT=3

bench-info:	
	@$(MAKE) --no-print-directory bench-all | grep -E "^(for|info)"


.dummy: 
