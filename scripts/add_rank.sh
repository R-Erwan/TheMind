#!/bin/bash

# Nom du fichier de classement
FICHIER_CLASSEMENT="./datas/rank.dat"

# Vérifier qu'il y a au moins 3 arguments
if [ "$#" -lt 3 ]; then
  echo "Usage: $0 <nombre_joueurs> <manches_successives> <nom_joueur1> [nom_joueur2 ... nom_joueurN]"
  exit 1
fi

# Récupération des arguments
nombre_joueurs=$1
manches_successives=$2
shift 2 # Ignore les deux premiers arguments pour traiter uniquement les noms de joueurs

# Construire la liste des joueurs séparée par un "|"
liste_joueurs=$(printf "%s|" "$@")
liste_joueurs=${liste_joueurs%|} # Retirer le dernier "|"

# Générer un horodatage
horodatage=$(date +"%Y-%m-%d")

# Ajouter la ligne au fichier de classement
echo "$nombre_joueurs,$manches_successives,$liste_joueurs,$horodatage" >> "$FICHIER_CLASSEMENT"

PURPLE='\033[0;35m'
RESET='\033[0m'

echo -e "${PURPLE}Partie ajoutée au classement avec les joueurs : $liste_joueurs ${RESET}"
