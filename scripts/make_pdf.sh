#!/bin/bash

# Vérifier qu'un fichier a été fourni en paramètre
if [ $# -ne 2 ]; then
    echo "Usage: $0 stats_file latex_file"
    exit 1
fi


# Fichier contenant les statistiques
STATS_FILE="$1"
TEXFILE="$2"
OUTPUTFILE=$(basename "$STATS_FILE")

# Vérification que le fichier d'entrée existe
if [ ! -f "$STATS_FILE" ]; then
    echo "Erreur : Fichier d'entrée $STATS_FILE introuvable."
    exit 1
fi

# Vérification que le fichier d'entrée existe
if [ ! -f "$TEXFILE" ]; then
    echo "Erreur : Fichier d'entrée $TEXFILE introuvable."
    exit 1
fi



# Fonction pour extraire une valeur depuis le fichier stats
get_value() {
    local key="$1"
    grep -m1 "^$key" "$STATS_FILE" | awk '{print $2}' # -m1 pour garder que la 1ère occurence commencant par key.
}

# Fonction pour remplacer une valeur dans le fichier LaTeX
replace_data() {
    local marker="$1"
    local new_value="$2"
    local file="$3"
    sed -i "/$marker/{n;s/.*/$new_value/;}" "$file" # recherche la ligne contenanat marker, n passe a la ligne suivante,s/.*/ remplace toute la ligne suivante
}

# Extraire les valeurs nécessaires
DATA_PLAYERS=$(get_value "PLAYER")
DATA_ROUNDS=$(get_value "ROUNDS")
DATA_WINROUNDS=$(get_value "WINROUNDS")
DATA_MAXROUND=$(get_value "MAXROUNDS")
DATA_TIME=$(get_value "REACTIONTIME")

# Mettre à jour le fichier LaTeX
replace_data "%DATA_PLAYERS" "$DATA_PLAYERS" "$TEXFILE"
replace_data "%DATA_ROUNDS" "$DATA_ROUNDS" "$TEXFILE"
replace_data "%DATA_WINROUNDS" "$DATA_WINROUNDS" "$TEXFILE"
replace_data "%DATA_MAXROUND" "$DATA_MAXROUND" "$TEXFILE"
replace_data "%DATA_TIME" "$DATA_TIME" "$TEXFILE"

# Compiler le fichier LaTeX
pdflatex -jobname="$OUTPUTFILE" "$TEXFILE" > /dev/null 2>&1
mv "$OUTPUTFILE".pdf ../pdf/"$OUTPUTFILE".pdf
rm -f "${OUTPUTFILE}".aux "${OUTPUTFILE}".log

YELLOW='\033[0;33m'
RESET='\033[0m'
FULL_PATH=$(realpath "../pdf/$OUTPUTFILE.pdf")
echo -e "Fichier pdf généré : ${YELLOW} $FULL_PATH ${RESET}"
