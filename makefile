OPT ?= -Os

VM_MAIN = main/main.c

CFILES = vm/vm.c vm/state.c vm/gc.c $(VM_MAIN)
OBJS = $(CFILES:%.c=%.o)

OUT = minivm

STDLIB=-lc -lm

default: $(OUT)

libminivm.a: $(OBJS)
	ar rcs libminivm.a $(OBJS)

$(OUT): $(OBJS)
	: mkdir -p bin
	$(CC) $(OPT) $(OBJS) -o $(OUT) $(STDLIB) $(LFLAGS)

$(OBJS): $(@:%.o=%.c) 
	$(CC) -c $(OPT) -o $@ $(@:%.o=%.c) $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) $(OUT)

cosmo:
	test ! -f cosmopolitan.zip && wget https://justine.lol/cosmopolitan/cosmopolitan.zip || true
	test ! -d cosmo && mkdir cosmo && cd cosmo && 7z x ../cosmopolitan.zip || true

minivm.com: cosmo
	$(CC) $(OPT) -static -fno-pie -mno-red-zone -nostdlib -nostdinc \
		-fno-omit-frame-pointer -o minivm.com.dbg $(CFILES) -Wl,--gc-sections -fuse-ld=bfd \
		-Wl,-T,cosmo/ape.lds cosmo/crt.o cosmo/ape.o cosmo/cosmopolitan.a $(CFLAGS)
	objcopy -S -O binary minivm.com.dbg minivm.com
