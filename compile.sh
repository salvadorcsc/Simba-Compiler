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

if [ "$NV_FLAG" -eq 1 ]; then
    sh build/scripts/simba.sh -nv "$dirpath/$filename.simba"
else
    sh build/scripts/simba.sh "$dirpath/$filename.simba"
fi