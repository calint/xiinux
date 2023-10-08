#!/bin/sh
# tools:
#   cppcheck: 2.10

SRC=../../src/main.cpp

date | tee cppcheck.log
cppcheck --enable=all --inline-suppr \
    --suppress=knownConditionTrueFalse \
    --suppress=missingIncludeSystem \
    $SRC 2>&1 | tee -a cppcheck.log
date | tee -a clang-tidy.log
