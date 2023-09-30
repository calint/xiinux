#!/bin/sh
set -e

./do-benchmark.sh xiinux localhost 8088
#./do-benchmark.sh bob localhost 8888
./do-benchmark.sh nginx localhost 80

grep responses *.rep > _summary.txt
grep Requests *.rep | grep -oE '([^\/]+)--[^:]+:  Requests/sec:\s+([0-9.]+)' | awk -F '--|: ' '{print $1, $2, $3, $4}' >> _summary.txt

./do-pretty-summary.py < _summary.txt > _summary-table.txt

cat _summary-table.txt
