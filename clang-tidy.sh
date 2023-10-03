#!/bin/sh

date | tee clang-tidy.log
clang-tidy --config-file=clang-tidy.cfg -header-filter=.* src/main.cpp -- -std=c++20 | tee -a clang-tidy.log
date | tee -a clang-tidy.log

