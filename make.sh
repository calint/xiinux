CC="g++ -std=c++23"
BIN=xiinux
SRC=src/main.cpp
DBG=""
#DBG="-g"
#DBG="$DBG --coverage -fprofile-arcs -ftest-coverage"
OPTS=-Os
#OPTS=-O3
#OPTS="-O3 -static"
WARNINGS="-Wall -Wextra -Wpedantic -Wfatal-errors \
    -Wno-unused-parameter -Wno-unused-result -Wno-stringop-truncation -Wno-array-bounds"
#LIB="-pthread -lgcov"
LIB="-pthread"

echo > all.src &&
for f in $(find src);do if [ -f $f ];then cat $f>>all.src;fi;done

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
