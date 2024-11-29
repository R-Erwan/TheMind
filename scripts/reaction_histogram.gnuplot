set terminal png size 1280,720
set output sprintf("%s/reaction_histogram.png", output_dir)
set title "Temps de réaction moyen par carte"
set xlabel "Carte"
set ylabel "Temps de réaction moyen (s)"
set style fill solid
set boxwidth 0.8
set xrange [0:*]

# Palette de couleurs alternées
set palette defined (0 "#c75f42", 1 "#edccb7")

# Enlever la barre de couleurs
unset colorbox

plot sprintf("%s/reaction_data.dat", output_dir) using 0:1:(int($0)%2) with boxes lc palette title ""
