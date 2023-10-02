#!/bin/sh

clang-tidy -header-filter=.* src/main.cpp -- -std=c++20
