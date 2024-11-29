set terminal png size 1280,720
set output sprintf("%s/loosing_cards.png", output_dir)
set title "Cartes qui causent une défaite"
set xlabel "Carte"
set ylabel "Nombre de défaites"
set style fill solid
set boxwidth 0.8
set xrange [0:*]

# Palette de couleurs alternées
set palette defined (0 "#c75f42", 1 "#edccb7")
# Enlever la barre de couleurs
unset colorbox

plot sprintf("%s/loosing_cards.dat", output_dir) using 0:1:(int($0)%2) with boxes lc palette title ""