#!/bin/sh

set -xe

CC=${CC:=/usr/bin/cc}
CFLAGS="-Wall -Wextra -Wswitch-enum -Wmissing-prototypes -Wconversion -pedantic -fno-strict-aliasing -ggdb -std=c11"
LIBS=

mkdir -p build/bin

$CC $CFLAGS -o build/bin/yvm ./src/main.c $LIBS
