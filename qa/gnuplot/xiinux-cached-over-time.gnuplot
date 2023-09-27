set terminal png
set output "xiinux-cached-over-time.png"
datafile = "data.txt"
first_line = system(sprintf("head -n 1 %s", datafile))
set title first_line
set xlabel "ms"
set ylabel "files"
plot datafile using 1:12 with linespoints
set output
