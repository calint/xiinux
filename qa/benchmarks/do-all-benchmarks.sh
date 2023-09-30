#!/bin/sh
./do-benchmark.sh xiinux localhost 8088
#./do-benchmark.sh bob localhost 8888
./do-benchmark.sh nginx localhost 80

date > 0.summary.txt
grep responses *.rep >> 0.summary.txt
grep Requests *.rep | grep -oE '([^\/]+)--[^:]+:  Requests/sec:\s+([0-9.]+)' | awk -F '--|: ' '{print $1, $2, $3, $4}' >> 0.summary.txt
echo
cat 0.summary.txt