#!/bin/sh

clang-tidy --config-file=clang-tidy.cfg -header-filter=.* src/main.cpp -- -std=c++20
