
EXE ?= .js
CC = emcc
TCC_SRCS = 
CFLAGS := $(CFLAGS)

# CFLAGS_TB = -DTB_HAS_WASM

include core.mak

CFLAGS := -fPIC $(CFLAGS)
LDFLAGS := -lnodefs.js -s BINARYEN_ASYNC_COMPILATION=0 -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT=node -s MAIN_MODULE=2 -s EXPORTED_RUNTIME_METHODS="['FS','callMain']" $(LDFLAGS) 

POST_INSTALL = mv build/bin/minivm.js build/bin/minivm.mjs
