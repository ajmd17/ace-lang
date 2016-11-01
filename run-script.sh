#!/bin/sh
#go to the bin directory here
cd bin
#compile the source
./ace-c test2.ace
#copy the bytecode file
cp out.bin ../../acevm/bin/out.bin
#go to the vm directory
cd ../../acevm/bin/
#run the vm
./acevm out.bin
#go back
cd ../../ace-c/
