#!/usr/bin/env sh

GC_SRCS="bdwgc/alloc.c bdwgc/allchblk.c bdwgc/blacklst.c bdwgc/dbg_mlc.c bdwgc/dyn_load.c bdwgc/finalize.c bdwgc/headers.c bdwgc/malloc.c bdwgc/mallocx.c bdwgc/mark.c bdwgc/mach_dep.c bdwgc/mark_rts.c bdwgc/misc.c bdwgc/new_hblk.c bdwgc/obj_map.c bdwgc/os_dep.c bdwgc/ptr_chck.c bdwgc/reclaim.c"
VM_SRCS="vm/ir.c vm/lib.c vm/type.c vm/lang/paka.c vm/obj.c"
JIT_SRCS="vm/jit/x64.tmp.c"
MAIN="main/asm.c"

luajit dynasm/dynasm.lua -o vm/jit/x64.tmp.c -D X64 -M -L vm/jit/x64.dasc

mkdir -p build/bin
tcc -o build/bin/minivm $MAIN $GC_SRCS $VM_SRCS $JIT_SRCS -lm
