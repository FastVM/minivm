
OPT = -Os -flto
PRE = 
TARGET = test
J != nproc

TEST_LUA = vendor/lua/lua
linux: .dummy
	$(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS=-DLUA_USE_LINUX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=LINUX CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)"

mac: .dummy
	$(PRE) make -Bj$(J) -C vendor/raylib/src CFLAGS="-w $(OPT) $(CLFAGS)"
	$(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS=-DLUA_USE_MACOSX MYLDFLAGS= MYLIBS=-ldl CFLAGS="$(OPT) $(CFLAGS)"
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=MAC CFLAGS="-DVM_USE_RAYLIB $(CFLAGS)" LDFLAGS="vendor/raylib/src/libraylib.a $(LDFLAGS)" CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)" RAYLIB=YES

windows: .dummy
	$(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS= MYLDFLAGS= MYLIBS=
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=WINDOWS CC="$(CC)" EXE=.exe TEST_LUA="$(TEST_LUA)"

freebsd: .dummy
	$(PRE) gmake -Bj$(J) -C vendor/lua CC="$(CC)" MYCFLAGS=-DLUA_USE_LINUX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) gmake -Bj$(J) -f tool/core.mak $(TARGET) OS=FREEBSD CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)"
	
EMCC_CFLAGS = -fPIC -DNDEBUG
EMCC_LDFLAGS = -lnodefs.js -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s MAIN_MODULE=2 -s BINARYEN_ASYNC_COMPILATION=0 -s ASYNCIFY=0 -s EXPORTED_RUNTIME_METHODS=['FS','callMain','NODEFS']
EMCC = emcc
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

format: .dummy
	clang-format -i $$(find vm main -name '*.c' -or -name '*.h')

.dummy:
