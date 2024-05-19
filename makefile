
OPT = -Os -flto

linux: .dummy
	make -Bj -f tool/core.mak test OS=LINUX CFLAGS_VM="-DVM_USE_TCC" CC=cc

mac: .dummy
	make -Bj -f tool/core.mak test OS=MAC CFLAGS_VM="-DVM_USE_TCC" CC=clang

windows:
	make -Bj -f tool/core.mak test OS=WINDOWS CFLAGS_VM="-DVM_USE_TCC" EXE=.exe

.dummy:
