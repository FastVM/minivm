#!/usr/bin/env sh

rm -f *.bin

make -Bj OPT="$*" CC=gcc && ./bin/minivm-asm asm/test.vasm

find .  -maxdepth 1 -name '*.bin'| xargs -I this ./dis.sh this
