
OPT = -Os -flto
PRE = 
TARGET = test
TEST_LUA = vendor/lua/lua
J = 

linux: .dummy
	$(PRE) make -j$(J) -C vendor/lua MYCFLAGS=-DLUA_USE_LINUX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=LINUX CC="$(CC)" TEST_LUA="$(TEST_LUA)"

mac: .dummy
	$(PRE) make -j$(J) -C vendor/lua MYCFLAGS=-DLUA_USE_MACOSX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=MAC CC="$(CC)" TEST_LUA="$(TEST_LUA)"

windows:
	$(PRE) make -j$(J) -C vendor/lua MYCFLAGS= MYLDFLAGS= MYLIBS=
	$(PRE) make -Bj$(J) -f tool/core.mak $(TARGET) OS=WINDOWS EXE=.exe TEST_LUA="$(TEST_LUA)"

.dummy:
