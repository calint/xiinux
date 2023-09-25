# tools:
#      g++: (Ubuntu 12.3.0-1ubuntu1~23.04) 12.3.0
# valgrind: 3.19.0

CC="g++ -std=c++23"
WARNINGS="-Wall -Wextra -Wpedantic -Wfatal-errors -Wsign-conversion -Wold-style-cast \
          -Wno-unused-parameter -Wno-unused-result"

#CC="clang++ -std=c++20"
#WARNINGS="-Weverything -Wfatal-errors \
#          -Wno-unused-parameter -Wno-unused-result \
#          -Wno-c++98-compat -Wno-weak-vtables \
#          -Wno-padded -Wno-global-constructors \
#          -Wno-exit-time-destructors -Wno-format-nonliteral"
BIN=xiinux
SRC=src/main.cpp
DBG=""
OPTS=-Os
#DBG="-g"
#DBG="$DBG --coverage -fprofile-arcs -ftest-coverage"
#OPTS=-O3
#OPTS="-O3 -static"
#LIB="-pthread -lgcov"
LIB="-pthread"

echo > all.src &&
for f in $(find src);do if [ -f $f ];then cat $f>>all.src;fi;done

echo &&
$CC  -o $BIN $SRC $DBG $LIB $OPTS $WARNINGS && 
echo    "            lines   words   chars" &&
echo -n "   source:" &&
cat all.src|wc &&
echo -n "  gzipped:" &&
cat all.src|gzip|wc &&
echo && ls -ho --color $BIN &&
echo &&
rm all.src
#valgrind ./$BIN
#valgrind --leak-check=full ./$BIN
#valgrind --leak-check=full --show-leak-kinds=all ./$BIN
#valgrind --leak-check=full --show-leak-kinds=all ./$BIN -bv
