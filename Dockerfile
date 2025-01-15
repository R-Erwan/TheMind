# Utiliser une image Debian comme base
FROM debian:stable-slim

# Mettre à jour le système et installer les dépendances
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gcc \
    gnuplot \
    texlive \
    texlive-latex-extra \
    texlive-lang-french \
    dos2unix \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Définir un répertoire de travail
WORKDIR /app

# Copier les fichiers de l'application
COPY ./ressources /app/ressources
COPY ./scripts /app/scripts
COPY ./src /app/src
COPY ./TheMindRobot /app/TheMindRobot
COPY ./TheMindClient /app/TheMindClient
COPY ./entrypoint.sh /app/entrypoint.sh
COPY ./CMakeLists.txt /app/CMakeLists.txt
COPY ./README.md /app/README.md

# Conversion des fichiers en format Unix
RUN find . -type f \( -name "*.sh" -o -name "*.txt" -o -name "*.cmake" -o -name "*.cpp" -o -name "*.h" \) -exec dos2unix {} +

# Droits d'exécution sur tous les scripts
RUN chmod -R 755 /app

# Compilation
RUN mkdir -p build && cd build && cmake .. && cmake --build .

# Création de l'arborescence des fichiers nécessaires
RUN mkdir -p /app/build/bin/server/datas && touch /app/build/bin/server/datas/rank.dat && \
    mkdir -p /app/build/bin/server/pdf


ENV PORT=4242

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Définir la commande par défaut pour démarrer le serveur
ENTRYPOINT ["/entrypoint.sh"]
