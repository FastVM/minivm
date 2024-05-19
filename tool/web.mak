
EXE ?= .mjs
CC = emcc
TCC_SRCS =
CFLAGS_TB =

CFLAGS += -fPIC -DNDEBUG
LDFLAGS += -fPIC -s MALLOC=mimalloc -s STACK_SIZE=64mb -s INITIAL_MEMORY=256mb -s SINGLE_FILE=1 -s BINARYEN_ASYNC_COMPILATION=0 -s ASYNCIFY=0 -s ENVIRONMENT=worker -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s MAIN_MODULE=2 -s EXPORTED_RUNTIME_METHODS="['FS','callMain']"

GCCJIT = NO
TCC = NO

include tool/core.mak