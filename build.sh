#!/bin/sh

# create build folder
mkdir -p build

# go to build folder
cd build

# generate makefiles
cmake ../src

# run make
make

# go back
cd ..