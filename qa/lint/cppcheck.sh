#!/bin/sh
# tools:
#   clang-tidy: Ubuntu LLVM version 15.0.7

SRC=../../src/main.cpp

date | tee cppcheck.log
cppcheck --enable=all $SRC | tee -a cppcheck.log
date | tee -a clang-tidy.log

