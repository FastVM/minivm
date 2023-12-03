#!/usr/bin/env bash

cd $(dirname $0)
cd ..

global_bench_file="build/bench/all.txt"

echo "" > "$global_bench_file"

for test in $(find test -name '*.lua')
do
    test_bench_file="build/bench/$test.txt"
    mkdir -p $(dirname "$test_bench_file")
    echo "--- $test ---" > "$test_bench_file"
done

bash ./test/minivm.sh
bash ./test/luajit.sh
# bash ./test/lua.sh
