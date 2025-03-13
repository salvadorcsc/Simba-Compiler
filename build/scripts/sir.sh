#!/bin/sh

NV_FLAG=0

if [ "$1" = "-nv" ]; then
    NV_FLAG=1
    shift
fi

if [ $# -ne 1 ]; then
    echo "Usage: $0 [-nv] <filename.sir>"
    exit 1
fi

filepath="$1"
dirpath=$(dirname "$filepath")
filename=$(basename "$1" .sir)

./build/sir.o "$dirpath/$filename.sasm" < "$dirpath/$filename.sir"

if [ "$NV_FLAG" -eq 1 ]; then
    sh build/scripts/sasm.sh -nv "$dirpath/$filename.sasm"
else
    sh build/scripts/sasm.sh "$dirpath/$filename.sasm"
fi