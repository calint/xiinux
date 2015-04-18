HOST=localhost:8088
OUT=log.txt
SUM=log-summary.txt
URI=
URIS="/ /qa/q01.txt /qa/q02.txt /?hello"
NCLIENTS="1 10 100 1000 10000 20000"
NREQUESTS=20000

date>$OUT
date>$SUM
for URI in $URIS;do
	echo
	echo uri: $URI>>$OUT
#	ab    -v2 -c1    -n1      $HOST$URI>>$OUT
	for C in $NCLIENTS;do
		echo concurrency: $C  $HOST$URI
		echo concurrency: $C  $HOST$URI>>$OUT
		ab    -v0 -c$C   -n$NREQUESTS  $HOST$URI>>$OUT
	done

	echo
	echo uri keep-alive: $URI>>$OUT
#	ab -k -v2 -c1    -n1      $HOST$URI>>$OUT
	for C in $NCLIENTS;do
		echo concurrency: $C  $HOST$URI
		echo concurrency: $C  $HOST$URI>>$OUT
		ab -k -v0 -c$C   -n$NREQUESTS  $HOST$URI>>$OUT
	done
done
cat $OUT|grep '^uri\|^Failed\|^Requests\|^concurrency'>>$SUM
date>>$OUT
date>>$SUM
cat $SUM
echo
