@echo off
rem cd to the bin directory here
cd ".\bin"
rem compile the source
athens.exe code.ace
rem copy the bytecode file
copy "out.bin" "..\..\ace-vm\bin"
rem go to the vm directory
cd "..\..\ace-vm\bin"
rem run the vm
ace-vm.exe out.bin
rem go back
cd "..\..\Athens"