PWD:=$(shell pwd)
LIB=lib
P=-p

default: mimalloc

mimalloc: .dummy
	mkdir $(P) $(LIB)
	cd $(LIB) && cmake $(PWD)/mimalloc -DCMAKE_C_COMPILER=$(CC) -DMI_OVERRIDE=ON
	$(MAKE) -s --no-print-directory -C $(LIB) mimalloc-static

.dummy: