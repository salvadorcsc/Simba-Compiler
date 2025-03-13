#!/bin/sh

NV_FLAG=0

if [ "$1" = "-nv" ]; then
    NV_FLAG=1
    shift
fi

if [ $# -ne 1 ]; then
    echo "Usage: $0 <filename.asm>"
    exit 1
fi

filepath="$1"
dirpath=$(dirname "$filepath")
filename=$(basename "$1" .asm)

nasm -f elf64 -g -F DWARF "$dirpath/$filename.asm"

ld -o "$dirpath/$filename" "$dirpath/$filename.o"

rm "$dirpath/$filename.o"

./"$dirpath/$filename"

if [ "$NV_FLAG" -eq 1 ]; then
    rm "$dirpath/$filename"
    rm "$dirpath/$filename.asm"
    rm "$dirpath/$filename.sasm"
    rm "$dirpath/$filename.sir"
fi