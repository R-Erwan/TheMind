#!/bin/bash

PROJECT_NAME="TheMind"
BUILD_DIR="build-server"
ROBOT_BUILD_DIR="build-robot"

echo "Installation de $PROJECT_NAME"

# Vérification que 'dos2unix' est installé
if ! command -v dos2unix &> /dev/null; then
  echo "❌ L'outil 'dos2unix' n'est pas installé. Installation..."
  sudo apt-get update
  sudo apt-get install -y dos2unix
fi

# Conversion des fichiers en format Unix
echo "🔄 Conversion des fichiers en format Unix..."
find . -type f \( -name "*.sh" -o -name "*.txt" -o -name "*.cmake" -o -name "*.cpp" -o -name "*.h" \) -exec dos2unix {} +

# Dépendances
echo "📦 Installation des dépendances..."
sudo apt-get update
sudo apt-get install -y cmake gcc gnuplot texlive texlive-latex-extra texlive-lang-french

# Arborescence
echo "📁 Création de l'arborescence des fichiers..."
mkdir -p {datas,pdf}
touch datas/rank.dat

# Droits d'éxécution
echo "🪪Setup des droits sur les scripts :"
chmod -R 777 *

# Compilation Serveur
if [ -d "$BUILD_DIR" ]; then
  echo "🧹 Nettoyage du répertoire $BUILD_DIR existant..."
  rm -rf "$BUILD_DIR"
fi
echo "📁 Création du répertoire $BUILD_DIR..."
mkdir "$BUILD_DIR"
cd "$BUILD_DIR"

echo "🔧 Configuration avec CMake..."
cmake ..

echo "⚙️ Compilation..."
cmake --build .
cd ..

# Compilation Robot
if [ -d  "$ROBOT_BUILD_DIR" ]; then
  echo "🧹 Nettoyage du répertoire $ROBOT_BUILD_DIR existant..."
  rm -rf "$ROBOT_BUILD_DIR"
fi
echo "📁 Création du répertoire $ROBOT_BUILD_DIR"
mkdir "$ROBOT_BUILD_DIR"
cd "$ROBOT_BUILD_DIR"

echo "🔧 Configuration du robot avec CMake..."
cmake ../TheMindRobot
echo "⚙️ Compilation du robot..."
cmake --build .

cd ..

# Verification

# Étape 4 : Fin
echo "✅ Installation terminée. Exécutable disponible dans $BUILD_DIR/$PROJECT_NAME"
echo "run ./$PROJECT_NAME dans le dossier $BUILD_DIR pour lancer le server."

