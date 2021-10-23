PWD:=$(shell pwd)
LIB=lib
P=-p

default: mimalloc

mimalloc: clean
	mkdir $(P) $(LIB)
	cd $(LIB) && cmake $(PWD)/mimalloc -DCMAKE_C_COMPILER=$(CC) -DMI_OVERRIDE=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_WORKS=1 $(CMAKE_FLAGS)
	cd $(LIB) && $(MAKE) --no-print-directory mimalloc-static

clean: .dummy
	: rm -r $(LIB)

.dummy: