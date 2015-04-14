CC="clang++ -std=c++11"
BIN=xiinux
SRC=src/*.cpp
DBG="-g -O0"
#OPTS=-Os
WARNINGS="-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wfatal-errors"
LIBS=-pthread

echo &&
$CC  -o $BIN $SRC $DBG $LIBS $OPTS $WARNINGS && 
echo    "             lines   words   chars" &&
echo -n "   source:" &&
cat $SRC|wc &&
echo -n "   zipped:" &&
cat $SRC|gzip|wc &&
echo && ls -o --color $BIN &&
echo &&
#valgrind --leak-check=yes --leak-check=full --show-leak-kinds=all ./$BIN
valgrind --leak-check=yes ./$BIN

