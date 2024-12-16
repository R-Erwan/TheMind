#!/bin/bash

# Nom du fichier de classement
FICHIER_CLASSEMENT="./datas/rank.dat"

# Argument : nombre_joueurs
if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <nombre_joueurs>"
  exit 1
fi

nombre_joueurs=$1

# Filtrer et afficher les 10 premières parties

awk -F ',' -v joueurs="$nombre_joueurs" '
  $1 == joueurs { print $0 }
' "$FICHIER_CLASSEMENT" | sort -t ',' -k2,2nr | head -n 10
#-k2,2nr -> Trie selon la 2ème colonne uniquement, n ordre numérique, r décroissant.