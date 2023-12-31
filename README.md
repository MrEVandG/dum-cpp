# DUMB
Dumb, the language with the file extension 'dum', is extremely stupid and a very massive waste of brainpower. It gets its name from the pure insanity programmers feel when trying to use it practically.

If you just want the commands to build and run Dumb, just scroll [to the bottom](#recap).

## Prerequisites

In order to use Dumb, you will need the following:
- a Linux machine
- a C++ 20 compiler
- the Netwide Assembler ([NASM](https://www.nasm.us/))
- the gnu linker (should be preinstalled on Linux, though)
- a well-deserved YouTube subscription to [Pixeled](https://www.youtube.com/@pixeled-yt) for making his follow-along series

## Building
As with many C++ projects, you need CMake! In order to build for a linux machine, run the following:
```bash
$ mkdir ./build
$ cmake -S . -B ./build
$ cmake --build build
```

## Running Dumb Code
The finished executable should be located at './build/dum'.
If you would like to evaluate a file, go to the main directory and run
```bash
$ ./build/dumb test.dum
```
Running Dumb generates an out file located at `./out`. This file should not need to be `chmod`ed.

Bash lets you view the previously run command's exit code using `$?`.
This is useful, considering at the time of writing this that exit codes are the only form of output in Dumb.
```bash
$ ./out
$ echo $?
```

## Recap
All you need to use Dumb is a Linux machine with NASM and CMake installed, as well as some low-level computer knowledge and a Bash terminal that can run these commands:
```bash
# Building the source code
$ mkdir ./build
$ cmake -S . -B ./build
$ cmake --build build
# Running actual Dumb
$ ./build/dum test.dum
$ ./out
$ echo $?
```