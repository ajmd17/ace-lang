@echo off
rem cd to the bin directory here
cd ".\bin"
rem compile the source
athens.exe code.ace
rem copy the bytecode file
copy "out.bin" "..\..\AceVM\bin"
rem go to the vm directory
cd "..\..\AceVM\bin"
rem run the vm
acevm.exe out.bin
rem go back
cd "..\..\Athens"