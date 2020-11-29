#!/bin/sh
echo "Compiling..."
make
echo "Compiling finished!"
echo "Running..."
mpirun -np 4 ./dist/program
echo "Running finished!"