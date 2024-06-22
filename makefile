
OPT = -Os -flto
PRE = 
TARGET = bins
J != nproc
EMCC = emcc

linux: .dummy
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=LINUX CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)"

RAYLIB_DIR = vendor/raylib/src
MAC_RAYLIB_OBJS = $(RAYLIB_DIR)/rcore.o $(RAYLIB_DIR)/rglfw.o $(RAYLIB_DIR)/rshapes.o $(RAYLIB_DIR)/rtextures.o $(RAYLIB_DIR)/rtext.o $(RAYLIB_DIR)/rmodels.o $(RAYLIB_DIR)/raudio.o $(RAYLIB_DIR)/utils.o

mac: .dummy
	$(PRE) make -Bj$(J) -C vendor/raylib/src CC="$(CC)" LDFLAGS="$(OPT)" CFLAGS="-w $(OPT) $(CLFAGS) -DPLATFORM_DESKTOP" PLATFORM=PLATFORM_DESKTOP
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=MAC CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)" CFLAGS="-DVM_USE_RAYLIB $(CFLAGS)" LDFLAGS="$(MAC_RAYLIB_OBJS) -framework Cocoa -framework OpenGL -framework IOKit $(LDFLAGS)" 

windows: .dummy
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=WINDOWS CC="$(CC)" EXE=.exe TEST_LUA="$(TEST_LUA)"

freebsd: .dummy
	$(PRE) gmake -Bj$(J) -C vendor/raylib/src CC="$(CC)" LDFLAGS="$(OPT)" CFLAGS="-w $(OPT) $(CLFAGS) -DPLATFORM_DESKTOP" PLATFORM=PLATFORM_DESKTOP
	$(PRE) gmake -Bj$(J) -f tool/core.mak $(TARGET) OS=FREEBSD CC="$(CC)" EXE= TEST_LUA="$(TEST_LUA)" CFLAGS="-DVM_USE_RAYLIB $(CFLAGS)" LDFLAGS="-L/usr/local/lib vendor/raylib/src/libraylib.a -lOpenGL -lm -lpthread $(LDFLAGS)" 

EMCC_CFLAGS = -fPIC -DNDEBUG
EMCC_LDFLAGS = -lnodefs.js -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s MAIN_MODULE=2 -s BINARYEN_ASYNC_COMPILATION=0 -s ASYNCIFY=1 -s EXPORTED_RUNTIME_METHODS=['FS','callMain','NODEFS']
EMCC = emcc
node: .dummy
	$(PRE) make -Bj$(J) -f tool/core.mak build/bin/minivm.mjs OS=LINUX CC=$(EMCC) EXE=.mjs \
		CFLAGS="$(EMCC_CFLAGS) $(CFLAGS)" \
		LDFLAGS="$(EMCC_LDFLAGS) $(LDFLAGS)" \
		TCC=NO \
		GCCJIT=NO \
		TARGET=bins

web: .dummy
	$(PRE) make -Bj$(J) CC=$(EMCC) -C vendor/raylib/src LDFLAGS="$(OPT)" CFLAGS="-w $(OPT) $(CLFAGS) -D_DEFAULT_SOURCE -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2" PLATFORM=PLATFORM_WEB
	$(PRE) make -Bj$(J) CC=$(EMCC) -f tool/core.mak build/bin/minivm.mjs OS=LINUX EXE=.mjs \
		CFLAGS='-fPIC -DVM_USE_RAYLIB -DNDEBUG' \
		LDFLAGS='--embed-file test@test -s ASYNCIFY=1 -s BINARYEN_ASYNC_COMPILATION=0 -s EXPORTED_RUNTIME_METHODS=['FS'] -s STACK_SIZE=16mb -s USE_GLFW=3 -s SINGLE_FILE=1 vendor/raylib/src/libraylib.a -s MALLOC=mimalloc -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1'
	mv build/bin/minivm.mjs web/minivm.js

web-raw: .dummy
	$(PRE) make -Bj$(J) CC=$(EMCC) -f tool/core.mak build/bin/minivm.mjs OS=LINUX EXE=.mjs \
		CFLAGS='-fPIC -DVM_USE_RAYLIB -DNDEBUG' \
		LDFLAGS='--embed-file test@test -s ASYNCIFY=1 -s BINARYEN_ASYNC_COMPILATION=0 -s EXPORTED_RUNTIME_METHODS=['FS'] -s STACK_SIZE=16mb -s USE_GLFW=3 -s SINGLE_FILE=1 -s MALLOC=mimalloc -s ENVIRONMENT=web -s ALLOW_MEMORY_GROWTH=1'

format: .dummy
	clang-format -i $$(find vm main -name '*.c' -or -name '*.h')

.dummy:
