#!/usr/bin/env sh

cd $(dirname "$0")

emcc src/empty/empty.c -o src/empty/empty.js -O3 -flto -s EXPORT_ES6=1 -s EXPORTED_RUNTIME_METHODS=FS,PROXYFS,ERRNO_CODES,allocateUTF8 -s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1 --no-entry -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$ERRNO_CODES' -s EXPORTED_FUNCTIONS=_main,_free,_malloc -s --js-library=src/empty/fsroot.js -lproxyfs.js
