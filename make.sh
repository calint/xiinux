#!/bin/bash

# tools:
#           g++: 12.3.0
#       clang++: 15.0.7
#      valgrind: 3.19.0
# llvm-profdata: 
#      llvm-cov: 15.0.7
#       genhtml: 1.16

CC="clang++ -std=c++20"
WARNINGS="-Weverything \
          -Wno-c++98-compat -Wno-weak-vtables -Wno-padded \
          -Wno-global-constructors -Wno-exit-time-destructors"

#CC="g++ -std=c++23"
#WARNINGS="-Wall -Wextra -Wpedantic \
#          -Weffc++ -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy \
#          -Wdisabled-optimization -Wlogical-op -Wmissing-declarations \
#          -Wsign-conversion -Wold-style-cast -Wshadow -Wmissing-include-dirs \
#          -Woverloaded-virtual -Wredundant-decls -Wshadow -Wctad-maybe-unsupported \
#          -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Wfloat-equal \
#          -Wnoexcept -Wno-unused-parameter -Wno-stringop-truncation"

BIN=xiinux
SRC=src/main.cpp
ETC=-Wfatal-errors
#ETC="$ETC -static"
DBG=
OPT=-O3
if [ "$1" = "qa" ]; then
    ETC="$ETC -fprofile-instr-generate -fcoverage-mapping"
    DBG=-g
fi

echo

CMD="$CC -o $BIN $SRC $DBG $ETC $OPT $WARNINGS"
echo $CMD
$CMD
if [ $? != "0" ]; then exit $?; fi

# stats on source code
echo > all.src &&

# find all files, concatinate into a file
#  exclude empty lines, comment lines, lines containing only '}'
#   and lines starting with '#'
find src -type f -exec cat {} + | \
    grep -vE '^\s*//|^\s*$|^\s*}\s*$|^#.*$' >> all.src

echo
echo    "            lines   words   chars"
echo -n "   source:"
cat all.src | wc
echo -n "  gzipped:"
cat all.src | gzip | wc
echo
ls -ho --color $BIN
echo
rm all.src

if [ "$1" != "qa" ]; then exit; fi

# run xiinux using valgrind and generate coverage reports

# don't end script at "^C"
trap '' SIGINT

#valgrind ./$BIN
#valgrind --leak-check=full ./$BIN
valgrind --leak-check=full --show-leak-kinds=all -s ./$BIN
#valgrind --leak-check=full --show-leak-kinds=all ./$BIN -bm
llvm-profdata merge -sparse default.profraw -o xiinux.profdata
llvm-cov export --format=lcov --instr-profile xiinux.profdata --object xiinux > lcov.info
genhtml --quiet lcov.info --output-directory qa/coverage/report/
rm default.profraw xiinux.profdata
echo
echo coverage report generated in "qa/coverage/report/"
echo
