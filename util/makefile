
BIN := bin
DIR := util

$(BIN)/minivm-dis: $(DIR)/dis.c $(DIR)/../vm/jump.c | $(BIN)
	$(CC) $(^) -o $(@)

$(BIN)/minivm-color: $(DIR)/color.c $(DIR)/../vm/jump.c | $(BIN)
	$(CC) $(^) -o $(@)

$(BIN): .dummy
	mkdir -p $(@)

.dummy:
