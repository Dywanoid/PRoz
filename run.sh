#!/bin/sh
echo "Compiling..."
make
echo "Compiling finished!"
echo "Running..."
mpirun --hostfile ./hostfile -np 8 ./dist/program
echo "Running finished!"