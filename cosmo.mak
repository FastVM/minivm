
CC=gcc
BIN=bin

SRCS=vm/vm.c vm/gc.c main/main.c

COSMO=ape.lds cosmopolitan.h crt.o ape.o cosmopolitan.a

default: all

all: $(COSMO)
	$(CC) -g -Os -static -nostdlib -nostdinc -fno-pie -no-pie -mno-red-zone -fno-omit-frame-pointer -pg -mnop-mcount -o $(BIN)/minivm.com.dbg $(SRCS) -fuse-ld=bfd -Wl,-T,ape.lds -include cosmopolitan.h crt.o ape.o cosmopolitan.a -I. -DVM_COSMO
	objcopy -S -O binary $(BIN)/minivm.com.dbg $(BIN)/minivm.com
	cp $(BIN)/minivm.com $(BIN)/minivm.out

$(COSMO):
	curl https://justine.lol/cosmopolitan/$@ -o $@
