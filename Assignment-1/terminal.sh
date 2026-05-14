#!/bin/sh

set -e

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$SCRIPT_DIR"

gcc Terminal.c -o terminal

if [ $? -eq 0 ]; then
    ./terminal
else
    echo "Compilation failed"
fi