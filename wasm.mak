CWD:=$(shell pwd)
BIN:=$(CWD)/bin
LIB:=$(CWD)/lib

CLANG=clang
LLC=llc
WASMLD=wasm-ld
OPT=opt

SRCS:=vm/minivm.c vm/gc.c vm/wasm.c

OBJS=$(patsubst %.c,$(LIB)/%.o,$(SRCS))

CFLAGS=--target=wasm32 -Os -ffast-math -emit-llvm -c -S -I. -DVM_NO_STD -nostdlib -g

default: all

all: $(BIN)/minivm.js

$(BIN)/minivm.js: $(BIN)/minivm.wasm
	cp main/minivm.js $@
	chmod +x $@

$(BIN)/minivm.wasm: $(OBJS)
	@mkdir -p $(basename $@)
	$(WASMLD) --export=vm_xrun --export=vm_xadd --export=vm_xset_putchar --allow-undefined -s $(OBJS) -o $@

$(OBJS): $(patsubst $(LIB)/%.ll,%.c,$@)
	@mkdir -p $(basename $@)
	$(CLANG) $(CFLAGS) $(patsubst $(LIB)/%.o,%.c,$@) -o $(patsubst $(LIB)/%.o,$(LIB)/%.bc,$@)
	$(LLC) -march=wasm32 -filetype=obj $(patsubst $(LIB)/%.o,$(LIB)/%.bc,$@) -o $@
	