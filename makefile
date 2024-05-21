
OPT = -Os -flto
PRE = 

linux: .dummy
	$(PRE) make -j -C vendor/lua MYCFLAGS=-DLUA_USE_LINUX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj -f tool/core.mak test OS=LINUX CC=cc TEST_LUA=vendor/lua/lua

mac: .dummy
	$(PRE) make -j -C vendor/lua MYCFLAGS=-DLUA_USE_MACOSX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj -f tool/core.mak test OS=MAC CC=clang TEST_LUA=vendor/lua/lua

windows:
	$(PRE) make -j -C vendor/lua MYCFLAGS=-DLUA_USE_POSIX MYLDFLAGS= MYLIBS=-ldl
	$(PRE) make -Bj -f tool/core.mak test OS=WINDOWS EXE=.exe TEST_LUA=vendor/lua/lua

.dummy:
