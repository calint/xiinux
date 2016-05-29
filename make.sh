#CC="clang++ -std=c++17"
CC="g++ -std=c++17"
BIN=xiinux
SRC=src/main.cpp
DBG="-O3"
#DBG="-g -O0"
#DBG="$DBG --coverage -fprofile-arcs -ftest-coverage"
#OPTS=-Os
#OPTS="-O3 -static"
OPTS=-O3
WARNINGS="-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wfatal-errors"
LIB="-pthread -lgcov"

echo > all.src &&
FILES=$(for f in $(find src);do if [ -f $f ];then cat $f>>all.src;fi;done)

echo &&
$CC  -o $BIN $SRC $DBG $LIB $OPTS $WARNINGS && 
echo    "             lines  words   chars" &&
echo -n "   source:" &&
cat all.src|wc &&
echo -n "   zipped:" &&
cat all.src|gzip|wc &&
echo && ls -ho --color $BIN &&
echo &&
rm all.src &&
echo
#valgrind --leak-check=yes --leak-check=full --show-leak-kinds=all ./$BIN -bv
#valgrind --leak-check=yes ./$BIN
#valgrind --tool=callgrind --collect-jumps=yes ./$BIN
