#!/bin/sh
#go to the bin directory here
cd bin
#compile the source
./athens code.ace
#copy the bytecode file
cp out.bin ../../AceVM/bin/out.bin
#go to the vm directory
cd ../../AceVM/bin/
#run the vm
./acevm out.bin
#go back
cd ../../Athens/
