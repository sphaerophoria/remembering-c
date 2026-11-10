#!/usr/bin/env bash
set -ex
COMPILE_ARGS="-g -ggdb -Wall -Wextra -Wconversion"

gcc $COMPILE_ARGS -o build/main main.c string_tape.c event_loop.c item_pool.c
gcc $COMPILE_ARGS -o build/tests tests.c string_tape.c item_pool.c

build/tests
