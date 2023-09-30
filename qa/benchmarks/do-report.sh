#!/bin/sh

./do-all-benchmarks.sh &&
cd gnuplot &&
./make-graphs.sh &&
mv far_side_dog_ok.tbl.png ../report &&
mv homepage.tbl.png ../report &&
mv small.tbl.png ../report &&
cd .. &&
mv 0.summary-table.txt report/summary.txt