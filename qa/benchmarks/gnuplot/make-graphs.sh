#!/bin/sh

cat ../0.summary-table.txt | grep ^homepage > homepage.tbl
cat ../0.summary-table.txt | grep ^small > small.tbl
cat ../0.summary-table.txt | grep ^far_side_dog_ok > far_side_dog_ok.tbl

gnuplot -c summary.gnuplot homepage.tbl
gnuplot -c summary.gnuplot small.tbl
gnuplot -c summary.gnuplot far_side_dog_ok.tbl

