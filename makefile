
OPT ?= -Os

default: all

all: minivm dis

minivm: .dummy
	$(CC) $(OPT) vm/minivm.c -o minivm $(CFLAGS)

pgo-llvm: .dummy
	$(MAKE) minivm CC='$(CC)' OPT='-O1' CFLAGS+='-fprofile-instr-generate=minivm.profraw'
	$(PGO)
	$(PROFDATA) merge -output=minivm.profdata minivm.profraw
	$(MAKE) minivm CC='$(CC)' OPT='$(OPT)' CFLAGS+='-fprofile-instr-use=minivm.profdata'

pgo-llvm-%: .dummy
	$(MAKE) pgo-llvm OPT='$(OPT)' PGO='$(PGO)' CC=clang-$(@:pgo-llvm-%=%) PROFDATA=llvm-profdata-$(@:pgo-llvm-%=%)

dis: .dummy
	$(CC) $(OPT) vm/dis.c -o dis $(CFLAGS)

.dummy:

clean: .dummy
	rm -f $(OBJS) minivm dis minivm.profdata minivm.profraw
