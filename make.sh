#CC="clang++ -std=c++11"
CC="g++ -std=c++11"
BIN=xiinux
SRC=src/*.cpp
#DBG="-O3"
DBG="-g -O0"
DBG="$DBG --coverage -fprofile-arcs -ftest-coverage"
#OPTS=-Os
WARNINGS="-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wfatal-errors"
LIB="-pthread -lgcov"

echo &&
$CC  -o $BIN $SRC $DBG $LIB $OPTS $WARNINGS && 
echo    "             lines  words   chars" &&
echo -n "   source:" &&
cat $SRC|wc &&
echo -n "   zipped:" &&
cat $SRC|gzip|wc &&
echo && ls -o --color $BIN &&
echo
#valgrind --leak-check=yes --leak-check=full --show-leak-kinds=all ./$BIN
#valgrind --leak-check=yes ./$BIN
#valgrind --tool=callgrind --collect-jumps=yes ./$BIN
