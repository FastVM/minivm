
BIN := bin
DIR := util

SRCS := vm/jump.c vm/int.c vm/gc.c

$(BIN)/minivm-asm: $(DIR)/asm.c $(SRCS) | $(BIN)
	$(CC) $(OPT) $(^) -o $(@) -lm $(CFLAGS)

$(BIN): .dummy
	@mkdir -p $(@)

.dummy:
