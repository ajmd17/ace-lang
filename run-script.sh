#!/bin/sh
#go to the bin directory here
cd bin
#compile the source
./ace-c test2.ace
#run the vm
./ace-vm out.bin
#go back
cd ../
