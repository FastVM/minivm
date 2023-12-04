#!/usr/bin/env bash

set -e

cd $(dirname $0)

cd vm/lua

mkdir -p trees
trap "cd $(pwd) && rm -r trees" EXIT

cp grammar.js trees

cd trees

tree-sitter generate

cp src/parser.c ..
cp src/tree_sitter/parser.h ..
