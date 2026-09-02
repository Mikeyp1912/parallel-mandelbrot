if (!exists("WIDTH")) WIDTH = 1000
if (!exists("HEIGHT")) HEIGHT = 1000

set terminal pngcairo size WIDTH,HEIGHT
set output 'plot/mandel.png'

unset key
unset border
unset xtics
unset ytics
unset colorbox

set size ratio -1

set palette defined ( \
    0.00 "#050816", \
    0.15 "#0B1F4D", \
    0.35 "#174A8B", \
    0.55 "#2E86C1", \
    0.72 "#7FDBFF", \
    0.88 "#F6E7B0", \
    1.00 "#FFF8E7" \
)

set cbrange [0:1]

plot 'plot/mandel.dat' using 1:2:3 with image
