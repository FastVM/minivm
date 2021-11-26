OUT = minivm

OPT ?= -Ofast

CFILES = vm/vm.c vm/state.c vm/gc.c main/main.c
OBJS = $(CFILES:%.c=%.o)

LTO ?= 0

LTO_1 = -flto
LTO_0 = 
CFLAGS += $(LTO_$(LTO))
LFLAGS += $(LTO_$(LTO)) 

default: $(OUT)

$(OUT): $(OBJS)
	: mkdir -p bin
	$(CC) $(OBJS) -o $(OUT) -lc -lm $(LFLAGS)

$(OBJS): $(@:%.o=%.c) 
	$(CC) -c $(OPT) -o $@ $(@:%.o=%.c) $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) $(OUT)

cosmo:
	test cosmo || test cosmopolitan.zip || wget https://justine.lol/cosmopolitan/cosmopolitan.zip
	test cosmo || (mkdir -p cosmo && cd cosmo && 7z x ../cosmopolitan.zip)

minivm.com: cosmo
	$(CC) $(OPT) -static -fno-pie -mno-red-zone -nostdlib -nostdinc \
		-fno-omit-frame-pointer -o minivm.com.dbg $(CFILES) -Wl,--gc-sections -fuse-ld=bfd \
		-Wl,-T,cosmo/ape.lds cosmo/crt.o cosmo/ape.o cosmo/cosmopolitan.a
	objcopy -S -O binary minivm.com.dbg minivm.com
