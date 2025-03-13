#!/bin/sh

NV_FLAG=0

if [ "$1" = "-nv" ]; then
    NV_FLAG=1
    shift
fi

if [ $# -ne 1 ]; then
    echo "Usage: $0 <filename.sasm>"
    exit 1
fi

filepath="$1"
dirpath=$(dirname "$filepath")
filename=$(basename "$1" .sasm)

./build/sasm.o "$dirpath/$filename.asm" < "$dirpath/$filename.sasm"

if [ "$NV_FLAG" -eq 1 ]; then
    sh build/scripts/assemble.sh -nv "$dirpath/$filename.asm"
else
    sh build/scripts/assemble.sh "$dirpath/$filename.asm"
fi