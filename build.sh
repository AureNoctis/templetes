#!/usr/bin/env bash

set -e

CC="clang"
CFLAGS="-Wall -Wextra -Iglad/include"
LIBS="-lglfw -lGL -ldl"
SRC_FILES="vizualiser.c glad/src/glad.c"
OUTPUT_DIR="build"
OUTPUT_BIN="$OUTPUT_DIR/vizualiser"

BUILD_MODE="debug"

if [ "$1" = "release" ]; then
    BUILD_MODE="release"
    CFLAGS="$CFLAGS -O3"
else
    CFLAGS="$CFLAGS -g -O0 -DDEBUG=1"
fi

mkdir -p "$OUTPUT_DIR"
$CC $CFLAGS $SRC_FILES $LIBS -o $OUTPUT_BIN
