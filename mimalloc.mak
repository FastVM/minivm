LIB=lib

default: mimalloc

mimalloc: .dummy
	mkdir -p $(LIB)
	cd $(LIB) && cmake ../mimalloc
	$(MAKE) --no-print-directory -C $(LIB)

.dummy:
