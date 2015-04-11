CC="clang++ -std=c++11"
BIN=xiinux
SRC=src/xiinux.cpp
OPTS="-pthread -Os"
#WARNINGS="-Wall -Wextra"
LIBS=

echo&&
$CC  -o $BIN $SRC $LIBS $OPTS $WARNINGS&& 
echo    "             lines   words  chars"&&
echo -n "   source:"&&cat $SRC|wc&&
echo -n "   zipped:"&&cat $SRC|gzip|wc&&
echo&&ls -o --color $BIN&&
echo
