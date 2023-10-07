#!/bin/bash

# tools:
#           g++: 12.3.0
#       clang++: 15.0.7
#      valgrind: 3.19.0
# llvm-profdata: 
#      llvm-cov: 15.0.7
#       genhtml: 1.16
# 
# builds 'xiinux' binary
# if first argument is 'qa' then:
#  * build with debug info and coverage support
#  * run 'xiinux' with valgrind
#  * ( '^C' to stop server )
#  * generate coverage report to 'qa/coverage/report/'

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
#          -Wnoexcept -Wno-stringop-truncation"

BIN=xiinux
SRC=src/main.cpp
ETC="-Wfatal-errors -Werror"
#ETC="$ETC -static"
SAN=
#SAN="-fsanitize=memory -fsanitize-blacklist=memory_sanitizer.txt -fsanitize-memory-track-origins=2"
#SAN="-fsanitize=undefined,address,leak"
DBG=-g
OPT=-O3
if [ "$1" = "qa" ]; then
    ETC="-fprofile-instr-generate $ETC"
    DBG=-g
fi

echo

CMD="$CC $SRC -o $BIN $OPT $DBG $SAN $ETC $WARNINGS"
echo $CMD
$CMD
[[ $? != "0" ]] && exit $?

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

[[ "$1" != "qa" ]] && exit

# run xiinux using valgrind and generate coverage reports

# don't end this script at '^C'
trap '' SIGINT

valgrind --leak-check=full --show-leak-kinds=all -s ./$BIN
# process coverage data
llvm-profdata merge -sparse default.profraw -o xiinux.profdata
llvm-cov export --format=lcov --instr-profile xiinux.profdata --object xiinux > lcov.info
# generate report
genhtml --quiet lcov.info --output-directory qa/coverage/report/
echo
echo coverage report generated in "qa/coverage/report/"
echo
# clean-up
rm default.profraw xiinux.profdata lcov.info
