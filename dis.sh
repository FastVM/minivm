#!/usr/bin/env sh

echo
echo --- $1 ---
ndisasm -b 64 $1
echo
echo
