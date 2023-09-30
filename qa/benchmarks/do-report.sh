#!/bin/sh
set -e

rm far_side_dog_ok-*
rm homepage-*
rm small-*
./do-all-benchmarks.sh &&
cd gnuplot &&
./make-graphs.sh &&
mv -f far_side_dog_ok.tbl.png ../report &&
mv -f homepage.tbl.png ../report &&
mv -f small.tbl.png ../report &&
cd .. &&
cp _summary-table.txt report/summary.txt
cat _summary.txt | grep responses > report/requests-completed.txt
rm far_side_dog_ok-*
rm homepage-*
rm small-*
