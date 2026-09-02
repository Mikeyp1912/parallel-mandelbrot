set terminal pngcairo size 1000,1000
set output 'mandel.png'

unset key
unset xtics
unset ytics
unset colorbox
set border 0

set size ratio -1
set cbrange [0:1]

set palette defined ( \
    0 '#000000', \
    1 '#000764', \
    2 '#2068cb', \
    3 '#edffff', \
    4 '#ffaa00', \
    5 '#000200' )

plot 'mandel.dat' using 1:2:3 with image
