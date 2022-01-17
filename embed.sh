#!/usr/bin/env sh

set -e

MVM=$(dirname $0)

ldc2 -O3 -flto=full -i -I $MVM -J $1 $MVM/vm/embed.d -of embed
