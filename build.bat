@echo off
if "%CC%"=="" ( set "CC=clang" )
if not exist bin mkdir bin || exit /b %errorlevel%
%CC% -fuse-ld=llvm-lib -Wno-deprecated-declarations -Oz vm/asm.c vm/gc.c vm/ir/build.c vm/ir/const.c vm/ir/info.c vm/ir/toir.c vm/ir/be/int3.c -static          -o "bin/libminivm.lib"  %* || exit /b %errorlevel%
%CC% -fuse-ld=lld      -Wno-deprecated-declarations -Oz vm/asm.c vm/gc.c vm/ir/build.c vm/ir/const.c vm/ir/info.c vm/ir/toir.c vm/ir/be/int3.c main/asm.c -flto -o "bin/minivm-asm.exe" %* || exit /b %errorlevel%
%CC% -fuse-ld=lld      -Wno-deprecated-declarations -Oz vm/asm.c vm/gc.c vm/ir/build.c vm/ir/const.c vm/ir/info.c vm/ir/toir.c vm/ir/be/int3.c main/run.c -flto -o "bin/minivm-run.exe" %* || exit /b %errorlevel%
