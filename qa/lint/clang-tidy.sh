#!/bin/sh
# tools:
#   clang-tidy: Ubuntu LLVM version 15.0.7

SRC=../../src/main.cpp

date | tee clang-tidy.log
clang-tidy --config-file=clang-tidy.cfg -header-filter=.* $SRC -- -std=c++20 | tee -a clang-tidy.log
date | tee -a clang-tidy.log

