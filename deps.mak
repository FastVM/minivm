PWD:=$(shell pwd)
LIB=$(PWD)/lib
P=-p

default: libmimalloc.a

libmimalloc.a: .dummy
	mkdir $(P) $(LIB)/mimalloc
	cd $(LIB)/mimalloc && cmake $(PWD)/mimalloc -DCMAKE_C_COMPILER="$(CC)" -DMI_OVERRIDE=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_WORKS=1 $(CMAKE_FLAGS)
	cd $(LIB)/mimalloc && $(MAKE) --no-print-directory mimalloc-static

clean: .dummy
	: rm -r $(LIB)

.dummy: