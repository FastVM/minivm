
CC=clang
DC=gdc
LD=$(CC)

BIN=bin
LIB=lib

OPT=-O3

P=-p

DFILES:=$(shell find optimize -type f -name '*.d') main/optimize.d
CFILES=vm/vm.c vm/gc.c vm/sys.c
DOBJS=$(patsubst %.d,$(LIB)/%.o,$(DFILES))
COBJS=$(patsubst %.c,$(LIB)/%.o,$(CFILES))
OBJS=$(DOBJS) $(COBJS)


ifeq ($(DC),gdc)
DO=-o
LDO=-o
else
XLFLAGS=$(DL)-lphobos2-ldc $(DL)-ldruntime-ldc $(DL)-lm $(DL)-lz $(DL)-ldl
else
DO=-of
ifeq ($(DC),$(LD))
LDO=-of
else
LDO=-o
endif
endif

MIMALLOC=$(DL)-lmimalloc -DVM_USE_MIMALLOC $(DL)-lpthread

default: all

all: $(BIN)/minivm $(BIN)/optimize

minivm $(BIN)/minivm: $(CFILES) main/main.c 
	$(CC) $^ -o $@ $(CFLAGS) -I. -lm $(OPT) $(MIMALLOC)

optimize $(BIN)/optimize: $(OBJS)
	@mkdir $(P) $(BIN)
	$(LD) $^ $(LDO)$(BIN)/optimize $(patsubst %,$(DL)%,$(LFLAGS)) $(XLFLAGS)

$(DOBJS): $(patsubst $(LIB)/%.o,%.d,$@)
	@mkdir $(P) $(dir $@) 
	$(DC) -c $(OPT) $(DO)$@ $(patsubst $(LIB)/%.o,%.d,$@) -I. $(DFLAGS)

$(COBJS) $(LIB)/minivm/main/main.o $(LIB)/minivm/vm/sys.o: $(patsubst $(LIB)/%.o,%.c,$@)
	@mkdir $(P) $(dir $@) 
	$(CC) $(FPIC) -c $(OPT) -o $@ $(patsubst $(LIB)/%.o,%.c,$@) -I./minivm $(CFLAGS) 

.dummy:
