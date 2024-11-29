set terminal pngcairo size 800,800 enhanced
set output sprintf("%s/pie_chart.png", output_dir)
set title "TODOs"

datafile = sprintf("%s/loosing_cards_donut.dat", output_dir)

stats datafile using 2 nooutput
fac = 360.0/STATS_sum

set table $data2
plot r=0 datafile using 1:2:(0):(r):(dr=$2*fac,r=r+dr,dr) with table
unset table


set polar
set angles degree
set theta top cw
set xrange [-10:10]
set yrange [-10:10]
set size ratio -1
unset border
unset raxis
unset tics
unset key

plot $data2 using 4:(4):5:(3):($0+1) with sectors lc variable lw 3 fill solid border lc bgnd, \
     $data2 using ($4+$5/2):(8+0.7*$3):(sprintf("%.1f",$2)) with labels
