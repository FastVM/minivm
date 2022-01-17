DC ?= ldc2

OPT ?= -O

OUT ?= minivm

default:
	$(DC) $(OPT) minivm.d -of$(OUT)
	