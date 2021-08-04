all: minivm

COSMO=ape.lds cosmopolitan.h cosmopolitan.a crt.o ape.o
SRCS=vm/main.c vm/minivm.c vm/asm.c vm/dis.c vm/debug.c vm/gc.c

$(COSMO):
	curl https://justine.lol/cosmopolitan/$@ -o $@

minivm: $(SRCS) $(COSMO)
	gcc -g -Os -static -fno-pie -no-pie -mno-red-zone -nostdlib -nostdinc \
		-fno-omit-frame-pointer -mnop-mcount \
		-o bin/minivm.com.dbg $(SRCS) -Wl,--gc-sections -fuse-ld=bfd \
		-Wl,-T,ape.lds -include cosmopolitan.h crt.o ape.o cosmopolitan.a \
		-I./ -DNO_HEADERS_AT_ALL
	objcopy -S -O binary bin/minivm.com.dbg bin/minivm.com
