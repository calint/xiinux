#!/bin/sh
set -e

./do-all-benchmarks.sh &&
cd gnuplot &&
./make-graphs.sh &&
mv -f far_side_dog_ok.tbl.png ../report &&
mv -f homepage.tbl.png ../report &&
mv -f small.tbl.png ../report &&
cd .. &&
cp 0.summary-table.txt report/summary.txt