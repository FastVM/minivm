#!/usr/bin/env bash

cd $(dirname $0)
cd ..

mkdir -p build/bench/png

python3 test/graph.py
