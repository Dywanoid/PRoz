#!/bin/sh
echo "Compiling..."
make
echo "Compiling finished!"
echo "Running..."
mpirun -np 4 ./program
echo "Running finished!"