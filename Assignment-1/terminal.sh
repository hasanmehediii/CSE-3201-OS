#!/bin/bash

gcc Terminal.c -o terminal

if [ $? -eq 0 ]; then
    ./terminal
else
    echo "Compilation failed"
fi