
OPT = -Os -flto
PRE = 

linux: .dummy
	$(PRE) make -Bj -f tool/core.mak test OS=LINUX CC=cc

mac: .dummy
	$(PRE) make -Bj -f tool/core.mak test OS=MAC CC=clang

windows:
	$(PRE) make -Bj -f tool/core.mak test OS=WINDOWS EXE=.exe


.dummy:
