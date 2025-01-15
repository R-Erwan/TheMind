#!/bin/bash

# Définir un port par défaut si non fourni
PORT=${PORT:-4242}

# Lancer le serveur avec les arguments dynamiques

cd build/bin/server
exec ./TheMindServeur "$PORT" "10"
