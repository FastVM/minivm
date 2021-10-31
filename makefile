
OPT=-O3

BIN=bin
LIB=lib
P=-p

CFILES=vm/vm.c vm/io.c vm/gc.c vm/obj/map.c

default: all

all: $(BIN)/minivm

$(LIB)/libminivm.a: $(CFILES:%.c=$(LIB)/%.o)
	ar rcs $@ $^
	ranlib $@

$(LIB)/%.o: %.c
	@mkdir $(P) $(dir $@)
	$(CC) -fPIC -c $^ -o $@ $(CFLAGS) -I. $(OPT)

minivm $(BIN)/minivm: $(CFILES) main/main.c 
	@mkdir $(P) $(BIN)
	$(CC) $^ -o $@ $(CFLAGS) -I. -lm $(OPT)

clean: .dummy
	: rm -r $(BIN) $(LIB)

.dummy:
