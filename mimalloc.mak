LIB=lib
BUILD=$(LIB)/mimalloc
MIMALLOC=$(shell pwd)/mimalloc

default: mimalloc

mimalloc: .dummy
	@mkdir -p $(LIB) $(BUILD)
	@cd $(BUILD) && cmake $(MIMALLOC) -DCMAKE_C_COMPILER=$(CC) -DMI_OVERRIDE=ON
	$(MAKE) -s --no-print-directory -C $(BUILD) mimalloc-static
	cp $(BUILD)/libmimalloc.a $(LIB)/libmimalloc.a

.dummy:
