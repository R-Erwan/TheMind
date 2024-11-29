set terminal png size 1280,720
set output sprintf("%s/rounds_levels.png", output_dir)
set title "Niveau des manches"
set xlabel "Round"
set ylabel "Niveau"
set ytics 1
set boxwidth 0.8
set xrange [0:*]
plot sprintf("%s/rounds_levels.dat", output_dir) \
    with linespoints linewidth 4 linecolor rgb "#c75f42" \
    title ''

