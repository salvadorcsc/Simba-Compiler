#!/bin/sh

NV_FLAG=0

if [ "$1" = "-nv" ]; then
    NV_FLAG=1
    shift
fi

if [ $# -ne 1 ]; then
    echo "Usage: $0 [-nv] <filename.simba>"
    exit 1
fi

filepath="$1"
dirpath=$(dirname "$filepath")
filename=$(basename "$1" .simba)

./build/simba.o "$dirpath/$filename.sir" < "$dirpath/$filename.simba"

if [ "$NV_FLAG" -eq 1 ]; then
    sh build/scripts/sir.sh -nv "$dirpath/$filename.sir"
else
    sh build/scripts/sir.sh "$dirpath/$filename.sir"
fi