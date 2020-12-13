#!/bin/sh
echo "Compiling..."
make
echo "Compiling finished!"
echo "Running..."
mpirun --hostfile ./hostfile -np 8 ./dist/program $1 $2 $3
echo "Running finished!"