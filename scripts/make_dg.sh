#!/bin/bash

# Variables
input_file="$1"
script_dir=$(dirname "$0")
data_dir="$(realpath ../datas)"  # Obtenir le chemin absolu


# Vérification que le fichier d'entrée existe
if [ ! -f "$input_file" ]; then
    echo "Erreur : Fichier d'entrée $input_file introuvable."
    exit 1
fi

# Étape 1 : Extraction des données
"$script_dir/extract_data.sh" "$input_file"

# Vérification des fichiers générés
if [ ! -f "$data_dir/reaction_data.dat" ] || [ ! -f "$data_dir/loosing_cards.dat" ] || [ ! -f "$data_dir/rounds_levels.dat" ]; then
    echo "Erreur : Un ou plusieurs fichiers de données n'ont pas été générés."
    exit 1
fi

gnuplot -e "output_dir='$data_dir'" "$script_dir/reaction_histogram.gnuplot"
gnuplot -e "output_dir='$data_dir'" "$script_dir/loosing_cards_histogram.gnuplot"
gnuplot -e "output_dir='$data_dir'" "$script_dir/rounds_levels.gnuplot"
#gnuplot -e "output_dir='$data_dir'" "$script_dir/loosing_cards_pie.gnuplot"

CYAN='\033[0;34m'
RESET='\033[0m'

echo -e "Graphiques générés :
${CYAN}- $data_dir/reaction_histogram.png
- $data_dir/loosing_cards_histogram.png
- $data_dir/rounds_levels.png${RESET}"
