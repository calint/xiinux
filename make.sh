# tools:
#      g++: (Ubuntu 12.3.0-1ubuntu1~23.04) 12.3.0
#  clang++: Ubuntu clang version 15.0.7
# valgrind: 3.19.0

#CC="g++ -std=c++23"
#WARNINGS="-Wall -Wextra -Wpedantic \
#          -Weffc++ -Wconversion -Wcast-align -Wcast-qual -Wctor-dtor-privacy \
#          -Wdisabled-optimization -Wlogical-op -Wmissing-declarations \
#          -Wsign-conversion -Wold-style-cast -Wshadow -Wmissing-include-dirs \
#          -Woverloaded-virtual -Wredundant-decls -Wshadow -Wctad-maybe-unsupported \
#          -Wsign-promo -Wstrict-null-sentinel -Wswitch-default -Wundef -Wfloat-equal \
#          -Wnoexcept -Wno-unused-parameter"

CC="clang++ -std=c++20"
WARNINGS="-Weverything \
          -Weffc++ -Wno-unused-parameter -Wno-c++98-compat -Wno-weak-vtables \
          -Wno-padded -Wno-global-constructors -Wno-exit-time-destructors \
          -Wno-format-nonliteral"

BIN=xiinux
SRC=src/main.cpp
#ETC="-static -Wfatal-errors"
ETC=-Wfatal-errors
#DBG=-g
#OPT=-Wfatal-errors
DBG=
OPT=-O3
#OPT=-Os

echo > all.src &&

#for f in $(find src);do if [ -f $f ];then cat $f>>all.src;fi;done

# find all files, concatinate into a file
#  exclude empty lines, comment lines, lines containing only '}'
#   and lines starting with '#'
find src -type f -exec cat {} + | \
    grep -vE '^\s*//|^\s*$|^\s*}\s*$|^#.*$' >> all.src

echo &&
$CC -o $BIN $SRC $DBG $ETC $OPT $WARNINGS && 
echo    "            lines   words   chars" &&
echo -n "   source:" &&
cat all.src|wc &&
echo -n "  gzipped:" &&
cat all.src|gzip|wc &&
echo && ls -ho --color $BIN &&
echo &&
rm all.src &&

#./clang-tidy.sh &&

#valgrind ./$BIN
#valgrind --leak-check=full ./$BIN
valgrind --leak-check=full --show-leak-kinds=all -s ./$BIN
#valgrind --leak-check=full --show-leak-kinds=all ./$BIN -bm
echo
