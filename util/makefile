
BIN := bin
DIR := util

SRCS := vm/jump.c vm/int.c vm/gc.c

$(BIN)/minivm-dis: $(DIR)/dis.c $(DIR)/../vm/jump.c | $(BIN)
	$(CC) $(OPT) $(^) -o $(@) $(CFLAGS)

$(BIN)/minivm-color: $(DIR)/color.c $(DIR)/../vm/jump.c | $(BIN)
	$(CC) $(OPT) $(^) -o $(@) $(CFLAGS)

$(BIN)/minivm-asm: $(DIR)/asm.c $(SRCS) | $(BIN)
	$(CC) $(OPT) $(^) -o $(@) $(CFLAGS)

$(BIN): .dummy
	@mkdir -p $(@)

.dummy:
