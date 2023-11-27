#!/usr/bin/env bash

tests_start=$(date +%s.%N)

make clean > /dev/null
make -j$(nproc) build/bin/minivm CFLAGS='-DNDEBUG' > /dev/null

test_failed=()
test_passed=()

for test in $(find test -name '*.lua')
do
    echo "--- test $test ---"
    test_start=$(date +%s.%N)
    test_out_file="build/runs/$test.log"
    test_err_file="build/runs/$test.err"
    test_bench_file="build/bench/$test.time"
    mkdir -p $(dirname "$test_out_file")
    mkdir -p $(dirname "$test_err_file")
    mkdir -p $(dirname "$test_bench_file")
    if ./build/bin/minivm $test > "$test_out_file" 2> "$test_err_file"
    then
        cat "$test_out_file"
        test_passed+=($test)
        test_end=$(date +%s.%N)
        test_took=$(echo "$test_end-$test_start" | bc | sed 's/^\./0./')
        echo $test_took > "$test_bench_file"
        echo "^ took ${test_took}s ^"
    else
        test_failed+=($test)
    fi
    echo
done
echo "passed: ${test_passed[@]}"
echo "failed: ${test_failed[@]}"
tests_end=$(date +%s.%N) 
tests_took=$(echo "$tests_end-$tests_start" | bc | sed 's/^\./0./')
echo "ran in ${tests_took}s"
