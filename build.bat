@echo off
if "%CC%"=="" ( set "CC=clang" )
if not exist bin mkdir bin || exit /b %errorlevel%
%CC% -fuse-ld=llvm-lib -Wno-deprecated-declarations -Oz vm/asm.c vm/gc.c vm/ir/build.c vm/ir/const.c vm/ir/info.c vm/ir/toir.c vm/ir/be/int3.c vm/ir/be/spall.c vm/ir/be/js.c -static                  -o "bin/libminivm.lib"  %* || exit /b %errorlevel%
%CC% -fuse-ld=lld      -Wno-deprecated-declarations -Oz vm/asm.c vm/gc.c vm/ir/build.c vm/ir/const.c vm/ir/info.c vm/ir/toir.c vm/ir/be/int3.c vm/ir/be/spall.c vm/ir/be/js.c main/asm.c    -flto=full -o "bin/minivm-asm.exe" %* || exit /b %errorlevel%
%CC% -fuse-ld=lld      -Wno-deprecated-declarations -Oz vm/asm.c vm/gc.c vm/ir/build.c vm/ir/const.c vm/ir/info.c vm/ir/toir.c vm/ir/be/int3.c vm/ir/be/spall.c vm/ir/be/js.c main/run.c    -flto=full -o "bin/minivm-run.exe" %* || exit /b %errorlevel%
%CC% -fuse-ld=lld      -Wno-deprecated-declarations -Oz vm/asm.c vm/gc.c vm/ir/build.c vm/ir/const.c vm/ir/info.c vm/ir/toir.c vm/ir/be/int3.c vm/ir/be/spall.c vm/ir/be/js.c main/js.c     -flto=full -o "bin/vm2js.exe"      %* || exit /b %errorlevel%
