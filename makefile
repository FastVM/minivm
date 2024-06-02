
# target: any
OPT = -Os -flto
PRE = 
TARGET = test
TEST_LUA = vendor/lua/lua
J = 

# target: node
EMCC_CFLAGS = -fPIC -DNDEBUG
EMCC_LDFLAGS = -lnodefs.js -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s MAIN_MODULE=2 -s BINARYEN_ASYNC_COMPILATION=0 -s ASYNCIFY=0 -s EXPORTED_RUNTIME_METHODS=['FS','callMain','NODEFS']
EMCC = emcc

linux: .dummy
	$(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS=-DLUA_USE_LINUX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=LINUX CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)"

mac: .dummy
	$(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS=-DLUA_USE_MACOSX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=MAC CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)"

windows: .dummy
	$(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS= MYLDFLAGS= MYLIBS=
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=WINDOWS CC="$(CC)" EXE=.exe TEST_LUA="$(TEST_LUA)"

node: .dummy
	$(PRE) make -Bj$(J) -f tool/core.mak build/bin/minivm.mjs OS=LINUX CC=$(EMCC) EXE=.mjs \
		CFLAGS="$(EMCC_CFLAGS) $(CFLAGS)" \
		LDFLAGS="$(EMCC_LDFLAGS) $(LDFLAGS)" \
		TCC=NO \
		GCCJIT=NO \
		TARGET=bins

web: .dummy
	$(PRE) make -Bj$(J) -f tool/core.mak build/bin/minivm.mjs OS=LINUX CC=$(EMCC) EXE=.mjs \
		CFLAGS='-fPIC -DNDEBUG' \
		LDFLAGS='-fPIC -s MALLOC=mimalloc -s STACK_SIZE=16mb -s INITIAL_MEMORY=32mb -s SINGLE_FILE=1 -s BINARYEN_ASYNC_COMPILATION=0 -s ASYNCIFY=0 -s ENVIRONMENT=worker -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s MAIN_MODULE=2 -s EXPORTED_RUNTIME_METHODS="['FS','callMain']"' \

.dummy:
