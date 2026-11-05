#!/usr/bin/env bash
set -ex
COMPILE_ARGS="-g -ggdb -Wall -Wextra"

gcc $COMPILE_ARGS -o build/main main.c
gcc $COMPILE_ARGS -o build/tests tests.c

build/tests
