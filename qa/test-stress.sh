HOST=localhost:8088
OUT=log.txt
SUM=log-summary.txt
URI=
URI=?hello

echo&&date&&
date>$OUT&&echo -n>$SUM&&
ab    -v2 -c1    -n1      $HOST/$URI>>$OUT&&
ab    -v0 -c10   -n10000  $HOST/$URI>>$OUT&&
ab    -v0 -c100  -n10000  $HOST/$URI>>$OUT&&
ab    -v0 -c1000 -n10000  $HOST/$URI>>$OUT&&

ab -k -v2 -c1    -n1      $HOST/$URI>>$OUT&&
ab -k -v0 -c10   -n100000 $HOST/$URI>>$OUT&&
ab -k -v0 -c100  -n100000 $HOST/$URI>>$OUT&&
ab -k -v0 -c1000 -n100000 $HOST/$URI>>$OUT&&

cat $OUT|grep Failed>>$SUM&&
cat $OUT|grep Requests>>$SUM&&
date>>$OUT&&date>>$SUM&&
cat $SUM&&echo
