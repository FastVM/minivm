#!/usr/bin/env sh

set -e

MVM=$(dirname $0)

mkdir -p $MVM/bin

cp $1 $MVM/bin/boot.bc

ldc2 -O3 -flto=full -i -I $MVM -J $MVM/bin $MVM/vm/embed.d -of embed
