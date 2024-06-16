
OPT = -Os -flto
PRE = 
TARGET = bins
J != nproc

TEST_LUA = vendor/lua/lua
linux: .dummy
	# $(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS=-DLUA_USE_LINUX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=LINUX CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)"

mac: .dummy
	$(PRE) make -Bj$(J) -C vendor/raylib/src LDFLAGS="$(OPT)" CFLAGS="-w $(OPT) $(CLFAGS) -DPLATFORM_DESKTOP" PLATFORM=PLATFORM_DESKTOP
	# $(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS=-DLUA_USE_MACOSX MYLDFLAGS= MYLIBS=-ldl CFLAGS="$(OPT) $(CFLAGS)"
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=MAC CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)" CFLAGS="-DVM_USE_RAYLIB $(CFLAGS)" LDFLAGS="vendor/raylib/src/libraylib.a -framework Cocoa -framework OpenGL -framework IOKit $(LDFLAGS)" 

windows: .dummy
	$(PRE) make -Bj$(J) -C vendor/lua MYCFLAGS= MYLDFLAGS= MYLIBS=
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=WINDOWS CC="$(CC)" EXE=.exe TEST_LUA="$(TEST_LUA)"

freebsd: .dummy
	$(PRE) gmake -j$(J) -C vendor/raylib/src LDFLAGS="$(OPT)" CFLAGS="-w $(OPT) $(CLFAGS) -DPLATFORM_DESKTOP" PLATFORM=PLATFORM_DESKTOP
	# $(PRE) gmake -j$(J) -C vendor/lua CC="$(CC)" MYCFLAGS=-DLUA_USE_LINUX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) gmake -Bj$(J) -f tool/core.mak $(TARGET) OS=FREEBSD CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)" CFLAGS="-DVM_USE_RAYLIB $(CFLAGS)" LDFLAGS="-L/usr/local/lib vendor/raylib/src/libraylib.a -lOpenGL -lm -lpthread $(LDFLAGS)" 

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
