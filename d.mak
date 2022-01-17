DC ?= ldc2

OPT ?= -O

OUT ?= minivm

default:
	$(DC) -i $(OPT) vm/minivm.d -of$(OUT)
	