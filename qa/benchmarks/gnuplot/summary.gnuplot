set terminal png
set output ARGV[1] . ".png"

data = ARGV[1]

set title "Data Plot"
set xlabel "clients"
set ylabel "requests/s"
set key outside
set grid

set yrange [0:]

plot \
     data using 2:($4*(strcol(3) eq "nginx" ? 1 : 0)) with points linecolor "green" title "nginx", \
     data using 2:($4*(strcol(3) eq "xiinux" ? 1 : 0)) with points linecolor "blue" title "xiinux"
