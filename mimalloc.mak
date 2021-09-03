LIB=lib

default: mimalloc

mimalloc: .dummy
	mkdir -p $(LIB)
	cd $(LIB) && cmake ../mimalloc -DCMAKE_C_COMPILER=$(CC)
	$(MAKE) -s --no-print-directory -C $(LIB) mimalloc-static

.dummy:
