HTTP=http://localhost:8088
STATS=test-siege.report
NCLIENTS=10
TIME=10s
URIS='/ /?page /typealine.html'
echo
echo "   s i e g e    b e n c h"&&
echo "     $(date)"&&
echo "      time: $TIME"&&
echo "   clients: $NCLIENTS"&&
echo "  time: $TIME   clients: $NCLIENTS">$STATS&&
for P in $URIS;do
	echo $P>>$STATS &&
	echo
	echo "            $HTTP$P" &&
	siege -bqt$TIME -l$STATS -c$NCLIENTS $HTTP$P
	echo
	cat $STATS
done
echo "     $(date)"&&
echo