BIN=xiinux
SRC=src/xiinux.cpp
#OPTS=-Os
#WARNINGS="-Wall -Wextra"
LIBS=-lX11
CC="clang++ -std=c++11"

echo &&
$CC  -o $BIN $SRC $LIBS $OPTS $WARNINGS && 
echo    "             lines   words  chars" &&
echo -n "       wc:" &&
cat $SRC|wc
echo -n "wc zipped:" &&
cat $SRC|gzip|wc &&
echo && ls -o --color $BIN &&
echo
