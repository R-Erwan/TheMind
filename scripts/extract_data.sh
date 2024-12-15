#!/bin/bash

# Vérification des arguments
if [ $# -lt 1 ]; then
    echo "Usage: $0 <fichier_d'entrée>"
    exit 1
fi

# Chemin vers le fichier d'entrée
input_file="$1"

# Dossier de sortie
output_dir="../datas"

# Vérification de l'existence du dossier de sortie
if [ ! -d "$output_dir" ]; then
    mkdir -p "$output_dir"
fi

# Fichiers de sortie
reaction_file="$output_dir/reaction_data.dat"
loosing_file="$output_dir/loosing_cards.dat"
rounds_file="$output_dir/rounds_levels.dat"
#donut_file="$output_dir/loosing_cards_donut.dat"

# Extraction des données -f2- garder les collones a partie de la 2ème
grep "REACTIONPERCARD" "$input_file" | cut -d' ' -f2- | tr ' ' '\n' > "$reaction_file"
grep "LOOSINGCARD" "$input_file" | cut -d' ' -f2- | tr ' ' '\n' > "$loosing_file"
grep "ROUNDSLIST" "$input_file" | cut -d' ' -f2- | tr ' ' '\n' > "$rounds_file"

# Préparer les données pour le donut chart
#awk '/LOOSINGCARD/{
#    for (i = 2; i <= NF; i++) {
#        if ($i > 0) {
#            print i-1, $i
#        }
#    }
#}' "$input_file" > "$donut_file"

# Obtenir les chemins absolus des fichiers générés
reaction_file_abs=$(realpath "$reaction_file")
loosing_file_abs=$(realpath "$loosing_file")
rounds_file_abs=$(realpath "$rounds_file")
CYAN='\033[0;34m'
RESET='\033[0m'

# Affichage des chemins absolus
echo -e "Fichiers de données générés :
${CYAN}- Réactions : $reaction_file_abs
- Cartes perdantes : $loosing_file_abs
- Niveaux des rounds : $rounds_file_abs ${RESET}"
