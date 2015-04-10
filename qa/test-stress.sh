HOST=localhost:8088
OUT=log.txt
SUM=log-summary.txt
URI=
URIS="/ /qa/q01.txt /?hello"

echo&&date&&
date>$OUT&&echo -n>$SUM&&
for URI in $URIS;do
	echo uri: $URI>>$OUT&&
	ab    -v2 -c1    -n1      $HOST$URI>>$OUT&&
	ab    -v0 -c10   -n10000  $HOST$URI>>$OUT&&
	ab    -v0 -c100  -n10000  $HOST$URI>>$OUT&&
	ab    -v0 -c1000 -n10000  $HOST$URI>>$OUT&&

	echo urik: $URI>>$OUT&&
	ab -k -v2 -c1    -n1      $HOST$URI>>$OUT&&
	ab -k -v0 -c10   -n10000  $HOST$URI>>$OUT&&
	ab -k -v0 -c100  -n10000  $HOST$URI>>$OUT&&
	ab -k -v0 -c1000 -n10000  $HOST$URI>>$OUT
done&&
cat $OUT|grep '^uri\|^Failed\|^Requests'>>$SUM&&
date>>$OUT&&date>>$SUM&&
cat $SUM&&echo
