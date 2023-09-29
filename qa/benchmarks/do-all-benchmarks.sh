#!/bin/sh
./do-benchmark.sh nginx localhost 80
./do-benchmark.sh xiinux localhost 8088

grep responses *.rep
grep Requests *.rep | grep -oE '([^\/]+)--[^:]+:  Requests/sec:\s+([0-9.]+)' | awk -F '--|: ' '{print $1, $2, $3, $4}' > 0.summary.txt
cat 0.summary.txt