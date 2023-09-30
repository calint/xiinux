#!/bin/sh
set -e

cat ../_summary-table.txt | grep ^homepage > homepage.tbl
cat ../_summary-table.txt | grep ^small > small.tbl
cat ../_summary-table.txt | grep ^far_side_dog_ok > far_side_dog_ok.tbl

gnuplot -c plot.gp homepage.tbl
gnuplot -c plot.gp small.tbl
gnuplot -c plot.gp far_side_dog_ok.tbl

rm *.tbl

